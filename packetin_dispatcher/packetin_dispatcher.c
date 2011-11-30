/*
 * OpenFlow Packet_in dispatcher
 *
 * Author: Kazushi SUGYO
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


#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "trema.h"


void
usage() {
  printf(
	 "OpenFlow Packet in dispatcher.\n"
	 "Usage: %s [OPTION]... [PACKETIN-DISPATCH-RULE]...\n"
	 "\n"
	 "  -n, --name=SERVICE_NAME     service name\n"
	 "  -d, --daemonize             run in the background\n"
	 "  -l, --logging_level=LEVEL   set logging level\n"
	 "  -h, --help                  display this help and exit\n"
	 "\n"
	 "PACKETIN-DISPATCH-RULE:\n"
	 "  dispatch-type::destination-service-name\n"
	 "\n"
	 "dispatch-type:\n"
	 "  arp_or_unicast              arp or unicast packets\n"
	 "  broadcast                   broadcast (arp not include)\n"
	 "\n"
	 "destination-service-name      destination service name\n"
	 , get_executable_name()
	 );
}


typedef struct {
  list_element *arp_or_unicast;
  list_element *broadcast;
} services;


static void
handle_packet_in( uint64_t datapath_id, uint32_t transaction_id,
                  uint32_t buffer_id, uint16_t total_len,
                  uint16_t in_port, uint8_t reason, const buffer *data,
                  void *user_data ) {
  services *services = user_data;
  list_element *list_head = services->arp_or_unicast;

  packet_info *packet_info = data->user_data;
  if ( !packet_type_arp( data ) && ( packet_info->eth_macda[ 0 ] & 0x1 ) == 0x1 ) {
    list_head = services->broadcast;
  }
  buffer *buf = create_packet_in( transaction_id, buffer_id, total_len, in_port,
                                  reason, data );

  openflow_service_header_t *message;
  message = append_front_buffer( buf, sizeof( openflow_service_header_t ) );
  message->datapath_id = htonll( datapath_id );
  message->service_name_length = htons( 0 );
  list_element *element;
  for ( element = list_head; element != NULL; element = element->next ) {
    const char *service_name = element->data;
    if ( !send_message( service_name, MESSENGER_OPENFLOW_MESSAGE,
                        buf->data, buf->length ) ) {
      error( "Failed to send a message to %s.", service_name );
      free_buffer( buf );
      return;
    }
  
    debug( "Sending a message to %s.", service_name );
  }

  free_buffer( buf );
}


static const char ARP_OR_UNICAST[] = "arp_or_unicast::";
static const char BROADCAST[] = "broadcast::";


static char *
match_type( const char *type, char *name ) {
  size_t len = strlen( type );
  if ( strncmp( name, type, len ) != 0 ) {
    return NULL;
  }

  return name + len;
}


static bool
set_match_type( int argc, char *argv[], services *services ) {
  create_list( &services->arp_or_unicast );
  create_list( &services->broadcast );
  int i;
  char *service_name;
  for ( i = 1; i < argc; i++ ) {
    if ( ( service_name = match_type( ARP_OR_UNICAST, argv[ i ] ) ) != NULL ) {
      append_to_tail( &services->arp_or_unicast, service_name );
    }
    else if ( ( service_name = match_type( BROADCAST, argv[ i ] ) ) != NULL ) {
      append_to_tail( &services->broadcast, service_name );
    }
    else {
      return false;
    }
  }

  return true;
}


int
main( int argc, char *argv[] ) {
  init_trema( &argc, &argv );

  services services;
  if ( !set_match_type( argc, argv, &services ) ) {
    usage();
    exit( EXIT_FAILURE );
  }

  set_packet_in_handler( handle_packet_in, &services );

  start_trema();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
