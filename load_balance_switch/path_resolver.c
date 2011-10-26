/*
 * Author: Shuji Ishii
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
#include <time.h>
#include "path_resolver.h"
#include "hash_table.h"
#include "doubly_linked_list.h"


typedef struct edge {
  uint64_t peer_dpid;    // key
  uint16_t port_no;
  uint16_t peer_port_no;
  uint32_t cost;
} edge;

typedef struct node {
  uint64_t dpid;        // key
  hash_table *edges;    // node * -> edge
  uint32_t distance;    // distance from root node
  bool visited;
  struct {
    struct node *node;
    struct edge *edge;
  } from;
} node;

typedef struct {
  uint64_t dpid;
  uint16_t port_no;
  uint32_t utilization;
  uint64_t tx_bytes;
  time_t updated_at;
} link_stats;


static bool
compare_topology( const void *x, const void *y ) {
  const topology_link_status *status_x = x;
  const topology_link_status *status_y = y;

  return ( status_x->from_dpid == status_y->from_dpid && status_x->from_portno == status_y->from_portno );
}


static unsigned int
hash_topology( const void *key ) {
  const topology_link_status *status = key;

  return hash_datapath_id( &status->from_dpid ) ^ status->from_portno;
}


static bool
compare_node( const void *x, const void *y ) {
  const node *node_x = x;
  const node *node_y = y;

  return ( node_x->dpid == node_y->dpid );
}


static unsigned int
hash_node( const void *key ) {
  const node *n = key;

  return hash_datapath_id( &n->dpid );
}


static edge *
lookup_edge( const node *from, const node *to ) {
  return lookup_hash_entry( from->edges, &to->dpid );
}


static node *
lookup_node( hash_table *node_table, const uint64_t dpid ) {
  node key;

  key.dpid = dpid;

  return lookup_hash_entry( node_table, &key );
}


static node *
allocate_node( hash_table *node_table, const uint64_t dpid ) {
  node *n = lookup_node( node_table, dpid );
  if ( n == NULL ) {
    n = xmalloc( sizeof( node ) );

    n->dpid = dpid;
    n->edges = create_hash( compare_datapath_id, hash_datapath_id );
    n->distance = UINT32_MAX;
    n->visited = false;
    n->from.node = NULL;
    n->from.edge = NULL;

    insert_hash_entry( node_table, n, n );
  }

  return n;
}


static void
free_edge( node *n, edge *e ) {
  assert( n != NULL );
  assert( e != NULL );
  edge *delete_me = delete_hash_entry( n->edges, &e->peer_dpid );
  if ( delete_me != NULL ) {
    xfree( delete_me );
  }
}


static void
add_edge( hash_table *node_table, const uint64_t from_dpid,
          const uint16_t from_port_no, const uint64_t to_dpid,
          const uint16_t to_port_no, const uint32_t cost ) {
  node *from = allocate_node( node_table, from_dpid );
  node *to = allocate_node( node_table, to_dpid );

  edge *e = lookup_edge( from, to );
  if ( e != NULL ) {
    free_edge( from, e );
  }
  e = xmalloc( sizeof( edge ) );
  e->port_no = from_port_no;
  e->peer_dpid = to_dpid;
  e->peer_port_no = to_port_no;
  e->cost = cost;

  insert_hash_entry( from->edges, &e->peer_dpid, e );
}


static uint32_t
calculate_link_cost( hash_table *link_stats_table, const topology_link_status *l ) {
  link_stats key;
  key.dpid = l->from_dpid;
  key.port_no = l->from_portno;
  link_stats *s = lookup_hash_entry( link_stats_table, &key );
  if ( s != NULL ) {
    return s->utilization;
  }
  else {
    return 0;
  }
}


static node *
pickup_next_candidate( hash_table *node_table ) {
  node *candidate = NULL;
  uint32_t min_cost = UINT32_MAX;

  hash_iterator iter;
  hash_entry *entry = NULL;
  node *n = NULL;

  // pickup candidate node whose distance is minimum from visited nodes
  init_hash_iterator( node_table, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    n = ( node * )entry->value;
    if ( !n->visited && n->distance < min_cost ) {
      min_cost = n->distance;
      candidate = n;
    }
  }

  return candidate;
}


static void
update_distance( hash_table *node_table, node *candidate ) {
  hash_iterator iter;
  hash_entry *entry = NULL;

  // update distance
  init_hash_iterator( node_table, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    node *n = ( node * )entry->value;
    edge *e;
    if ( n->visited || ( e = lookup_edge( candidate, n ) ) == NULL ) {
      continue;
    }
    if ( candidate->distance + e->cost < n->distance ) {
      // short path via edge 'e'
      n->distance = candidate->distance + e->cost;
      n->from.node = candidate; // (candidate)->(n)
      n->from.edge = e;
    }
  }
}


static dlist_element *
build_hop_list( node *src_node, uint16_t src_port_no,
                node *dst_node, uint16_t dst_port_no ) {
  node *n;
  uint16_t prev_out_port = UINT16_MAX;
  dlist_element *h = create_dlist();

  n = dst_node;
  while ( n != NULL ) {
    path_resolver_hop *hop = xmalloc( sizeof( path_resolver_hop ) );
    hop->dpid = n->dpid;

    if ( prev_out_port != 0xffffU ) {
      hop->out_port_no = prev_out_port;
    }
    else {
      hop->out_port_no = dst_port_no;
    }

    edge *e = n->from.edge;
    if ( e != NULL ) {
      prev_out_port = e->port_no;
      hop->in_port_no = e->peer_port_no;
    }

    h = insert_before_dlist( h, hop );
    n = n->from.node;
  }

  // adjust first hop
  path_resolver_hop *hh = ( path_resolver_hop * ) h->data; // points source node
  if ( hh->dpid != src_node->dpid ) {
    free_hop_list( h );
    return NULL;
  }
  
  hh->in_port_no = src_port_no;

  // trim last element
  delete_dlist_element( get_last_element( h ) );

  return h;
}


static hash_table *
create_node_table() {
  return create_hash( compare_node, hash_node );
}


static void
build_topology_table( path_resolver *table ) {
  hash_iterator iter;
  hash_entry *entry;

  init_hash_iterator( table->topology_table, &iter );
  table->node_table = create_node_table();
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    topology_link_status *l = entry->value;
    add_edge( table->node_table, l->from_dpid, l->from_portno, l->to_dpid,
              l->to_portno, calculate_link_cost( table->link_stats_table, l ) );
  }
}


static dlist_element *
dijkstra( hash_table *node_table, uint64_t in_dpid, uint16_t in_port_no,
          uint64_t out_dpid, uint16_t out_port_no ) {
  node *src_node = lookup_node( node_table, in_dpid );
  if ( src_node == NULL ) {
    return NULL;
  }

  src_node->distance = 0;
  src_node->from.node = NULL;
  src_node->from.edge = NULL;

  node *candidate;
  while ( ( candidate = pickup_next_candidate( node_table ) ) != NULL ) {
    candidate->visited = true;
    update_distance( node_table, candidate );
  }

  // build path hop list
  node *dst_node = lookup_node( node_table, out_dpid );
  if ( dst_node == NULL ) {
    return NULL;
  }

  return build_hop_list( src_node, in_port_no, dst_node, out_port_no );
}


static void
free_node( hash_table *node_table, node *n ) {
  if ( n == NULL ) {
    return;
  }

  hash_iterator iter;
  hash_entry *entry;

  init_hash_iterator( n->edges, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    edge *e = entry->value;
    free_edge( n, e );
  }

  delete_hash( n->edges );
  delete_hash_entry( node_table, n );
  xfree( n );
}


static void
free_all_node( hash_table *node_table ) {
  hash_iterator iter;
  hash_entry *e;

  init_hash_iterator( node_table, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    node *n = e->value;
    free_node( node_table, n );
  }
}


static void
delete_node_table( hash_table *node_table ) {
  free_all_node( node_table );
  delete_hash( node_table );
}


dlist_element *
resolve_path( path_resolver *table, uint64_t in_dpid, uint16_t in_port,
              uint64_t out_dpid, uint16_t out_port ) {
  assert( table != NULL );
  assert( table->topology_table != NULL );
  if ( table->node_table != NULL ) {
    delete_node_table( table->node_table );
  }
  build_topology_table( table );
  return dijkstra( table->node_table, in_dpid, in_port, out_dpid, out_port );
}


void
free_hop_list( dlist_element *hops ) {
  dlist_element *e = get_first_element( hops );
  while ( e != NULL ) {
    dlist_element *delete_me = e;
    e = e->next;
    if ( delete_me->data != NULL ) {
      xfree( delete_me->data );
    }
    xfree( delete_me );
  }
}


static bool
compare_link_stats( const void *x, const void *y ) {
  const link_stats *stats_x = x;
  const link_stats *stats_y = y;

  return ( stats_x->dpid == stats_y->dpid && stats_x->port_no == stats_y->port_no );
}


static unsigned int
hash_link_stats( const void *key ) {
  const link_stats *stats = key;

  return hash_datapath_id( &stats->dpid ) ^ stats->port_no;
}


static void
handle_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                    uint16_t type, uint16_t flags, const buffer *data,
                    void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( flags );

  if ( type != OFPST_PORT ) {
    return;
  }
  if ( data == NULL ) {
    return;
  }

  struct ofp_port_stats *stats = data->data;
  debug( "datapath_id = %#" PRIx64 ", port = %u, tx_bytes = %" PRIu64 ".",
         datapath_id, stats->port_no, stats->tx_bytes );

  path_resolver *table = user_data;
  link_stats entry;
  entry.dpid = datapath_id;
  entry.port_no = stats->port_no;
  link_stats *s = lookup_hash_entry( table->link_stats_table, &entry );
  if ( s != NULL ) {
    uint64_t diff_bytes = 0;
    if ( stats->tx_bytes >= s->tx_bytes ) {
      diff_bytes = stats->tx_bytes - s->tx_bytes;
    }
    else {
      diff_bytes = UINT64_MAX - s->tx_bytes + stats->tx_bytes;
    }
    s->utilization = diff_bytes / 100; // FIXME: magic number
    s->tx_bytes = stats->tx_bytes;
    s->updated_at = time( NULL );
  }
  else {
    s = xmalloc( sizeof( link_stats ) );
    s->dpid = datapath_id;
    s->port_no = stats->port_no;
    s->utilization = 0;
    s->tx_bytes = stats->tx_bytes;
    s->updated_at = time( NULL );
    insert_hash_entry( table->link_stats_table, s, s );
  }
}


static void
update_link_costs( void *user_data ) {
  path_resolver *table = user_data;
  hash_iterator iter;
  hash_entry *entry;

  init_hash_iterator( table->topology_table, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    topology_link_status *link = entry->value;
    buffer *request = create_port_stats_request( get_transaction_id(), 0, link->from_portno );
    send_openflow_message( link->from_dpid, request );
    free_buffer( request );
  }
}


static hash_table *
create_link_stats_table() {
  return create_hash( compare_link_stats, hash_link_stats );
}


static hash_table *
create_topology_table() {
  return create_hash( compare_topology, hash_topology );
}


path_resolver *
create_path_resolver() {
  path_resolver *table = xmalloc( sizeof( path_resolver ) );
  table->topology_table = create_topology_table();
  table->node_table = NULL;
  table->link_stats_table = create_link_stats_table();

  set_stats_reply_handler( handle_stats_reply, table );
  add_periodic_event_callback( 5, update_link_costs, table );

  return table;
}


static void
free_topology( hash_table *topology_table ) {
  hash_iterator iter;
  hash_entry *e;

  init_hash_iterator( topology_table, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    xfree( e->value );
  }
}


static void
delete_topology_table( hash_table *topology_table ) {
  free_topology( topology_table );
  delete_hash( topology_table );
}


static void
free_link_stats( hash_table *link_stats_table ) {
  hash_iterator iter;
  hash_entry *e;

  init_hash_iterator( link_stats_table, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    xfree( e->value );
  }
}


static void
delete_link_stats_table( hash_table *link_stats_table ) {
  free_link_stats( link_stats_table );
  delete_hash( link_stats_table );
}


bool
delete_path_resolver( path_resolver *table ) {
  assert( table != NULL );
  assert( table->topology_table != NULL );

  if ( table->node_table != NULL ) {
    delete_node_table( table->node_table );
  }
  delete_topology_table( table->topology_table );
  if ( table->link_stats_table != NULL ) {
    delete_link_stats_table( table->link_stats_table );
  }
  xfree( table );

  return true;
}


void
update_topology( path_resolver *table, const topology_link_status *s ) {
  assert( table != NULL );
  assert( table->topology_table != NULL );

  bool updated = false;

  hash_entry *e = lookup_hash_entry( table->topology_table, s );
  if ( s->status == TD_LINK_UP ) {
    if ( e == NULL ) {
      topology_link_status *new = xmalloc( sizeof( topology_link_status ) );
      *new = *s;
      insert_hash_entry( table->topology_table, new, new );
      updated = true;
    }
  }
  else {
    if ( e != NULL ) {
      topology_link_status *delete = delete_hash_entry( table->topology_table, s );
      xfree( delete );
      updated = true;
    }
  }

  if ( updated && table->node_table != NULL ) {
    delete_node_table( table->node_table );
    table->node_table = NULL;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
