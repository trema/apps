/*
 * Author: Shuji Ishii, Kazushi SUGYO
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


#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include "trema.h"
#include "libtopology.h"
#include "topology_service_interface_option_parser.h"
#include "lldp.h"
#include "probe_timer_table.h"


static bool response_all_port_status_down = false;


static char option_description[] =
                                   "  -m, --lldp_mac_dst=MAC_ADDR     Destination Mac address for sending LLDP\n"
                                   "  -i, --lldp_over_ip          Send LLDP messages over IP\n"
                                   "  -o, --lldp_ip_src=IP_ADDR   Source IP address for sending LLDP over IP\n"
                                   "  -r, --lldp_ip_dst=IP_ADDR   Destination IP address for sending LLDP over IP\n";
static char short_options[] = "m:io:r:";
static struct option long_options[] = {
  { "lldp_mac_dst", required_argument, NULL, 'm' },
  { "lldp_over_ip", no_argument, NULL, 'i' },
  { "lldp_ip_src", required_argument, NULL, 'o' },
  { "lldp_ip_dst", required_argument, NULL, 'r' },
  { NULL, 0, NULL, 0  },
};


static uint8_t lldp_default_dst[ ETH_ADDRLEN ] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };


typedef struct {
  lldp_options lldp;
} topology_discovery_options;


void
usage( void ) {
  topology_service_interface_usage( get_executable_name(), "topology discovery", option_description );
}


static void
update_port_status( const topology_port_status *s ) {
  if ( s->port_no > OFPP_MAX ) {
    return;
  }
  probe_timer_entry *entry = delete_probe_timer_entry( &( s->dpid ), s->port_no );
  if ( s->status == TD_PORT_DOWN ) {
    if ( entry != NULL ) {
      probe_request( entry, PROBE_TIMER_EVENT_DOWN, 0, 0 );
      free_probe_timer_entry( entry );
    }
    return;
  }
  if ( entry == NULL ) {
    entry = allocate_probe_timer_entry( &( s->dpid ), s->port_no, s->mac );
  }
  probe_request( entry, PROBE_TIMER_EVENT_UP, 0, 0 );
}


static void
response_all_port_status( void *param, size_t n_entries, const topology_port_status *s ) {
  UNUSED( param );

  size_t i;
  for ( i = 0; i < n_entries; i++ ) {
    update_port_status( &s[ i ] );
  }
  response_all_port_status_down = true;
}


static void
port_status_updated( void *param, const topology_port_status *status ) {
  UNUSED( param );

  if ( response_all_port_status_down ) {
    update_port_status( status );
  }
}


static void
subscribe_topology_response( void *user_data ) {
  UNUSED( user_data );
  add_callback_port_status_updated( port_status_updated, NULL );
  get_all_port_status( response_all_port_status, NULL );
  response_all_port_status_down = false;
}


static void
reset_getopt() {
  optind = 0;
  opterr = 1;
}


static bool
set_mac_address_from_string( uint8_t *address, const char *string ) {
  assert( address != NULL );
  assert( string != NULL );

  struct ether_addr *addr = ether_aton( string );
  if ( addr == NULL ) {
    error( "Invalid MAC address specified." );
    return false;
  }

  memcpy( address, addr->ether_addr_octet, ETH_ADDRLEN );

  return true;
}


static bool
set_ip_address_from_string( uint32_t *address, const char *string ) {
  assert( address != NULL );
  assert( string != NULL );

  struct in_addr addr;

  if ( inet_aton( string, &addr ) == 0 ) {
    error( "Invalid IP address specified." );
    return false;
  }

  *address = ntohl( addr.s_addr );

  return true;
}


static void
parse_options( topology_discovery_options *options, int *argc, char **argv[] ) {
  assert( options != NULL );
  assert( argc != NULL );
  assert( *argc >= 0 );
  assert( argv != NULL );

  int argc_tmp = *argc;
  char *new_argv[ *argc ];

  for ( int i = 0; i <= *argc; ++i ) {
    new_argv[ i ] = ( *argv )[ i ];
  }

  memcpy( options->lldp.lldp_mac_dst, lldp_default_dst, ETH_ADDRLEN );
  options->lldp.lldp_over_ip = false;
  options->lldp.lldp_ip_src = 0;
  options->lldp.lldp_ip_dst = 0;

  int c;
  while ( ( c = getopt_long( *argc, *argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'm':
        if ( set_mac_address_from_string( options->lldp.lldp_mac_dst, optarg ) == false ) {
          usage();
          exit( EXIT_FAILURE );
          return;
        }
        info( "%s is used as destination address for sending LLDP.", ether_ntoa( ( const struct ether_addr * ) options->lldp.lldp_mac_dst ) );
        break;

      case 'i':
        debug( "Enabling LLDP over IP" );
        options->lldp.lldp_over_ip = true;
        break;

      case 'o':
        if ( set_ip_address_from_string( &options->lldp.lldp_ip_src, optarg ) == false ) {
          usage();
          exit( EXIT_FAILURE );
          return;
        }
        info( "%s ( %#x ) is used as source address for sending LLDP over IP.", optarg, options->lldp.lldp_ip_src );
        break;

      case 'r':
        if ( set_ip_address_from_string( &options->lldp.lldp_ip_dst, optarg ) == false ) {
          usage();
          exit( EXIT_FAILURE );
          return;
        }
        info( "%s ( %#x ) is used as destination address for sending LLDP over IP.", optarg, options->lldp.lldp_ip_dst );
        break;

      default:
        continue;
    }

    if ( optarg == 0 || strchr( new_argv[ optind - 1 ], '=' ) != NULL ) {
      argc_tmp -= 1;
      new_argv[ optind - 1 ] = NULL;
    }
    else {
      argc_tmp -= 2;
      new_argv[ optind - 1 ] = NULL;
      new_argv[ optind - 2 ] = NULL;
    }
  }

  if ( options->lldp.lldp_over_ip == true && ( options->lldp.lldp_ip_src == 0 || options->lldp.lldp_ip_dst == 0 ) ) {
    printf( "-o and -r options are mandatory." );
    usage();
    exit( EXIT_FAILURE );
    return;
  }

  for ( int i = 0, j = 0; i < *argc; ++i ) {
    if ( new_argv[ i ] != NULL ) {
      ( *argv )[ j ] = new_argv[ i ];
      j++;
    }
  }

  ( *argv )[ *argc ] = NULL;
  *argc = argc_tmp;

  reset_getopt();
}


int
main( int argc, char *argv[] ) {
  topology_discovery_options options;

  init_trema( &argc, &argv );
  parse_options( &options, &argc, &argv );
  init_topology_service_interface_options( &argc, &argv );
  init_probe_timer_table();
  init_libtopology( get_topology_service_interface_name() );

  init_lldp( options.lldp );
  subscribe_topology( subscribe_topology_response, NULL );

  start_trema();

  finalize_lldp();
  finalize_libtopology();
  finalize_probe_timer_table();
  finalize_topology_service_interface_options();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
