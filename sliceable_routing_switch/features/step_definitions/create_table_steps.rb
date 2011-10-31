#
# Author: Kazushi SUGYO
#
# Copyright (C) 2008-2011 NEC Corporation
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


Given /^the following filter records$/ do | table |
  create_filter_table
  table.hashes.each do | hash |
    filter_add hash
  end
end


Given /^the following slice records$/ do | table |
  create_slice_table
  table.hashes.each do | hash |
    slice_create hash
  end
end


Given /^the following port binding records$/ do | table |
  table.hashes.each do | hash |
    slice_add_port hash
  end
end


Given /^the following mac binding records$/ do | table |
  table.hashes.each do | hash |
    slice_add_mac hash
  end
end


Given /^the following port and mac binding records$/ do | table |
  table.hashes.each do | hash |
    slice_add_port_mac hash
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
