/*
 * Broadcast helper application.
 *
 * Author: Shuji Ishii
 *
 * Copyright (C) 2008-2011 NEC Corporation
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


#include <arpa/inet.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "trema.h"
#include "libtopology.h"
#include "port.h"
#include "topology_service_interface_option_parser.h"


typedef struct {
  list_element *switches;

  bool second_stage_down;
  bool last_stage_down;
} broadcast_helper;


static void
send_packet_out( uint64_t datapath_id, openflow_actions *actions, const buffer *original ) {
  const uint32_t transaction_id = get_transaction_id();
  const uint32_t buffer_id = UINT32_MAX;
  const uint16_t in_port = OFPP_NONE;
  buffer *copy = NULL;
  const buffer *packet = original;

  if ( original->length + ETH_FCS_LENGTH < ETH_MINIMUM_LENGTH ) {
    copy = duplicate_buffer( original );
    fill_ether_padding( copy );
    packet = copy;
  }
  buffer *packet_out = create_packet_out( transaction_id, buffer_id, in_port,
                                          actions, packet );

  send_openflow_message( datapath_id, packet_out );

  free_buffer( packet_out );
  if ( copy != NULL ) {
    free_buffer( copy );
  }
}


static void
port_status_updated( void *user_data, const topology_port_status *status ) {
  assert( user_data != NULL );
  assert( status != NULL );

  broadcast_helper *broadcast_helper = user_data;
  if ( broadcast_helper->second_stage_down == false ) {
    return;
  }

  debug( "Port status updated: dpid:%#" PRIx64 ", port:%u, %s, %s",
         status->dpid, status->port_no,
         ( status->status == TD_PORT_UP ? "up" : "down" ),
         ( status->external == TD_PORT_EXTERNAL ? "external" : "internal or inactive" ) );

  if ( status->port_no > OFPP_MAX && status->port_no != OFPP_LOCAL ) {
    warn( "Ignore this update ( port = %u )", status->port_no );
    return;
  }

  port_info *p = lookup_port( broadcast_helper->switches, status->dpid, status->port_no );

  if ( status->status == TD_PORT_UP ) {
    if ( p != NULL ) {
      update_port( p, status->external );
      return;
    }
    add_port( &broadcast_helper->switches, status->dpid, status->port_no, status->external );
  }
  else {
    if ( p == NULL ) {
      debug( "Ignore this update (not found nor already deleted)" );
      return;
    }
    delete_port( &broadcast_helper->switches, p );
  }
}


static int
build_packet_out_actions( port_info *port, openflow_actions *actions, uint64_t dpid, uint16_t in_port ) {
  const uint16_t max_len = UINT16_MAX;
  if ( !port->external_link || port->switch_to_switch_reverse_link ) {
    // don't send to not external port
    return 0;
  }
  if ( port->dpid == dpid && port->port_no == in_port ) {
    // don't send to input port
    return 0;
  }

  append_action_output( actions, port->port_no, max_len );
  return 1;
}


static void
send_packet_out_for_each_switch( switch_info *sw, const buffer *packet, uint64_t dpid, uint16_t in_port ) {
  openflow_actions *actions = create_actions();
  int number_of_actions = foreach_port( sw->ports, build_packet_out_actions, actions, dpid, in_port );

  // check if no action is build
  if ( number_of_actions > 0 ) {
    send_packet_out( sw->dpid, actions, packet );
  }
  delete_actions( actions );
}


static void
flood_packet( uint64_t datapath_id, uint16_t in_port, const buffer *packet, list_element *switches ) {
  foreach_switch( switches, send_packet_out_for_each_switch, packet, datapath_id, in_port );
}


static void
handle_packet_in( uint64_t datapath_id, uint32_t transaction_id,
                  uint32_t buffer_id, uint16_t total_len,
                  uint16_t in_port, uint8_t reason, const buffer *data,
                  void *user_data ) {
  assert( in_port != 0 );
  assert( data != NULL );
  assert( user_data != NULL );

  broadcast_helper *broadcast_helper = user_data;

  debug( "Packet-In received ( datapath_id = %#" PRIx64 ", transaction_id = %#" PRIx32 ", "
         "buffer_id = %#" PRIx32 ", total_len = %u, in_port = %u, reason = %#x, "
         "data_len = %zu ).", datapath_id, transaction_id, buffer_id,
         total_len, in_port, reason, data->length );

  if ( in_port > OFPP_MAX && in_port != OFPP_LOCAL ) {
    error( "Packet-In from invalid port ( in_port = %u ).", in_port );
    return;
  }

  const port_info *port = lookup_port( broadcast_helper->switches, datapath_id, in_port );
  if ( port == NULL ) {
    debug( "Ignoring Packet-In from unknown port." );
    return;
  }

  if ( !port->external_link || port->switch_to_switch_reverse_link ) {
    debug( "Ignoring Packet-In from not external link." );
    return;
  }

  // Host's location is unknown, so flood packet
  flood_packet( datapath_id, in_port, data, broadcast_helper->switches );
}


static void
init_ports( list_element **switches, size_t n_entries, const topology_port_status *s ) {
  for ( size_t i = 0; i < n_entries; i++ ) {
    if ( s[ i ].status == TD_PORT_UP ) {
      add_port( switches, s[ i ].dpid, s[ i ].port_no, s[ i ].external );
    }
  }
}


static void
update_port_status_by_link( list_element *switches, const topology_link_status *s ) {
  port_info *port = lookup_port( switches, s->from_dpid, s->from_portno );
  if ( port != NULL ) {
    debug( "Link status updated: dpid:%#" PRIx64 ", port:%u, %s",
           s->from_dpid, s->from_portno,
           ( s->status == TD_LINK_UP ? "up" : "down" ) );
    port->switch_to_switch_link = ( s->status == TD_LINK_UP );
  }
  if ( s->to_portno == 0 ) {
    return;
  }
  port_info *peer = lookup_port( switches, s->to_dpid, s->to_portno );
  if ( peer != NULL ) {
    debug( "Reverse link status updated: dpid:%#" PRIx64 ", port:%u, %s",
           s->to_dpid, s->to_portno,
           ( s->status == TD_LINK_UP ? "up" : "down" ) );
    peer->switch_to_switch_reverse_link = ( s->status == TD_LINK_UP );
  }
}


static void
link_status_updated( void *user_data, const topology_link_status *status ) {
  assert( user_data != NULL );
  assert( status != NULL );

  broadcast_helper *broadcast_helper = user_data;
  if ( broadcast_helper->last_stage_down == false ) {
    return;
  }

  update_port_status_by_link( broadcast_helper->switches, status );
}


static void
update_link_status( broadcast_helper *broadcast_helper, size_t n_entries,
                              const topology_link_status *status ) {
  for ( size_t i = 0; i < n_entries; i++ ) {
    update_port_status_by_link( broadcast_helper->switches, &status[ i ] );
  }
}


static void
init_last_stage( void *user_data, size_t n_entries, const topology_link_status *status ) {
  assert( user_data != NULL );

  broadcast_helper *broadcast_helper = user_data;
  broadcast_helper->last_stage_down = true;

  update_link_status( broadcast_helper, n_entries, status );
}


static void
init_second_stage( void *user_data, size_t n_entries, const topology_port_status *s ) {
  assert( user_data != NULL );

  broadcast_helper *broadcast_helper = user_data;
  broadcast_helper->second_stage_down = true;

  // Initialize ports
  init_ports( &broadcast_helper->switches, n_entries, s );

  // Set asynchronous event handlers

  // (1) Set packet-in handler
  set_packet_in_handler( handle_packet_in, broadcast_helper );

  // (2) Get all link status
  add_callback_link_status_updated( link_status_updated, broadcast_helper );
  get_all_link_status( init_last_stage, broadcast_helper );
}


static void
after_subscribed( void *user_data ) {
  assert( user_data != NULL );

  // Get all ports' status
  // init_last_stage() will be called
  add_callback_port_status_updated( port_status_updated, user_data );
  get_all_port_status( init_second_stage, user_data );
}


static broadcast_helper *
create_broadcast_helper( const char *topology_service ) {
  assert( topology_service != NULL );

  // Allocate broadcast_helper object
  broadcast_helper *broadcast_helper = xmalloc( sizeof( broadcast_helper ) );
  broadcast_helper->switches = NULL;
  broadcast_helper->second_stage_down = false;
  broadcast_helper->last_stage_down = false;

  // Initialize port database
  broadcast_helper->switches = create_ports( &broadcast_helper->switches );

  // Initialize libraries
  init_libtopology( topology_service );

  // Ask topology manager to notify any topology change events.
  // after_subscribed() will be called
  subscribe_topology( after_subscribed, broadcast_helper );

  return broadcast_helper;
}


static void
delete_broadcast_helper( broadcast_helper *broadcast_helper ) {
  assert( broadcast_helper != NULL );

  // Finalize libraries
  finalize_libtopology();

  // Delete ports
  delete_all_ports( &broadcast_helper->switches );

  // Delete broadcast_helper object
  xfree( broadcast_helper );
}


void
usage() {
  topology_service_interface_usage( get_executable_name(), "Broadcast helper.", NULL );
}


int
main( int argc, char *argv[] ) {
  // Initialize Trema world
  init_trema( &argc, &argv );
  init_topology_service_interface_options( &argc, &argv );

  // Initialize broadcast_helper
  broadcast_helper *broadcast_helper = create_broadcast_helper( get_topology_service_interface_name() );

  // Main loop
  start_trema();

  // Finalize broadcast_helper
  delete_broadcast_helper( broadcast_helper );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
