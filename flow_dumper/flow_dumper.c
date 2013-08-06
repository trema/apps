/*
 * Flow dumper application.
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2011-2013 NEC Corporation
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
#include "trema.h"


static int n_switches = 0;


static void
handle_list_switches_reply( const list_element *switches, void *user_data ) {
  UNUSED( user_data );

#ifdef TREMA_EDGE
  buffer *stats_request = create_flow_multipart_request( get_transaction_id(), 0,
                                                         OFPTT_ALL, OFPP_ANY, OFPG_ANY, 0, 0, NULL );
#else
  struct ofp_match match;
  memset( &match, 0, sizeof( struct ofp_match ) );
  match.wildcards = OFPFW_ALL;
  buffer *stats_request = create_flow_stats_request( get_transaction_id(), 0, match, 0xff, OFPP_NONE );
#endif

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
#ifdef TREMA_EDGE
  uint16_t match_len = ( uint16_t ) ( stats->match.length + PADLEN_TO_64( stats->match.length ) );

  char match_str[ 2048 ];
  memset( match_str, '\0', sizeof( match_str ) );
  { // XXX
    struct ofp_match *tmp_match = xcalloc( 1, match_len );
    hton_match( tmp_match, &stats->match );
    oxm_matches *tmp_matches = parse_ofp_match( tmp_match );
    match_to_string( tmp_matches, match_str, sizeof( match_str ) );
    xfree( tmp_match );
    delete_oxm_matches( tmp_matches );
  }
  char inst_str[ 4096 ];
  memset( inst_str, '\0', sizeof( inst_str ) );
  uint16_t inst_len = ( uint16_t ) ( stats->length - ( offsetof( struct ofp_flow_stats, match ) + match_len ) );
  if ( inst_len > 0 ) {
    struct ofp_instruction *inst = ( struct ofp_instruction * ) ( ( char * ) stats + offsetof( struct ofp_flow_stats, match ) + match_len );
    instructions_to_string( inst, inst_len, inst_str, sizeof( inst_str ) );
  }

  info( "[%#016" PRIx64 "] table_id = %u, priority = %u, cookie = %#" PRIx64 ", idle_timeout = %u, "
        " hard_timeout = %u, duration = %u.%09u, packet_count = %" PRIu64 ", byte_count = %" PRIu64
        ", match = [%s], instructions = [%s]",
        datapath_id, stats->table_id, stats->priority, stats->cookie, stats->idle_timeout,
        stats->hard_timeout, stats->duration_sec, stats->duration_nsec, stats->packet_count, stats->byte_count,
        match_str, inst_str );
#else
  char match_str[ 512 ];
  memset( match_str, '\0', sizeof( match_str ) );
  match_to_string( &stats->match, match_str, sizeof( match_str ) );

  char actions_str[ 256 ];
  memset( actions_str, '\0', sizeof( actions_str ) );
  uint16_t actions_length = stats->length - offsetof( struct ofp_flow_stats, actions );
  if ( actions_length > 0 ) {
    actions_to_string( stats->actions, actions_length, actions_str, sizeof( actions_str ) );
  }

  info( "[%#016" PRIx64 "] table_id = %u, priority = %u, cookie = %#" PRIx64 ", idle_timeout = %u, "
        " hard_timeout = %u, duration = %u.%09u, packet_count = %" PRIu64 ", byte_count = %" PRIu64
        ", match = [%s], actions = [%s]",
        datapath_id, stats->table_id, stats->priority, stats->cookie, stats->idle_timeout,
        stats->hard_timeout, stats->duration_sec, stats->duration_nsec, stats->packet_count, stats->byte_count,
        match_str, actions_str );
#endif
}


static bool
type_is_flow_stats( uint16_t type ) {
#ifdef TREMA_EDGE
  return type == OFPMP_FLOW;
#else
  return type == OFPST_FLOW;
#endif
}


static bool
more_requests( uint16_t flags ) {
#ifdef TREMA_EDGE
  return ( flags & OFPMPF_REPLY_MORE ) == OFPMPF_REPLY_MORE;
#else
  return ( flags & OFPSF_REPLY_MORE ) == OFPSF_REPLY_MORE;
#endif
}


static void
handle_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                    uint16_t type, uint16_t flags, const buffer *data,
                    void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( user_data );

  if ( !type_is_flow_stats( type ) ) {
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

  if ( !more_requests( flags ) ) {
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
#ifdef TREMA_EDGE
  set_multipart_reply_handler( handle_stats_reply, NULL );
#else
  set_stats_reply_handler( handle_stats_reply, NULL );
#endif

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
