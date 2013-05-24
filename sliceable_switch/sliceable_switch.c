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
#include "icmp.h"
#include "filter.h"
#include "libpathresolver.h"
#include "libtopology.h"
#include "port.h"
#include "redirector.h"
#include "slice.h"
#include "sliceable_switch.h"
#include "topology_service_interface_option_parser.h"
#include "libpath.h"


static const uint16_t FLOW_TIMER = 60;
static const uint16_t DISCARD_DURATION_DENY = 60;
static const uint16_t DISCARD_DURATION_NO_PATH = 1;
static const uint16_t PRIORITY = UINT16_MAX;
static const uint16_t DISCARD_PRIORITY = UINT16_MAX;


typedef struct {
  uint16_t idle_timeout;
  uint16_t mode;
  bool handle_arp_with_packetout;
  bool setup_reverse_flow;
  char slice_db_file[ PATH_MAX ];
  char filter_db_file[ PATH_MAX ];
} switch_options;


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


typedef struct packet_out_params {
  buffer *packet;
  uint64_t out_datapath_id;
  uint16_t out_port_no;
  uint16_t out_vid;
  uint16_t in_vid;
} packet_out_params;


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
handle_setup( int status, const path *p, void *user_data ) {
  UNUSED(p);
  assert(user_data);

  packet_out_params *params = user_data;
  if ( status != SETUP_SUCCEEDED ) {
    error( "Failed to set up path ( status = %d ).", status );
  }
  output_packet( params->packet, params->out_datapath_id, params->out_port_no, params->out_vid );
  free_buffer( params->packet );
  xfree( params );
}


static bool
packet_to_set_reverse_path( const buffer *packet ) {

  packet_info *pinfo = ( packet_info * ) packet->user_data;
  if ( is_ether_multicast( pinfo->eth_macsa ) || is_ether_multicast( pinfo->eth_macda ) ) {
    return false;
  }

  if ( packet_type_ipv4_tcp( packet ) || packet_type_ipv4_udp( packet ) || packet_type_icmpv4_echo_request( packet ) ) {
    return true;
  }
  return false;
} 


static void
set_ipv4_reverse_match( struct ofp_match *r, const struct ofp_match *match) {
  memcpy( r, match, sizeof( struct ofp_match ) );

  uint32_t clean_field = OFPFW_DL_SRC + OFPFW_DL_DST + OFPFW_TP_SRC + OFPFW_TP_DST + OFPFW_NW_SRC_MASK + OFPFW_NW_DST_MASK;
  r->wildcards &= ( OFPFW_ALL - clean_field );
  if ( match->wildcards & OFPFW_DL_DST ) {
    r->wildcards |= OFPFW_DL_SRC;
  }
  if ( match->wildcards & OFPFW_DL_SRC ) {
    r->wildcards |= OFPFW_DL_DST;
  }
  if ( match->wildcards & OFPFW_TP_DST ) {
    r->wildcards |= OFPFW_TP_SRC;
  }
  if ( match->wildcards & OFPFW_TP_SRC ) {
    r->wildcards |= OFPFW_TP_DST;
  }
  r->wildcards |= ( match->wildcards & OFPFW_NW_DST_MASK ) >> OFPFW_NW_DST_SHIFT << OFPFW_NW_SRC_SHIFT;
  r->wildcards |= ( match->wildcards & OFPFW_NW_SRC_MASK ) >> OFPFW_NW_SRC_SHIFT << OFPFW_NW_DST_SHIFT;

  memcpy( r->dl_src, match->dl_dst, OFP_ETH_ALEN );
  memcpy( r->dl_dst, match->dl_src, OFP_ETH_ALEN );
  r->nw_src = match->nw_dst;
  r->nw_dst = match->nw_src;
  switch ( match->nw_proto ) {
  case IPPROTO_TCP:
    r->tp_src = match->tp_dst;
    r->tp_dst = match->tp_src;
    break;
  case IPPROTO_UDP:
    r->tp_src = match->tp_dst;
    r->tp_dst = match->tp_src;
    break;
  case IPPROTO_ICMP:
    if ( match->icmp_type == ICMP_TYPE_ECHOREQ ) {
      r->icmp_type = ICMP_TYPE_ECHOREP;
      r->wildcards &= ( OFPFW_ALL - OFPFW_ICMP_TYPE );
      r->wildcards |= OFPFW_ICMP_CODE; 
    }
    break;
  default:
    break;
  }
}


static openflow_actions *
create_openflow_actions_to_update_vid( uint16_t in_vid, uint16_t out_vid ) {
  if ( in_vid == out_vid ) {
    return NULL;
  }

  openflow_actions *vlan_actions = create_actions();
  if ( out_vid == VLAN_NONE ) {
    append_action_strip_vlan( vlan_actions );
  }
  else {
    append_action_set_vlan_vid( vlan_actions, out_vid );
  }
  return vlan_actions;
}


static void
setup_reverse_path( int status, const path *p, void *user_data ) {
  assert(user_data);

  packet_out_params *params = user_data;
  if ( status != SETUP_SUCCEEDED ) {
    error( "Failed to set up path ( status = %d ).", status );
    output_packet( params->packet, params->out_datapath_id, params->out_port_no, params->out_vid );
    free_buffer( params->packet );
    xfree( params );
    return;
  }

  struct ofp_match rmatch;
  set_ipv4_reverse_match( &rmatch, &(p->match) );
  rmatch.dl_vlan = params->out_vid;

  openflow_actions *vlan_actions;
  vlan_actions = create_openflow_actions_to_update_vid( params->out_vid, params->in_vid );

  path *rpath = create_path( rmatch, p->priority, p->idle_timeout, p->hard_timeout );
  assert( rpath != NULL );
  list_element *hops = p->hops;
  dlist_element *rhops = create_dlist();
  while ( hops != NULL ) {
    hop *h = hops->data;
    assert( h != NULL );
    hop *rh = create_hop( h->datapath_id, h->out_port, h->in_port, vlan_actions );
    if ( vlan_actions ) {
      delete_actions( vlan_actions );
      vlan_actions = NULL;
    }
    assert( rh != NULL );
    rhops = insert_before_dlist( rhops, rh );
    hops = hops->next;
  }
  while ( rhops != NULL && rhops->data != NULL ) {
    append_hop_to_path( rpath, ( hop * ) rhops->data );
    rhops = rhops->next;
  }
  bool ret = setup_path( rpath, handle_setup, params, NULL, NULL );
  if ( ret != true ) {
    error( "Failed to set up reverse path." );
    output_packet( params->packet, params->out_datapath_id, params->out_port_no, params->out_vid );
    free_buffer( params->packet );
    xfree( params );
  }

  delete_path( rpath );
  delete_dlist( rhops );
}


static void
discard_packet_in( uint64_t datapath_id, uint16_t in_port, const buffer *packet, const uint16_t hard_timeout ) {
  const uint32_t wildcards = 0;
  struct ofp_match match;
  set_match_from_packet( &match, in_port, wildcards, packet );
  char match_str[ 1024 ];
  match_to_string( &match, match_str, sizeof( match_str ) );

  const uint16_t idle_timeout = 0;
  const uint32_t buffer_id = UINT32_MAX;
  const uint16_t flags = 0;

  info( "Discarding packets for a certain period ( datapath_id = %#" PRIx64
        ", match = [%s], duration = %u [sec] ).", datapath_id, match_str, hard_timeout );

  buffer *flow_mod = create_flow_mod( get_transaction_id(), match, get_cookie(),
                                      OFPFC_ADD, idle_timeout, hard_timeout,
                                      DISCARD_PRIORITY, buffer_id,
                                      OFPP_NONE, flags, NULL );

  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );
}


static void
make_path( sliceable_switch *sliceable_switch, uint64_t in_datapath_id, uint16_t in_port, uint16_t in_vid,
           uint64_t out_datapath_id, uint16_t out_port, uint16_t out_vid, const buffer *packet ) {
  dlist_element *hops = resolve_path( sliceable_switch->pathresolver, in_datapath_id, in_port, out_datapath_id, out_port );
  if ( hops == NULL ) {
    warn( "No available path found ( %#" PRIx64 ":%u -> %#" PRIx64 ":%u ).",
          in_datapath_id, in_port, out_datapath_id, out_port );
    discard_packet_in( in_datapath_id, in_port, packet, DISCARD_DURATION_NO_PATH );
    return;
  }

  // check if the packet is ARP or not
  if ( sliceable_switch->handle_arp_with_packetout && packet_type_arp( packet ) ) {
    // send packet out for tail switch
    free_hop_list( hops );
    output_packet( packet, out_datapath_id, out_port, out_vid );
    return;
  }

  const uint32_t wildcards = 0;
  struct ofp_match match;
  set_match_from_packet( &match, in_port, wildcards, packet );

  if ( lookup_path( in_datapath_id, match, PRIORITY ) != NULL ) {
    warn( "Duplicated path found." );
    output_packet( packet, out_datapath_id, out_port, out_vid );
    return;
  }

  const uint16_t hard_timeout = 0;
  path *p = create_path( match, PRIORITY, sliceable_switch->idle_timeout, hard_timeout );
  assert( p != NULL );
  openflow_actions *vlan_actions = create_openflow_actions_to_update_vid( in_vid, out_vid );
  for ( dlist_element *e = get_first_element( hops ); e != NULL; e = e->next ) {
    pathresolver_hop *rh = e->data;
    openflow_actions *actions = NULL;
    if ( e->next == NULL ) {
      actions = vlan_actions;
    }
    hop *h = create_hop( rh->dpid, rh->in_port_no, rh->out_port_no, actions );
    assert( h != NULL );
    append_hop_to_path( p, h );
  } // for(;;)
  if ( vlan_actions ) {
    delete_actions( vlan_actions );
  }

  dlist_element *e = get_last_element( hops );
  pathresolver_hop *last_hop = e->data;
  packet_out_params *params = xmalloc( sizeof( struct packet_out_params ) );
  params->packet = duplicate_buffer( packet );
  params->out_datapath_id = last_hop->dpid;
  params->out_port_no = last_hop->out_port_no;
  params->out_vid = out_vid;
  params->in_vid = in_vid;

  bool ret;
  if ( sliceable_switch->setup_reverse_flow && packet_to_set_reverse_path( packet ) ) {
    ret = setup_path( p, setup_reverse_path, params, NULL, NULL );
  }
  else {
    ret = setup_path( p, handle_setup, params, NULL, NULL );
  }
  if ( ret != true ) {
    error( "Failed to set up path." );
    output_packet( packet, out_datapath_id, out_port, out_vid );
    free_buffer( params->packet );
    xfree( params );
  }

  delete_path( p );

  // free them
  free_hop_list( hops );
}


static void
port_status_updated( void *user_data, const topology_port_status *status ) {
  assert( user_data != NULL );
  assert( status != NULL );

  sliceable_switch *sliceable_switch = user_data;
  if ( sliceable_switch->second_stage_down == false ) {
    return;
  }

  debug( "Port status updated: dpid:%#" PRIx64 ", port:%u(%s), %s, %s",
         status->dpid, status->port_no, status->name,
         ( status->status == TD_PORT_UP ? "up" : "down" ),
         ( status->external == TD_PORT_EXTERNAL ? "external" : "internal or inactive" ) );

  if ( status->port_no > OFPP_MAX && status->port_no != OFPP_LOCAL ) {
    warn( "Ignore this update ( port = %u )", status->port_no );
    return;
  }

  port_info *p = lookup_port( sliceable_switch->switches, status->dpid, status->port_no );

  delete_fdb_entries( sliceable_switch->fdb, status->dpid, status->port_no );

  if ( status->status == TD_PORT_UP ) {
    if ( p != NULL ) {
      update_port( p, status->external );
      return;
    }
    add_port( &sliceable_switch->switches, status->dpid, status->port_no, status->name, status->external );
  }
  else {
    if ( p == NULL ) {
      debug( "Ignore this update (not found nor already deleted)" );
      return;
    }
    delete_port( &sliceable_switch->switches, p );
    struct ofp_match match;
    memset( &match, 0, sizeof( struct ofp_match ) );
    match.wildcards = OFPFW_ALL;
    match.wildcards &= ~OFPFW_IN_PORT;
    match.in_port = status->port_no;
    teardown_path_by_match( match );
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

  sliceable_switch *sliceable_switch = user_data;

  debug( "Packet-In received ( datapath_id = %#" PRIx64 ", transaction_id = %#" PRIx32 ", "
         "buffer_id = %#" PRIx32 ", total_len = %u, in_port = %u, reason = %#x, "
         "data_len = %zu ).", datapath_id, transaction_id, buffer_id,
         total_len, in_port, reason, data->length );

  if ( in_port > OFPP_MAX && in_port != OFPP_LOCAL ) {
    error( "Packet-In from invalid port ( in_port = %u ).", in_port );
    return;
  }

  const port_info *port = lookup_port( sliceable_switch->switches, datapath_id, in_port );
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
         && lookup_fdb( sliceable_switch->fdb, src, &datapath_id, &in_port ) ) {
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

  if ( !update_fdb( sliceable_switch->fdb, src, datapath_id, in_port ) ) {
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

    if ( lookup_fdb( sliceable_switch->fdb, dst, &out_datapath_id, &out_port ) ) {
      // Host is located, so resolve path and send flowmod
      if ( ( datapath_id == out_datapath_id ) && ( in_port == out_port ) ) {
        debug( "Input port and out port are the same ( datapath_id = %#" PRIx64 ", port = %u ).",
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

      make_path( sliceable_switch, datapath_id, in_port, vid, out_datapath_id, out_port, out_vid, data );
    } else {
      if ( lookup_path( datapath_id, match, PRIORITY ) != NULL ) {
        teardown_path( datapath_id, match, PRIORITY );
      }

      // Host's location is unknown, so flood packet
      flood_packet( datapath_id, in_port, slice, data, sliceable_switch->switches );
    }
    return;
  }

deny:
  {
    // Drop packets for a certain period
    discard_packet_in( datapath_id, in_port, data, DISCARD_DURATION_DENY );
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
      add_port( switches, s[ i ].dpid, s[ i ].port_no, s[ i ].name, s[ i ].external );
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

  sliceable_switch *sliceable_switch = user_data;
  if ( sliceable_switch->last_stage_down == false ) {
    return;
  }

  update_topology( sliceable_switch->pathresolver, status );
  update_port_status_by_link( sliceable_switch->switches, status );
}


static void
update_link_status( sliceable_switch *sliceable_switch, size_t n_entries,
                    const topology_link_status *status ) {
  for ( size_t i = 0; i < n_entries; i++ ) {
    update_topology( sliceable_switch->pathresolver, &status[ i ] );
    update_port_status_by_link( sliceable_switch->switches, &status[ i ] );
  }
}


static void
init_last_stage( void *user_data, size_t n_entries, const topology_link_status *status ) {
  assert( user_data != NULL );

  sliceable_switch *sliceable_switch = user_data;
  sliceable_switch->last_stage_down = true;

  update_link_status( sliceable_switch, n_entries, status );
}


static void
init_second_stage( void *user_data, size_t n_entries, const topology_port_status *s ) {
  assert( user_data != NULL );

  sliceable_switch *sliceable_switch = user_data;
  sliceable_switch->second_stage_down = true;

  // Initialize ports
  init_ports( &sliceable_switch->switches, n_entries, s );

  // Initialize aging FDB
  init_age_fdb( sliceable_switch->fdb );

  // Set asynchronous event handlers

  // (1) Set packet-in handler
  set_packet_in_handler( handle_packet_in, sliceable_switch );

  // (2) Get all link status
  add_callback_link_status_updated( link_status_updated, sliceable_switch );
  get_all_link_status( init_last_stage, sliceable_switch );
}


static void
after_subscribed( void *user_data ) {
  assert( user_data != NULL );

  // Get all ports' status
  // init_second_stage() will be called
  add_callback_port_status_updated( port_status_updated, user_data );
  get_all_port_status( init_second_stage, user_data );
}


static sliceable_switch *
create_sliceable_switch( const char *topology_service, const switch_options *options ) {
  assert( topology_service != NULL );
  assert( options != NULL );

  // Allocate sliceable_switch object
  sliceable_switch *instance = xmalloc( sizeof( sliceable_switch ) );
  instance->idle_timeout = options->idle_timeout;
  instance->handle_arp_with_packetout = options->handle_arp_with_packetout;
  instance->setup_reverse_flow = options->setup_reverse_flow;
  instance->switches = NULL;
  instance->fdb = NULL;
  instance->pathresolver = NULL;
  instance->second_stage_down = false;
  instance->last_stage_down = false;

  info( "idle_timeout is set to %u [sec].", instance->idle_timeout );
  if ( instance->handle_arp_with_packetout ) {
    info( "Handle ARP with packetout" );
  }

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
delete_sliceable_switch( sliceable_switch *sliceable_switch ) {
  assert( sliceable_switch != NULL );

  // Delete pathresolver table
  delete_pathresolver( sliceable_switch->pathresolver );

  // Finalize libraries
  finalize_libtopology();

  // Delete ports
  delete_all_ports( &sliceable_switch->switches );

  // Delete forwarding database
  delete_fdb( sliceable_switch->fdb );

  // Finalize packet filter
  finalize_filter();

  // Finalize multiple slices support
  finalize_slice();

  // Finalize redirector
  finalize_redirector();

  // Delete sliceable_switch object
  xfree( sliceable_switch );
}


static char option_description[] = "  -i, --idle_timeout=TIMEOUT      idle timeout value of flow entry\n"
                                   "  -A, --handle_arp_with_packetout handle ARP with packetout\n"
                                   "  -s, --slice_db=DB_FILE          slice database\n"
                                   "  -a, --filter_db=DB_FILE         filter database\n"
                                   "  -m, --loose                     enable loose mac-based slicing\n"
                                   "  -r, --restrict_hosts            restrict hosts on switch port\n"
                                   "  -u, --setup_reverse_flow        setup going and returning flows\n";
static char short_options[] = "i:As:a:mru";
static struct option long_options[] = {
  { "idle_timeout", required_argument, NULL, 'i' },
  { "handle_arp_with_packetout", 0, NULL, 'A' },
  { "slice_db", required_argument, NULL, 's' },
  { "filter_db", required_argument, NULL, 'a' },
  { "loose", no_argument, NULL, 'm' },
  { "restrict_hosts", no_argument, NULL, 'r' },
  { "setup_reverse_flow", no_argument, NULL, 'u' },
  { NULL, 0, NULL, 0  },
};


static void
reset_getopt() {
  optind = 0;
  opterr = 1;
}


void usage( void );


static void
init_switch_options( switch_options *options, int *argc, char **argv[] ) {
  assert( options != NULL );
  assert( argc != NULL );
  assert( *argc >= 0 );
  assert( argv != NULL );

  // set default values
  options->idle_timeout = FLOW_TIMER;
  options->handle_arp_with_packetout = false;
  memset( options->slice_db_file, '\0', sizeof( options->slice_db_file ) );
  memset( options->filter_db_file, '\0', sizeof( options->filter_db_file ) );
  options->mode = 0;
  options->setup_reverse_flow = false;

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

      case 'A':
        options->handle_arp_with_packetout = true;
        break;

      case 's':
        if ( optarg != NULL ) {
          if ( realpath( optarg, options->slice_db_file ) == NULL ) {
            memset( options->slice_db_file, '\0', sizeof( options->slice_db_file ) );
          }
        }
        break;

      case 'a':
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

      case 'u':
        options->setup_reverse_flow = true;
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
  topology_service_interface_usage( get_executable_name(), "Sliceable switch.", option_description );
}


int
main( int argc, char *argv[] ) {
  // Check if this application runs with root privilege
  if ( geteuid() != 0 ) {
    printf( "Sliceable switch must be run with root privilege.\n" );
    exit( EXIT_FAILURE );
  }

  // Initialize Trema world
  init_trema( &argc, &argv );
  // Init path management library (libpath)
  init_path();
  switch_options options;
  init_switch_options( &options, &argc, &argv );
  init_topology_service_interface_options( &argc, &argv );

  // Initialize sliceable_switch
  sliceable_switch *sliceable_switch = create_sliceable_switch( get_topology_service_interface_name(), &options );

  // Main loop
  start_trema();

  // Finalize sliceable_switch
  delete_sliceable_switch( sliceable_switch );
  // Finalize path management library (libpath)
  finalize_path();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
