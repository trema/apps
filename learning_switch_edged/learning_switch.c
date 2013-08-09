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

#include <assert.h>
#include "trema.h"
#include "datapath_db.h"
#include "fdb.h"
#include "flow_entry.h"


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
learning_switch_increment_stat( uint64_t datapath_id, const char *key ) {
  char str[ 1024 ];
  int n = snprintf( str, sizeof( str ), "learning_switch.%#" PRIx64 ".%s", datapath_id, key );
  assert( n > 0 && ( size_t ) n < sizeof( str ) );
  increment_stat( str );
}


static const time_t LOOP_GUARD = 5;

static void
forwarding( hash_table *fdb, uint64_t datapath_id, packet_in message, uint32_t in_port ) {
  if ( fdb == NULL ) {
    error( "datapath is not initialized yet." );
    return;
  }
  learning_switch_increment_stat( datapath_id, "packets_received" );

  packet_info packet_info = get_packet_info( message.data );
  time_t updated_at = 0;
  uint32_t old_in_port = lookup_fdb( fdb, packet_info.eth_macsa, &updated_at );
  uint32_t out_port = lookup_fdb( fdb, packet_info.eth_macda, NULL );

  char macsa_str[ MAC_STRING_LENGTH ], macda_str[ MAC_STRING_LENGTH ];
  mac_to_string( packet_info.eth_macsa, macsa_str, sizeof( macsa_str ) );
  mac_to_string( packet_info.eth_macda, macda_str, sizeof( macda_str ) );

  // loop check
  time_t now = time( NULL );
  debug( "forwarding: table id = %u, datapath id = %#" PRIx64 ", macsa = %s, old in port = %u, "
         "in port = %u, macda = %s, out port = %u, updated_at = %" PRId64 ", now = %" PRId64,
         message.table_id, datapath_id, macsa_str, old_in_port, in_port,
	 macda_str, out_port, ( uint64_t ) updated_at, ( uint64_t ) now );

  if ( old_in_port != ENTRY_NOT_FOUND_IN_FDB && old_in_port != in_port &&
       out_port == ENTRY_NOT_FOUND_IN_FDB &&
       updated_at + LOOP_GUARD > now ) {
    warn( "loop maybe: datapath id = %#" PRIx64 ", macsa = %s, old in port = %u, "
          "in port = %u, macda = %s",
          datapath_id, macsa_str, old_in_port, in_port, macda_str );
    learning_switch_increment_stat( datapath_id, "packets_discarded" );
    return;
  }

  // mac address learning
  update_fdb( fdb, packet_info.eth_macsa, in_port );

  // set of flow entries
  if ( old_in_port != in_port ) {
    delete_input_flow_entry( packet_info.eth_macsa, datapath_id, old_in_port );
    delete_output_flow_entry( packet_info.eth_macsa, datapath_id, old_in_port );
  }
  if ( message.table_id == INPUT_TABLE_ID ) {
    debug( "insert input flow entry ( reactive, macsa = %s, datapath id = %#" PRIx64 ", in port = %u",
           macsa_str, datapath_id, in_port );
    insert_input_flow_entry( packet_info.eth_macsa, datapath_id, in_port );
    debug( "insert output flow entry ( proactive, macda = %s, datapath id = %#" PRIx64 ", out port = %u",
           macsa_str, datapath_id, in_port );
    insert_output_flow_entry( packet_info.eth_macsa, datapath_id, in_port );
  }
  if ( out_port != ENTRY_NOT_FOUND_IN_FDB ) {
    debug( "insert output flow entry ( %s, macda = %s, datapath id = %#" PRIx64 ", out port = %u",
           ( message.table_id == OUTPUT_TABLE_ID ? "reactive" : "just in case"),
           macda_str, datapath_id, out_port );
    insert_output_flow_entry( packet_info.eth_macda, datapath_id, out_port );
  }

  // send a packet
  if ( out_port == ENTRY_NOT_FOUND_IN_FDB ) {
    out_port = OFPP_ALL;
    learning_switch_increment_stat( datapath_id, "broadcast_forwarding" );
  }
  else {
    learning_switch_increment_stat( datapath_id, "unicast_forwarding" );
  }
  debug( "send_packet: %#" PRIx64 " %s ( in port = %u ) -> %s ( out port = %u )",
         datapath_id, macsa_str, in_port, macda_str, out_port );
  send_packet( datapath_id, out_port, message, in_port );
}


static void
delete_forwarding_entries( hash_table *fdb, uint64_t datapath_id, uint32_t port_number ) {
  if ( fdb == NULL ) {
    error( "datapath is not initialized yet." );
    return;
  }

  delete_forwarding_entries_by_port_number( fdb, port_number );
  delete_input_flow_entries_by_inport( datapath_id, port_number );
  delete_output_flow_entries_by_outport( datapath_id, port_number );
}


static void
handle_packet_in( uint64_t datapath_id, packet_in message ) {
  hash_table *db = message.user_data;

  if ( message.data == NULL ) {
    error( "data must not be NULL" );
    return;
  }
  if ( !packet_type_ether( message.data ) ) {
    error( "data must be ethernet frame" );
    return;
  }
  if ( message.reason == OFPR_INVALID_TTL ) {
    warn( "reason.invalid-ttl not supported ( datapath_id = %#" PRIx64 ")", datapath_id );
    return;
  }

  uint32_t in_port = get_in_port_from_oxm_matches( message.match );
  if ( in_port == 0 ) {
    error( "in_port missing in oxm-matches" );
    return;
  }

  hash_table *fdb = lookup_datapath_entry( db, datapath_id );
  forwarding( fdb, datapath_id, message, in_port );
}


static void
handle_port_status( uint64_t datapath_id, uint32_t transaction_id,
                    uint8_t reason, struct ofp_port desc, void *user_data ) {
  UNUSED( transaction_id );
  hash_table *db = user_data;

  hash_table *fdb = lookup_datapath_entry( db, datapath_id );
  switch ( reason ) {
  case OFPPR_DELETE:
    delete_forwarding_entries( fdb, datapath_id, desc.port_no );
    break;
  case OFPPR_MODIFY:
    if ( ( desc.config & OFPPC_PORT_DOWN ) == OFPPC_PORT_DOWN ||
         ( desc.state & OFPPS_LINK_DOWN ) == OFPPS_LINK_DOWN ) {
      delete_forwarding_entries( fdb, datapath_id, desc.port_no );
    }
    break;
  }
}


static void *
create_datapath_data( uint64_t datapath_id ) {
  UNUSED( datapath_id );
  return ( void * ) create_fdb();
}


static void
delete_datapath_data( void *datapath_data ) {
  hash_table *fdb = datapath_data;
  delete_fdb( fdb );
}


static void
handle_switch_ready( uint64_t datapath_id, void *user_data ) {
  hash_table *db = user_data;

  insert_datapath_entry( db, datapath_id, create_datapath_data( datapath_id ), delete_datapath_data );
  insert_table_miss_flow_entry( datapath_id, OUTPUT_TABLE_ID );
  insert_table_miss_flow_entry( datapath_id, INPUT_TABLE_ID );
}


static void
handle_switch_disconnected( uint64_t datapath_id, void *user_data ) {
  hash_table *db = user_data;

  delete_datapath_entry( db, datapath_id );
}


static void
delete_aged_entries( void *datapath_data ) {
  hash_table *fdb = datapath_data;
  delete_aged_forwarding_entries( fdb );
}


static void
handle_periodic_event( void *user_data ) {
  hash_table *db = user_data;

  foreach_datapath_db( db, delete_aged_entries );
}


static const time_t PERIODIC_INTERVAL = 5;

int
main( int argc, char *argv[] ) {
  init_trema( &argc, &argv );
  hash_table *db = create_datapath_db();
  add_periodic_event_callback( PERIODIC_INTERVAL, handle_periodic_event, db );
  set_packet_in_handler( handle_packet_in, db );
  set_port_status_handler( handle_port_status, db );
  set_switch_ready_handler( handle_switch_ready, db );
  set_switch_disconnected_handler( handle_switch_disconnected, db );

  start_trema();

  delete_datapath_db( db );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
