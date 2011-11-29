/*
 * Flow dumper application.
 * 
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2011 NEC Corporation
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
#include <inttypes.h>
#include "trema.h"


static int n_switches = 0;


static void
handle_list_switches_reply( const list_element *switches, void *user_data ) {
  UNUSED( user_data );

  struct ofp_match match;
  memset( &match, 0, sizeof( struct ofp_match ) );
  match.wildcards = OFPFW_ALL;
  buffer *stats_request = create_flow_stats_request( get_transaction_id(), 0, match, 0xff, OFPP_NONE );

  n_switches = 0;
  const list_element *element;
  for ( element = switches; element != NULL; element = element->next ) {
    uint64_t datapath_id = *( uint64_t * ) element->data;
    send_openflow_message( datapath_id, stats_request );
    n_switches++;
  }

  free_buffer( stats_request );
}


static void
dump_flow_stats( uint64_t datapath_id, struct ofp_flow_stats *stats ) {
  char match_str[ 256 ];
  memset( match_str, '\0', sizeof( match_str ) );
  match_to_string( &stats->match, match_str, sizeof( match_str ) );

  char actions_str[ 256 ];
  memset( actions_str, '\0', sizeof( actions_str ) );
  uint16_t actions_length = stats->length - offsetof( struct ofp_flow_stats, actions );
  if ( actions_length > 0 ) {
    actions_to_string( stats->actions, actions_length, actions_str, sizeof( actions_str ) );
  }

  info( "[%#016" PRIx64 "] priority = %u, match = [%s], actions = [%s]",
        datapath_id, stats->priority, match_str, actions_str );
}


static void
handle_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                    uint16_t type, uint16_t flags, const buffer *data,
                    void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( user_data );

  if ( type != OFPST_FLOW ) {
    return;
  }

  if ( data != NULL ) {
    size_t offset = 0;
    while ( ( data->length - offset ) >= sizeof( struct ofp_flow_stats ) ) {
      struct ofp_flow_stats *stats = ( struct ofp_flow_stats * ) ( ( char * ) data->data + offset );
      dump_flow_stats( datapath_id, stats );
      offset += stats->length;
    }
  }

  if ( ( flags & OFPSF_REPLY_MORE ) == 0 ) {
    if ( --n_switches <= 0 ) {
      stop_trema();
    }
  }
}


static void
timed_out( void *user_data ) {
  UNUSED( user_data );

  error( "timed out." );

  stop_trema();
}


int
main( int argc, char *argv[] ) {
  // Initialize the Trema world
  init_trema( &argc, &argv );

  // Set event handlers
  set_list_switches_reply_handler( handle_list_switches_reply );
  set_stats_reply_handler( handle_stats_reply, NULL );

  // Ask switch manager to obtain the list of switches
  send_list_switches_request( NULL );

  // Set a timer callback
  add_periodic_event_callback( 10, timed_out, NULL );

  // Main loop
  start_trema();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
