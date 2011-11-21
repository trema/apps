/*
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


#ifndef LIBPATH_H
#define LIBPATH_H


#include <inttypes.h>
#include "trema.h"


typedef struct {
  uint64_t datapath_id;
  uint16_t in_port;
  uint16_t out_port;
  openflow_actions *extra_actions;
} hop;

typedef struct {
  struct ofp_match match;
  uint16_t priority;
  uint16_t idle_timeout;
  uint16_t hard_timeout;
  int n_hops;
  list_element *hops;
} path;

enum {
  SETUP_SUCCEEDED,
  SETUP_CONFLICTED_ENTRY,
  SETUP_SWITCH_ERROR,
  SETUP_UNKNOWN_ERROR,
};

typedef void ( *setup_handler )(
  int status,
  const path *path,
  void *user_data
);

enum {
  TEARDOWN_TIMEOUT,
  TEARDOWN_MANUALLY_REQUESTED,
};

typedef void ( *teardown_handler )(
  int reason,
  const path *path,
  void *user_data
);


hop *create_hop( uint64_t datapath_id, uint16_t in_port, uint16_t out_port, openflow_actions *extra_actions );
void delete_hop( hop *hop );
hop *copy_hop( const hop *hop );

path *create_path( struct ofp_match match, uint16_t priority, uint16_t idle_timeout, uint16_t hard_timeout );
void append_hop_to_path( path *path, hop *hop );
void delete_path( path *path );
path *copy_path( const path *path );

bool setup_path( path *path, setup_handler setup_callback, void *setup_user_data,
                 teardown_handler teardown_callback, void *teardown_user_data );

bool teardown_path( uint64_t in_datapath_id, struct ofp_match match, uint16_t priority );
bool teardown_path_by_match( struct ofp_match match );

const path *lookup_path( uint64_t in_datapath_id, struct ofp_match match, uint16_t priority );
bool lookup_path_by_match( struct ofp_match match, int *n_paths, path **paths );

bool init_path( void );
bool finalize_path( void );


#endif // LIBPATH_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
