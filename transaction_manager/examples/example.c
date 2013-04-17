/*
 * Author: Yasunobu Chiba
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
#include <inttypes.h>
#include "trema.h"
#include "transaction_manager.h"


static void
transaction_succeeded( uint64_t datapath_id, const buffer *message, void *user_data ) {
  info( "A transaction is completed successfully ( datapath_id = %#" PRIx64 ", message = %p, user_data = %p ).",
        datapath_id, message, user_data );
  dump_buffer( message, info );
}


static void
transaction_failed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  error( "Failed to execute a transaction ( datapath_id = %#" PRIx64 ", message = %p, user_data = %p ).",
         datapath_id, message, user_data );
  dump_buffer( message, error );
}


static void
handle_switch_ready( uint64_t datapath_id, void *user_data ) {
  UNUSED( user_data );

  info( "%#" PRIx64 " is connected.", datapath_id );

  const uint32_t transaction_id = get_transaction_id();
  buffer *message = create_set_config( transaction_id, 0, 128 );

  info( "Executing a transaction ( datapath_id = %#" PRIx64 ", transaction_id = %#x, message = %p ).",
        datapath_id, transaction_id, message );
  dump_buffer( message, info);

  bool ret = execute_transaction( datapath_id, message, transaction_succeeded, NULL, transaction_failed, NULL );
  if ( !ret ) {
    error( "Failed to execute a transaction ( datapath_id = %#" PRIx64 " ).", datapath_id );
  }

  free_buffer( message );
}


int
main( int argc, char *argv[] ) {
  // Initialize the Trema world
  init_trema( &argc, &argv );

  // Init transaction manager
  init_transaction_manager();

  // Set switch ready handler
  set_switch_ready_handler( handle_switch_ready, NULL );

  // Main loop
  start_trema();

  // Finalize transaction manager
  finalize_transaction_manager();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
