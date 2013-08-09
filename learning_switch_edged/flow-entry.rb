#
# Copyright (C) 2008-2013 NEC Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#


require_relative "fdb"


module FlowEntry
  INPUT_TABLE_ID = 0
  OUTPUT_TABLE_ID = 3
  IDLE_TIMEOUT = 60
  HARD_TIMEOUT = FDB::MAX_AGE


  def insert_output_flow_entry mac, datapath_id, port_number
    action = SendOutPort.new( port_number: port_number )
    inst = Instructions::ApplyAction.new( actions: [ action ] )
    match = Match.new( eth_dst: mac )

    send_flow_mod(
      datapath_id,
      table_id: OUTPUT_TABLE_ID,
      command: OFPFC_MODIFY_STRICT,
      idle_timeout: IDLE_TIMEOUT,
      hard_timeout: HARD_TIMEOUT,
      priority: OFP_DEFAULT_PRIORITY,
      buffer_id: OFP_NO_BUFFER,
      match: match,
      instructions: [ inst ]
    )
  end


  def delete_output_flow_entry mac, datapath_id, port_number
    match = Match.new( eth_dst: mac )

    send_flow_mod(
      datapath_id,
      table_id: OUTPUT_TABLE_ID,
      command: OFPFC_DELETE_STRICT,
      priority: OFP_DEFAULT_PRIORITY,
      out_port: port_number,
      out_group: OFPG_ANY,
      match: match
    )
  end


  def delete_output_flow_entries_by_outport mac, datapath_id, port_number
    send_flow_mod(
      datapath_id,
      table_id: OUTPUT_TABLE_ID,
      command: OFPFC_DELETE,
      priority: OFP_DEFAULT_PRIORITY,
      out_port: port_number,
      out_group: OFPG_ANY
    )
  end


  def insert_input_flow_entry mac, datapath_id, port_number
    inst = Instructions::GotoTable.new( OUTPUT_TABLE_ID )
    match = Match.new( in_port: port_number, eth_src: mac )

    send_flow_mod(
      datapath_id,
      table_id: INPUT_TABLE_ID,
      command: OFPFC_MODIFY_STRICT,
      idle_timeout: IDLE_TIMEOUT,
      hard_timeout: HARD_TIMEOUT,
      priority: OFP_DEFAULT_PRIORITY,
      buffer_id: OFP_NO_BUFFER,
      match: match,
      instructions: [ inst ]
    )
  end


  def delete_input_flow_entry mac, datapath_id, port_number
    match = Match.new( in_port: port_number, eth_src: mac )

    send_flow_mod(
      datapath_id,
      table_id: INPUT_TABLE_ID,
      command: OFPFC_DELETE_STRICT,
      priority: OFP_DEFAULT_PRIORITY,
      out_port: OFPP_ANY,
      out_group: OFPG_ANY,
      match: match
    )
  end


  def delete_input_flow_entries_by_inport mac, datapath_id, port_number
    match = Match.new( in_port: port_number )

    send_flow_mod(
      datapath_id,
      table_id: INPUT_TABLE_ID,
      command: OFPFC_DELETE,
      priority: OFP_DEFAULT_PRIORITY,
      out_port: OFPP_ANY,
      out_group: OFPG_ANY,
      match: match
    )
  end


  def insert_table_miss_flow_entry datapath_id, table_id
    action = SendOutPort.new( port_number: OFPP_CONTROLLER, max_len: OFPCML_NO_BUFFER )
    inst = Instructions::ApplyAction.new( actions: [ action ] )

    send_flow_mod(
      datapath_id,
      table_id: table_id,
      command: OFPFC_ADD,
      idle_timeout: 0,
      hard_timeout: 0,
      priority: OFP_LOW_PRIORITY,
      buffer_id: OFP_NO_BUFFER,
      instructions: [ inst ]
    )
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
