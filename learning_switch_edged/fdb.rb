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


require_relative "mac"


class ForwardingEntry
  attr_reader :mac
  attr_reader :port_number
  attr_reader :created_at
  attr_reader :updated_at


  def initialize mac, port_number
    @mac = mac
    @port_number = port_number
    @created_at = Time.now
    @updated_at = @created_at
  end


  def update port_number
    @port_number = port_number
    @created_at = Time.now
    @updated_at = @created_at
  end


  def update_timestamp
    @updated_at = Time.now
  end


  def aged? time
    @updated_at < time
  end
end


class FDB
  include Trema::Logger


  MAX_AGE = 300


  def initialize
    @db = {}
  end


  def lookup mac
    entry = @db[ mac ]
    if entry and entry.aged?( Time.new - MAX_AGE )
      entry = nil
    end
    debug "lookup fdb: #{ mac } -> #{ entry ? entry.port_number : 0 }"
    entry
  end


  def update mac, port_number
    if !mac.valid?
      warn "learn fdb: invalid mac address ( mac address = #{ mac }, port_number = #{ port_number } )"
      return
    end
    entry = @db[ mac ]
    if entry.nil?
      debug "learn fdb: #{ mac } -> #{ port_number }"
      @db[ mac ] = ForwardingEntry.new( mac, port_number )
    elsif entry.port_number != port_number
      debug "learn fdb: #{ mac } -> #{ port_number } ( old port number = #{ entry.port_number })"
      entry.update( port_number )
    else
      entry.update_timestamp
    end
  end


  def delete_entries_by_port_number port_number
    @db.delete_if do | mac, entry |
      debug "delete fdb: #{ mac } ( port number = #{ entry.port_number } )"
      entry.port_number == port_number
    end
  end


  def delete_aged_entries
    time = Time.new - MAX_AGE
    @db.delete_if do | mac, entry |
      debug "delete fdb: #{ mac } ( port number = #{ entry.port_number } )"
      entry.aged? time
    end
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
