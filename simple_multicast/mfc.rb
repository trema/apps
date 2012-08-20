#
# Multicast Forwarding Cache (MFC)
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


require "set"


#
# A database for multicast forwarding
#
class MFC
  def initialize
    @db = Hash.new do | hash, key |
      hash[ key ] = Set.new
    end
  end


  def learn group, port
    members( group ).add( port )
  end


  def remove group, port
    members( group ).delete( port )
  end


  def members group
    @db[ group.to_i ]
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
