/*
 * Sample routing switch application.
 *
 * This application provides a switching HUB function using multiple
 * OpenFlow switches.
 *
 * Author: Shuji Ishii, Yasunobu Chiba
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
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "trema.h"
#include "fdb.h"
#include "path_resolver.h"
#include "libtopology.h"
#include "path_manager.h"
#include "port.h"
#include "topology_service_interface_option_parser.h"


static const uint16_t FLOW_TIMER = 60;
static const uint16_t PACKET_IN_DISCARD_DURATION = 1;


typedef struct routing_switch_options {
  uint16_t idle_timeout;
} routing_switch_options;


typedef struct routing_switch {
  uint16_t idle_timeout;
  list_element *switches;
  hash_table *fdb;
  path_resolver *path_resolver;

  bool second_stage_down;
  bool last_stage_down;
} routing_switch;


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
output_packet( const buffer *packet, uint64_t dpid, uint16_t port_no ) {
  openflow_actions *actions = create_actions();
  append_action_output( actions, port_no, UINT16_MAX );

  send_packet_out( dpid, actions, packet );
  delete_actions( actions );
}


static void
output_packet_from_last_switch( dlist_element *hops, const buffer *packet ) {
  dlist_element *e = get_last_element( hops );
  path_resolver_hop *last_hop = e->data;
  output_packet( packet, last_hop->dpid, last_hop->out_port_no );
}


static void
discard_packet_in( uint64_t datapath_id, uint16_t in_port, const buffer *packet ) {
  const uint32_t wildcards = 0;
  struct ofp_match match;
  set_match_from_packet( &match, in_port, wildcards, packet );
  char match_str[ 1024 ];
  match_to_string( &match, match_str, sizeof( match_str ) );

  info( "Discarding packets for a certain period ( datapath_id = %#" PRIx64
        ", match = [%s], duration = %u [sec] ).", datapath_id, match_str, PACKET_IN_DISCARD_DURATION );

  buffer *flow_mod = create_flow_mod( get_transaction_id(), match, get_cookie(),
                                      OFPFC_ADD, 0, PACKET_IN_DISCARD_DURATION,
                                      UINT16_MAX, UINT32_MAX,
                                      OFPP_NONE, 0, NULL );

  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
}


static int
count_hops( dlist_element *hops ) {
  int i = 0;
  for ( dlist_element *e = get_first_element( hops ); e != NULL; e = e->next ) {
    i++;
  }

  return i;
}


static void
make_path( routing_switch *routing_switch, uint64_t in_datapath_id, uint16_t in_port,
           uint64_t out_datapath_id, uint16_t out_port, const buffer *packet ) {
  dlist_element *hops = resolve_path( routing_switch->path_resolver, in_datapath_id, in_port, out_datapath_id, out_port );

  if ( hops == NULL ) {
    warn( "No available path found ( %#" PRIx64 ":%u -> %#" PRIx64 ":%u ).",
          in_datapath_id, in_port, out_datapath_id, out_port );
    discard_packet_in( in_datapath_id, in_port, packet );
    return;
  }

  // ask path manager to install flow entries
  size_t length = offsetof( path_manager_path, hops ) + sizeof( path_manager_hop ) * count_hops( hops );
  path_manager_path *path = xmalloc( length );
  set_match_from_packet( &path->match, OFPP_NONE, 0, packet );
  path->n_hops = count_hops( hops );
  dlist_element *e = get_first_element( hops );
  for( int i = 0; e != NULL; e = e->next, i++ ) {
    path_resolver_hop *hop = e->data;
    path->hops[ i ].datapath_id = hop->dpid;
    path->hops[ i ].in_port = hop->in_port_no;
    path->hops[ i ].out_port = hop->out_port_no;
  }
  send_message( PATH_SETUP_SERVICE_NAME, MESSENGER_PATH_SETUP_REQUEST, ( void * ) path, length );
  xfree( path );

  // send packet out to tail switch
  output_packet_from_last_switch( hops, packet );

  // free hop list
  free_hop_list( hops );
}


static void
port_status_updated( void *user_data, const topology_port_status *status ) {
  assert( user_data != NULL );
  assert( status != NULL );

  routing_switch *routing_switch = user_data;
  if ( routing_switch->second_stage_down == false ) {
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

  port_info *p = lookup_port( routing_switch->switches, status->dpid, status->port_no );

  delete_fdb_entries( routing_switch->fdb, status->dpid, status->port_no );

  if ( status->status == TD_PORT_UP ) {
    if ( p != NULL ) {
      update_port( p, status->external );
      return;
    }
    add_port( &routing_switch->switches, status->dpid, status->port_no, status->external );
  }
  else {
    if ( p == NULL ) {
      debug( "Ignore this update (not found nor already deleted)" );
      return;
    }
    delete_port( &routing_switch->switches, p );
  }
}


static int
build_packet_out_actions( port_info *port, openflow_actions *actions, uint64_t dpid, uint16_t in_port ) {
  if ( !port->external_link || port->switch_to_switch_reverse_link ) {
    // don't send to non-external port
    return 0;
  }
  if ( port->dpid == dpid && port->port_no == in_port ) {
    // don't send to input port
    return 0;
  }

  append_action_output( actions, port->port_no, UINT16_MAX );
  return 1;
}


static void
send_packet_out_for_each_switch( switch_info *sw, const buffer *packet, uint64_t dpid, uint16_t in_port ) {
  openflow_actions *actions = create_actions();
  int number_of_actions = foreach_port( sw->ports, build_packet_out_actions, actions, dpid, in_port );

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

  routing_switch *routing_switch = user_data;

  debug( "Packet-In received ( datapath_id = %#" PRIx64 ", transaction_id = %#" PRIx32 ", "
         "buffer_id = %#" PRIx32 ", total_len = %u, in_port = %u, reason = %#x, "
         "data_len = %u ).", datapath_id, transaction_id, buffer_id,
         total_len, in_port, reason, data->length );

  const port_info *port = lookup_port( routing_switch->switches, datapath_id, in_port );
  if ( port == NULL ) {
    debug( "Ignoring Packet-In from unknown port." );
    return;
  }

  packet_info packet_info = get_packet_info( data );
  const uint8_t *src = packet_info.eth_macsa;
  const uint8_t *dst = packet_info.eth_macda;

  if ( in_port > OFPP_MAX && in_port != OFPP_LOCAL ) {
    error( "Packet-In from invalid port ( in_port = %u ).", in_port );
    return;
  }
  if ( !port->external_link || port->switch_to_switch_reverse_link ) {
    if ( !port->external_link
         && port->switch_to_switch_link
         && port->switch_to_switch_reverse_link
         && !is_ether_multicast( dst )
         && lookup_fdb( routing_switch->fdb, src, &datapath_id, &in_port ) ) {
      debug( "Found a Packet-In from switch-to-switch link." );
    }
    else {
      debug( "Ignoring Packet-In from non-external link." );
      return;
    }
  }

  if ( !update_fdb( routing_switch->fdb, src, datapath_id, in_port ) ) {
    return;
  }

  uint16_t out_port;
  uint64_t out_datapath_id;

  if ( lookup_fdb( routing_switch->fdb, dst, &out_datapath_id, &out_port ) ) {
    if ( ( datapath_id == out_datapath_id ) && ( in_port == out_port ) ) {
      // in and out are same
      return;
    }
    // Host is located, so resolve path and install flow entries
    make_path( routing_switch, datapath_id, in_port, out_datapath_id, out_port, data );
  }
  else {
    // Host's location is unknown, so flood packet
    flood_packet( datapath_id, in_port, data, routing_switch->switches );
  }
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

  routing_switch *routing_switch = user_data;
  if ( routing_switch->last_stage_down == false ) {
    return;
  }

  update_topology( routing_switch->path_resolver, status );
  update_port_status_by_link( routing_switch->switches, status );
}


static void
update_link_status( routing_switch *routing_switch, size_t n_entries,
                              const topology_link_status *status ) {
  for ( size_t i = 0; i < n_entries; i++ ) {
    update_topology( routing_switch->path_resolver, &status[ i ] );
    update_port_status_by_link( routing_switch->switches, &status[ i ] );
  }
}


static void
init_last_stage( void *user_data, size_t n_entries, const topology_link_status *status ) {
  assert( user_data != NULL );

  routing_switch *routing_switch = user_data;
  routing_switch->last_stage_down = true;

  update_link_status( routing_switch, n_entries, status );
}


static void
init_second_stage( void *user_data, size_t n_entries, const topology_port_status *s ) {
  assert( user_data != NULL );

  routing_switch *routing_switch = user_data;
  routing_switch->second_stage_down = true;

  // Initialize ports
  init_ports( &routing_switch->switches, n_entries, s );

  // Initialize aging FDB
  init_age_fdb( routing_switch->fdb );

  // Set packet-in handler
  set_packet_in_handler( handle_packet_in, routing_switch );

  // Get all link status
  add_callback_link_status_updated( link_status_updated, routing_switch );
  get_all_link_status( init_last_stage, routing_switch );
}


static void
after_subscribed( void *user_data ) {
  assert( user_data != NULL );

  // Get all ports' status
  // init_second_stage() will be called
  add_callback_port_status_updated( port_status_updated, user_data );
  get_all_port_status( init_second_stage, user_data );
}


static routing_switch *
create_routing_switch( const char *topology_service, const routing_switch_options *options ) {
  assert( topology_service != NULL );
  assert( options != NULL );

  // Allocate routing_switch object
  routing_switch *routing_switch = xmalloc( sizeof( struct routing_switch ) );
  routing_switch->idle_timeout = options->idle_timeout;
  routing_switch->switches = NULL;
  routing_switch->fdb = NULL;
  routing_switch->second_stage_down = false;
  routing_switch->last_stage_down = false;

  info( "idle_timeout is set to %u [sec].", routing_switch->idle_timeout );

  // Create path_resolver table
  routing_switch->path_resolver = create_path_resolver();

  // Create forwarding database
  routing_switch->fdb = create_fdb();

  // Initialize port database
  routing_switch->switches = create_ports( &routing_switch->switches );

  // Initialize libraries
  init_libtopology( topology_service );

  // Ask topology manager to notify any topology change events.
  // after_subscribed() will be called
  subscribe_topology( after_subscribed, routing_switch );

  return routing_switch;
}


static void
delete_routing_switch( routing_switch *routing_switch ) {
  assert( routing_switch != NULL );

  // Delete pathresolver table
  delete_path_resolver( routing_switch->path_resolver );

  // Finalize libraries
  finalize_libtopology();

  // Delete ports
  delete_all_ports( &routing_switch->switches );

  // Delete forwarding database
  delete_fdb( routing_switch->fdb );

  // Delete routing_switch object
  xfree( routing_switch );
}


static char option_description[] = "  -i, --idle_timeout=TIMEOUT      idle timeout value of flow entry\n";
static char short_options[] = "i:";
static struct option long_options[] = {
  { "idle_timeout", 1, NULL, 'i' },
  { NULL, 0, NULL, 0  },
};


static void
reset_getopt() {
  optind = 0;
  opterr = 1;
}


void
usage() {
  topology_service_interface_usage( get_executable_name(), "Switching HUB.", option_description );
}


static void
init_routing_switch_options( routing_switch_options *options, int *argc, char **argv[] ) {
  assert( options != NULL );
  assert( argc != NULL );
  assert( *argc >= 0 );
  assert( argv != NULL );

  // set default values
  options->idle_timeout = FLOW_TIMER;

  int argc_tmp = *argc;
  char *new_argv[ *argc ];

  for ( int i = 0; i <= *argc; ++i ) {
    new_argv[ i ] = ( *argv )[ i ];
  }

  int c;
  uint32_t idle_timeout;
  while ( ( c = getopt_long( *argc, *argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'i':
        idle_timeout = ( uint32_t ) atoi( optarg );
        if ( idle_timeout == 0 || idle_timeout > UINT16_MAX ) {
          printf( "Invalid idle_timeout value.\n" );
          usage();
          finalize_topology_service_interface_options();
          exit( EXIT_FAILURE );
          return;
        }
        options->idle_timeout = ( uint16_t ) idle_timeout;
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
}


int
main( int argc, char *argv[] ) {
  // Initialize Trema world
  init_trema( &argc, &argv );

  // Setup options
  routing_switch_options options;
  init_routing_switch_options( &options, &argc, &argv );
  init_topology_service_interface_options( &argc, &argv );

  // Initialize routing_switch
  routing_switch *routing_switch = create_routing_switch( get_topology_service_interface_name(), &options );

  // Main loop
  start_trema();

  // Finalize routing_switch
  delete_routing_switch( routing_switch );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
