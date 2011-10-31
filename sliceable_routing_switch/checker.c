/*
 * Dumps packet-in message.
 *
 * Author: Yasuhito Takamiya <yasuhito@gmail.com>
 *
 * Copyright (C) 2008-2011 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "trema.h"


static void
handle_packet_in( uint64_t datapath_id, packet_in message ) {
  if ( !packet_type_ipv4( message.data ) ) {
    return;
  }
  packet_info *packet_info = message.data->user_data;
  struct in_addr in;
  in.s_addr = htonl( packet_info->ipv4_saddr );
  uint16_t vid = 0xffff;
  if ( packet_type_eth_vtag( message.data ) ) {
     vid = packet_info->vlan_vid;
  }
  info( "%s: datapath_id: %#" PRIx64 ", in_port: %u, vid: %#x, ether address: %02x:%02x:%02x:%02x:%02x:%02x",
        inet_ntoa( in ), datapath_id, message.in_port, vid,
	packet_info->eth_macsa[0], packet_info->eth_macsa[1], packet_info->eth_macsa[2],
	packet_info->eth_macsa[3], packet_info->eth_macsa[4], packet_info->eth_macsa[5] );
}


int
main( int argc, char *argv[] ) {
  init_trema( &argc, &argv );
  set_packet_in_handler( handle_packet_in, NULL );
  start_trema();
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
