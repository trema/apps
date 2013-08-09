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


require_relative "datapath-db"
require_relative "fdb"
require_relative "flow-entry"
require_relative "port"


class LearningSwitch < Controller
  include FlowEntry


  LOOP_GUARD = 5


  add_timer_event :periodic_event, 5, :periodic


  def start
    @db = DatapathDB.new
  end


  def packet_in datapath_id, message
    fdb = @db.lookup( datapath_id ).fdb
    if message.reason == OFPR_INVALID_TTL
      warn "reason.invalid-ttl not supported ( datapath_id = #{ datapath_id } )";
      return
    end
    forwarding( fdb, datapath_id, message )
  end


  def port_status datapath_id, message
    fdb = @db.lookup( datapath_id ).fdb
    if message.delete? or ( message.modify? and message.down? )
      fdb.delete_entries_by_port_number( message.port_no )
      delete_input_flow_entries_by_inport( datapath_id, message.port_no )
      delete_output_flow_entries_by_inport( datapath_id, message.port_no )
    end
  end


  def switch_ready datapath_id
    @db.insert( datapath_id, FDB.new )
    insert_table_miss_flow_entry( datapath_id, OUTPUT_TABLE_ID )
    insert_table_miss_flow_entry( datapath_id, INPUT_TABLE_ID )
  end


  def switch_disconnected
    @db.delete( datapath_id )
  end


  def periodic_event
    @db.each_value do | fdb |
      fdb.delete_aged_entries
    end
  end


  private


  def forwarding fdb, datapath_id, message
    in_port = message.in_port
    # loop check
    now = Time.new
    old_in = fdb.lookup message.eth_src
    out = fdb.lookup message.eth_dst
    debug "forwarding: table id = #{ message.table_id }, datapath id = #{ datapath_id }, " +
      "macsa = #{ message.eth_src }, old in port = #{ old_in ? old_in.port_number : 0 }, " +
      "in port = #{ in_port }, macda = #{ message.eth_dst }, out port = #{ out ? out.port_number : "(nil)" }, " +
      "updated_at = #{ old_in ? old_in.updated_at : "(nil)" }, now = #{ now }"
    if ( old_in and old_in.port_number != in_port and
         out.nil? and old_in.updated_at + LOOP_GUARD > now )
      warn "loop maybe: datapath id = #{ datapath_id }, macsa = #{ message.eth_src }, " +
        "old in port = #{ old_in.port_number }. in port = #{ in_port }, macda = #{ message.eth_dst }"
      return
    end

    # mac address learning
    fdb.update( message.eth_src, in_port )

    # set of flow entries
    if old_in and old_in.port_number != in_port
      delete_input_flow_entry( message.eth_src, datapath_id, old_in.port_number )
      delete_output_flow_entry( message.eth_src, datapath_id, old_in.port_number )
    end
    if message.table_id == INPUT_TABLE_ID
      insert_input_flow_entry( message.eth_src, datapath_id, in_port )
      insert_output_flow_entry( message.eth_src, datapath_id, in_port )
    end
    if out
      insert_output_flow_entry( message.eth_dst, datapath_id, out.port_number )
    end

    # send a packet
    out_port = out ? out.port_number : OFPP_ALL
    debug "send_packet: #{ datapath_id } #{ message.eth_src } ( in port = #{ in_port } ) -> " +
      "#{ message.eth_dst } ( out port = #{ out_port } )"
    send_packet( datapath_id, out_port, message )
  end


  def send_packet datapath_id, out_port, message
    action = SendOutPort.new( port_number: out_port )

    message.buffer_id = OFP_NO_BUFFER
    send_packet_out(
      datapath_id,
      packet_in: message,
      actions: [ action ]
    )
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
