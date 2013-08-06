/*
 * Copyright (C) 2008-2013 NEC Corporation
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

#ifndef TREMA_EDGE
#error "not supported"
#endif

#include "trema.h"
#include "fdb.h"
#include "flow_entry.h"


typedef struct {
  uint64_t datapath_id;
  hash_table *fdb;
} switch_entry;


static switch_entry *
allocate_switch_entry( uint64_t datapath_id ) {
  switch_entry *entry = xmalloc( sizeof( switch_entry ) );
  entry->datapath_id = datapath_id;
  entry->fdb = create_fdb();
  return entry;
}


static void
free_switch_entry( switch_entry *entry ) {
  delete_fdb( entry->fdb );
  xfree( entry );
}


static hash_table *
create_switch( void ) {
  return create_hash( compare_datapath_id, hash_datapath_id );
}


static void
insert_switch_entry( hash_table *db, uint64_t datapath_id ) {
    switch_entry *entry = allocate_switch_entry( datapath_id );
    switch_entry *old_switch = insert_hash_entry( db, &entry->datapath_id, entry );
    if ( old_switch != NULL ) {
      warn( "duplicated switch ( datapath_id = %#" PRIx64 ", entry = %p )", datapath_id, old_switch );
      free_switch_entry( old_switch );
    }
    debug( "insert switch: %#" PRIx64 " ( entry = %p )", datapath_id, entry );
}


static switch_entry *
lookup_switch_entry( hash_table *db, uint64_t datapath_id ) {
  switch_entry *entry = lookup_hash_entry( db, &datapath_id );
  debug( "lookup switch: %#" PRIx64 " ( entry = %p )", datapath_id, entry );
  return entry;
}


static void
delete_switch_entry( hash_table *db, uint64_t datapath_id ) {
  switch_entry *entry = delete_hash_entry( db, &datapath_id );
  free_switch_entry( entry );
  debug( "delete switch: %#" PRIx64 " ( entry = %p )", datapath_id, entry );
}


static void
delete_switch( hash_table *db ) {
  hash_iterator iter;
  init_hash_iterator( db, &iter );
  hash_entry *e;
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    free_switch_entry( e->value );
  }
  delete_hash( db );
}


static void
send_packet( uint64_t datapath_id, uint32_t out_port, packet_in message, uint32_t in_port ) {
  openflow_actions *actions = create_actions();
  append_action_output( actions, out_port, OFPCML_NO_BUFFER );

  buffer *frame = duplicate_buffer( message.data );
  fill_ether_padding( frame );
  buffer *packet_out = create_packet_out(
    get_transaction_id(),
    OFP_NO_BUFFER,
    in_port,
    actions,
    frame
  );
  send_openflow_message( datapath_id, packet_out );

  free_buffer( packet_out );
  free_buffer( frame );
  delete_actions( actions );
}


static void
handle_packet_in( uint64_t datapath_id, packet_in message ) {
  hash_table *db = message.user_data;
  switch_entry *entry = lookup_switch_entry( db, datapath_id );
  if ( entry == NULL ) {
    error( "switch entry is not initialized yet." );
    return;
  }

  if ( message.data == NULL ) {
    error( "data must not be NULL" );
    return;
  }
  if ( !packet_type_ether( message.data ) ) {
    error( "data must be ethernet frane" );
    return;
  }

  uint32_t in_port = get_in_port_from_oxm_matches( message.match );
  if ( in_port == 0 ) {
    error( "in_port missing in oxm-matches" );
    return;
  }

  packet_info packet_info = get_packet_info( message.data );
  uint32_t old_port_number = learn_fdb( entry->fdb, packet_info.eth_macsa, in_port );
  if ( old_port_number != ENTRY_NOT_FOUND_IN_FDB ) {
    delete_input_flow_entry( packet_info.eth_macsa, datapath_id, old_port_number );
    delete_output_flow_entry( packet_info.eth_macsa, datapath_id, old_port_number );
  }
  if ( message.table_id == INPUT_TABLE_ID ) {
    insert_input_flow_entry( packet_info.eth_macsa, datapath_id, in_port );
    insert_output_flow_entry( packet_info.eth_macsa, datapath_id, in_port );
  }

  uint32_t out_port = lookup_fdb( entry->fdb, packet_info.eth_macda );
  if ( out_port == ENTRY_NOT_FOUND_IN_FDB ) {
    out_port = OFPP_ALL;
  }
  else {
    insert_output_flow_entry( packet_info.eth_macda, datapath_id, out_port );
  }
  char macsa_str[ 20 ], macda_str[ 20 ];
  mac_to_string( packet_info.eth_macsa, macsa_str, sizeof( macsa_str ) );
  mac_to_string( packet_info.eth_macda, macda_str, sizeof( macda_str ) );
  info( "send_packet: %#" PRIx64 " %s ( in port = %u ) -> %s ( out port = %u )",
        datapath_id, macsa_str, in_port, macda_str, out_port );
  send_packet( datapath_id, out_port, message, in_port );
}


static void
handle_port_status( uint64_t datapath_id, uint32_t transaction_id,
                    uint8_t reason, struct ofp_port desc, void *user_data ) {
  UNUSED( transaction_id );
  hash_table *db = user_data;
  switch_entry *entry = lookup_switch_entry( db, datapath_id );
  if ( entry == NULL ) {
    error( "switch entry is not initialized yet." );
    return;
  }

  bool delete_fdb = false;
  switch ( reason ) {
    case OFPPR_DELETE:
      delete_fdb = true;
      break;
    case OFPPR_MODIFY:
      if ( ( desc.config & OFPPC_PORT_DOWN ) == OFPPC_PORT_DOWN ||
           ( desc.state & OFPPS_LINK_DOWN ) == OFPPS_LINK_DOWN ) {
        delete_fdb = true;
      }
      break;
  }
  if ( delete_fdb ) {
    delete_fdb_entries( entry->fdb, desc.port_no );
    delete_input_flow_entry_by_inport( datapath_id, desc.port_no );
    delete_output_flow_entry_by_outport( datapath_id, desc.port_no );
  }
}


static void
handle_switch_ready( uint64_t datapath_id, void *user_data ) {
  hash_table *db = user_data;

  insert_switch_entry( db, datapath_id );
  insert_output_table_miss_flow_entry( datapath_id );
  insert_input_table_miss_flow_entry( datapath_id );
}


static void
handle_switch_disconnected( uint64_t datapath_id, void *user_data ) {
  hash_table *db = user_data;

  delete_switch_entry( db, datapath_id );
}


static void
handle_periodic_event( void *user_data ) {
  hash_table *db = user_data;

  hash_iterator iter;
  init_hash_iterator( db, &iter );
  hash_entry *e;
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    switch_entry *entry = e->value;
    delete_expired_fdb_entries( entry->fdb );
  }
}


static const int PERIODIC_INTERVAL = 5;


int
main( int argc, char *argv[] ) {
  init_trema( &argc, &argv );

  hash_table *db = create_switch();

  add_periodic_event_callback( PERIODIC_INTERVAL, handle_periodic_event, db );
  set_packet_in_handler( handle_packet_in, db );
  set_port_status_handler( handle_port_status, db );

  set_switch_ready_handler( handle_switch_ready, db );
  set_switch_disconnected_handler( handle_switch_disconnected, db );

  start_trema();

  delete_switch( db );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
