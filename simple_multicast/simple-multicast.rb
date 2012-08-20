#
# Simple multicast switch application in Ruby
#
# Author: Kazuya Suzuki
#
# Copyright (C) 2012 NEC Corporation
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


require "mfc"

#
# A OpenFlow controller class which handles IPv4 multicast.
#
class SimpleMulticast < Controller


  def start
    @mfc = MFC.new
  end


  def packet_in datapath_id, message
    if message.igmp?
      handle_igmp message
    else
      members = @mfc.members( message.ipv4_daddr )
      flow_mod datapath_id, members, message
      packet_out datapath_id, members, message
    end
  end


  ##############################################################################
  private
  ##############################################################################


  def handle_igmp message
    group = message.igmp_group
    port = message.in_port

    if message.igmp_v2_membership_report?
      @mfc.learn group, port
    elsif message.igmp_v2_leave_group?
      @mfc.remove group, port
    end
  end


  def flow_mod datapath_id, members, message
    send_flow_mod_add(
      datapath_id,
      :match => ExactMatch.from( message ),
      :actions => output_actions( members ),
      :hard_timeout => 5
    )
  end


  def packet_out datapath_id, members, message
    send_packet_out(
      datapath_id,
      :packet_in => message,
      :actions => output_actions( members )
    )
  end


  def output_actions members
    members.collect do | each |
      ActionOutput.new( :port => each )
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
