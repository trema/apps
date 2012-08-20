/*
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


#include <assert.h>
#include <inttypes.h>
#include "libpath.h"
#include "trema.h"
#include "utils.h"


static int n_paths = 0;


static void
handle_setup( int status, const path *p, void *user_data ) {
  info( "**** Path setup completed ( status = %s, user_data = %p ) *** ",
        status_to_string( status ), user_data );
  dump_path( p );
}


static void
handle_teardown( int reason, const path *p, void *user_data ) {
  info( "**** Path teardown completed ( reason = %s, user_data = %p ) *** ",
        reason_to_string( reason ), user_data );
  dump_path( p );
  if ( --n_paths <= 0 ) {
    stop_trema();
  }
}


static void
lookup( void *user_data ) {
  assert( user_data != NULL );

  path *p = user_data;
  list_element *element = p->hops;
  hop *h = element->data;

  char match_string[ 128 ];
  match_to_string( &p->match, match_string, sizeof( match_string ) );

  info( "**** Looking up path *** " );
  info( "match = [%s], priority = %u, idle_timeout = %u, hard_timeout = %u.",
        match_string, p->priority, p->idle_timeout, p->hard_timeout );

  const path *found = lookup_path( h->datapath_id, p->match, p->priority );
  if ( found == NULL ) {
    error( "No path found." );
    return;
  }
  dump_path( found );

  delete_path( p );
}


static void
setup( void *user_data ) {
  UNUSED( user_data );

  uint16_t priority = UINT16_MAX / 2;
  uint16_t idle_timeout = 10;
  uint16_t hard_timeout = 0;

  info( "**** Setting up a path *** " );

  struct ofp_match match;
  memset( &match, 0, sizeof( struct ofp_match ) );
  match.wildcards = ( OFPFW_ALL & ~OFPFW_DL_TYPE );
  match.dl_type = 1;

  path *p = create_path( match, priority, idle_timeout, hard_timeout );
  assert( p != NULL );
  hop *h = create_hop( 0x1, 1, 2, NULL );
  assert( h != NULL );
  append_hop_to_path( p, h );

  dump_path( p );

  bool ret = setup_path( p, handle_setup, NULL, handle_teardown, NULL );
  assert( ret == true );

  n_paths++;

  delete_path( p );

  info( "**** Setting up a path *** " );

  match.dl_type = 2;
  p = create_path( match, priority, idle_timeout, hard_timeout );
  assert( p != NULL );
  h = create_hop( 0x1, 1, 2, NULL );
  assert( h != NULL );
  append_hop_to_path( p, h );

  dump_path( p );

  ret = setup_path( p, handle_setup, NULL, handle_teardown, NULL );
  assert( ret == true );

  n_paths++;

  struct itimerspec spec;
  memset( &spec, 0, sizeof( struct itimerspec ) );
  spec.it_value.tv_sec = 5;
  add_timer_event_callback( &spec, lookup, p );
}


int
main( int argc, char *argv[] ) {
  // Initialize the Trema world
  init_trema( &argc, &argv );

  // Init path management library (libpath)
  init_path();

  // Set timer event to setup a path
  struct itimerspec spec;
  memset( &spec, 0, sizeof( struct itimerspec ) );
  spec.it_value.tv_sec = 1;
  add_timer_event_callback( &spec, setup, NULL );

  // Main loop
  start_trema();

  // Finalize path management library (libpath)
  finalize_path();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
