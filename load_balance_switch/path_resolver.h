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


#ifndef PATH_RESOLVER_H
#define PATH_RESOLVER_H


#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "libtopology.h"
#include "hash_table.h"
#include "doubly_linked_list.h"


typedef struct {
  hash_table *topology_table;
  hash_table *node_table;
  hash_table *link_stats_table;
} path_resolver;


typedef struct {
  uint64_t dpid;
  uint16_t in_port_no;
  uint16_t out_port_no;
} path_resolver_hop;


dlist_element *resolve_path( path_resolver *table, uint64_t in_dpid, uint16_t in_port,
                             uint64_t out_dpid, uint16_t out_port );
void free_hop_list( dlist_element *hops );
path_resolver *create_path_resolver( void );
bool delete_path_resolver( path_resolver *table );
void update_topology( path_resolver *table, const topology_link_status *s );


#endif	// PATH_RESOLVER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
