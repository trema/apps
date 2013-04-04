#
# Author: Ryota MIBU <r-mibu@cq.jp.nec.com>
#
# Copyright (C) 2013 NEC Corporation
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

# dump_flows output example:
#   NXST_FLOW reply (xid=0x4):
#    cookie=0x3, duration=1.278s, table=0, n_packets=0, n_bytes=0, idle_timeout=60,priority=65535,udp,in_port=1,vlan_tci=0x0000,dl_src=00:00:00:00:00:02,dl_dst=00:00:00:00:00:01,nw_src=192.168.0.2,nw_dst=192.168.0.1,nw_tos=0,tp_src=1,tp_dst=1 actions=output:2

def count_flow_entries switch, spec=""
  count = 0
  `./trema dump_flows #{ switch }`.each_line do | line |
    next if line =~ /^NXST_FLOW/
    found = true
    spec.split(/[, ]+/).each do | param |
      found = false if not line.include?( param )
    end
    count += 1 if found
  end
  return count
end

Then /^the number of flow entries on (.+) should be ([0-9]+)$/ do | switch, n |
  count_flow_entries( switch ).should == n.to_i
end

Then /^(.+) should have a flow entry like (.+)$/ do | switch, spec |
  count_flow_entries( switch, spec ).should == 1
end

Then /^(.+) should not have a flow entry like (.+)$/ do | switch, spec |
  count_flow_entries( switch, spec ).should == 0
end

### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
