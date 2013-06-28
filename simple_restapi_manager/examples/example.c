/*
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
#include <inttypes.h>
#include "trema.h"
#include "simple_restapi_manager.h"


static void
handle_switch_ready( uint64_t datapath_id, void *user_data ) {
  UNUSED( user_data );
  
  info( "%#" PRIx64 " is connected.", datapath_id );
}

/*** Define your REST API callback function here ***/
static char *
handle_query_test_restapi( const struct mg_request_info *request_info, void *request_data ) {
  return "It is from test rest api...";
}

static char *
handle_query_test2_restapi( const struct mg_request_info *request_info, void *request_data ) {
  return "It is from test2 rest api...";
}
/***************************************************/


int
main( int argc, char *argv[] ) {
  
  /* Initialize the Trema world */
  init_trema( &argc, &argv );
  
  /* Init restapi manager */
  init_restapi_manager();
  
  /* Start restapi manager */
  start_restapi_manager();
  
  /*** Add your REST API ***/
  add_restapi_url( "^/test/abc$", "GET", handle_query_test_restapi );
  add_restapi_url( "^/test2/def$", "GET", handle_query_test2_restapi );
  /*************************/
  
  /* Set switch ready handler */
  set_switch_ready_handler( handle_switch_ready, NULL );

  /* Main loop */
  start_trema();

  /* Finalize transaction manager */
  finalize_restapi_manager();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
