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


class DatapathEntry
  attr_reader :id
  attr_reader :fdb


  def initialize datapath_id, fdb
    @id = datapath_id
    @fdb = fdb
  end
end


class DatapathDB
  include Trema::Logger


  def initialize
    @db = {}
  end


  def insert datapath_id, fdb
    debug "insert switch: #{ datapath_id }"
    if @db[ datapath_id ]
      warn "duplicated switch ( datapath_id = #{ datapath_id } )"
    end
    @db[ datapath_id ] = DatapathEntry.new( datapath_id, fdb )
  end


  def lookup datapath_id
    debug "lookup switch: #{ datapath_id }"
    @db[ datapath_id ]
  end


  def delete datapath_id
    debug "delete switch: #{ datapath_id }"
    @db.delete( datapath_id )
  end


  def each_value &block
    @db.each_value do | each |
      block.call each.fdb
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
