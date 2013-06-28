/*
 * Monitoring
 *
 * Author: TeYen Liu
 *
 * Copyright (C) 2013 TeYen Liu
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
#include <getopt.h>
#include <inttypes.h>
#include <netinet/in.h>
#include "monitoring.h"
#include "libmonitoring.h"
#include "monitoring_interface.h"
#include "utilmonitoring.h"
#include "libtopology.h"
#include "topology_service_interface_option_parser.h"


typedef struct {
  uint64_t flow_bit_rate_conditon;
  uint16_t flow_times_condition;
  uint64_t port_setting_feature_rate;
  uint16_t port_percentage_condition;
  
} monitoring_options;


// it is related with the routing factor
int const MY_FEATURE_AGING_INTERVAL = 10;
int const MY_STATS_AGING_INTERVAL = 4;

static monitoring_switch *global_monitoring_switch;


// ******************************** Send Notification Starts ****************************************
static void
send_port_loading_notification( port_info *port, void *user_data ) {

  monitoring_switch *my_monitoring_switch = user_data;
  port_loading my_port_loading;
  
  /* Prepare Port Loading Information */
  my_port_loading.avg_rx_bytes = port->avg_rx_bytes;
  my_port_loading.avg_tx_bytes = port->avg_tx_bytes;
  my_port_loading.avg_rx_packets = port->avg_rx_packets;
  my_port_loading.avg_tx_packets = port->avg_tx_packets;
  
  my_port_loading.curr = port->phy_port.curr;
  my_port_loading.supported = port->phy_port.supported;
  my_port_loading.advertised = port->phy_port.advertised;
  my_port_loading.peer = port->phy_port.peer;
  
  my_port_loading.port_feature_rate = port->port_feature_rate;
  my_port_loading.port_bit_rate = port->port_bit_rate;
  my_port_loading.loading = port->loading;
  
  // Send message to who uses libmonitoring for notification
  buffer *notification = 
          create_monitoring_port_loading_notification( port->dpid, port->port_no, 
          &my_port_loading, my_monitoring_switch->port_percentage_condition );
  
  char *owner;
  for ( list_element *e = my_monitoring_switch->subscribers; e != NULL; e = e->next ) {
    owner = ( char * ) e->data;
    send_message( owner, MONITORING_PORT_LOADING_NOTIFICATION,
                  notification->data, notification->length );
  }
  free_buffer( notification );
}

static void
send_flow_loading_notification( flow_info *flow, void *user_data ) {

  monitoring_switch *my_monitoring_switch = user_data;
  flow_loading my_flow_loading;
  
  my_flow_loading.match = flow->flow_stats.match;
  my_flow_loading.flow_bit_rate = flow->flow_bit_rate;
  my_flow_loading.big_flow_flag = flow->big_flow_flag;
  
  // Send message to who uses libmonitoring for notification
  buffer *notification = 
          create_monitoring_flow_loading_notification( flow->dpid, &my_flow_loading, 
          my_monitoring_switch->flow_bit_rate_conditon );
  
  char *owner;
  for ( list_element *e = my_monitoring_switch->subscribers; e != NULL; e = e->next ) {
    owner = ( char * ) e->data;
    send_message( owner, MONITORING_FLOW_LOADING_NOTIFICATION,
                  notification->data, notification->length );    
  }
  free_buffer( notification );
}
// ******************************** Send Notification Ends ******************************************

// ******************************** Check Notification Starts ******************************************
static void
check_port_loading_notification( void *user_data, port_info *port, uint16_t port_percentage_cond ) {
  if ( port->loading >  port_percentage_cond ) {
    send_port_loading_notification( port, user_data );
  }
}

/**
 * This method will calculate and check if the big flow occurs.
 * If a big flow occurs, this method first find out the port number from actions in flow_stats and 
 * check the port loading. If the loading is bigger than 90, then send flow loading notification.
 * @param user_data
 * @param flow
 * @param flow_bit_rate_cond
 * @param flow_times_condi
 * @param stats
 */
static void
check_flow_loading_notification( void *user_data, flow_info *flow, uint64_t flow_bit_rate_cond, 
        uint16_t flow_times_condi, struct ofp_flow_stats *stats ) {
    
  monitoring_switch *my_monitoring_switch = user_data;
  
  /* if the flow is null or is a big flow before, then we don't need to deal with again 
   * because the notification is notified before.   
   */
  if ( flow == NULL ) {
   return;
  }
  
  if ( flow->flow_bit_rate > flow_bit_rate_cond /* magic number */ ) {
    flow->big_flow_number_times++;
  } else {
    flow->big_flow_number_times = 0;
    flow->big_flow_flag == false;
  }
  
  /* Give a warning about big flow */
  if ( flow->big_flow_number_times > flow_times_condi && flow->big_flow_flag == false ) {
    
    // Find out the output port number
    uint16_t actions_length = (uint16_t) (stats->length - offsetof( struct ofp_flow_stats, actions ));  
    if ( actions_length > 0 ){
       uint16_t port_no = get_outport_from_flowstats( stats->actions, actions_length );
       port_info *port = lookup_port( my_monitoring_switch->switches, flow->dpid, port_no );
       if( port == NULL ){
         return;
       }
       /* FIXME: 90 is a magic number */
       if( port->loading < 90 ){
         return;
       }
    }  
    
    // So this flow loading notification is based on the port loading 90%
    flow->big_flow_flag = true;
    send_flow_loading_notification( flow, user_data );
    info( "[[...Warning...]] Big flow occurs, match = %s, flow_bit_rate = %u, big_flow_number_times = %u", 
            get_static_match_str( &flow->flow_stats.match ), 
            flow->flow_bit_rate, flow->big_flow_number_times );
  }
}
// ******************************** Check Notification Ends ******************************************

// ******************************** Send Message Starts ******************************************
static void
send_echo_reply( uint64_t datapath_id ) {
  info ( "[send_echo_reply]!" );
  buffer *echo_reply = create_echo_reply( get_transaction_id(), NULL );
  bool ret = send_openflow_message( datapath_id, echo_reply );
  if ( !ret ) {          
    error( "Failed to send an echo reply message to the switch with datapath ID = %#" PRIx64 ".", datapath_id );
  }
  free_buffer( echo_reply ); 
}

static void
send_echo_request( uint64_t datapath_id ) {
  info ( "[send_echo_request]!" );
  buffer *echo_request = create_echo_request( get_transaction_id(), NULL );
  bool ret = send_openflow_message( datapath_id, echo_request );
  if ( !ret ) {          
    error( "Failed to send an echo request message to the switch with datapath ID = %#" PRIx64 ".", datapath_id );
  }
  free_buffer( echo_request ); 
}

static void
send_features_request( void *user_data ) {
    
  monitoring_switch *my_monitoring_switch = user_data;
  
  uint32_t transaction_id = get_transaction_id();
  buffer *features_req = create_features_request(transaction_id);
  
  // Get all ports of the switch
  list_element *switches = my_monitoring_switch->switches;
  
  for ( list_element *e = switches; e != NULL; e = e->next ) {
    switch_info *sw = e->data;
    if (sw != NULL) {
        send_openflow_message( sw->dpid, features_req);
    }
  }
  free_buffer(features_req);
  
  /* We don't want send features request too many times,
   * so after the first request sent, we stop it */
  //delete_timer_event(send_features_request, NULL);
}

/* for dyanmic routing purpose */
static void
send_flow_stats_request( void *user_data ) {
    
  monitoring_switch *my_monitoring_switch = user_data;
    
  struct ofp_match match;
  
  /* Initialize match */
  memset( &match, 0, sizeof( struct ofp_match ) );
  
  /* Wildcard all fields. */
  match.wildcards = OFPFW_ALL; //0x3fffff
  
  buffer *flow_stat_req = create_flow_stats_request(
    get_transaction_id(), 
    0 /* flags */, 
    match, 0xff /* all tables */, 
    OFPP_NONE /* out_port */
  );
          
  for ( list_element *e = my_monitoring_switch->switches; e != NULL; e = e->next ) {
     switch_info *sw = e->data;
     send_openflow_message( sw->dpid, flow_stat_req );
     debug( "send flow request to datapath_id: %#" PRIx64, sw->dpid );
  }
  free_buffer( flow_stat_req );
}

/* for dyanmic routing purpose */
static void
send_port_stats_request( void *user_data ) {
  
  monitoring_switch *my_monitoring_switch = user_data;
    
  /* Get all ports of the switch */
  list_element *switches = my_monitoring_switch->switches;
  
  for ( list_element *e = switches; e != NULL; e = e->next ) {
    switch_info *sw = e->data;
    if (sw == NULL) 
      break;
    
    for ( list_element *e2 = sw->ports; e2 != NULL; e2 = e2->next ) {
      port_info *p = e2->data;
      if (p ==NULL)
        break;
      
      buffer *port_stat_req = create_port_stats_request(
      get_transaction_id(), 0, p->port_no);
      
      send_openflow_message( sw->dpid, port_stat_req );
      //info( "send port request to datapath_id: %#" PRIx64 ", port: %u", sw->dpid, p->port_no);
      free_buffer( port_stat_req );
    }
  }
}
// ******************************** Send Message Ends ******************************************

// ******************************** Handle Message Starts ******************************************
static void
handle_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                    uint16_t type, uint16_t flags, const buffer *data,
                    void *user_data ) {

  UNUSED( transaction_id );
  UNUSED( flags );
  
  debug( "[stats_reply] datapath_id: %#" PRIx64 " type: %#x flags: %#x", datapath_id, type, flags );
  monitoring_switch *my_monitoring_switch = user_data;
  
  if ( data != NULL ) {
    size_t offset = 0;
    
    switch (type) {
      case OFPST_FLOW:
        /* Individual flow statistics.
         * The request body is struct ofp_flow_stats_request.
         * The reply body is an array of struct ofp_flow_stats. */
        while ( ( data->length - offset ) >= sizeof( struct ofp_flow_stats ) ) {
          struct ofp_flow_stats *flow_stats = ( struct ofp_flow_stats * ) ( ( char * ) data->data + offset );
          flow_info *flow = lookup_flow( my_monitoring_switch->switches, datapath_id, 
                  &flow_stats->match );
          /* update flow statistics on flow_info obj */
          update_flow_stats( user_data, datapath_id, flow, flow_stats );
          check_flow_loading_notification( user_data, flow, my_monitoring_switch->flow_bit_rate_conditon,
                  my_monitoring_switch->flow_times_condition, flow_stats );
          //dump_flow_stats( datapath_id, flow_stats );
          offset += flow_stats->length;
        }
        break;
        
      case OFPST_PORT:
        /* Physical port statistics.
         * The request body is struct ofp_port_stats_request.
         * The reply body is an array of struct ofp_port_stats. */
        while ( ( data->length - offset ) >= sizeof( struct ofp_port_stats ) ) {
          struct ofp_port_stats *port_stats = ( struct ofp_port_stats * ) ( ( char * ) data->data + offset );
          port_info *port = lookup_port( my_monitoring_switch->switches, datapath_id, 
                  port_stats->port_no );
          /* update port statistics on port_info obj */
          update_port_stats(user_data, datapath_id, port, port_stats, MY_STATS_AGING_INTERVAL );
          check_port_loading_notification( user_data, port, my_monitoring_switch->port_percentage_condition );
          //dump_port_stats( datapath_id, port_stats );
          offset += sizeof(struct ofp_port_stats);
        }
        break;
      default:
          info(" Here is a type:%#x which is not handled!", type);
        break;
    }
  }
}

static void
handle_features_reply( uint64_t datapath_id, uint32_t transaction_id,
                        uint32_t n_buffers, uint8_t n_tables,
                        uint32_t capabilities, uint32_t actions,
                        const list_element *phy_ports, void *user_data ) {
  UNUSED( transaction_id );
  
  monitoring_switch *my_monitoring_switch = user_data;
  struct ofp_phy_port *port = NULL;
  
  debug( "[features_reply] datapath_id: %#" PRIx64 "n_buffers: %lu, n_tables: %u, capabilities, actions: %lu, #ports: %d", 
          datapath_id, n_buffers, n_tables, capabilities, actions, list_length_of( phy_ports ) );

  for ( const list_element *e = phy_ports; e != NULL; e = e->next ) {
    port = e->data;
    /* Dump phy_port info */
    //dump_phy_port( datapath_id, port );
    
    port_info *p = lookup_port( my_monitoring_switch->switches, datapath_id, port->port_no );
    if ( p == NULL ) {
        debug( "Ignore physical port update (not found nor already deleted)" );
    } else {
        p->phy_port = *port;
        p->port_feature_rate = calculate_port_rate_support( port, 
                my_monitoring_switch->port_setting_feature_rate );
    }
  }
}
// ******************************** Handle Message Ends ******************************************

// ******************************** Handle Request Starts ******************************************
static void
handle_subscribe_request( const messenger_context_handle *handle,
                      monitoring_subscribe_request *request ) {
  assert( handle != NULL );
  assert( request != NULL );
  
  add_subscriber( &global_monitoring_switch->subscribers, request->owner );
  info( "[handle_subscribe_request] subscriber = %s", request->owner );
}

static void
handle_unsubscribe_request( const messenger_context_handle *handle,
                      monitoring_subscribe_request *request ) {
  assert( handle != NULL );
  assert( request != NULL );
  
  delete_subscriber( &global_monitoring_switch->subscribers, request->owner );
  info( "[handle_unsubscribe_request] subscriber = %s", request->owner );
}

static void
handle_port_loading_request( const messenger_context_handle *handle,
                      monitoring_port_loading_request *request ) {
  assert( handle != NULL );
  assert( request != NULL );
  
  port_loading my_port_loading;
  
  /* Prepare Port Loading Information */
  
  port_info *p = lookup_port( global_monitoring_switch->switches, request->datapath_id, request->port_no );
  
  my_port_loading.avg_rx_bytes = p->avg_rx_bytes;
  my_port_loading.avg_tx_bytes = p->avg_tx_bytes;
  my_port_loading.avg_rx_packets = p->avg_rx_packets;
  my_port_loading.avg_tx_packets = p->avg_tx_packets;
  
  my_port_loading.curr = p->phy_port.curr;
  my_port_loading.supported = p->phy_port.supported;
  my_port_loading.advertised = p->phy_port.advertised;
  my_port_loading.peer = p->phy_port.peer;
  
  my_port_loading.port_feature_rate = p->port_feature_rate;
  my_port_loading.port_bit_rate = p->port_bit_rate;
  my_port_loading.loading = p->loading;
  
  buffer *reply = create_monitoring_port_loading_reply( request->datapath_id, 
          request->port_no, &my_port_loading );
  
  send_reply_message( handle, MONITORING_PORT_LOADING_REPLY,
                        reply->data, reply->length );
  free_buffer( reply );
}

static void
handle_monitoring_request( const messenger_context_handle *handle, uint16_t tag, void *data, size_t length ) {
  assert( handle != NULL );
  
  debug( "Handling a request ( handle = %p, tag = %#x, data = %p, length = %u ).",
         handle, tag, data, length );
  
  switch ( tag ) {
    case MONITORING_PORT_LOADING_REQUEST:
    {   
      if ( length < sizeof( monitoring_port_loading_request ) ) {
        error( "Too short setup request message ( length = %u ).", length );
        return;
      }
      monitoring_port_loading_request *request = data;
      handle_port_loading_request( handle, request );
    }
    break;
    case MONITORING_SUBSCRIBE_REQUEST:
    {
      if ( length < sizeof( monitoring_subscribe_request ) ) {
        error( "Too short setup request message ( length = %u ).", length );
        return;
      }
      monitoring_subscribe_request *request = data;
      handle_subscribe_request( handle, request );
    }
    break;
    case MONITORING_UNSUBSCRIBE_REQUEST:
    {   
      if ( length < sizeof( monitoring_unsubscribe_request ) ) {
        error( "Too short setup request message ( length = %u ).", length );
        return;
      }
      monitoring_unsubscribe_request *request = data;
      handle_unsubscribe_request( handle, request );
    }
    break;
    default:
    {
      error( "Undefined request tag ( tag = %#x ).", tag );
    }
    break;
  }
}
// ******************************** Handle Request Ends ******************************************

// ******************************** Handle Flow Removed Begins ******************************************
static void
handle_flow_removed( uint64_t datapath_id, flow_removed message ) {
  monitoring_switch *my_monitoring_switch = message.user_data;
  info( "[handle_flow_removed] dpid = %#" PRIx64 ", match = %s", 
          datapath_id, message.match );
  
  flow_info *fl = lookup_flow( my_monitoring_switch->switches, datapath_id, &message.match );
  if( fl != NULL ) {
    delete_flow( &my_monitoring_switch->switches, fl );
  }
}
// ******************************** Handle Flow Removed Ends ******************************************

void
update_topology_port_status( void *user_data, const topology_port_status *status ) {
  assert( user_data != NULL );
  assert( status != NULL );

  monitoring_switch *my_monitoring_switch = user_data;

  debug( "Port status updated: dpid:%#" PRIx64 ", port:%u(%s), %s, %s",
         status->dpid, status->port_no, status->name,
         ( status->status == TD_PORT_UP ? "up" : "down" ),
         ( status->external == TD_PORT_EXTERNAL ? "external" : "internal or inactive" ) );

  if ( status->port_no > OFPP_MAX && status->port_no != OFPP_LOCAL ) {
    warn( "Ignore this update ( port = %u )", status->port_no );
    return;
  }

  port_info *p = lookup_port( my_monitoring_switch->switches, status->dpid, status->port_no );

  if ( status->status == TD_PORT_UP ) {
    if ( p != NULL ) {
      update_port( p, status->external );
      /* Setup the port feature rate */
      return;
    }
    add_port( &my_monitoring_switch->switches, status->dpid, status->port_no, 
            status->name, status->external, my_monitoring_switch->port_setting_feature_rate );
  }
  else {
    if ( p == NULL ) {
      debug( "Ignore this update (not found nor already deleted)" );
      return;
    }
    delete_port( &my_monitoring_switch->switches, p );
  }
}

static void
init_second_stage( void *user_data, size_t n_entries, const topology_port_status *s ) {
  assert( user_data != NULL );

  monitoring_switch *my_monitoring_switch = user_data;

  // Initialize ports
  init_ports( &my_monitoring_switch->switches, n_entries, s, my_monitoring_switch->port_setting_feature_rate );

  // Set asynchronous event handlers
  // (1) Set port status update callback
  add_callback_port_status_updated( update_topology_port_status, my_monitoring_switch );

}

static void
after_subscribed( void *user_data ) {
  assert( user_data != NULL );

  // Get all ports' status
  // init_second_stage() will be called
  get_all_port_status( init_second_stage, user_data );
}


static monitoring_switch *
create_monitoring_switch (const char *topology_service,const monitoring_options *options) {
  
  // Allocate routing_switch object
  monitoring_switch *monitoring_switch = xmalloc( sizeof( struct monitoring_switch ) );
  monitoring_switch->switches = NULL;
  
  // Default important parameters
  monitoring_switch->flow_bit_rate_conditon = options->flow_bit_rate_conditon;  // bit per second
  monitoring_switch->flow_times_condition = options->flow_times_condition;
  monitoring_switch->port_setting_feature_rate = options->port_setting_feature_rate;
  monitoring_switch->port_percentage_condition = options->port_percentage_condition; // percentage
  
  // Initialize port database
  monitoring_switch->switches = create_switches( &monitoring_switch->switches );

  // Initialize libraries
  init_libtopology( topology_service );

  // Ask topology manager to notify any topology change events.
  // after_subscribed() will be called
  subscribe_topology( after_subscribed, monitoring_switch );
  
  // Initialize subscriber of monitoring information
  create_subscribers( &monitoring_switch->subscribers );
  
  return monitoring_switch;
}

static void
delete_monitoring_switch( monitoring_switch *my_monitoring_switch ) {
  assert( my_monitoring_switch != NULL );

  // Finalize libraries
  finalize_libtopology();

  // Delete ports
  delete_all_ports( &my_monitoring_switch->switches );

  // Delete routing_switch object
  xfree( my_monitoring_switch );
  
  // Delete Subscribers
  delete_subscribers( &my_monitoring_switch->subscribers );
}

static char option_description[] =
  "  -b, --flow_bit_rate_conditon=BitRate       The condition setting of flow bit rate\n"
  "  -t, --flow_times_condition=FlowTimes The times of flow that reaches the flow bit rate condition\n"
  "  -f, --port_setting_feature_rate=FeatureRate    The setting of port bandwidth\n"
  "  -p, --port_percentage_condition=PortPercnetage    The condition setting of port loading condition\n";


static char short_options[] = "b:t:f:p:";
static struct option long_options[] = {
  { "flow_bit_rate_conditon", required_argument, NULL, 'b' },
  { "flow_times_condition", required_argument, NULL, 't' },
  { "port_setting_feature_rate", required_argument, NULL, 'f' },
  { "port_percentage_condition", required_argument, NULL, 'p' },
  { NULL, 0, NULL, 0  },
};

static void
reset_getopt() {
  optind = 0;
  opterr = 1;
}

static void
init_monitoring_options( monitoring_options *options, int *argc, char **argv[] ) {
  assert( options != NULL );
  assert( argc != NULL );
  assert( *argc >= 0 );
  assert( argv != NULL );
  
  int argc_tmp = *argc;
  char *new_argv[ *argc ];

  for ( int i = 0; i <= *argc; ++i ) {
    new_argv[ i ] = ( *argv )[ i ];
  }
  
  /* set default values */
  options->flow_bit_rate_conditon = 4194304; // 4Mb per second
  options->flow_times_condition = 2; // after 2 times, big flow will be alerted
  options->port_setting_feature_rate = 104857600; // 100Mb per second
  options->port_percentage_condition = 80; // port loading threshold
  
  int c;
  uint64_t flow_bit_rate_cond = 0;
  uint16_t flow_times_cond = 0;
  uint64_t port_setting_feature_rt = 0;
  uint16_t port_percentage_cond = 0;
  
  while ( ( c = getopt_long( *argc, *argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'b':
        flow_bit_rate_cond = ( uint64_t ) atoi( optarg );
        if ( flow_bit_rate_cond == 0 || flow_bit_rate_cond > UINT64_MAX ) {
          info( "Invalid flow_bit_rate_conditon value.\n" );
          // Default: 4Mb per second
        } else {
          options->flow_bit_rate_conditon = flow_bit_rate_cond;
        }
        break;

      case 't':
        flow_times_cond = ( uint16_t ) atoi( optarg );
        if ( flow_times_cond == 0 || flow_times_cond > UINT16_MAX ) {
          info( "Invalid flow_times_condition value.\n" );
          // Default: after 2 times, big flow will be alerted
        } else {
          options->flow_times_condition = flow_times_cond;
        }
        break;
      
      case 'f':
        port_setting_feature_rt = ( uint64_t ) atoi( optarg );
        if ( port_setting_feature_rt == 0 || port_setting_feature_rt > UINT64_MAX ) {
          info( "Invalid port_setting_feature_rate value.\n" );
          // Default: 100Mb per second
        } else {
          options->port_setting_feature_rate = port_setting_feature_rt;
        }
        break;

      case 'p':
        port_percentage_cond = ( uint16_t ) atoi( optarg );
        if ( port_percentage_cond == 0 || port_percentage_cond > UINT16_MAX ) {
          info( "Invalid port_percentage_condition value.\n" );
          // Default: port loading threshold 80%
        } else {
          options->port_percentage_condition = port_percentage_cond;
        }
        break;
        
      default:
        continue;
    }
    
    if ( optarg == 0 || strchr( new_argv[ optind - 1 ], '=' ) != NULL ) {
      argc_tmp -= 1;
      new_argv[ optind - 1 ] = NULL;
    }
    else {
      argc_tmp -= 2;
      new_argv[ optind - 1 ] = NULL;
      new_argv[ optind - 2 ] = NULL;
    }
  }
  
  for ( int i = 0, j = 0; i < *argc; ++i ) {
    if ( new_argv[ i ] != NULL ) {
      ( *argv )[ j ] = new_argv[ i ];
      j++;
    }
  }

  ( *argv )[ *argc ] = NULL;
  *argc = argc_tmp;
  
  reset_getopt();
  
  info("options->flow_bit_rate_conditon:%u", options->flow_bit_rate_conditon );
  info("options->flow_times_condition:%u", options->flow_times_condition );
  info("options->port_setting_feature_rate:%u", options->port_setting_feature_rate );
  info("options->port_percentage_condition:%u", options->port_percentage_condition );
}

static void
test_list_elements() {
    info( "[test_list_elements] runs..." );
    add_subscriber( &global_monitoring_switch->subscribers, "subscriber1-Danny" );
    add_subscriber( &global_monitoring_switch->subscribers, "subscriber2-TeYen" );
    
    for ( list_element *e = global_monitoring_switch->subscribers; e != NULL; e = e->next ) {
        char *name = e->data;
        info( "[test_list_elements] subscriber: %s", name );
    }
}

int
main( int argc, char *argv[] ) {
  
  monitoring_options options;
  info( "argv[argc]:%s", argv[argc-1] );
  init_monitoring_options( &options, &argc, &argv );
  info( "argv[argc]:%s", argv[argc-1] );
  init_trema( &argc, &argv );
  init_topology_service_interface_options( &argc, &argv );
  monitoring_switch *my_monitoring_switch = create_monitoring_switch(get_topology_service_interface_name(), &options);
  global_monitoring_switch = my_monitoring_switch;
  
  // Query Information
  set_flow_removed_handler( handle_flow_removed, my_monitoring_switch );
  set_features_reply_handler( handle_features_reply, my_monitoring_switch );
  set_stats_reply_handler( handle_stats_reply, my_monitoring_switch );
  
  // Add periodic callback function
  add_periodic_event_callback( MY_STATS_AGING_INTERVAL, send_flow_stats_request, my_monitoring_switch );
  add_periodic_event_callback( MY_STATS_AGING_INTERVAL, send_port_stats_request, my_monitoring_switch );
  add_periodic_event_callback( MY_FEATURE_AGING_INTERVAL /* as early as possible */, send_features_request, my_monitoring_switch );
  
  // Add a callback for handling requests from peers
  add_message_requested_callback( MONITORING_SERVICE, handle_monitoring_request );
  //add_periodic_event_callback( 4, test_list_elements, NULL );
  start_trema();

  // Finalize routing_switch
  delete_monitoring_switch( my_monitoring_switch );
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
