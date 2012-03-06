/*
 * Author: Shuji Ishii, Yasunobu Chiba, Lei SUN
 *
 * Copyright (C) 2008-2012 NEC Corporation
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
#include <getopt.h>
#include <inttypes.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "trema.h"
#include "fdb.h"
#include "filter.h"
#include "icmp.h"
#include "libpathresolver.h"
#include "libtopology.h"
#include "port.h"
#include "redirector.h"
#include "slice.h"
#include "sliceable_routing_switch.h"
#include "topology_service_interface_option_parser.h"


/*
#define SET_IPV4_REVERSE_PATH
This is an experimental option to setup going and returning flows for IPv4 packet.
This could be reduce a number of packet_in and resolve_path, and is useful for
accepting packets that are responce to a request packet from dynamic client tdp/udp
ports like 30001.  For ICMPv4 protocols, just "Echo Reply" is modified as the
returning flow.
WARN: The returning flow is modified without filter check.
      This might be harmful to user's security policy.
*/


static const uint16_t FLOW_TIMER = 60;
static const uint16_t PACKET_IN_DISCARD_DURATION = 1;


typedef struct {
  uint16_t idle_timeout;
  char slice_db_file[ PATH_MAX ];
  char filter_db_file[ PATH_MAX ];
  uint16_t mode;
} routing_switch_options;


typedef struct {
  const buffer *packet;
  uint64_t in_datapath_id;
  uint16_t in_port;
  uint16_t in_vid;
  uint16_t slice;
} switch_params;


typedef struct {
  switch_params *switch_params;
  openflow_actions *actions;
} port_params;


static void
modify_flow_entry( const pathresolver_hop *hop, const struct ofp_match match, const uint16_t idle_timeout, const uint16_t out_vid ) {
  struct ofp_match m = match;
  m.in_port = hop->in_port_no;

  openflow_actions *actions = create_actions();
  uint16_t in_vid = m.dl_vlan;

  if ( out_vid != in_vid ) {
    if ( in_vid != VLAN_NONE && out_vid == VLAN_NONE ) {
      append_action_strip_vlan( actions );
    }
    else {
      append_action_set_vlan_vid( actions, out_vid );
    }
  }

  const uint16_t max_len = UINT16_MAX;
  append_action_output( actions, hop->out_port_no, max_len );

  const uint16_t hard_timeout = 0;
  const uint16_t priority = UINT16_MAX;
  const uint32_t buffer_id = UINT32_MAX;
  const uint16_t flags = 0;
  buffer *flow_mod = create_flow_mod( get_transaction_id(), m, get_cookie(),
                                      OFPFC_ADD, idle_timeout, hard_timeout,
                                      priority, buffer_id, 
                                      hop->out_port_no, flags, actions );

  send_openflow_message( hop->dpid, flow_mod );
  delete_actions( actions );
  free_buffer( flow_mod );
}


#ifdef SET_IPV4_REVERSE_PATH


static bool
packet_to_set_reverse_path( const buffer *packet ) {

  if ( packet_type_ipv4_tcp( packet ) ) {
    return true;
  }
  else if ( packet_type_ipv4_udp( packet ) ) {
    return true;
  }
  else if ( packet_type_ipv4_icmpv4( packet ) ) {
    packet_info *pinfo = ( packet_info * ) packet->user_data;
    if ( pinfo->icmpv4_type == ICMP_TYPE_ECHOREQ ) {
      return true;
    }
  }
  return false;
}


static void
set_reverse_hop( pathresolver_hop *r_hop, const pathresolver_hop *hop ) {
  r_hop->dpid = hop->dpid;
  r_hop->in_port_no = hop->out_port_no;
  r_hop->out_port_no = hop->in_port_no;
}


static void
set_ipv4_reverse_match( struct ofp_match *r_match, const uint16_t out_vid, const struct ofp_match *match ) {

  memcpy( r_match, match, sizeof( struct ofp_match ) );

  r_match->in_port = 0;
  memcpy( r_match->dl_src, match->dl_dst, OFP_ETH_ALEN );
  memcpy( r_match->dl_dst, match->dl_src, OFP_ETH_ALEN );
  r_match->dl_vlan = out_vid;
  r_match->nw_src = match->nw_dst;
  r_match->nw_dst = match->nw_src;
  switch ( match->nw_proto ) {
  case IPPROTO_ICMP:
    r_match->icmp_type = ICMP_TYPE_ECHOREP;
    r_match->icmp_code = 0;
    break;
  case IPPROTO_TCP:
    r_match->tp_src = match->tp_dst;
    r_match->tp_dst = match->tp_src;
    break;
  case IPPROTO_UDP:
    r_match->tp_src = match->tp_dst;
    r_match->tp_dst = match->tp_src;
    break;
  default:
    break;
  }
}


#endif // SET_IPV4_REVERSE_PATH


static void
send_packet_out( uint64_t datapath_id, openflow_actions *actions, const buffer *original ) {
  const uint32_t transaction_id = get_transaction_id();
  const uint32_t buffer_id = UINT32_MAX;
  const uint16_t in_port = OFPP_NONE;
  buffer *copy = NULL;
  const buffer *packet = original;

  if ( original->length + ETH_FCS_LENGTH < ETH_MINIMUM_LENGTH ) {
    copy = duplicate_buffer( original );
    fill_ether_padding( copy );
    packet = copy;
  }
  buffer *packet_out = create_packet_out( transaction_id, buffer_id, in_port,
                                          actions, packet );

  send_openflow_message( datapath_id, packet_out );

  free_buffer( packet_out );
  if ( copy != NULL ) {
    free_buffer( copy );
  }
}


static void
output_packet( const buffer *packet, uint64_t dpid, uint16_t port_no, uint16_t out_vid ) {

  openflow_actions *actions = create_actions();
  uint16_t in_vid = VLAN_NONE;
  if ( packet_type_eth_vtag( packet ) ) {
    packet_info packet_info = get_packet_info( packet );
    in_vid = packet_info.vlan_vid;
  }
  if ( out_vid != in_vid ) {
    if ( in_vid != VLAN_NONE && out_vid == VLAN_NONE ) {
      append_action_strip_vlan( actions );
    }
    else {
      append_action_set_vlan_vid( actions, out_vid );
    }
  }
  const uint16_t max_len = UINT16_MAX;
  append_action_output( actions, port_no, max_len );

  send_packet_out( dpid, actions, packet );
  delete_actions( actions );
}


static void
output_packet_from_last_switch( const pathresolver_hop *last_hop, const buffer *packet, uint16_t out_vid ) {
  output_packet( packet, last_hop->dpid, last_hop->out_port_no, out_vid );
}


static uint32_t
count_hops( const dlist_element *hops ) {
  uint32_t i = 0;
  for ( const dlist_element *e = hops; e != NULL; e = e->next ) {
    i++;
  }
  return i;
}


static void
discard_packet_in( uint64_t datapath_id, uint16_t in_port, const buffer *packet ) {
  const uint32_t wildcards = 0;
  struct ofp_match match;
  set_match_from_packet( &match, in_port, wildcards, packet );
  char match_str[ 1024 ];
  match_to_string( &match, match_str, sizeof( match_str ) );

  const uint16_t idle_timeout = 0;
  const uint16_t hard_timeout = PACKET_IN_DISCARD_DURATION;
  const uint16_t priority = UINT16_MAX;
  const uint32_t buffer_id = UINT32_MAX;
  const uint16_t flags = 0;

  info( "Discarding packets for a certain period ( datapath_id = %#" PRIx64
        ", match = [%s], duration = %u [sec] ).", datapath_id, match_str, hard_timeout );

  buffer *flow_mod = create_flow_mod( get_transaction_id(), match, get_cookie(),
                                      OFPFC_ADD, idle_timeout, hard_timeout,
                                      priority, buffer_id,
                                      OFPP_NONE, flags, NULL );

  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
}


static void
make_path( routing_switch *routing_switch, uint64_t in_datapath_id, uint16_t in_port, uint16_t in_vid,
           uint64_t out_datapath_id, uint16_t out_port, uint16_t out_vid, const buffer *packet ) {
  dlist_element *hops = resolve_path( routing_switch->pathresolver, in_datapath_id, in_port, out_datapath_id, out_port );
  if ( hops == NULL ) {
    warn( "No available path found ( %#" PRIx64 ":%u -> %#" PRIx64 ":%u ).",
          in_datapath_id, in_port, out_datapath_id, out_port );
    discard_packet_in( in_datapath_id, in_port, packet );
    return;
  }

  const uint32_t wildcards = 0;
  struct ofp_match match;
  set_match_from_packet( &match, 0, wildcards, packet );

  // count elements
  uint32_t hop_count = count_hops( hops );

  // send flow entry from tail switch
  for ( dlist_element *e = get_last_element( hops ); e != NULL; e = e->prev, hop_count-- ) {
    uint16_t idle_timeout = ( uint16_t ) ( routing_switch->idle_timeout + hop_count );
    if ( e->next != NULL ) {
      modify_flow_entry( e->data, match, idle_timeout, in_vid );
    }
    else {
      modify_flow_entry( e->data, match, idle_timeout, out_vid );
    }
  } // for(;;)

#ifdef SET_IPV4_REVERSE_PATH

  if ( packet_to_set_reverse_path( packet ) ) {
    struct ofp_match r_match;
    set_ipv4_reverse_match( &r_match, out_vid, &match );

    pathresolver_hop r_hop;
    hop_count = count_hops( hops );

    // send flow entry from head switch
    for ( dlist_element *e = get_first_element( hops ); e != NULL; e = e->next, hop_count-- ) {
      uint16_t idle_timeout = ( uint16_t ) ( routing_switch->idle_timeout + hop_count );
      pathresolver_hop *hop = ( pathresolver_hop * ) e->data;
      set_reverse_hop( &r_hop, hop);
      if ( e->prev != NULL ) {
        modify_flow_entry( &r_hop, r_match, idle_timeout, out_vid );
      }
      else {
        modify_flow_entry( &r_hop, r_match, idle_timeout, in_vid );
      }
    } // for(;;)
  }

#endif // SET_IPV4_REVERSE_PATH

  // send packet out for tail switch
  dlist_element *e = get_last_element( hops );
  pathresolver_hop *last_hop = e->data;
  output_packet_from_last_switch( last_hop, packet, out_vid );

  // free them
  free_hop_list( hops );
}


static void
set_miss_send_len_maximum( uint64_t datapath_id ) {
  uint32_t id = get_transaction_id();
  const uint16_t config_flags = OFPC_FRAG_NORMAL;
  const uint16_t miss_send_len = UINT16_MAX;
  buffer *buf = create_set_config( id, config_flags, miss_send_len );
  send_openflow_message( datapath_id, buf );

  free_buffer( buf );
}


static void
receive_features_reply( uint64_t datapath_id, uint32_t transaction_id,
                        uint32_t n_buffers, uint8_t n_tables,
                        uint32_t capabilities, uint32_t actions,
                        const list_element *phy_ports, void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( n_buffers );
  UNUSED( n_tables );
  UNUSED( capabilities );
  UNUSED( actions );
  UNUSED( phy_ports );
  UNUSED( user_data );

  set_miss_send_len_maximum( datapath_id );
}


static void
handle_switch_ready( uint64_t datapath_id, void *user_data ) {
  UNUSED( user_data );

  uint32_t id = get_transaction_id();
  buffer *buf = create_features_request( id );
  send_openflow_message( datapath_id, buf );
  free_buffer( buf );
}


static void
port_status_updated( void *user_data, const topology_port_status *status ) {
  assert( user_data != NULL );
  assert( status != NULL );

  routing_switch *routing_switch = user_data;

  debug( "Port status updated: dpid:%#" PRIx64 ", port:%u, %s, %s",
         status->dpid, status->port_no,
         ( status->status == TD_PORT_UP ? "up" : "down" ),
         ( status->external == TD_PORT_EXTERNAL ? "external" : "internal or inactive" ) );

  if ( status->port_no > OFPP_MAX && status->port_no != OFPP_LOCAL ) {
    warn( "Ignore this update ( port = %u )", status->port_no );
    return;
  }

  port_info *p = lookup_port( routing_switch->switches, status->dpid, status->port_no );

  delete_fdb_entries( routing_switch->fdb, status->dpid, status->port_no );

  if ( status->status == TD_PORT_UP ) {
    if ( p != NULL ) {
      update_port( p, status->external );
      return;
    }
    add_port( &routing_switch->switches, status->dpid, status->port_no, status->external );
  }
  else {
    if ( p == NULL ) {
      debug( "Ignore this update (not found nor already deleted)" );
      return;
    }
    delete_port( &routing_switch->switches, p );
  }
}


static int
build_packet_out_actions( port_info *port, void *user_data ) {
  if ( !port->external_link || port->switch_to_switch_reverse_link ) {
    // don't send to non-external port
    return 0;
  }

  port_params *params = user_data;

  if ( port->dpid == params->switch_params->in_datapath_id &&
       port->port_no == params->switch_params->in_port ) {
    // don't send to input port
    return 0;
  }

  int n_actions_original = params->actions->n_actions;
  uint16_t slice = params->switch_params->slice;
  uint16_t out_vid = params->switch_params->in_vid;
  bool found = get_port_vid( slice, port->dpid, port->port_no, &out_vid );
  if ( found ) {
    if ( out_vid == VLAN_NONE ) {
      append_action_strip_vlan( params->actions );
    }
    else {
      append_action_set_vlan_vid( params->actions, out_vid );
    }
  }
  else {
    if ( !loose_mac_based_slicing_enabled() ) {
      return 0;
    }
    if ( !mac_slice_maps_exist( slice ) ) {
      return 0;
    }
  }

  const uint16_t max_len = UINT16_MAX;
  append_action_output( params->actions, port->port_no, max_len );

  return params->actions->n_actions - n_actions_original;
}


static void
send_packet_out_for_each_switch( switch_info *sw, void *user_data ) {
  openflow_actions *actions = create_actions();
  port_params params;
  params.switch_params = user_data;
  params.actions = actions;
  foreach_port( sw->ports, build_packet_out_actions, &params );

  // check if no action is build
  if ( actions->n_actions > 0 ) {
    const buffer *packet = params.switch_params->packet;
    send_packet_out( sw->dpid, actions, packet );
  }

  delete_actions( actions );
}


static void
flood_packet( uint64_t datapath_id, uint16_t in_port, uint16_t slice, const buffer *packet, list_element *switches ) {
  uint16_t in_vid = VLAN_NONE;
  if ( packet_type_eth_vtag( packet ) ) {
    packet_info packet_info = get_packet_info( packet );
    in_vid = packet_info.vlan_vid;
  }

  switch_params params;
  params.packet = packet;
  params.in_datapath_id = datapath_id;
  params.in_port = in_port;
  params.in_vid = in_vid;
  params.slice = slice;

  foreach_switch( switches, send_packet_out_for_each_switch, &params );
}


static void
handle_packet_in( uint64_t datapath_id, uint32_t transaction_id,
                  uint32_t buffer_id, uint16_t total_len,
                  uint16_t in_port, uint8_t reason, const buffer *data,
                  void *user_data ) {
  assert( in_port != 0 );
  assert( data != NULL );
  assert( user_data != NULL );

  routing_switch *routing_switch = user_data;

  debug( "Packet-In received ( datapath_id = %#" PRIx64 ", transaction_id = %#lx, "
         "buffer_id = %#lx, total_len = %u, in_port = %u, reason = %#x, "
         "data_len = %u ).", datapath_id, transaction_id, buffer_id,
         total_len, in_port, reason, data->length );

  if ( in_port > OFPP_MAX && in_port != OFPP_LOCAL ) {
    error( "Packet-In from invalid port ( in_port = %u ).", in_port );
    return;
  }

  const port_info *port = lookup_port( routing_switch->switches, datapath_id, in_port );
  if ( port == NULL ) {
    debug( "Ignoring Packet-In from unknown port." );
    return;
  }

  packet_info packet_info = get_packet_info( data );
  const uint8_t *src = packet_info.eth_macsa;
  const uint8_t *dst = packet_info.eth_macda;

  if ( !port->external_link || port->switch_to_switch_reverse_link ) {
    if ( !port->external_link
         && port->switch_to_switch_link
         && port->switch_to_switch_reverse_link
         && !is_ether_multicast( dst )
         && lookup_fdb( routing_switch->fdb, src, &datapath_id, &in_port ) ) {
      debug( "Found a Packet-In from switch-to-switch link." );
    }
    else {
      debug( "Ignoring Packet-In from non-external link." );
      return;
    }
  }

  uint16_t vid = VLAN_NONE;
  if ( packet_type_eth_vtag( data ) ) {
    vid = packet_info.vlan_vid;
  }

  if ( !update_fdb( routing_switch->fdb, src, datapath_id, in_port ) ) {
    return;
  }

  char match_str[ 1024 ];
  struct ofp_match match;
  set_match_from_packet( &match, in_port, 0, data );
  match_to_string( &match, match_str, sizeof( match_str ) );

  uint16_t slice = lookup_slice( datapath_id, in_port, vid, src );
  if ( slice == SLICE_NOT_FOUND ) {
    warn( "No slice found ( dpid = %#" PRIx64 ", vid = %u, match = [%s] ).", datapath_id, vid, match_str );
    goto deny;
  }

  int action = filter( datapath_id, in_port, slice, data );
  switch ( action ) {
  case ALLOW:
    debug( "Filter: ALLOW ( dpid = %#" PRIx64 ", slice = %#x, match = [%s] ).", datapath_id, slice, match_str );
    goto allow;
  case DENY:
    debug( "Filter: DENY ( dpid = %#" PRIx64 ", slice = %#x, match = [%s] ).", datapath_id, slice, match_str );
    goto deny;
  case LOCAL:
    debug( "Filter: LOCAL ( dpid = %#" PRIx64 ", slice = %#x, match = [%s] ).", datapath_id, slice, match_str );
    goto local;
  default:
    error( "Undefined filter action ( action = %#x ).", action );
    goto deny;
  }

allow:
  {
    uint16_t out_port;
    uint64_t out_datapath_id;

    if ( lookup_fdb( routing_switch->fdb, dst, &out_datapath_id, &out_port ) ) {
      // Host is located, so resolve path and send flowmod
      if ( ( datapath_id == out_datapath_id ) && ( in_port == out_port ) ) {
        debug( "Input port and out port are the same ( datapath_id = %#llx, port = %u ).",
               datapath_id, in_port );
        return;
      }

      uint16_t out_vid = vid;
      bool found = get_port_vid( slice, out_datapath_id, out_port, &out_vid );
      if ( found == false ) {
        uint16_t out_slice = lookup_slice_by_mac( dst );
        if ( out_slice != slice ) {
          debug( "Destination is on different slice ( slice = %#x, out_slice = %#x ).",
                 slice, out_slice );
          goto deny;
        }
      }

      make_path( routing_switch, datapath_id, in_port, vid, out_datapath_id, out_port, out_vid, data );
    } else {
      // Host's location is unknown, so flood packet
      flood_packet( datapath_id, in_port, slice, data, routing_switch->switches );
    }
    return;
  }

deny:
  {
    // Drop packets for a certain period
    buffer *flow_mod = create_flow_mod( transaction_id, match, get_cookie(),
                                        OFPFC_ADD, 0, FLOW_TIMER,
                                        UINT16_MAX, UINT32_MAX,
                                        OFPP_NONE, 0, NULL );
    send_openflow_message( datapath_id, flow_mod );
    free_buffer( flow_mod );
    return;
  }

local:
  {
    // Redirect to controller's local IP stack
    redirect( datapath_id, in_port, data );
    return;
  }
}


static void
init_ports( list_element **switches, size_t n_entries, const topology_port_status *s ) {
  for ( size_t i = 0; i < n_entries; i++ ) {
    if ( s[ i ].status == TD_PORT_UP ) {
      add_port( switches, s[ i ].dpid, s[ i ].port_no, s[ i ].external );
    }
  }
}


static void
update_port_status_by_link( list_element *switches, const topology_link_status *s ) {
  port_info *port = lookup_port( switches, s->from_dpid, s->from_portno );
  if ( port != NULL ) {
    debug( "Link status updated: dpid:%#" PRIx64 ", port:%u, %s",
           s->from_dpid, s->from_portno,
           ( s->status == TD_LINK_UP ? "up" : "down" ) );
    port->switch_to_switch_link = ( s->status == TD_LINK_UP );
  }
  if ( s->to_portno == 0 ) {
    return;
  }
  port_info *peer = lookup_port( switches, s->to_dpid, s->to_portno );
  if ( peer != NULL ) {
    debug( "Reverse link status updated: dpid:%#" PRIx64 ", port:%u, %s",
           s->to_dpid, s->to_portno,
           ( s->status == TD_LINK_UP ? "up" : "down" ) );
    peer->switch_to_switch_reverse_link = ( s->status == TD_LINK_UP );
  }
}


static void
link_status_updated( void *user_data, const topology_link_status *status ) {
  assert( user_data != NULL );
  assert( status != NULL );

  routing_switch *routing_switch = user_data;
  update_topology( routing_switch->pathresolver, status );
  update_port_status_by_link( routing_switch->switches, status );
}


static void
update_link_status( routing_switch *routing_switch, size_t n_entries,
                    const topology_link_status *status ) {
  for ( size_t i = 0; i < n_entries; i++ ) {
    update_topology( routing_switch->pathresolver, &status[ i ] );
    update_port_status_by_link( routing_switch->switches, &status[ i ] );
  }
}


static void
init_last_stage( void *user_data, size_t n_entries, const topology_link_status *status ) {
  assert( user_data != NULL );

  routing_switch *routing_switch = user_data;

  update_link_status( routing_switch, n_entries, status );
  add_callback_link_status_updated( link_status_updated, routing_switch );
}


static void
init_second_stage( void *user_data, size_t n_entries, const topology_port_status *s ) {
  assert( user_data != NULL );

  routing_switch *routing_switch = user_data;

  // Initialize ports
  init_ports( &routing_switch->switches, n_entries, s );

  // Initialize aging FDB
  init_age_fdb( routing_switch->fdb );

  // Set asynchronous event handlers
  // (0) Set features_request_reply handler
  set_features_reply_handler( receive_features_reply, routing_switch );

  // (1) Set switch_ready handler
  set_switch_ready_handler( handle_switch_ready, routing_switch );

  // (2) Set port status update callback
  add_callback_port_status_updated( port_status_updated, routing_switch );

  // (3) Set packet-in handler
  set_packet_in_handler( handle_packet_in, routing_switch );

  // (4) Get all link status
  get_all_link_status( init_last_stage, routing_switch );
}


static void
after_subscribed( void *user_data ) {
  assert( user_data != NULL );

  // Get all ports' status
  // init_second_stage() will be called
  get_all_port_status( init_second_stage, user_data );
}


static routing_switch *
create_routing_switch( const char *topology_service, const routing_switch_options *options ) {
  assert( topology_service != NULL );
  assert( options != NULL );

  // Allocate routing_switch object
  routing_switch *instance = xmalloc( sizeof( routing_switch ) );
  instance->idle_timeout = options->idle_timeout;
  instance->switches = NULL;
  instance->fdb = NULL;
  instance->pathresolver = NULL;

  info( "idle_timeout is set to %u [sec].", instance->idle_timeout );

  // Create pathresolver table
  instance->pathresolver = create_pathresolver();

  // Create forwarding database
  instance->fdb = create_fdb();

  // Initialize port database
  instance->switches = create_ports( &instance->switches );

  // Initialize libraries
  init_libtopology( topology_service );

  // Ask topology manager to notify any topology change events.
  // after_subscribed() will be called
  subscribe_topology( after_subscribed, instance );

  // Initialize filter
  init_filter( options->filter_db_file );

  // Initialize multiple slices support
  init_slice( options->slice_db_file, options->mode, instance );

  // Initialize redirector
  init_redirector();

  return instance;
}


static void
delete_routing_switch( routing_switch *routing_switch ) {
  assert( routing_switch != NULL );

  // Delete pathresolver table
  delete_pathresolver( routing_switch->pathresolver );

  // Finalize libraries
  finalize_libtopology();

  // Delete ports
  delete_all_ports( &routing_switch->switches );

  // Delete forwarding database
  delete_fdb( routing_switch->fdb );

  // Finalize packet filter
  finalize_filter();

  // Finalize multiple slices support
  finalize_slice();

  // Finalize redirector
  finalize_redirector();

  // Delete routing_switch object
  xfree( routing_switch );
}


static char option_description[] = "  -i, --idle_timeout=TIMEOUT  idle timeout value of flow entry\n"
                                   "  -s, --slice_db=DB_FILE      slice database\n"
                                   "  -f, --filter_db=DB_FILE     filter database\n"
                                   "  -m, --loose                 enable loose mac-based slicing\n"
                                   "  -r, --restrict_hosts        restrict hosts on switch port\n";
static char short_options[] = "i:s:f:mr";
static struct option long_options[] = {
  { "idle_timeout", required_argument, NULL, 'i' },
  { "slice_db", required_argument, NULL, 's' },
  { "filter_db", required_argument, NULL, 'f' },
  { "loose", no_argument, NULL, 'm' },
  { "restrict_hosts", no_argument, NULL, 'r' },
  { NULL, 0, NULL, 0  },
};


static void
reset_getopt() {
  optind = 0;
  opterr = 1;
}


void usage( void );


static void
init_routing_switch_options( routing_switch_options *options, int *argc, char **argv[] ) {
  assert( options != NULL );
  assert( argc != NULL );
  assert( *argc >= 0 );
  assert( argv != NULL );

  // set default values
  options->idle_timeout = FLOW_TIMER;
  memset( options->slice_db_file, '\0', sizeof( options->slice_db_file ) );
  memset( options->filter_db_file, '\0', sizeof( options->filter_db_file ) );
  options->mode = 0;

  int argc_tmp = *argc;
  char *new_argv[ *argc ];

  for ( int i = 0; i <= *argc; ++i ) {
    new_argv[ i ] = ( *argv )[ i ];
  }

  int c;
  uint32_t idle_timeout;
  while ( ( c = getopt_long( *argc, *argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'i':
        idle_timeout = ( uint32_t ) atoi( optarg );
        if ( idle_timeout == 0 || idle_timeout > UINT16_MAX ) {
          printf( "Invalid idle_timeout value.\n" );
          usage();
          finalize_topology_service_interface_options();
          exit( EXIT_SUCCESS );
          return;
        }
        options->idle_timeout = ( uint16_t ) idle_timeout;
        break;

      case 's':
        if ( optarg != NULL ) {
          if ( realpath( optarg, options->slice_db_file ) == NULL ) {
            memset( options->slice_db_file, '\0', sizeof( options->slice_db_file ) );
          }
        }
        break;

      case 'f':
        if ( optarg != NULL ) {
          if ( realpath( optarg, options->filter_db_file ) == NULL ) {
            memset( options->filter_db_file, '\0', sizeof( options->filter_db_file ) );
          }
        }
        break;

      case 'm':
        options->mode |= LOOSE_MAC_BASED_SLICING;
        break;

      case 'r':
        options->mode |= RESTRICT_HOSTS_ON_PORT;
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

  if ( strlen( options->filter_db_file ) == 0 ) {
    printf( "--filter_db option is mandatory.\n" );
    usage();
    exit( EXIT_FAILURE );
  }

  if ( strlen( options->slice_db_file ) == 0 ) {
    printf( "--slice_db option is mandatory.\n" );
    usage();
    exit( EXIT_FAILURE );
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


void
usage() {
  topology_service_interface_usage( get_executable_name(), "Sliceable routing switch.", option_description );
}


int
main( int argc, char *argv[] ) {
  // Check if this application runs with root privilege
  if ( geteuid() != 0 ) {
    printf( "Sliceable routing switch must be run with root privilege.\n" );
    exit( EXIT_FAILURE );
  }

  // Initialize Trema world
  init_trema( &argc, &argv );
  routing_switch_options options;
  init_routing_switch_options( &options, &argc, &argv );
  init_topology_service_interface_options( &argc, &argv );

  // Initialize routing_switch
  routing_switch *routing_switch = create_routing_switch( get_topology_service_interface_name(), &options );

  // Main loop
  start_trema();

  // Finalize routing_switch
  delete_routing_switch( routing_switch );

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
