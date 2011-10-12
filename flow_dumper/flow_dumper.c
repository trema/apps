/*
 * Flow dumper application.
 * 
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2011 NEC Corporation
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
#include <inttypes.h>
#include "trema.h"


static int n_switches = 0;


static void
actions_to_string( struct ofp_action_header *actions, uint16_t actions_length,
                   char *string, size_t string_length ) {
  if ( actions == NULL || actions_length == 0 ) {
    return;
  }

  memset( string, '\0', string_length );

  size_t offset = 0;
  while ( ( actions_length - offset ) >= sizeof( struct ofp_action_header ) ) {
    size_t remaining_string_length = string_length - strlen( string );
    if ( strlen( string ) > 0 && remaining_string_length > 2 ) {
      snprintf( string + strlen( string ), remaining_string_length, ", " );
      remaining_string_length -= 2;
    }
    struct ofp_action_header *header = ( struct ofp_action_header * ) ( ( char * ) actions + offset );
    switch( header->type ) {
      case OFPAT_OUTPUT:
      {
        struct ofp_action_output *action = ( struct ofp_action_output * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "output: port=%u max_len=%u",
                  action->port, action->max_len );
      }
      break;

      case OFPAT_SET_VLAN_VID:
      {
        struct ofp_action_vlan_vid *action = ( struct ofp_action_vlan_vid * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "set_vlan_vid: vid=%#x",
                  action->vlan_vid );
      }
      break;

      case OFPAT_SET_VLAN_PCP:
      {
        struct ofp_action_vlan_vid *action = ( struct ofp_action_vlan_vid * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "set_vlan_vid: vid=%#x",
                  action->vlan_vid );
      }
      break;

      case OFPAT_STRIP_VLAN:
      {
        snprintf( string + strlen( string ), remaining_string_length, "strip_vlan" );
      }
      break;

      case OFPAT_SET_DL_SRC:
      {
        struct ofp_action_dl_addr *action = ( struct ofp_action_dl_addr * ) header;
        snprintf( string + strlen( string ), remaining_string_length,
                  "set_dl_src: dl_addr=%02x:%02x%02x%02x%02x%02x",
                  action->dl_addr[ 0 ], action->dl_addr[ 1 ], action->dl_addr[ 2 ],
                  action->dl_addr[ 3 ], action->dl_addr[ 4 ], action->dl_addr[ 5 ] );
      }
      break;

      case OFPAT_SET_DL_DST:
      {
        struct ofp_action_dl_addr *action = ( struct ofp_action_dl_addr * ) header;
        snprintf( string + strlen( string ), remaining_string_length,
                  "set_dl_dst: dl_addr=%02x:%02x:%02x:%02x:%02x:%02x",
                  action->dl_addr[ 0 ], action->dl_addr[ 1 ], action->dl_addr[ 2 ],
                  action->dl_addr[ 3 ], action->dl_addr[ 4 ], action->dl_addr[ 5 ] );
      }
      break;

      case OFPAT_SET_NW_SRC:
      {
        struct ofp_action_nw_addr *action = ( struct ofp_action_nw_addr * ) header;
        char nw_addr[ 16 ];
        struct in_addr addr;
        addr.s_addr = htonl( action->nw_addr );
        memset( nw_addr, '\0', sizeof( nw_addr ) );
        inet_ntop( AF_INET, &addr, nw_addr, sizeof( nw_addr ) );
        snprintf( string + strlen( string ), remaining_string_length, "set_nw_src: nw_addr=%s",
                  nw_addr );
      }
      break;

      case OFPAT_SET_NW_DST:
      {
        struct ofp_action_nw_addr *action = ( struct ofp_action_nw_addr * ) header;
        char nw_addr[ 16 ];
        struct in_addr addr;
        addr.s_addr = htonl( action->nw_addr );
        memset( nw_addr, '\0', sizeof( nw_addr ) );
        inet_ntop( AF_INET, &addr, nw_addr, sizeof( nw_addr ) );
        snprintf( string + strlen( string ), remaining_string_length, "set_nw_dst: nw_addr=%s",
                  nw_addr );
      }
      break;

      case OFPAT_SET_NW_TOS:
      {
        struct ofp_action_nw_tos *action = ( struct ofp_action_nw_tos * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "set_nw_tos: nw_tos=%#x",
                  action->nw_tos );
      }
      break;

      case OFPAT_SET_TP_SRC:
      {
        struct ofp_action_tp_port *action = ( struct ofp_action_tp_port * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "set_tp_src: tp_port=%u",
                  action->tp_port );
      }
      break;

      case OFPAT_SET_TP_DST:
      {
        struct ofp_action_tp_port *action = ( struct ofp_action_tp_port * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "set_tp_dst: tp_port=%u",
                  action->tp_port );
      }
      break;

      case OFPAT_ENQUEUE:
      {
        struct ofp_action_enqueue *action = ( struct ofp_action_enqueue * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "enqueue: port=%u queue_id=%u",
                  action->port, action->queue_id );
      }
      break;

      case OFPAT_VENDOR:
      {
        struct ofp_action_vendor_header *action = ( struct ofp_action_vendor_header * ) header;
        snprintf( string + strlen( string ), remaining_string_length, "vendor: vendor=%#x",
                  action->vendor );
      }
      break;

      default:
      {
        snprintf( string + strlen( string ), remaining_string_length, "undefined" );
      }
      break;
    }

    offset += header->len;
  }

  string[ string_length - 1 ] = '\0';
}


static void
handle_list_switches_reply( const list_element *switches, void *user_data ) {
  UNUSED( user_data );

  struct ofp_match match;
  memset( &match, 0, sizeof( struct ofp_match ) );
  match.wildcards = OFPFW_ALL;
  buffer *stats_request = create_flow_stats_request( get_transaction_id(), 0, match, 0xff, OFPP_NONE );

  n_switches = 0;
  const list_element *element;
  for ( element = switches; element != NULL; element = element->next ) {
    uint64_t datapath_id = *( uint64_t * ) element->data;
    send_openflow_message( datapath_id, stats_request );
    n_switches++;
  }

  free_buffer( stats_request );
}


static void
dump_flow_stats( uint64_t datapath_id, struct ofp_flow_stats *stats ) {
  char match_str[ 256 ];
  memset( match_str, '\0', sizeof( match_str ) );
  match_to_string( &stats->match, match_str, sizeof( match_str ) );

  char actions_str[ 256 ];
  memset( actions_str, '\0', sizeof( actions_str ) );
  uint16_t actions_length = stats->length - offsetof( struct ofp_flow_stats, actions );
  if ( actions_length > 0 ) {
    actions_to_string( stats->actions, actions_length, actions_str, sizeof( actions_str ) );
  }

  info( "[%#016" PRIx64 "] priority = %u, match = [%s], actions = [%s]",
        datapath_id, stats->priority, match_str, actions_str );
}


static void
handle_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                    uint16_t type, uint16_t flags, const buffer *data,
                    void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( user_data );

  if ( type != OFPST_FLOW ) {
    return;
  }

  if ( data != NULL ) {
    size_t offset = 0;
    while ( ( data->length - offset ) >= sizeof( struct ofp_flow_stats ) ) {
      struct ofp_flow_stats *stats = ( struct ofp_flow_stats * ) ( ( char * ) data->data + offset );
      dump_flow_stats( datapath_id, stats );
      offset += stats->length;
    }
  }

  if ( ( flags & OFPSF_REPLY_MORE ) == 0 ) {
    if ( --n_switches <= 0 ) {
      stop_trema();
    }
  }
}


int
main( int argc, char *argv[] ) {
  // Initialize the Trema world
  init_trema( &argc, &argv );

  // Set event handlers
  set_list_switches_reply_handler( handle_list_switches_reply );
  set_stats_reply_handler( handle_stats_reply, NULL );

  // Ask switch manager to obtain the list of switches
  send_list_switches_request( NULL );

  // Main loop
  start_trema();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
