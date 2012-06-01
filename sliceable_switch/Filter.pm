#
# Packet filter configuration module for sliceable switch application.
#
# Author: Yasunobu Chiba
#
# Copyright (C) 2011-2012 NEC Corporation
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


package Filter;

use strict;
use bignum;
use Time::HiRes qw(gettimeofday);
use DBI;
use Carp qw(croak);
use Slice;

use constant{
    SUCCEEDED => 0,
    FAILED => -1,
    NO_SLICE_FOUND => -2,
    UNDEFINED_ACTION => -3
};

use constant{
    ACTION_ALLOW => 0,
    ACTION_DENY => 1,
    ACTION_LOCAL => 2
};

use constant{
    OFPFW_IN_PORT => 0x00000001,
    OFPFW_DL_VLAN => 0x00000002,
    OFPFW_DL_SRC => 0x00000004,
    OFPFW_DL_DST => 0x00000008,
    OFPFW_DL_TYPE => 0x00000010,
    OFPFW_NW_PROTO => 0x00000020,
    OFPFW_TP_SRC => 0x00000040,
    OFPFW_TP_DST => 0x00000080,
    OFPFW_NW_SRC_SHIFT => 8,
    OFPFW_NW_SRC_MASK => 0x00003f00,
    OFPFW_NW_SRC_ALL => 0x00002000,
    OFPFW_NW_DST_SHIFT => 14,
    OFPFW_NW_DST_MASK => 0x000fc000,
    OFPFW_NW_DST_ALL => 0x00080000,
    OFPFW_DL_VLAN_PCP => 0x00100000,
    OFPFW_NW_TOS => 0x00200000,
    OFPFW_ALL => 0x003fffff,
};

use constant{
    WILDCARD_IN_DATAPATH_ID => 0x00000001,
    WILDCARD_SLICE_NUMBER => 0x00000002,
    WILDCARD_ALL => 0x00000003
};

my $Debug = 0;

sub new(){
    my ($class, $filter_db_file, $slice_db_file) = @_;
    my $hash = {
	dbh => undef,
	slice => undef
    };

    $hash->{'slice'} = Slice->new($slice_db_file);
    if(!defined($hash->{'slice'})){
	return undef;
    }

    $hash->{'dbh'} = DBI->connect("dbi:SQLite:dbname=$filter_db_file", "", "");
    if(!defined($hash->{'dbh'})){
	if(defined($hash->{'slice'})){
	    $hash->{'slice'}->close();
	}
	return undef;
    }

    bless($hash, $class);
}


sub close(){
    my $self = shift;

    if(defined($self->{'dbh'})){
	$self->{'dbh'}->disconnect();
    }
    if(defined($self->{'slice'})){
	$self->{'slice'}->close();
    }
}


sub add_filter(){
    my($self, $priority, $ofp_wildcards, $in_port, $dl_src, $dl_dst, $dl_vlan,
       $dl_vlan_pcp, $dl_type, $nw_tos, $nw_proto, $nw_src, $nw_dst,
       $tp_src, $tp_dst, $wildcards, $in_datapath_id, $slice, $action, $id) = @_;

    if(!defined($id)){
	my ($seconds, $microseconds) = gettimeofday();
	$id = sprintf("%10u%06u%04u", $seconds, $microseconds, int(rand(10000)));
    }

    my $slice_number = 0;
    if(defined($slice) && length($slice) > 0){
	if($self->{'slice'}->get_slice_number_by_id($slice, \$slice_number) < 0){
	    return NO_SLICE_FOUND;
	}
    }

    my $action_number = ACTION_DENY;
    if(defined($action) && length($action) > 0){
	if(action_string_to_number($action, \$action_number) < 0){
	    return UNDEFINED_ACTION;
	}
    }

    if(defined($ofp_wildcards) && oct($ofp_wildcards) eq 'NaN'){
	$ofp_wildcards = string_to_ofp_wildcards($ofp_wildcards);
    }
    elsif(!defined($ofp_wildcards)){
	$ofp_wildcards = 0;
    }

    if(defined($wildcards) && oct($wildcards) eq 'NaN'){
	$wildcards = string_to_wildcards($wildcards);
    }
    elsif(!defined($wildcards)){
	$wildcards = 0;
    }

    if(defined($dl_src) && $dl_src =~ /:/){
	$dl_src = mac_string_to_int($dl_src);
    }
    if(defined($dl_dst) && $dl_dst =~ /:/){
	$dl_dst = mac_string_to_int($dl_dst);
    }

    if(defined($nw_src) && $nw_src =~ /\./){
	$nw_src = ip_string_to_int($nw_src);
    }
    else{
	$nw_src = oct($nw_src);
    }
    if(defined($nw_dst) && $nw_dst =~ /\./){
	$nw_dst = ip_string_to_int($nw_dst);
    }
    else{
	$nw_dst = oct($nw_dst);
    }

    my $ret = $self->{'dbh'}->do("INSERT INTO filter values ($priority,$ofp_wildcards," .
				 "$in_port,$dl_src,$dl_dst,$dl_vlan,$dl_vlan_pcp," .
				 "$dl_type,$nw_tos,$nw_proto,$nw_src,$nw_dst,$tp_src," .
				 "$tp_dst,$wildcards,$in_datapath_id,$slice_number," .
				 "$action_number,'$id')");
    if($ret <= 0){
	return FAILED;
    }
    debug("Filter entry is added successfully.");

    return SUCCEEDED;
}


sub delete_filters(){
    my($self, $priority, $ofp_wildcards, $in_port, $dl_src, $dl_dst, $dl_vlan,
       $dl_vlan_pcp, $dl_type, $nw_tos, $nw_proto, $nw_src, $nw_dst,
       $tp_src, $tp_dst, $wildcards, $in_datapath_id, $slice, $action) = @_;

    my $statement = "DELETE FROM filter";
    my $prefix = " WHERE";

    if(defined($priority)){
	$statement .= $prefix . " priority = $priority";
	$prefix = " AND"
    }
    if(defined($ofp_wildcards)){
	$ofp_wildcards = string_to_ofp_wildcards($ofp_wildcards);
	$statement .= $prefix . " ofp_wildcards = $ofp_wildcards";
	$prefix = " AND"
    }
    if(defined($in_port)){
	$statement .= $prefix . " in_port = $in_port";
	$prefix = " AND"
    }
    if(defined($dl_src)){
	if($dl_src =~ /:/){
	    $dl_src = mac_string_to_int($dl_src);
	}
	$statement .= $prefix . " dl_src = $dl_src";
	$prefix = " AND"
    }
    if(defined($dl_dst)){
	if($dl_dst =~ /:/){
	    $dl_dst = mac_string_to_int($dl_dst);
	}
	$statement .= $prefix . " dl_dst = $dl_dst";
	$prefix = " AND"
    }
    if(defined($dl_vlan)){
	$statement .= $prefix . " dl_vlan = $dl_vlan";
	$prefix = " AND"
    }
    if(defined($dl_vlan_pcp)){
	$statement .= $prefix . " dl_vlan_pcp = $dl_vlan_pcp";
	$prefix = " AND"
    }
    if(defined($dl_type)){
	$statement .= $prefix . " dl_type = $dl_type";
	$prefix = " AND"
    }
    if(defined($nw_tos)){
	$statement .= $prefix . " nw_tos = $nw_tos";
	$prefix = " AND"
    }
    if(defined($nw_proto)){
	$statement .= $prefix . " nw_tos = $nw_proto";
	$prefix = " AND"
    }
    if(defined($nw_src)){
	if($nw_src =~ /\./){
	    $nw_src = ip_string_to_int($nw_src);
	}
	else{
	    $nw_src = oct($nw_src);
	}
	$statement .= $prefix . " nw_src = $nw_src";
	$prefix = " AND"
    }
    if(defined($nw_dst)){
	if($nw_dst =~ /\./){
	    $nw_dst = ip_string_to_int($nw_dst);
	}
	else{
	    $nw_dst = oct($nw_dst);
	}
	$statement .= $prefix . " nw_dst = $nw_dst";
	$prefix = " AND"
    }
    if(defined($tp_src)){
	$statement .= $prefix . " tp_src = $tp_src";
	$prefix = " AND"
    }
    if(defined($tp_dst)){
	$statement .= $prefix . " tp_dst = $tp_dst";
	$prefix = " AND"
    }
    if(defined($wildcards)){
	$wildcards = string_to_wildcards($wildcards);
	$statement .= $prefix . " wildcards = $wildcards";
	$prefix = " AND"
    }
    if(defined($in_datapath_id)){
	$statement .= $prefix . " in_datapath_id = $in_datapath_id";
	$prefix = " AND"
    }
    if(defined($slice) && length($slice) > 0){
	my $slice_number;
	if($self->{'slice'}->get_slice_number_by_id($slice, \$slice_number) < 0){
	    return NO_SLICE_FOUND;
	}
	$statement .= $prefix . " slice_number = $slice_number";
	$prefix = " AND"
    }
    if(defined($action) && length($action) > 0){
	my $action_number;
	if(action_string_to_number($action, \$action_number) < 0){
	    return UNDEFINED_ACTION;
	}
	$statement .= $prefix . " action = $action_number";
	$prefix = " AND"
    }

    debug("statement = %s", $statement);

    my $ret = $self->{'dbh'}->do($statement);
    if($ret <= 0){
	return FAILED;
    }
    debug("Filter entry is deleted successfully.");

    return SUCCEEDED;
}


sub delete_filter_by_id(){
    my($self, $id) = @_;

    my $ret = $self->{'dbh'}->do("DELETE FROM filter WHERE id = '$id'");
    if($ret <= 0){
	return FAILED;
    }
    debug("Filter entry is deleted successfully.");

    return SUCCEEDED;
}


sub get_filters(){
    my ($self, $id) = @_;

    my $statement = "SELECT * FROM filter";

    if(defined($id)){
	$statement .= " WHERE id = '$id'";
    }

    my $sth = $self->{'dbh'}->prepare($statement);
    my $ret = $sth->execute();

    my %filters = ();
    while(my @row = $sth->fetchrow_array){
	my $id = $row[18];
	$filters{$id}{'priority'} = $row[0];
	$filters{$id}{'ofp_wildcards'} = $row[1];
	$filters{$id}{'in_port'} = $row[2];
	$filters{$id}{'dl_src'} = int_to_mac_string($row[3]);
	$filters{$id}{'dl_dst'} = int_to_mac_string($row[4]);
	$filters{$id}{'dl_vlan'} = $row[5];
	$filters{$id}{'dl_vlan_pcp'} = $row[6];
	$filters{$id}{'dl_type'} = $row[7];
	$filters{$id}{'nw_tos'} = $row[8];
	$filters{$id}{'nw_proto'} = $row[9];
	$filters{$id}{'nw_src'} = int_to_ip_string($row[10]);
	$filters{$id}{'nw_dst'} = int_to_ip_string($row[11]);
	$filters{$id}{'tp_src'} = $row[12];
	$filters{$id}{'tp_dst'} = $row[13];
	$filters{$id}{'wildcards'} = $row[14];
	$filters{$id}{'in_datapath_id'} = $row[15];
	my $slice_id;
	$self->{'slice'}->get_slice_id_by_number($row[16], \$slice_id);
	if(defined($slice_id) && length($slice_id) > 0){
	    $filters{$id}{'slice'} = $slice_id;
	}
	else{
	    $filters{$id}{'slice'} = sprintf("UNDEFINED (%#x)", $row[16]);
	}

	if($row[17] == ACTION_ALLOW){
	    $filters{$id}{'action'} = "ALLOW";
	}
	elsif($row[17] == ACTION_DENY){
	    $filters{$id}{'action'} = "DENY";
	}
	elsif($row[17] == ACTION_LOCAL){
	    $filters{$id}{'action'} = "LOCAL";
	}
	else{
	    $filters{$id}{'action'} = sprintf("UNDEFINED (%#x)", $row[17]);
	}
    }

    return %filters;
}


sub list_filters(){
    my ($self) = @_;

    my %filters = $self->get_filters();

    my $count = 0;
    my $out = sprintf("%24s\n", "ID");
    foreach my $id (keys(%filters)){
	$out .= sprintf("%24s\n", $id);
	$count++;
    }

    if($count == 0){
	$out = "No filters found.";
    }

    info($out);
}


sub show_filters(){
    my ($self, $id) = @_;

    my %filters = $self->get_filters($id);

    my $out = "";
    my $count = 0;
    foreach my $id (keys(%filters)){
	if($count > 0){
	    $out .= "\n";
	}
	$out .= "[$id]\n";
	$out .= sprintf("%20s: %u\n", "priority", $filters{$id}{'priority'});
	$out .= sprintf("%20s: %#x (%s)\n", "ofp_wildcards",
			$filters{$id}{'ofp_wildcards'}, ofp_wildcards_to_string($filters{$id}{'ofp_wildcards'}));
	$out .= sprintf("%20s: %#x\n", "in_port", $filters{$id}{'in_port'});
	$out .= sprintf("%20s: %s\n", "dl_src", $filters{$id}{'dl_src'});
	$out .= sprintf("%20s: %s\n", "dl_dst", $filters{$id}{'dl_dst'});
	$out .= sprintf("%20s: %#x\n", "dl_vlan", $filters{$id}{'dl_vlan'});
	$out .= sprintf("%20s: %#x\n", "dl_vlan_pcp", $filters{$id}{'dl_vlan_pcp'});
	$out .= sprintf("%20s: %#x\n", "dl_type", $filters{$id}{'dl_type'});
	$out .= sprintf("%20s: %#x\n", "nw_tos", $filters{$id}{'nw_tos'});
	$out .= sprintf("%20s: %#x\n", "nw_proto", $filters{$id}{'nw_proto'});
	if($filters{$id}{'ofp_wildcards'} & OFPFW_NW_SRC_MASK){
	    my $val = ($filters{$id}{'ofp_wildcards'} & OFPFW_NW_SRC_MASK) >> OFPFW_NW_SRC_SHIFT;
	    if($val > 32){
		$val = 32;
	    }
	    $out .= sprintf("%20s: %s/%u\n", "nw_src", $filters{$id}{'nw_src'}, 32 - $val);
	}
	else{
	    $out .= sprintf("%20s: %s\n", "nw_src", $filters{$id}{'nw_src'});
	}
	if($filters{$id}{'ofp_wildcards'} & OFPFW_NW_DST_MASK){
	    my $val = ($filters{$id}{'ofp_wildcards'} & OFPFW_NW_DST_MASK) >> OFPFW_NW_DST_SHIFT;
	    if($val > 32){
		$val = 32;
	    }
	    $out .= sprintf("%20s: %s/%u\n", "nw_dst", $filters{$id}{'nw_dst'}, 32 - $val);
	}
	else{
	    $out .= sprintf("%20s: %s\n", "nw_dst", $filters{$id}{'nw_dst'});
	}
	$out .= sprintf("%20s: %u\n", "tp_src", $filters{$id}{'tp_src'});
	$out .= sprintf("%20s: %u\n", "tp_dst", $filters{$id}{'tp_dst'});
	$out .= sprintf("%20s: %#x (%s)\n", "wildcards",
			$filters{$id}{'wildcards'}, wildcards_to_string($filters{$id}{'wildcards'}));
	$out .= sprintf("%20s: %#x\n", "in_datapath_id", $filters{$id}{'in_datapath_id'});
	$out .= sprintf("%20s: %s\n", "slice", $filters{$id}{'slice'});
	$out .= sprintf("%20s: %s\n", "action", $filters{$id}{'action'});
	$count++;
    }

    if($count == 0){
	$out = "No filters found.";
    }

    info($out);

    return SUCCEEDED;
}


sub int_to_mac_string(){
    my ($mac) = @_;

    my $string = sprintf("%04x%08x", $mac >> 32, $mac & 0xffffffff);
    $string =~ s/(.{2})(.{2})(.{2})(.{2})(.{2})(.{2})/$1:$2:$3:$4:$5:$6/;

    return $string;
}


sub mac_string_to_int(){
    my ($string) = @_;

    $string =~ s/://g;

    return hex( "0x" . $string);
}


sub int_to_ip_string(){
    my ($ip) = @_;

    my $string = sprintf("%u.%u.%u.%u", $ip >> 24 & 0xff, $ip >> 16 & 0xff, $ip >> 8 & 0xff, $ip & 0xff);

    return $string;
}


sub ip_string_to_int(){
    my ($string) = @_;

    my @octs = split('\.', $string);
    my $int = ($octs[0] << 24) + ($octs[1] << 16) + ($octs[2] << 8) + $octs[3];

    return $int;
}


sub action_string_to_number(){
    my ($string, $number) = @_;

    if($string =~ /^allow$/i){
	${$number} = ACTION_ALLOW;
	return SUCCEEDED;
    }
    elsif($string =~ /^deny$/i){
	${$number} = ACTION_DENY;
	return SUCCEEDED;
    }
    elsif($string =~ /^local$/i){
	${$number} = ACTION_LOCAL;
	return SUCCEEDED;
    }

    return FAILED;
}


sub set_default_filter_criteria(){
    my $self = shift;
    my @criteria = @_;
    $#criteria = 17;

    my $build_ofp_wildcards = 0;
    my $build_wildcards = 0;

    if(!defined($criteria[0])){
	$criteria[0] = 0;
    }
    if(!defined($criteria[1])){
	$build_ofp_wildcards = 1;
	$criteria[1] = 0;
    }
    if(!defined($criteria[2])){
	$criteria[2] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_IN_PORT;
	}
    }
    if(!defined($criteria[3])){
	$criteria[3] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_DL_SRC;
	}
    }
    if(!defined($criteria[4])){
	$criteria[4] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_DL_DST;
	}
    }
    if(!defined($criteria[5])){
	$criteria[5] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_DL_VLAN;
	}
    }
    if(!defined($criteria[6])){
	$criteria[6] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_DL_VLAN_PCP;
	}
    }
    if(!defined($criteria[7])){
	$criteria[7] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_DL_TYPE;
	}
    }
    if(!defined($criteria[8])){
	$criteria[8] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_NW_TOS;
	}
    }
    if(!defined($criteria[9])){
	$criteria[9] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_NW_PROTO;
	}
    }
    if(!defined($criteria[10])){
	$criteria[10] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_NW_SRC_MASK;
	}
    }
    elsif($criteria[10] =~ s/\/(\d+)//){
	if($build_ofp_wildcards){
	    my $bit_count = 32 - $1;
	    if($bit_count >= 0){
		$criteria[1] |= $bit_count << OFPFW_NW_SRC_SHIFT;
	    }
	    else{
		$criteria[1] |= OFPFW_NW_SRC_MASK;
	    }
	}
    }
    if(!defined($criteria[11])){
	$criteria[11] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_NW_DST_MASK;
	}
    }
    elsif($criteria[11] =~ s/\/(\d+)//) {
	if($build_ofp_wildcards){
	    my $bit_count = 32 - $1;
	    if($bit_count >= 0){
		$criteria[1] |= $bit_count << OFPFW_NW_DST_SHIFT;
	    }
	    else{
		$criteria[1] |= OFPFW_NW_DST_MASK;
	    }
	}
    }
    if(!defined($criteria[12])){
	$criteria[12] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_TP_SRC;
	}
    }
    if(!defined($criteria[13])){
	$criteria[13] = 0;
	if($build_ofp_wildcards){
	    $criteria[1] |= OFPFW_TP_DST;
	}
    }
    if(!defined($criteria[14])){
	$build_wildcards = 1;
	$criteria[14] = 0;
    }
    if(!defined($criteria[15])){
	$criteria[15] = 0;
	if($build_wildcards){
	    $criteria[14] |= WILDCARD_IN_DATAPATH_ID;
	}
    }
    if(!defined($criteria[16])){
	$criteria[16] = "";
	if($build_wildcards){
	    $criteria[14] |= WILDCARD_SLICE_NUMBER;
	}
    }
    if(!defined($criteria[17])){
	$criteria[17] = "deny";
    }

    return @criteria;
}


sub ofp_wildcards_to_string(){
    my $self = shift;
    my $wc = shift;

    if(defined($self) && !defined($wc)){
	$wc = $self;
    }

    my @strings = ();

    if($wc & OFPFW_IN_PORT){
	push(@strings, "in_port");
    }
    if($wc & OFPFW_DL_SRC){
	push(@strings, "dl_src");
    }
    if($wc & OFPFW_DL_DST){
	push(@strings, "dl_dst");
    }
    if($wc & OFPFW_DL_VLAN){
	push(@strings, "dl_vlan");
    }
    if($wc & OFPFW_DL_VLAN_PCP){
	push(@strings, "dl_vlan_pcp");
    }
    if($wc & OFPFW_DL_TYPE){
	push(@strings, "dl_type");
    }
    if($wc & OFPFW_NW_TOS){
	push(@strings, "nw_tos");
    }
    if($wc & OFPFW_NW_PROTO){
	push(@strings, "nw_proto");
    }
    if($wc & OFPFW_NW_SRC_MASK){
	my $val = ($wc & OFPFW_NW_SRC_MASK) >> OFPFW_NW_SRC_SHIFT;
	$val = 32 if ($val > 32);
	push(@strings, sprintf("nw_src:%u", $val));
    }
    if($wc & OFPFW_NW_DST_MASK){
	my $val = ($wc & OFPFW_NW_DST_MASK) >> OFPFW_NW_DST_SHIFT;
	$val = 32 if ($val > 32);
	push(@strings, sprintf("nw_dst:%u", $val));
    }
    if($wc & OFPFW_TP_SRC){
	push(@strings, "tp_src");
    }
    if($wc & OFPFW_TP_DST){
	push(@strings, "tp_dst");
    }

    my $out;
    if(@strings > 0){
	$out = join(',', @strings);
    }
    else{
	$out = "NONE";
    }
    return $out;
}


sub wildcards_to_string(){
    my $self = shift;
    my $wc = shift;

    if(defined($self) && !defined($wc)){
	$wc = $self;
    }

    my @strings = ();

    if($wc & WILDCARD_IN_DATAPATH_ID){
	push(@strings, "in_datapath_id");
    }
    if($wc & WILDCARD_SLICE_NUMBER){
	push(@strings, "slice");
    }

    my $out;
    if(@strings > 0){
	$out = join(',', @strings);
    }
    else{
	$out = "NONE";
    }
    return $out;
}


sub string_to_ofp_wildcards(){
    my $string = shift;

    my $wc = 0;
    my @params = split(',', $string);

    foreach my $p (@params){
	if($p =~ /^in_port$/i){
	    $wc |= OFPFW_IN_PORT;
	}
	if($p =~ /^dl_src$/i){
	    $wc |= OFPFW_DL_SRC;
	}
	if($p =~ /^dl_dst$/i){
	    $wc |= OFPFW_DL_DST;
	}
	if($p =~ /^dl_vlan$/i){
	    $wc |= OFPFW_DL_VLAN;
	}
	if($p =~ /^dl_vlan_pcp$/i){
	    $wc |= OFPFW_DL_VLAN_PCP;
	}
	if($p =~ /^dl_type$/i){
	    $wc |= OFPFW_DL_TYPE;
	}
	if($p =~ /^nw_tos$/i){
	    $wc |= OFPFW_NW_TOS;
	}
	if($p =~ /^nw_proto$/i){
	    $wc |= OFPFW_NW_PROTO;
	}
	if($p =~ /^nw_src:(\d+)$/i){
	    if($1 < 32){
		$wc |= $1 << OFPFW_NW_SRC_SHIFT;
	    }
	    else{
		$wc |= OFPFW_NW_SRC_MASK;
	    }
	}
	if($p =~ /^nw_dst:(\d+)$/i){
	    if($1 < 32){
		$wc |= $1 << OFPFW_NW_DST_SHIFT;
	    }
	    else{
		$wc |= OFPFW_NW_DST_MASK;
	    }
	}
	if($p =~ /^tp_src$/i){
	    $wc |= OFPFW_TP_SRC;
	}
	if($p =~ /^tp_dst$/i){
	    $wc |= OFPFW_TP_DST;
	}
    }

    return $wc;
}


sub string_to_wildcards(){
    my $string = shift;

    my $wc = 0;
    my @params = split(',', $string);

    foreach my $p (@params){
	if($p =~ /^in_datapath_id$/i){
	    $wc |= WILDCARD_IN_DATAPATH_ID;
	}
	if($p =~ /^slice$/i){
	    $wc |= WILDCARD_SLICE_NUMBER;
	}
    }

    return $wc;
}


sub debug(){
    if($Debug){
	printf(@_);
	printf("\n");
    }
}


sub info(){
    printf(@_);
    printf("\n");
}


sub error(){
    printf(STDERR @_);
    printf(STDERR "\n");
}


1;
