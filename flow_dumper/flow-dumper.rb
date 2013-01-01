# -*- coding: utf-8 -*-
#
# Flow dumper
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


module Trema
  class FlowStatsReply
    def to_s
      "table_id = #{ table_id }, " +
      "priority = #{ priority }, " +
      "cookie = #{ cookie }, " +
      "idle_timeout = #{ idle_timeout }, " +
      "hard_timeout = #{ hard_timeout }, " +
      "duration = #{ duration_sec }, " +
      "packet_count = #{ packet_count }, " +
      "byte_count = #{ byte_count }, " +
      "match = [#{ match.to_s }], " +
      "actions = [#{ actions.to_s }]"
    end
  end
end


class FlowDumper < Controller
  def start
    send_list_switches_request
  end

  
  def list_switches_reply switches
    request = FlowStatsRequest.new( :match => Match.new )
    switches.each do | each |
      send_message each, request
    end
    @num_switches = switches.size
  end


  def stats_reply datapath_id, message
    message.stats.each do | each |
      if each.is_a?( FlowStatsReply )
        info "[#{ datapath_id.to_hex }] #{ each.to_s }"
      end
    end

    @num_switches -= 1
    if @num_switches == 0
      shutdown!
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
