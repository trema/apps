/*
 * Monitoring
 *
 * Author: TeYen Liu
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <assert.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "monitoring.h"
#include "toolutils.h"
#include "utilmonitoring.h"

static const uint32_t DEFAULT_SUPPORT_RATE_DATA = 192;


// ******************************** Subscribers Begins ******************************************
list_element *
create_subscribers( list_element **subscribers ) {
  assert( subscribers != NULL );

  create_list( subscribers );
  return *subscribers;
}

void
delete_subscribers( list_element **subscribers ) {
  assert( subscribers != NULL );
  
  for ( list_element *e = *subscribers; e != NULL; e = e->next ) {
    xfree(e->data);
  }
  delete_list( subscribers );
}
// ******************************** Subscribers Ends ******************************************

// ******************************** Subscriber Begins ******************************************
char *
lookup_subscriber( list_element *subscribers, const char *owner ) {
  for ( list_element *e = subscribers; e != NULL; e = e->next ) {
    char *name = e->data;
    if ( strcmp( name, owner ) == 0 ) {
      return name;
    }
  }
  return NULL;
}

void
add_subscriber( list_element **subscribers, const char *owner ) {
  
  if ( lookup_subscriber( *subscribers, owner ) == NULL ) {
    char *name = xmalloc( MESSENGER_SERVICE_NAME_LENGTH );
    memcpy( name, owner, MESSENGER_SERVICE_NAME_LENGTH );
    name[ MESSENGER_SERVICE_NAME_LENGTH - 1 ] = '\0';
    
    info( "[add_subscriber] Add subscriber name: %s, owner:%s", name, owner );
    append_to_tail( subscribers, name );
  }
}

void
delete_subscriber( list_element **subscribers, const char *owner ) {
  char *name = lookup_subscriber( *subscribers, owner );
  if ( name == NULL ) {
    return;
  } else {
    delete_element( subscribers, name );
    xfree( name );
  }
}
// ******************************** Subscriber Ends ******************************************

// ******************************** Switch Begins ******************************************
list_element *
create_switches( list_element **switches ) {
  assert( switches != NULL );

  create_list( switches );
  return *switches;
}

static switch_info *
lookup_switch( list_element *switches, uint64_t dpid ) {
  for ( list_element *e = switches; e != NULL; e = e->next ) {
    switch_info *sw = e->data;
    if ( sw->dpid == dpid ) {
      return sw;
    }
  }

  return NULL;
}

static switch_info *
allocate_switch( uint64_t dpid ) {
  switch_info *sw = xmalloc( sizeof( switch_info ) );
  sw->dpid = dpid;
  create_list( &sw->ports );
  create_list( &sw->flows );

  return sw;
}

static void
delete_switch( list_element **switches, switch_info *delete_switch ) {
  
  list_element *ports = delete_switch->ports;
  list_element *flows = delete_switch->flows;

  // delete ports
  for ( list_element *p = ports; p != NULL; p = p->next ) {
    xfree( p->data );
  }
  delete_list( ports );
  
  // delete flows
  for ( list_element *fl = flows; fl != NULL; fl = fl->next ) {
    xfree( fl->data );
  }
  delete_list( flows );

  // delete switch
  delete_element( switches, delete_switch );
  xfree( delete_switch );
}

// ******************************** Switch Ends ******************************************

// ******************************** Flow Begins ******************************************
flow_info *
lookup_flow( list_element *switches, uint64_t dpid, struct ofp_match *match ) {
  assert( match != NULL );

  switch_info *sw = lookup_switch( switches, dpid );
  if ( sw == NULL ) {
    return NULL;
  }

  for ( list_element *e = sw->flows; e != NULL; e = e->next ) {
    flow_info *fl = e->data;
    if ( compare_match_strict( match, &fl->flow_stats.match ) ) {
      return fl;
    }
  }
  return NULL;
}

static flow_info *
allocate_flow( uint64_t dpid, struct ofp_flow_stats *flow_stats ) {
  
  flow_info *flow = xmalloc( sizeof( flow_info ) );
  flow->dpid = dpid;
  flow->prev_duration_sec = 0;
  flow->prev_byte_count = 0;
  flow->flow_bit_rate = 0;
  flow->big_flow_number_times = 0;
  flow->flow_stats = *flow_stats;
  flow->big_flow_flag = false;
  return flow;
}

void
delete_flow( list_element **switches, flow_info *delete_flow ) {
  assert( switches != NULL );
  assert( delete_flow != NULL );

  // lookup switch
  switch_info *sw = lookup_switch( *switches, delete_flow->dpid );
  if( sw == NULL ) {
    debug( "[delete_flow] No such a switch: dpid = %#" PRIx64 ", port = %u", delete_flow->dpid );
    return;
  }
  
  info( "[delete_flow] Deleting a big flow candidate: dpid = %#" PRIx64 ", match = %s, idle_timeout = %u, hard_timeout = %u, duration_sec = %u", 
        delete_flow->dpid, get_static_match_str( &delete_flow->flow_stats.match ),
          delete_flow->flow_stats.idle_timeout, 
          delete_flow->flow_stats.hard_timeout,
          delete_flow->flow_stats.duration_sec );

  delete_element( &sw->flows, delete_flow );
  xfree( delete_flow );
}

void
update_flow( flow_info *flow, struct ofp_flow_stats *flow_stats ) {
  assert( flow_stats != 0 );
  
  /* calculate the parameters */
  flow->flow_stats = *flow_stats;
  if( flow->flow_stats.duration_sec < flow->prev_duration_sec ) {
    /* reset the parameters due to the new flow generated */
    flow->prev_duration_sec = 0;
    flow->prev_byte_count = 0;
    flow->flow_bit_rate = 0;
    flow->big_flow_number_times = 0;
    flow->big_flow_flag = false;
  }
  
  if ( flow->flow_stats.duration_sec - flow->prev_duration_sec <= 0 ) {
    flow->flow_bit_rate = 0;  
  } else {
    
    // Byte Count could be reset to start from 0 so that we need to deal with this situation
    if (flow->flow_stats.byte_count >= flow->prev_byte_count ) {  
      flow->flow_bit_rate = ( flow->flow_stats.byte_count - flow->prev_byte_count ) 
            * 8 / ( flow->flow_stats.duration_sec - flow->prev_duration_sec );
    } else {
      flow->flow_bit_rate = ( UINT64_MAX - flow->prev_byte_count + flow->flow_stats.byte_count ) 
            * 8 / ( flow->flow_stats.duration_sec - flow->prev_duration_sec );
    }
  }
  
  flow->prev_byte_count = flow->flow_stats.byte_count;
  flow->prev_duration_sec = flow->flow_stats.duration_sec;
  
  debug( "[update_flow] Updating a big flow candidate: dpid = %#" PRIx64 ", match = %s, idle_timeout = %u, hard_timeout = %u, duration_sec = %u, flow_bit_rate = %u, big_flow_number_times = %u", 
      flow->dpid, get_static_match_str( &flow->flow_stats.match ),
        flow->flow_stats.idle_timeout, 
        flow->flow_stats.hard_timeout,
        flow->flow_stats.duration_sec,
        flow->flow_bit_rate, 
        flow->big_flow_number_times );
}

void
add_flow( list_element **switches, uint64_t dpid, struct ofp_flow_stats *flow_stats ) {
  assert( flow_stats != NULL );

  debug( "[add_flow] Adding a big flow candidate: dpid = %#" PRIx64 ", match = %s",
         dpid, get_static_match_str( &flow_stats->match) );

  // lookup switch
  switch_info *sw = lookup_switch( *switches, dpid );
  if ( sw == NULL ) {
    return;
  }

  flow_info *new_flow = allocate_flow( dpid, flow_stats );
  update_flow( new_flow, flow_stats );
  append_to_tail( &sw->flows, new_flow );
}
// ******************************** Flow Ends ******************************************

// ******************************** Port Begins ******************************************
port_info *
lookup_port( list_element *switches, uint64_t dpid, uint16_t port_no ) {
  assert( port_no != 0 );

  switch_info *sw = lookup_switch( switches, dpid );
  if ( sw == NULL ) {
    return NULL;
  }

  for ( list_element *e = sw->ports; e != NULL; e = e->next ) {
    port_info *p = e->data;
    if ( dpid == p->dpid && port_no == p->port_no ) {
      return p;
    }
  }
  return NULL;
}

static port_info *
allocate_port( uint64_t dpid, uint16_t port_no, uint64_t port_setting_feature_rate ) {
  
  port_info *port = xmalloc( sizeof( port_info ) );
  port->dpid = dpid;
  port->port_no = port_no;
  port->external_link = false;
  port->switch_to_switch_link = false;
  port->switch_to_switch_reverse_link = false;
  port->curr_rx_bytes = 0;
  port->curr_rx_packets = 0;
  port->curr_tx_bytes = 0;
  port->curr_tx_packets = 0;
  port->prev_rx_bytes = 0;
  port->prev_rx_packets = 0;
  port->prev_tx_bytes = 0;
  port->prev_tx_packets = 0;
  port->avg_rx_bytes = 0;
  port->avg_rx_packets = 0;
  port->avg_tx_bytes = 0;
  port->avg_tx_packets = 0;
  port->port_bit_rate = 0;
  port->port_feature_rate = port_setting_feature_rate;
  port->loading = 0;
  
  return port;
}

void
delete_port( list_element **switches, port_info *delete_port ) {
  assert( switches != NULL );
  assert( delete_port != NULL );

  info( "[delete_port] Deleting a port: dpid = %#" PRIx64 ", port = %u",
        delete_port->dpid, delete_port->port_no );

  // lookup switch
  switch_info *sw = lookup_switch( *switches, delete_port->dpid );
  if( sw == NULL ) {
    debug( "[delete_port] No such port: dpid = %#" PRIx64 ", port = %u", delete_port->dpid, delete_port->port_no );
    return;
  }

  delete_element( &sw->ports, delete_port );
  xfree( delete_port );

  // delete switch when there is no port
  if ( sw->ports == NULL ) {
    delete_switch( switches, sw );
  }
}

void
update_port( port_info *port, uint8_t external ) {
  assert( port != 0 );
  UNUSED( port );
  UNUSED( external );
  //port->external_link = ( external == TD_PORT_EXTERNAL );
}

void
add_port( list_element **switches, uint64_t dpid, uint16_t port_no, const char *name, uint8_t external, uint64_t port_setting_feature_rate ) {
  assert( port_no != 0 );

  info( "[update_port] Adding a port: dpid = %#" PRIx64 ", port = %u ( \"%s\" )", dpid, port_no, name );

  // lookup switch
  switch_info *sw = lookup_switch( *switches, dpid );
  if ( sw == NULL ) {
    sw = allocate_switch( dpid );
    append_to_tail( switches, sw );
  }

  port_info *new_port = allocate_port( dpid, port_no, port_setting_feature_rate );
  update_port( new_port, external );
  append_to_tail( &sw->ports, new_port );
}

void
delete_all_ports( list_element **switches ) {
  if ( switches != NULL ) {
    for ( list_element *s = *switches ; s != NULL ; s = s->next ) {
      switch_info *sw = s->data;
      for ( list_element *p = sw->ports; p != NULL; p = p->next ) {
        xfree( p->data );
      }
      delete_list( sw->ports );

      xfree( sw );
    }

    delete_list( *switches );

    *switches = NULL;
  }
}

void
init_ports( list_element **switches, size_t n_entries, const topology_port_status *s, 
        uint64_t port_setting_feature_rate ) {
  for ( size_t i = 0; i < n_entries; i++ ) {
    if ( s[ i ].status == TD_PORT_UP ) {
      add_port( switches, s[ i ].dpid, s[ i ].port_no, s[ i ].name, s[ i ].external,
              port_setting_feature_rate );
    }
  }
}
// ******************************** Port Ends ******************************************


void 
dump_phy_port(uint64_t datapath_id, struct ofp_phy_port *phy_port) {
    info( "[dump_phy_port]" );
    info( "[%#016" PRIx64 "] port = %u, curr = %u, advertised = %u, supported = %u, peer = %u",
            datapath_id, phy_port->port_no, phy_port->curr, phy_port->advertised, 
            phy_port->supported, phy_port->peer );
}

/* for dyanmic routing purpose */
void                                                                                                                                                                      
dump_flow_stats( uint64_t datapath_id, struct ofp_flow_stats *stats ) {                                                                                                         
                                                                                                                                                                         
  info( "[dump_flow_stats]" );                                                                                                                                              
  info( "[%#016" PRIx64 "] priority = %u, match = [%s], actions = [%s]",                                                                                                         
        datapath_id, stats->priority, get_static_match_str( &stats->match ), get_static_actions_str( stats ) );                                                                                                                  
}


/* for dyanmic routing purpose */
void                                                                                                                                                                      
dump_port_stats( uint64_t datapath_id, struct ofp_port_stats *stats ) {                                                                                                                                                                                                                                                                                   

  UNUSED(datapath_id);
  UNUSED(stats);
  
  debug( "[dump_port_stats]" );
  debug( "[%#016" PRIx64 "] [port_no = %i, pad = %i, rx_packets = %i, tx_packets = %i, \
rx_bytes = %i, tx_bytes = %i, rx_dropped = %i, tx_dropped = %i, rx_errors = %i, \
tx_errors = %i, rx_frame_err = %i, rx_over_err = %i, rx_crc_err = %i, collisions = %i]", 
        datapath_id, 
        stats->port_no, 
        stats->pad, 
        stats->rx_packets,
        stats->tx_packets,
        stats->rx_bytes,
        stats->tx_bytes,
        stats->rx_dropped,
        stats->tx_dropped,
        stats->rx_errors,
        stats->tx_errors,
        stats->rx_frame_err,
        stats->rx_over_err,
        stats->rx_crc_err,
        stats->collisions );
}

uint32_t
pickup_the_support_data( struct ofp_phy_port *port ) {
  if (port->curr > 0) {
    return port->curr;
  }
  if (port->advertised > 0) {
    return port->advertised;
  }
  if (port->supported > 0) {
    return port->supported;
  }
  if (port->peer > 0) {
    return port->peer;
  }
  return DEFAULT_SUPPORT_RATE_DATA;
}


/* 
 * Calculate port rate support from integer to binary mapping
 */
uint64_t
calculate_port_rate_support(struct ofp_phy_port *port, uint64_t port_setting_feature_rate ) {
    
    if ( port_setting_feature_rate > 0 )
        return port_setting_feature_rate;
    
    uint32_t quotient = 0;
    uint16_t remainder = 0;
    uint64_t rate_support = 0;
    uint32_t value = 0;
    
    value = pickup_the_support_data( port );
    
    for ( int i = 0; i < 12; i++ ) {
      quotient = value / 2;
      remainder = value % 2;
      
     /* For instance 
     * curr support:192 convert to binary will be 11000000, it means it is 10GBit/sec full-duplex or Copper medium
     * OFPPF_10MB_HD = 1 << 0, // 10 Mb half-duplex rate support. 
     * OFPPF_10MB_FD = 1 << 1, // 10 Mb full-duplex rate support. 
     * OFPPF_100MB_HD = 1 << 2, // 100 Mb half-duplex rate support. 
     * OFPPF_100MB_FD = 1 << 3, // 100 Mb full-duplex rate support. 
     * OFPPF_1GB_HD = 1 << 4, // 1 Gb half-duplex rate support. 
     * OFPPF_1GB_FD = 1 << 5, // 1 Gb full-duplex rate support. 
     * OFPPF_10GB_FD = 1 << 6, // 10 Gb full-duplex rate support. 
     * OFPPF_COPPER = 1 << 7, // Copper medium. 
     * OFPPF_FIBER = 1 << 8, // Fiber medium. 
     * OFPPF_AUTONEG = 1 << 9, // Auto-negotiation. 
     * OFPPF_PAUSE = 1 << 10, // Pause. 
     * OFPPF_PAUSE_ASYM = 1 << 11 // Asymmetric pause.
     */
      
      if (remainder == 1) {
        switch (i) {
          case 0:
          case 1:
            rate_support = 10000000; //10Mb
            break;
          case 2:
          case 3:
            rate_support = 100000000; //100Mb
            break;
          case 4:
          case 5:
            rate_support = 1000000000; //1Gb
            break;            
          case 6:
          case 7:
          case 8:
          case 9:
            rate_support = 10000000000; // 10Gb
            break;
          default:
            rate_support = 0; // Pause or Asymmetric pause
            break;
        }
      }
      if (quotient == 0)
        break;
      value = quotient;
    }
    
    return rate_support;
}

static void
calculate_avg_port_traffic( port_info *port, int time_period ) {
  uint64_t diff_value;
  
  // Data could be reset to start from 0 so that we need to deal with this situation
  if (port->curr_rx_packets >= port->prev_rx_packets ) {
    diff_value = port->curr_rx_packets - port->prev_rx_packets;
  } else {
    diff_value = UINT64_MAX - port->prev_rx_packets + port->curr_rx_packets;
  }
  port->avg_rx_packets = diff_value / time_period;
    
  if (port->curr_tx_packets >= port->prev_tx_packets ) {
    diff_value = port->curr_tx_packets - port->prev_tx_packets;
  } else {
    diff_value = UINT64_MAX - port->prev_tx_packets + port->curr_tx_packets;
  }
  port->avg_tx_packets = diff_value / time_period;
    
  if (port->curr_rx_bytes >= port->prev_rx_bytes ) {
    diff_value = port->curr_rx_bytes - port->prev_rx_bytes;
  } else {
    diff_value = UINT64_MAX - port->prev_rx_bytes + port->curr_rx_bytes;
  }
  port->avg_rx_bytes = diff_value / time_period;
   
  if (port->curr_tx_bytes >= port->prev_tx_bytes ) {
    diff_value = port->curr_tx_bytes - port->prev_tx_bytes;
  } else {
    diff_value = UINT64_MAX - port->prev_tx_bytes + port->curr_tx_bytes;
  }
  port->avg_tx_bytes = diff_value / time_period;
    
  if( port->port_feature_rate > 0 ){                     
    /* by bit to calculate */
    port->port_bit_rate = port->avg_rx_bytes * 8;
    port->loading = ( uint16_t ) ( port->port_bit_rate * 100 / port->port_feature_rate );
  }
}

/* for dyanmic routing purpose */
void
update_port_stats( void *user_data, uint64_t datapath_id, port_info *port, 
        struct ofp_port_stats *stats, int time_period ) {
  UNUSED( datapath_id );
  assert( user_data != NULL );
  
  if ( port != NULL ) {
    port->prev_rx_packets = port->curr_rx_packets;
    port->prev_tx_packets = port->curr_tx_packets;
    port->prev_rx_bytes = port->curr_rx_bytes;
    port->prev_tx_bytes = port->curr_tx_bytes;
    
    port->curr_rx_packets = stats->rx_packets;
    port->curr_tx_packets = stats->tx_packets;
    port->curr_rx_bytes = stats->rx_bytes;
    port->curr_tx_bytes = stats->tx_bytes;
    
    calculate_avg_port_traffic( port, time_period );
  }
}

void
update_flow_stats( void *user_data, uint64_t datapath_id, flow_info *flow, 
        struct ofp_flow_stats *stats ) {
  assert( user_data != NULL );
  
  monitoring_switch *my_monitoring_switch = user_data;
  debug( "[update_flow_stats] dpid = %#" PRIx64 ", match = %s",
         datapath_id, get_static_match_str( &stats->match ) );
  
  if( flow == NULL ) {
    add_flow( &my_monitoring_switch->switches, datapath_id, stats );
  } else {
    update_flow( flow, stats );
    /* We both delete flow element by reeciving flow_removed message 
     * or by ourselves to delete_flow() */
    //if ( fl->flow_stats.duration_sec >= fl->flow_stats.hard_timeout - time_period ) {
    //  delete_flow( &my_monitoring_switch->switches, fl );
    //  return;
    //}
  } 
}

/**
 * It will get only one output port number from flow stats
 * @param actions
 * @param actions_length
 * @return port number
 */
uint16_t
get_outport_from_flowstats( const struct ofp_action_header *actions, uint16_t actions_length ){
  uint16_t ret = 0;
  int action_count = 0;
  size_t offset = 0;
  while ( ( actions_length - offset ) >= sizeof( struct ofp_action_header ) ) {
    const struct ofp_action_header *header = ( const struct ofp_action_header * ) ( ( const char * ) actions + offset );
    switch( header->type ) {
      case OFPAT_OUTPUT:
        action_count++;
        // We don't want to deal with multicast situation for the case OFPAT_OUTPUT
        if( action_count > 1 ) { 
          return 0;
        }
        struct ofp_action_output *action_output = header;
        ret = action_output->port;
      default:
        break;
    }
  }
  return ret;
}

