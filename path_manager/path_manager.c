/*
 * Sample path management application.
 *
 * Author: Yasunobu Chiba
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


#include <inttypes.h>
#include "path_manager.h"
#include "trema.h"


static const uint16_t IDLE_TIMEOUT = 60;
static const uint16_t HARD_TIMEOUT = 0;

static hash_table *path_db = NULL;


static unsigned int
hash_match( const void *key ) {
  return hash_core( key, sizeof( struct ofp_match ) );
}


static bool
compare_match_without_in_port( const void *x, const void *y ) {
  struct ofp_match match_x = *( const struct ofp_match * ) x;
  struct ofp_match match_y = *( const struct ofp_match * ) y;
  match_x.in_port = match_y.in_port = OFPP_NONE;

  return compare_match_strict( &match_x, &match_y );
}


static void
create_path_db( void ) {
  path_db = create_hash( compare_match_without_in_port, hash_match );
}


static void
delete_path_db( void ) {
  delete_hash( path_db );
}


static path_manager_path *
lookup_path_db_entry( struct ofp_match match ) {
  match.in_port = OFPP_NONE;
  return lookup_hash_entry( path_db, ( void * ) &match );
}


static void
delete_path_db_entry( struct ofp_match match ) {
  match.in_port = OFPP_NONE;
  path_manager_path *entry = lookup_path_db_entry( match );
  if ( entry == NULL ) {
    return;
  }
  entry = delete_hash_entry( path_db, &match );
  if ( entry != NULL ) {
    xfree( entry );
  }
}


static void
insert_path_db_entry( path_manager_path *path ) {
  path->match.in_port = OFPP_NONE;
  path_manager_path *entry = lookup_path_db_entry( path->match );
  if ( entry != NULL ) {
    char match_string[ 256 ];
    match_to_string( &path->match, match_string, sizeof( match_string ) );
    warn( "Path entry is already registered ( match = [%s] ). Replacing it.", match_string );
    delete_path_db_entry( path->match );
  }
  insert_hash_entry( path_db, path, path );
}


static void
add_flow_entry( struct ofp_match match, uint64_t datapath_id, uint16_t in_port, uint16_t out_port ) {
  match.in_port = in_port;

  openflow_actions *actions = create_actions();
  append_action_output( actions, out_port, UINT16_MAX );
  buffer *flow_mod = create_flow_mod( get_transaction_id(), match, get_cookie(),
                                      OFPFC_ADD, IDLE_TIMEOUT, HARD_TIMEOUT,
                                      UINT16_MAX, UINT32_MAX, OFPP_NONE, OFPFF_SEND_FLOW_REM, actions );

  send_openflow_message( datapath_id, flow_mod );

  delete_actions( actions );
  free_buffer( flow_mod );
}


static void
install_flow_entries_on_path( struct ofp_match match, int n_hops, path_manager_hop *hops ) {
  for ( int i = n_hops - 1; i >= 0; i-- ) {
    add_flow_entry( match, hops[ i ].datapath_id, hops[ i ].in_port, hops[ i ].out_port );
  }
}


static void
handle_path_setup_request(  uint16_t tag, void *data, size_t length ) {
  if ( tag != MESSENGER_PATH_SETUP_REQUEST ) {
    return;
  }
  if ( length < sizeof( path_manager_path ) ) {
    error( "Too short path setup request message ( length = %u ).", length );
    return;
  }

  path_manager_path *path = xmalloc( length );
  memcpy( path, data, length );

  char match_string[ 256 ];
  match_to_string( &path->match, match_string, sizeof( match_string ) );

  if ( path->n_hops <= 0 || path->hops == NULL ) {
    error( "Invalid path setup request ( match = [%s], n_hops = %u, hops = %p ).", match_string, path->n_hops, path->hops );
    xfree( path );
    return;
  }

  info( "Path setup request received ( match = [%s], n_hops = %u, hops = %p ).", match_string, path->n_hops, path->hops );
  for ( int i = 0; i < path->n_hops; i++ ) {
    info( "  %#" PRIx64 ": %u -> %u", path->hops[ i ].datapath_id, path->hops[ i ].in_port, path->hops[ i ].out_port );
  }

  install_flow_entries_on_path( path->match, path->n_hops, path->hops );
  insert_path_db_entry( path );
}


static void
handle_flow_removed( uint64_t datapath_id, uint32_t transaction_id, struct ofp_match match, uint64_t cookie,
                     uint16_t priority, uint8_t reason, uint32_t duration_sec, uint32_t duration_nsec,
                     uint16_t idle_timeout, uint64_t packet_count, uint64_t byte_count, void *user_data ) {
  char match_string[ 256 ];
  match_to_string( &match, match_string, sizeof( match_string ) );
  info( "Flow removed received from %#" PRIx64 " ( match = [%s] ).", datapath_id, match_string );

  delete_path_db_entry( match );
}


int
main( int argc, char *argv[] ) {
  // Initialize Trema world
  init_trema( &argc, &argv );

  // Set a callback for handling Flow Removed events
  set_flow_removed_handler( handle_flow_removed, NULL );

  // Add a callback for handling path setup requests
  add_message_received_callback( PATH_SETUP_SERVICE_NAME, handle_path_setup_request );

  // Create a path database for managing paths and flow entries
  create_path_db();

  // Main loop
  start_trema();

  // Cleanup
  delete_path_db();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
