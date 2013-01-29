# -*- coding: utf-8 -*-
#
# Show switch features
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
  class Port
    def port_description
      if number == Controller::OFPP_LOCAL
        ":Local"
      elsif number == 0 or number > Controller::OFPP_MAX
        ":Invalid port"
      else
        ""
      end
    end


    def state_description
      if up?
        "(Port up)"
      elsif down?
        "(Port down)"
      elsif port_down?
        "(Port is administratively down)"
      elsif link_down?
        "(No physical link present)"
      else
        raise "Unknown Status"
      end
    end
  end


  class DescStatsReply
    def to_a
      "Manufacturer description: #{ mfr_desc }\n" +
      "Hardware description: #{ hw_desc }\n" +
      "Software description: #{ sw_desc }\n" +
      "Serial number: #{ serial_num }\n" +
      "Human readable description of datapath: #{ dp_desc }"
    end
  end

  
  class TableStatsReply
    def to_a
      "Table no: #{ table_id } (#{ name })\n" +
      "  Max flows: #{ max_entries }\n" +
      "  Wildcards: #{ wildcards.to_hex }"
    end
  end
end


class ShowSwitchFeatures < Controller
  def switch_ready datapath_id
    send_message datapath_id, DescStatsRequest.new
    send_message datapath_id, TableStatsRequest.new
    send_message datapath_id, FeaturesRequest.new
  end


  def stats_reply datapath_id, message
    message.stats.each do | each |
      if each.is_a?( DescStatsReply ) or each.is_a?( TableStatsReply )
        info each.to_a
      end
    end
  end


  def features_reply datapath_id, message
    info "Datapath ID: #{ datapath_id.to_hex }"

    message.ports.each do | each |
      info "Port no: #{ each.number }(#{ each.number.to_hex }#{ each.port_description })#{ each.state_description }"
      info "  Hardware address: #{ each.hw_addr.to_s }"
      info "  Port name: #{ each.name }"
    end

    shutdown!
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
