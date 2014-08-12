# -*- mode: cperl -*-

# REST API for sliceable switch application
# using PSGI (Perl Web Server Gateway Interface)
#
# Author: Akihiro Motoki
#
# Copyright (C) 2014 NEC Corporation
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

# Requirements:
#   libplack-perl
#   sqlite3 libdbi-perl libdbd-sqlite3-perl
#
# How to run:
#   cd $TREMA_APPS_REPO/sliceable_switch
#   plackup -r -p 8888 -I <TREMA_APPS>/sliceable_switch \
#           -R <TREMA_APPS>/sliceable_switch
#           <TREMA_APPS>/sliceable_switch/config.pgsi

use strict;
use warnings;
use utf8;
use bignum;
# use Plack::Builder;
use Plack::Request;
use Encode qw(encode);
use JSON qw(to_json from_json);
use Time::HiRes qw(gettimeofday);
use Slice;
use Filter;

my $Debug = 0;

my $SliceDBFile = "/home/sliceable_switch/db/slice.db";
my $FilterDBFile = "/home/sliceable_switch/db/filter.db";
my $Slice;
my $Filter;

my $app = sub {
    my $env = shift;
    my $req = Plack::Request->new($env);
    my $res;

    my $path_string = $req->path_info;
    if($path_string !~ /^\/networks|filters/){
        return reply_not_found($req);
    }

    my @path = split('/', $path_string);
    shift(@path);
    my $method = $req->method;

    $Slice = Slice->new($SliceDBFile);
    if(!defined($Slice)){
        return reply_error($req, "Failed to open slice database.");
    }

    $Filter = Filter->new($FilterDBFile, $SliceDBFile);
    if(!defined($Filter)){
        if(defined($Slice)){
            $Slice->close();
        }
        return reply_error($req, "Failed to open filter database.");
    }

    if($method eq "GET"){
        $res = handle_get_requests($req, \@path);
    }
    elsif($method eq "POST"){
        $res = handle_post_requests($req, \@path);
    }
    elsif($method eq "PUT"){
        $res = handle_put_requests($req, \@path);
    }
    elsif($method eq "DELETE"){
        $res = handle_delete_requests($req, \@path);
    }
    else{
        $res = reply_not_implemented($req, "Unhandled request method ($method)");
    }

    $Slice->close();
    return $res;
};


sub handle_get_requests(){
  my $req = shift;
  my $path = shift;
  print $path;

  if($path->[0] eq "networks"){
    my $slice_id;
    my $binding_id;

    if(@$path >= 2){
      $slice_id = $path->[1];
    }
    if(@$path >= 4){
      $binding_id = $path->[3];
    }

    if(@$path == 1){
      list_networks($req);
    }
    elsif(@$path == 2){
      show_network($req, $slice_id);
    }
    elsif($path->[2] eq "ports" && @$path == 3){
      show_ports($req, $slice_id);
    }
    elsif($path->[2] eq "attachments" && @$path == 3){
      show_attachments($req, $slice_id);
    }
    elsif($path->[2] eq "ports" && @$path == 4){
      show_port($req, $slice_id, $binding_id);
    }
    elsif($path->[2] eq "attachments" && @$path == 4){
      show_attachment($req, $slice_id, $binding_id);
    }
    elsif($path->[2] eq "ports" && $path->[4] eq "attachments" && @$path == 5){
      show_attachments_on_port($req, $slice_id, $binding_id);
    }
    elsif($path->[2] eq "ports" && $path->[4] eq "attachments" && @$path == 6){
      $binding_id = $path->[5];
      show_attachment_on_port($req, $slice_id, $binding_id);
    }
    else{
      reply_not_found($req);
    }
  }
  elsif($path->[0] eq "filters"){
    if(@$path == 1){
      show_filters($req);
    }
    elsif(@$path == 2){
      my $filter_id = $path->[1];
      show_filters($req, $path->[1]);
    }
    else{
      reply_not_found($req);
    }
  }
  else{
    reply_not_found($req);
  }
}


sub list_networks(){
    my $req = shift;

    my @slices = $Slice->get_slices();
    reply_ok_with_json($req, to_json(\@slices));
}


sub show_network(){
    my $req = shift;
    my $slice_id = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_ALL, $slice_id);
    my %output = ();

    my $slice_description;
    if($Slice->get_slice_description_by_id($slice_id, \$slice_description) < 0){
        return reply_error("Failed to retrieve slice description.");
    }

    if(!defined($slice_description)){
        $slice_description = "";
    }
    $output{'description'} = $slice_description;
    @{$output{'bindings'}} = ();

    foreach my $id (keys(%bindings)){
        if($bindings{$id}{'type'} == Slice::BINDING_TYPE_PORT){
            my %binding = ();
            $binding{'id'} = sprintf("%s", $id);
            $binding{'type'} = $bindings{$id}{'type'};
            $binding{'datapath_id'} = sprintf("%s", $bindings{$id}{'datapath_id'});
            $binding{'port'} = $bindings{$id}{'port'};
            $binding{'vid'} = $bindings{$id}{'vid'};
            push(@{$output{'bindings'}}, \%binding);
        }
        elsif($bindings{$id}{'type'} == Slice::BINDING_TYPE_MAC){
            my %binding = ();
            $binding{'id'} = $id;
            $binding{'type'} = $bindings{$id}{'type'};
            $binding{'mac'} = $bindings{$id}{'mac'};
            push(@{$output{'bindings'}}, \%binding);
        }
        elsif($bindings{$id}{'type'} == Slice::BINDING_TYPE_PORT_MAC){
            my %binding = ();
            $binding{'id'} = sprintf("%s", $id);
            $binding{'type'} = $bindings{$id}{'type'};
            $binding{'datapath_id'} = sprintf("%s", $bindings{$id}{'datapath_id'});
            $binding{'port'} = $bindings{$id}{'port'};
            $binding{'vid'} = $bindings{$id}{'vid'};
            $binding{'mac'} = $bindings{$id}{'mac'};
            push(@{$output{'bindings'}}, \%binding);
        }
    }

    reply_ok_with_json($req, to_json(\%output));
}


sub show_ports(){
    my $req = shift;
    my $slice_id = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_PORT, $slice_id);
    my @output = ();

    foreach my $id (keys(%bindings)){
        my %binding = ();
        $binding{'id'} = sprintf("%s", $id);
        $binding{'datapath_id' } = sprintf("%s", $bindings{$id}{'datapath_id'});
        $binding{'port'} = $bindings{$id}{'port'};
        $binding{'vid'} = $bindings{$id}{'vid'};
        push(@output, \%binding);
    }

    reply_ok_with_json($req, to_json(\@output));
}


sub show_attachments(){
    my $req = shift;
    my $slice_id = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_MAC, $slice_id);
    my @output = ();

    foreach my $id (keys(%bindings)){
        my %binding = ();
        $binding{'id'} = sprintf("%s", $id);
        $binding{'mac'} = sprintf("%s", $bindings{$id}{'mac'});
        push(@output, \%binding);
    }

    reply_ok_with_json($req, to_json(\@output));
}


sub show_port(){
    my $req = shift;
    my $slice_id = shift;
    my $port = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_PORT, $slice_id, $port);

    my $found = 0;
    my %output = ();
    foreach my $id (keys(%bindings)){
        $output{'config'}{'datapath_id'} = sprintf("%s", $bindings{$id}{'datapath_id'});
        $output{'config'}{'port'} = $bindings{$id}{'port'};
        $output{'config'}{'vid'} = $bindings{$id}{'vid'};
        $found++;
    }

    %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_PORT_MAC, $slice_id);

    foreach my $id (keys(%bindings)){
        if($bindings{$id}{'port'} != $output{'config'}{'port'}){
            next;
        }
        my %binding = ();
        $binding{'id'} = sprintf("%s", $id);
        $binding{'mac'} = sprintf("%s", $bindings{$id}{'mac'});
        push(@{$output{'attachments'}}, \%binding);
    }

    if($found == 0){
        return reply_not_found($req);
    }

    reply_ok_with_json($req, to_json(\%output));
}


sub show_attachment(){
    my $req = shift;
    my $slice_id = shift;
    my $binding_id = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_MAC, $slice_id, $binding_id);

    my %binding = ();
    foreach my $id (keys(%bindings)){
        $binding{'id'} = sprintf("%s", $id);
        $binding{'mac'} = sprintf("%s", $bindings{$id}{'mac'});
    }

    if(!%binding){
        return reply_not_found($req);
    }

    reply_ok_with_json($req, to_json(\%binding));
}


sub show_attachments_on_port(){
    my $req = shift;
    my $slice_id = shift;
    my $port = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_PORT_MAC, $slice_id);

    my @output = ();
    foreach my $id (keys(%bindings)){
        my %binding = ();
        $binding{'id'} = sprintf("%s", $id);
        $binding{'mac'} = sprintf("%s", $bindings{$id}{'mac'});
        push(@output, \%binding);
    }

    reply_ok_with_json($req, to_json(\@output));
}


sub show_attachment_on_port(){
    my $req = shift;
    my $slice_id = shift;
    my $binding_id = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my %bindings = $Slice->get_bindings(Slice::BINDING_TYPE_PORT_MAC, $slice_id, $binding_id);

    my %binding = ();
    foreach my $id (keys(%bindings)){
        $binding{'id'} = sprintf("%s", $id);
        $binding{'mac'} = sprintf("%s", $bindings{$id}{'mac'});
    }

    if(!%binding){
        return reply_not_found($req);
    }

    reply_ok_with_json($req, to_json(\%binding));
}


sub show_filters(){
    my $req = shift;
    my $id = shift;
    my %filters = $Filter->get_filters($id);

    my $count = 0;
    my @out = ();
    foreach my $id (keys(%filters)){
        my %f = %{$filters{$id}};
        $f{'id'} = $id;
        $f{'ofp_wildcards'} = $Filter->ofp_wildcards_to_string($f{'ofp_wildcards'});
        $f{'wildcards'} = $Filter->wildcards_to_string($f{'wildcards'});
        push(@out, \%f);
        $count++;
    }

    if(defined($id)){
        if($count > 0){
            reply_ok_with_json($req, to_json(\%{$out[0]}));
        }
        else{
            reply_not_found($req);
        }
    }
    else{
        reply_ok_with_json($req, to_json(\@out));
    }
}


sub handle_post_requests(){
    my $req = shift;
    my $path = shift;

    if($path->[0] eq "networks"){
        my ($slice_id, $binding_id) = ();
        if(@$path >= 2){
            $slice_id = $path->[1];
        }
        if(@$path >= 4){
            $binding_id = $path->[3];
        }

        if(@$path == 1){
            create_network($req);
        }
        elsif(@$path == 2){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "ports" && @$path == 3){
            create_port($req, $slice_id);
        }
        elsif($path->[2] eq "attachments" && @$path == 3){
            create_attachment($req, $slice_id);
        }
        elsif($path->[2] eq "ports" && @$path == 4){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "attachments" && @$path == 4){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "ports" && $path->[4] eq "attachments" && @$path == 5){
            create_attachment_on_port($req, $slice_id, $binding_id);
        }
        elsif($path->[2] eq "ports" && $path->[4] eq "attachments" && @$path == 6){
            reply_method_not_allowed($req);
        }
        else{
            reply_not_found($req);
        }
    }
    elsif($path->[0] eq "filters"){
        if(@$path == 1){
            add_filters($req);
        }
        elsif(@$path == 2){
            reply_method_not_allowed($req);
        }
        else{
            reply_not_found($req);
        }
    }
    else{
        reply_not_found($req);
    }
}


sub create_network(){
    my $req = shift;
    my $content_string = get_request_body($req);
    my $content = from_json($content_string);
    my $slice_id = ${$content}{'id'};
    my $description = ${$content}{'description'};

    if(!defined($slice_id)){
        my ($seconds, $microseconds) = gettimeofday();
        $slice_id = sprintf("%10u%06u%02u", $seconds, $microseconds, int(rand(100)));
    }

    if($Slice->get_slice_number_by_id($slice_id) == Slice::SUCCEEDED){
        return reply_unprocessable_entity("Failed to create a slice (duplicated slice id).");
    }

    if($Slice->create_slice($slice_id, $description) < 0){
        return reply_error("Failed to create a slice.");
    }

    my %created = ();
    $created{'id'} = $slice_id;
    $created{'description'} = $description || "";

    reply_accepted_with_json($req, to_json(\%created));
}


sub create_port(){
    my $req = shift;
    my $slice_id = shift;

    my $content_string = get_request_body($req);
    my $content = from_json($content_string);
    my $dpid = oct(${$content}{'datapath_id'});
    my $port = ${$content}{'port'};
    my $vid = ${$content}{'vid'};
    my $binding_id = sprintf("%04x%08x:%04x:%04x", $dpid >> 32, $dpid & 0xffffffff, $port, $vid);
    if(defined(${$content}{'id'})){
        $binding_id = ${$content}{'id'};
    }
    my $err;
    $Slice->get_bindings(undef, $slice_id, $binding_id, \$err);
    if($err == Slice::NO_SLICE_FOUND){
        return reply_not_found($req);
    }
    elsif($err == Slice::SUCCEEDED){
        return reply_unprocessable_entity("Failed to create a binding (duplicated binding id).");
    }

    if($Slice->add_port($binding_id, $slice_id, $dpid, $port, $vid) < 0){
        return reply_error("Failed to add a port to '$slice_id'");
    }

    reply_accepted($req);
}


sub create_attachment(){
    my $req = shift;
    my $slice_id = shift;

    my $content_string = get_request_body($req);
    my $content = from_json($content_string);
    my $mac = ${$content}{'mac'};
    my $binding_id = $mac;
    if(defined(${$content}{'id'})){
        $binding_id = ${$content}{'id'};
    }
    my $err;
    $Slice->get_bindings(undef, $slice_id, $binding_id, \$err);
    if($err == Slice::NO_SLICE_FOUND){
        return reply_not_found($req);
    }
    elsif($err == Slice::SUCCEEDED){
        return reply_unprocessable_entity("Failed to create a binding (duplicated binding id).");
    }

    $mac = mac_string_to_int($mac);
    if($Slice->add_mac($binding_id, $slice_id, $mac) < 0){
        return reply_error("Failed to add a mac-based binding to '$slice_id'");
    }

    reply_accepted($req);
}


sub create_attachment_on_port(){
    my $req = shift;
    my $slice_id = shift;
    my $port = shift;

    my $content_string = get_request_body($req);
    my $content = from_json($content_string);
    my $mac = ${$content}{'mac'};
    my $binding_id = ${$content}{'id'};

    if(!defined(${$content}{'id'})){
        $binding_id = sprintf("%04x%08x", $mac >> 32, $mac & 0xffffffff);
    }

    my $err;
    $Slice->get_bindings(undef, $slice_id, $binding_id, \$err);
    if($err == Slice::NO_SLICE_FOUND){
        return reply_not_found($req);
    }
    elsif($err == Slice::SUCCEEDED){
        return reply_unprocessable_entity("Failed to create a binding (duplicated binding id).");
    }

    $mac = mac_string_to_int($mac);

    my $ret = $Slice->add_mac_on_port($binding_id, $slice_id, $port, $mac);
    if($ret == Slice::NO_BINDING_FOUND){
        return reply_unprocessable_entity("Failed to create a binding (port not found).");
    }
    elsif($ret < 0){
        return reply_error("Failed to add a mac-based binding on a port to '$slice_id'");
    }

    reply_accepted($req);
}


sub add_filters(){
    my $req = shift;

    my $content_string = get_request_body($req);
    my $content = from_json($content_string);

    if(ref($content) ne 'ARRAY'){
        $content = [$content];
    }

    foreach my $filter (@{$content}){
        my @criteria = ();
        $criteria[0] = oct(${$filter}{'priority'}) if(defined(${$filter}{'priority'}));
        $criteria[1] = ${$filter}{'ofp_wildcards'};
        $criteria[2] = oct(${$filter}{'in_port'}) if(defined(${$filter}{'in_port'}));
        $criteria[3] = ${$filter}{'dl_src'};
        $criteria[4] = ${$filter}{'dl_dst'};
        $criteria[5] = oct(${$filter}{'dl_vlan'}) if(defined(${$filter}{'dl_vlan'}));
        $criteria[6] = oct(${$filter}{'dl_vlan_pcp'}) if(defined(${$filter}{'dl_vlan_pcp'}));
        $criteria[7] = oct(${$filter}{'dl_type'}) if(defined(${$filter}{'dl_type'}));
        $criteria[8] = oct(${$filter}{'nw_tos'}) if(defined(${$filter}{'nw_tos'}));
        $criteria[9] = oct(${$filter}{'nw_proto'}) if(defined(${$filter}{'nw_proto'}));
        $criteria[10] = ${$filter}{'nw_src'};
        $criteria[11] = ${$filter}{'nw_dst'};
        $criteria[12] = oct(${$filter}{'tp_src'}) if(defined(${$filter}{'tp_src'}));
        $criteria[13] = oct(${$filter}{'tp_dst'}) if(defined(${$filter}{'tp_dst'}));
        $criteria[14] = ${$filter}{'wildcards'};
        $criteria[15] = oct(${$filter}{'in_datapath_id'}) if(defined(${$filter}{'in_datapath_id'}));
        $criteria[16] = ${$filter}{'slice'};
        $criteria[17] = ${$filter}{'action'};
        my $id = ${$filter}{'id'};

        my %filters = $Filter->get_filters($id);
        if(%filters && length(keys(%filters)) > 0){
            return reply_unprocessable_entity("Failed to add a filter entry (duplicated filter id).");
        }

        @criteria = $Filter->set_default_filter_criteria(@criteria);
        if($Filter->add_filter(@criteria, $id) < 0){
            return reply_error("Failed to add a filter entry.");
        }
    }

    reply_accepted($req);
}


sub handle_put_requests(){
    my $req = shift;
    my $path = shift;

    if($path->[0] eq "networks"){
        my $slice_id;
        if(@$path >= 2){
            $slice_id = $path->[1];
        }

        if(@$path == 1){
            reply_method_not_allowed($req);
        }
        elsif(@$path == 2){
            modify_network($req, $slice_id);
        }
        elsif($path->[2] eq "ports" && @$path == 3){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "attachments" && @$path == 3){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "ports" && @$path == 4){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "attachments" && @$path == 4){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "ports" && $path->[4] eq "attachments" && @$path == 5){
            reply_method_not_allowed($req);
        }
        elsif($path->[2] eq "ports" && $path->[4] eq "attachments" && @$path == 6){
            reply_method_not_allowed($req);
        }
        else{
            reply_not_found($req);
        }
    }
    elsif($path->[0] eq "filters"){
        if(@$path == 1){
            reply_method_not_allowed($req);
        }
        elsif(@$path == 2){
            reply_method_not_allowed($req);
        }
        else{
            reply_not_found($req);
        }
    }
    else{
        reply_not_found($req);
    }
}


sub modify_network(){
    my $req = shift;
    my $slice_id = shift;

    my $content_string = get_request_body($req);
    my $content = from_json($content_string);
    my $description = ${$content}{'description'};

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    if($Slice->update_slice($slice_id, $description) < 0){
        return reply_error("Failed to modify a slice.");
    }

    reply_accepted($req);
}


sub handle_delete_requests(){
    my $req = shift;
    my $path = shift;

    if($path->[0] eq "networks"){
        my ($slice_id, $binding_id) = ();
        if(@$path >= 2){
            $slice_id = $path->[1];
        }
        if(@$path >= 4){
            $binding_id = $path->[3];
        }

        if($path->[0] eq "networks" && @$path == 1){
            reply_method_not_allowed($req);
        }
        elsif($path->[0] eq "networks" && @$path == 2){
            destroy_network($req, $slice_id);
        }
        elsif($path->[0] eq "networks" && $path->[2] eq "ports" && @$path == 3){
            reply_method_not_allowed($req);
        }
        elsif($path->[0] eq "networks" && $path->[2] eq "attachments" && @$path == 3){
            reply_method_not_allowed($req);
        }
        elsif($path->[0] eq "networks" && $path->[2] eq "ports" && @$path == 4){
            delete_binding($req, $slice_id, $binding_id);
        }
        elsif($path->[0] eq "networks" && $path->[2] eq "attachments" && @$path == 4){
            delete_binding($req, $slice_id, $binding_id);
        }
        elsif($path->[0] eq "networks" && $path->[2] eq "ports" &&
	      $path->[4] eq "attachments" && @$path == 5){
            reply_method_not_allowed($req);
        }
        elsif($path->[0] eq "networks" && $path->[2] eq "ports" &&
	      $path->[4] eq "attachments" && @$path == 6){
            $binding_id = $path->[5];
            delete_binding($req, $slice_id, $binding_id);
        }
        else{
            reply_not_found($req);
        }
    }
    elsif($path->[0] eq "filters"){
        if(@$path == 1){
            reply_method_not_allowed($req);
        }
        elsif(@$path == 2){
            my $id = $path->[1];
            delete_filter($req, $id);
        }
        else{
            reply_not_found($req);
        }
    }
    else{
        reply_not_found($req);
    }
}


sub destroy_network(){
    my $req = shift;
    my $slice_id = shift;

    my $ret = $Slice->destroy_slice($slice_id);
    if($ret == Slice::NO_SLICE_FOUND){
        return reply_not_found($req);
    }
    elsif($ret < 0){
        return reply_error("Failed to destroy a slice.");
    }

    reply_accepted($req);
}


sub delete_binding(){
    my $req = shift;
    my $slice_id = shift;
    my $binding_id = shift;

    if($Slice->get_slice_number_by_id($slice_id) < 0){
        return reply_not_found($req);
    }

    my $ret = $Slice->delete_binding_by_id($slice_id, $binding_id);
    if($ret == Slice::NO_SLICE_FOUND){
        return reply_not_found($req);
    }
    elsif($ret == Slice::NO_BINDING_FOUND){
        return reply_not_found($req);
    }
    elsif($ret < 0){
        return reply_error("Failed to delete a binding.");
    }

    reply_accepted($req);
}


sub delete_filter(){
    my $req = shift;
    my $filter_id = shift;

    my %filters = $Filter->get_filters($filter_id);
    if(!%filters){
        return reply_not_found($req);
    }

    if($Filter->delete_filter_by_id($filter_id) < 0){
        return reply_error("Failed to delete a filter.");
    }

    reply_accepted($req);
}

#------------------------------------------------------------

sub get_request_body(){
    my $req = shift;
    return $req->content;
}


sub mac_string_to_int(){
    my $string = shift;

    $string =~ s/://g;

    return hex( "0x" . $string);
}


sub reply_ok_with_json(){
    my $req = shift;
    my $json = shift;

    my $res = $req->new_response;
    $res->status(200);
    $res->headers({'Content-Type' => 'application/json'});
    $json .= "\n";
    $res->body($json);
    $res->finalize;
}


sub reply_accepted(){
    my $req = shift;
    send_response($req, 202);
}


sub reply_accepted_with_json(){
    my $req = shift;
    my $json = shift;

    my $res = $req->new_response;
    $res->status(202);
    $res->headers({'Content-Type' => 'application/json'});
    $json .= "\n";
    $res->body($json);
    $res->finalize;
}


sub reply_not_found(){
    my $req = shift;
    send_response($req, 404);
}


sub reply_method_not_allowed(){
    my $req = shift;
    send_response($req, 405);
}


sub reply_unprocessable_entity(){
    my $req =shift;
    my $msg = shift;
    send_response($req, 422, $msg);
}


sub reply_error(){
    my $req =shift;
    my $msg = shift;
    send_response($req, 500, $msg);
}


sub reply_not_implemented(){
    my $req =shift;
    my $msg = shift;
    send_response($req, 501, $msg);
}


sub send_response(){
    my $req = shift;
    my $code = shift;
    my $body = shift;

    my $res = $req->new_response;
    $res->status($code);
    $res->headers({'Content-Type' => 'text/plain; charset=UTF-8'});
    if (defined($body)) {
        $res->body(encode('UTF-8', $body));
    }
    $res->finalize;
}
