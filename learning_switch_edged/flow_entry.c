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

#include <assert.h>
#include "fdb.h"
#include "flow_entry.h"


static uint8_t mac_mask[ OFP_ETH_ALEN ] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };


static oxm_matches *
create_oxm_matches_input( uint32_t port_no, uint8_t *mac ) {
  oxm_matches *match = create_oxm_matches();
  append_oxm_match_in_port( match, port_no );
  if ( mac != NULL ) {
    append_oxm_match_eth_src( match, mac, mac_mask );
  }
  return match;
}


static oxm_matches *
create_oxm_matches_output( uint8_t *mac ) {
  assert( mac != NULL );
  oxm_matches *match = create_oxm_matches();
  append_oxm_match_eth_dst( match, mac, mac_mask );
  return match;
}


void
insert_output_flow_entry( uint8_t *mac, uint64_t datapath_id, uint32_t port_no ) {
  assert( mac != NULL );
  openflow_actions *actions = create_actions();
  append_action_output( actions, port_no, OFPCML_NO_BUFFER );
  openflow_instructions *insts = create_instructions();
  append_instructions_apply_actions( insts, actions );
  oxm_matches *match = create_oxm_matches_output( mac );

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    get_cookie(),
    0,
    OUTPUT_TABLE_ID,
    OFPFC_MODIFY_STRICT,
    60,
    MAX_AGE,
    OFP_DEFAULT_PRIORITY,
    OFP_NO_BUFFER,
    0,
    0,
    0,
    match,
    insts
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
  delete_oxm_matches( match );
  delete_instructions( insts );
  delete_actions( actions );
}


void
delete_output_flow_entry( uint8_t *mac, uint64_t datapath_id, uint32_t port_no ) {
  assert( mac != NULL );
  oxm_matches *match = create_oxm_matches_output( mac );

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    0,
    0,
    OUTPUT_TABLE_ID,
    OFPFC_DELETE_STRICT,
    0,
    0,
    OFP_DEFAULT_PRIORITY,
    OFP_NO_BUFFER,
    port_no,
    0,
    0,
    match,
    NULL
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
  delete_oxm_matches( match );
}


void
delete_output_flow_entry_by_outport( uint64_t datapath_id, uint32_t port_no ) {
  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    0,
    0,
    OUTPUT_TABLE_ID,
    OFPFC_DELETE_STRICT,
    0,
    0,
    OFP_DEFAULT_PRIORITY,
    OFP_NO_BUFFER,
    port_no,
    0,
    0,
    NULL,
    NULL
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
}


void
insert_input_flow_entry( uint8_t *mac, uint64_t datapath_id, uint32_t port_no ) {
  assert( mac != NULL );
  openflow_instructions *insts = create_instructions();
  append_instructions_goto_table( insts, OUTPUT_TABLE_ID );
  oxm_matches *match = create_oxm_matches_input( port_no, mac );

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    get_cookie(),
    0,
    INPUT_TABLE_ID,
    OFPFC_MODIFY_STRICT,
    60,
    MAX_AGE,
    OFP_DEFAULT_PRIORITY,
    OFP_NO_BUFFER,
    0,
    0,
    0,
    match,
    insts
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
  delete_oxm_matches( match );
  delete_instructions( insts );
}


void
delete_input_flow_entry( uint8_t *mac, uint64_t datapath_id, uint32_t port_no ) {
  assert( mac != NULL );
  oxm_matches *match = create_oxm_matches_input( port_no, mac );

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    0,
    0,
    INPUT_TABLE_ID,
    OFPFC_DELETE_STRICT,
    0,
    0,
    OFP_DEFAULT_PRIORITY,
    OFP_NO_BUFFER,
    0,
    0,
    0,
    match,
    NULL
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
  delete_oxm_matches( match );
}


void
delete_input_flow_entry_by_inport( uint64_t datapath_id, uint32_t port_no ) {
  oxm_matches *match = create_oxm_matches_input( port_no, NULL );

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    0,
    0,
    INPUT_TABLE_ID,
    OFPFC_DELETE_STRICT,
    0,
    0,
    OFP_DEFAULT_PRIORITY,
    OFP_NO_BUFFER,
    0,
    0,
    0,
    match,
    NULL
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
  delete_oxm_matches( match );
}


static void
set_table_miss_flow_entry( uint64_t datapath_id, uint8_t table_id ) {
  openflow_actions *actions = create_actions();
  append_action_output( actions, OFPP_CONTROLLER, OFPCML_NO_BUFFER );
  openflow_instructions *insts = create_instructions();
  append_instructions_apply_actions( insts, actions );

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    get_cookie(),
    0,
    table_id,
    OFPFC_ADD,
    0,
    0,
    OFP_LOW_PRIORITY,
    OFP_NO_BUFFER,
    0,
    0,
    OFPFF_SEND_FLOW_REM,
    NULL,
    insts
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
  delete_instructions( insts );
  delete_actions( actions );
}


void
insert_output_table_miss_flow_entry( uint64_t datapath_id ) {
  set_table_miss_flow_entry( datapath_id, OUTPUT_TABLE_ID );
}


void
insert_input_table_miss_flow_entry( uint64_t datapath_id ) {
  set_table_miss_flow_entry( datapath_id, INPUT_TABLE_ID );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
