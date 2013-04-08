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


Given /^the REST interface for sliceable_switch$/ do
  deploy_config_cgi
  create_filter_table
  create_slice_table
end


Given /^the following records were inserted into slices table$/ do | table |
  table.hashes.each do | hash |
    insert_slice_record hash
  end
end


Given /^the following records were inserted into bindings table$/ do | table |
  table.hashes.each do | hash |
    insert_binding_record hash
  end
end


def request_via_rest_if path, method="GET", req_body=""
  raise "unkown method #{ method }" if not method =~ /(GET|POST|PUT|DELETE)/
  raise "no request body" if method =~ /(POST|PUT)/ and req_body.empty?

  ENV.update({'PATH_INFO', path, 'REQUEST_METHOD', method})
  command = "#{ config_cgi_file }"
  command = "echo '#{ req_body }' | " + command if not req_body.empty?
  ret = `#{ command }`
  raise "Failed to execute : #{ command }" if $? != 0
  File.open( rest_api_output, "w" ) do | output |
    output.write ret
  end
end


def response_status_from_rest_if
  File.read( rest_api_output ).split( /\r\n/ )[ 0 ].sub( 'Status: ', '' )
end


def response_body_from_rest_if
  head = /^Status: \d+\r\n[^\r\n]+\r\n\r\n/
  File.read( rest_api_output ).sub( head, '' )
end


When /^I try to send request (GET|DELETE) (.*)$/ do | method, path |
  request_via_rest_if( path, method )
end


When /^I try to send request (POST|PUT) (.*) with$/ do | method, path, req_body |
  request_via_rest_if( path, method, req_body )
end


Then /^the response status shoud be (.*)$/ do | status |
  response_status_from_rest_if.chomp.should == status
end


Then /^the response body shoud be:$/ do | body |
  response_body_from_rest_if.chomp.should == body.chomp
end


Then /^the response body shoud be like:$/ do | body_regx |
  response_body_from_rest_if.chomp.should =~ Regexp.new( body_regx )
end


Then /^the slice records shoud be:$/ do | table |
  expected = table.hashes.map { | hash | "#{ hash[ 'number' ] }|#{ hash[ 'id' ] }|#{ hash[ 'description' ] }" }
  show_slice_records.each_line do | line |
    line.chomp.should == expected.shift
  end
  expected.count.should == 0
end


Then /^the slice records shoud be like:$/ do | table |
  expected = table.hashes.map { | hash | "#{ hash[ 'number' ] }|#{ hash[ 'id' ] }|#{ hash[ 'description' ] }" }
  show_slice_records.each_line do | line |
    line.chomp.should =~ Regexp.new( expected.shift )
  end
  expected.count.should == 0
end


Then /^the binding records shoud be:$/ do | table |
  expected = table.hashes.map { | hash | "#{ hash[ 'type' ] }|#{ hash[ 'datapath_id' ] }|" +
                                         "#{ hash[ 'port' ] }|#{ hash[ 'vid' ] }|" +
                                         "#{ hash[ 'mac' ] }|#{ hash[ 'id' ] }|" +
                                         "#{ hash[ 'slice_number' ] }" }
  show_binding_records.each_line do | line |
    line.chomp.should == expected.shift
  end
  expected.count.should == 0
end


### Local variables:
### mode: Ruby
### coding: utf-8-unix
### indent-tabs-mode: nil
### End:
