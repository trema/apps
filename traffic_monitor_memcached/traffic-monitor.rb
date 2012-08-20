#
# Simple layer-2 switch with traffic monitoring.
#
# Author: Yasuhito Takamiya <yasuhito@gmail.com>
#
# Copyright (C) 2008-2012 NEC Corporation
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


require "counter"
require "fdb"


class TrafficMonitor < Trema::Controller
  def start
    @counter = Counter.new
    @fdbs = Hash.new do | hash, datapath_id |
      hash[ datapath_id ] = FDB.new datapath_id.to_hex
    end
  end


  def packet_in datapath_id, message
    macsa = message.macsa
    macda = message.macda

    fdb = @fdbs[ datapath_id ]
    fdb.learn macsa, message.in_port
    @counter.add macsa, 1
    out_port = fdb.lookup( macda )
    if out_port
      packet_out datapath_id, message, out_port
      flow_mod datapath_id, macsa, macda, out_port
    else
      flood datapath_id, message
    end
  end


  def flow_removed datapath_id, message
    macsa = message.match.dl_src
    macda = message.match.dl_dst
    info "flow_removed: [#{ datapath_id.to_hex }] match = [dl_src = #{ macsa.to_s }, dl_dst = #{ macda.to_s } ], packet_count = #{ message.packet_count }"
    @counter.add message.match.dl_src, message.packet_count
  end


  ##############################################################################
  private
  ##############################################################################


  def flow_mod datapath_id, macsa, macda, out_port
    idle_timeout = 5
    hard_timeout = 10
    info "flow_mod: [#{ datapath_id.to_hex }] idle_timeout = #{ idle_timeout }, hard_timeout = #{ hard_timeout }, match = [dl_src = #{ macsa.to_s }, dl_dst = #{ macda.to_s } ], actions = [ output: port=#{ out_port } ]"
    send_flow_mod_add(
      datapath_id,
      :idle_timeout => idle_timeout,
      :hard_timeout => hard_timeout,
      :match => Match.new( :dl_src => macsa, :dl_dst => macda ),
      :actions => Trema::ActionOutput.new( out_port )
    )
  end


  def packet_out datapath_id, message, out_port
    macsa = message.macsa
    macda = message.macda
    out_port_desc = ""
    if out_port == 0xfffb
      out_port_desc = "(FLOOD)"
    end
    info "packet_out: [#{ datapath_id.to_hex }], message = [dl_src = #{ macsa.to_s }, dl_dst = #{ macda.to_s } ], actions = [ output: port=#{ out_port }#{ out_port_desc } ]"
    send_packet_out(
      datapath_id,
      :packet_in => message,
      :actions => Trema::ActionOutput.new( out_port )
    )
  end


  def flood datapath_id, message
    packet_out datapath_id, message, OFPP_FLOOD
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
