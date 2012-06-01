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


def slice_db_file
  "tmp/slice.db"
end


def filter_db_file
  "tmp/filter.db"
end


def src_directory
  File.join( File.dirname( __FILE__ ), "..", ".." )
end


def filter_add args
  filter = File.join( src_directory, "filter" )
  run "#{ filter } add #{ args[ 'filter_id' ] } #{ args[ 'rule_specification' ] }"
end


def slice_create args
  slice = File.join( src_directory, "slice" )
  run "#{ slice } create #{ args[ 'slice_id' ] } #{ args[ 'description' ] }"
end


def slice_add_port args
  slice = File.join( src_directory, "slice" )
  run "#{ slice } add-port #{ args[ 'slice_id' ] } #{ args[ 'dpid' ] } #{ args[ 'port' ] } #{ args[ 'vid' ] } #{ args[ 'binding_id' ] }"
end


def slice_add_mac args
  slice = File.join( src_directory, "slice" )
  run "#{ slice } add-mac #{ args[ 'slice_id' ] } #{ args[ 'address' ] } #{ args[ 'binding_id' ] }"
end


def slice_add_port_mac args
  slice = File.join( src_directory, "slice" )
  run "#{ slice } add-port-mac #{ args[ 'slice_id' ] } #{ args[ 'port_binding_id' ] } #{ args[ 'address' ] } #{ args[ 'binding_id' ] }"
end


def create_filter_table
  if FileTest.exists?( filter_db_file )
    run "sqlite3 #{ filter_db_file } 'delete from filter;'"
  else
    create_table_sql = File.join( src_directory, "create_filter_table.sql" )
    run "sqlite3 #{ filter_db_file } < #{ create_table_sql }"
  end
end


def create_slice_table
  if FileTest.exists?( slice_db_file )
    run "sqlite3 #{ slice_db_file } 'delete from slices; delete from bindings;'"
  else
    create_table_sql = File.join( src_directory, "create_slice_table.sql" )
    run "sqlite3 #{ slice_db_file } < #{ create_table_sql }"
  end
end


def create_filter_table_from script
  if FileTest.exists?( filter_db_file )
    File.delete filter_db_file
  end
  run "#{ script } | sqlite3 #{ filter_db_file }"
end


def create_slice_table_from script
  if FileTest.exists?( slice_db_file )
    File.delete slice_db_file
  end
  run "#{ script } | sqlite3 #{ slice_db_file }"
end


ENV[ "SLICE_DB_FILE" ] = slice_db_file
ENV[ "FILTER_DB_FILE" ] = filter_db_file
ENV[ "PERL5LIB" ] = src_directory


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
