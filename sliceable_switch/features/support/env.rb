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

$LOAD_PATH.unshift( File.expand_path( File.dirname( __FILE__ ) + "/../../../../trema/ruby" ) )


require "tempfile"
require "trema/executables"


def run command
  raise "Failed to execute #{ command }" unless system( command )
end


def ps_entry_of name
  `ps -ef | grep -w "#{ name } " | grep -v grep`
end


def cucumber_log name
  File.join Trema.log, name
end


def new_tmp_log
  system "rm #{ Trema.log }/tmp.*" # cleanup
  `mktemp --tmpdir=#{ Trema.log }`.chomp
end


# show_stats output format:
# ip_dst,tp_dst,ip_src,tp_src,n_pkts,n_octets
def count_packets stats
  return 0 if stats.split.size <= 1
  stats.split[ 1..-1 ].inject( 0 ) do | sum, each |
    sum += each.split( "," )[ 4 ].to_i
  end
end


def slice_db_file
  "tmp/slice.db"
end


def filter_db_file
  "tmp/filter.db"
end


def config_cgi_file
  File.join Trema.home, "tmp/config.cgi"
end


def rest_api_output
  "tmp/rest_api_output"
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


def show_slice_records
  `sqlite3 #{ slice_db_file } 'select * from slices;'`
end


def show_binding_records
  `sqlite3 #{ slice_db_file } 'select * from bindings;'`
end


def show_filter_records
  `sqlite3 #{ filter_db_file } 'select * from filter;'`
end


def insert_slice_record hash
  vals = [ hash[ 'number' ], "'#{ hash[ 'id' ] }'", "'#{ hash[ 'description' ] }'" ]
  run "sqlite3 #{ slice_db_file } \"INSERT INTO 'slices' VALUES(#{ vals.join(',') });\""
end


def insert_binding_record hash
  mac = hash[ 'mac' ].empty? ? "NULL" : hash[ 'mac' ]
  datapath_id = hash[ 'datapath_id' ].empty? ? "NULL" : hash[ 'datapath_id' ]
  port = hash[ 'port' ].empty? ? "NULL" : hash[ 'port' ]
  vid = hash[ 'vid' ].empty? ? "NULL" : hash[ 'vid' ]
  vals = [ hash[ 'type' ], datapath_id, port, vid, mac, "'#{ hash[ 'id' ] }'", hash[ 'slice_number' ] ]
  run "sqlite3 #{ slice_db_file } \"INSERT INTO 'bindings' VALUES(#{ vals.join(',') });\""
end


def insert_filter_record hash
  int_keys = [ 'priority', 'ofp_wildcards', 'in_port', 'dl_src', 'dl_dst', 'dl_vlan', 'dl_vlan_pcp',
               'dl_type', 'nw_tos', 'nw_proto', 'nw_src', 'nw_dst', 'tp_src', 'tp_dst', 'wildcards',
               'in_datapath_id', 'slice_number', 'action' ]
  vals = []
  int_keys.each { | key | vals.push( hash[ key ].to_i(0) ) }
  vals.push( "'#{ hash[ 'id' ] }'" )
  run "sqlite3 #{ filter_db_file } \"INSERT INTO 'filter' VALUES(#{ vals.join(',') });\""
end


def deploy_config_cgi
  File.open( File.join( src_directory, "config.cgi" ), "r" ) do | src |
    File.open( config_cgi_file, "w" ) do | dst |
      dst.write src.read.gsub( "/home/sliceable_switch/db", Trema.tmp )
    end
  end
  File.chmod( 0755, config_cgi_file )
end


ENV[ "SLICE_DB_FILE" ] = slice_db_file
ENV[ "FILTER_DB_FILE" ] = filter_db_file
ENV[ "PERL5LIB" ] = src_directory


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
