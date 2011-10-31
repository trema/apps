#! /bin/sh
#
# Copyright (C) 2011 NEC Corporation
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

BASE_URI="http://127.0.0.1:8888/networks"

CLIENT="./httpc"

run(){
    method=$1
    target=$BASE_URI$2
    content=$3
    echo "#### BEGIN: $method $target ####"
    $CLIENT $method $target $content
    echo "#### END: $method $target ####"
    echo
}

tests(){
    run GET ""
    run POST "" create_network.json
    run POST "/slice_created_via_rest_if/ports" create_port.json
    run GET "/slice_created_via_rest_if/ports"
    run POST "/slice_created_via_rest_if/ports/port_created_via_rest_if/attachments" create_port_mac.json
    run GET "/slice_created_via_rest_if/ports/port_created_via_rest_if/attachments"
    run GET "/slice_created_via_rest_if/ports/port_created_via_rest_if/attachments/port_mac_created_via_rest_if"
    run GET "/slice_created_via_rest_if/ports/port_created_via_rest_if"
    run POST "/slice_created_via_rest_if/attachments" create_mac.json
    run GET "/slice_created_via_rest_if/attachments"
    run GET "/slice_created_via_rest_if/attachments/mac_created_via_rest_if"
    run GET "/slice_created_via_rest_if"
    run DELETE "/slice_created_via_rest_if/ports/port_created_via_rest_if/attachments/port_mac_created_via_rest_if"
    run DELETE "/slice_created_via_rest_if/ports/port_created_via_rest_if"
    run DELETE "/slice_created_via_rest_if/attachments/mac_created_via_rest_if"
    run DELETE "/slice_created_via_rest_if"
}

tests
