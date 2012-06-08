#
# Forwarding database (FDB) of layer-2 switch.
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


require 'rubygems'
require 'memcache'


class FDB
  DEFAULT_AGE_MAX = 300

  def initialize namespace
    @db = MemCache.new( '127.0.0.1:11211',
                        {
			  :namespace => "fdb-" << namespace,
			  :namespace_separator => ','
			}
		      )
  end


  def lookup mac
    @db[ mac.to_s ]
  end


  def learn mac, port_number
    @db.set mac.to_s, port_number, DEFAULT_AGE_MAX
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
