/*
 * Author: Yasunobu Chiba
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


#include <errno.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sqlite3.h>
#include "filter.h"


#define FILTER_DB_UPDATE_INTERVAL 30


typedef struct {
  hash_table *hash;
  list_element *list;
} filter_table;


#define WILDCARD_IN_DATAPATH_ID 0x00000001
#define WILDCARD_SLICE_NUMBER   0x00000002
#define WILDCARD_ALL ( WILDCARD_IN_DATAPATH_ID | WILDCARD_SLICE_NUMBER )


typedef struct {
  struct ofp_match ofp_match;
  uint32_t wildcards;
  uint64_t in_datapath_id;
  uint16_t slice_number;
} filter_match;


typedef struct {
  filter_match match;
  uint16_t priority;
  uint8_t action;
} filter_entry;


static filter_table filter_db;
static time_t last_filter_db_mtime = 0;
static char filter_db_file[ PATH_MAX ];


static bool
compare_filter_entry( const void *x, const void *y ) {
  const filter_match *match_x = x;
  const filter_match *match_y = y;

  bool ret = compare_match( &match_x->ofp_match, &match_y->ofp_match );

  uint32_t wildcards = match_x->wildcards | match_y->wildcards;

  ret = ( ( wildcards & WILDCARD_IN_DATAPATH_ID || match_x->in_datapath_id == match_y->in_datapath_id ) &&
          ( wildcards & WILDCARD_SLICE_NUMBER || match_x->slice_number == match_y->slice_number ) && ret );

  return ret;
}


static unsigned int
hash_filter_entry( const void *key ) {
  /* FNV-1a hash
     see: http://isthe.com/chongo/tech/comp/fnv/index.html
  */
#define HDB_FNV_32_PRIME ((uint32_t)0x01000193)
#define HDB_FNV_32_OFFSET_BASIS ((uint32_t)0x811c9dc5)

  int i;
  int key_size = sizeof( filter_match );
  uint32_t hash = HDB_FNV_32_OFFSET_BASIS;
  filter_match match;
  memcpy( &match, key, sizeof( filter_match ) );
  uint8_t *k = ( uint8_t * ) &match;

  for (i = 0; i < key_size; i++) {
    hash ^= ( uint32_t ) *k++;
    hash *= HDB_FNV_32_PRIME;
  }

  return ( unsigned int ) hash;
}


static bool
create_filter_db() {
  if ( filter_db.hash != NULL || filter_db.list != NULL ) {
    return false;
  }

  filter_db.hash = create_hash( compare_filter_entry, hash_filter_entry );
  create_list( &filter_db.list );

  return true;
}


static bool
delete_filter_db() {
  if ( filter_db.hash == NULL || filter_db.list == NULL ) {
    return false;
  }

  hash_iterator iter;
  hash_entry *entry;
  init_hash_iterator( filter_db.hash, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
      xfree( entry->value );
  }
  delete_hash( filter_db.hash );
  filter_db.hash = NULL;

  list_element *element = filter_db.list;
  while ( element != NULL ) {
    xfree( element->data );
    element = element->next;
  }
  delete_list( filter_db.list );
  filter_db.list = NULL;

  return true;
}


static filter_entry*
lookup_filter( filter_match match ) {
  filter_entry *entry;

  char match_str[ 1024 ];
  match_to_string( &match.ofp_match, match_str, sizeof( match_str ) );

  debug( "Looking up filter entry ( wildcards = %#x, in_datapath_id = %#llx, slice_number = %#x, ofp_match = [%s] ).",
         match.wildcards, match.in_datapath_id, match.slice_number, match_str );

  if ( match.wildcards == 0 && match.ofp_match.wildcards == 0 ) {
    entry = lookup_hash_entry( filter_db.hash, &match );
    if ( entry != NULL ) {
      debug( "A filter entry found (hash)." );

      return entry;
    }
  }

  list_element *element = filter_db.list;
  while ( element != NULL ) {
    entry = element->data;
    element = element->next;
    if ( compare_filter_entry( &match, &entry->match ) ) {
      debug( "A filter entry found (list)." );

      return entry;
    }
  }

  debug( "Filter entry not found." );

  return NULL;
}


static bool
compare_filter_entry_strict( const filter_match *x, const filter_match *y ) {
  const filter_match *match_x = x;
  const filter_match *match_y = y;

  uint32_t wildcards_x = match_x->wildcards & WILDCARD_ALL;
  uint32_t wildcards_y = match_y->wildcards & WILDCARD_ALL;

  if ( wildcards_x != wildcards_y ) {
    return false;
  }

  bool ret = compare_match_strict( &match_x->ofp_match, &match_y->ofp_match );

  ret = ( ( wildcards_x & WILDCARD_IN_DATAPATH_ID || match_x->in_datapath_id == match_y->in_datapath_id ) &&
          ( wildcards_x & WILDCARD_SLICE_NUMBER || match_x->slice_number == match_y->slice_number ) && ret );

  return ret;
}


static filter_entry*
lookup_filter_strict( filter_match match, uint16_t priority ) {
  filter_entry *entry;

  char match_str[ 1024 ];
  match_to_string( &match.ofp_match, match_str, sizeof( match_str ) );

  debug( "Looking up filter entry ( wildcards = %#x, in_datapath_id = %#llx, slice_number = %#x, ofp_match = [%s] ).",
         match.wildcards, match.in_datapath_id, match.slice_number, match_str );

  if ( match.wildcards == 0 && match.ofp_match.wildcards == 0 ) {
    entry = lookup_hash_entry( filter_db.hash, &match );
    if ( entry != NULL && entry->priority == priority ) {
      debug( "A filter entry found (hash)." );

      return entry;
    }
  }

  list_element *element = filter_db.list;
  while ( element != NULL ) {
    entry = element->data;
    element = element->next;
    if ( compare_filter_entry_strict( &match, &entry->match ) && ( entry->priority == priority ) ) {
      debug( "A filter entry found (list)." );

      return entry;
    }
  }

  debug( "Filter entry not found." );

  return NULL;
}


static void
add_filter_entry( filter_match match, uint16_t priority, uint8_t action ) {
  filter_entry *new_entry;

  char match_str[ 1024 ];

  match_to_string( &match.ofp_match, match_str, sizeof( match_str ) );
  info( "Adding a filter entry ( wildcards = %#x, in_datapath_id = %#llx, slice_number = %#x, ofp_match = [%s], priority = %u, action = %#x ).",
        match.wildcards, match.in_datapath_id, match.slice_number, match_str, priority, action );

  new_entry = lookup_filter_strict( match, priority );
  if ( new_entry != NULL ) {
    warn( "Filter entry is already registered." );
    return;
  }

  new_entry = xmalloc( sizeof( filter_entry ) );

  new_entry->match = match;
  if ( match.wildcards == 0 && match.ofp_match.wildcards == 0 ) {
    new_entry->priority = UINT16_MAX;
  }
  else {
    new_entry->priority = priority;
  }
  new_entry->action = action;

  if ( match.wildcards == 0 && match.ofp_match.wildcards == 0 ) {
    insert_hash_entry( filter_db.hash, &new_entry->match, new_entry );
    return;
  }

  filter_entry *entry;
  list_element *element = filter_db.list;
  while ( element != NULL ) {
    entry = element->data;
    if ( entry->priority <= new_entry->priority ) {
      break;
    }
    element = element->next;
  }

  if ( element == NULL ) {
    append_to_tail( &filter_db.list, new_entry );
    return;
  }
  if ( element == filter_db.list ) {
    insert_in_front( &filter_db.list, new_entry );
    return;
  }

  insert_before( &filter_db.list, element->data, new_entry );
}


static uint64_t
string_to_uint64( const char *str ) {
  uint64_t u64 = 0;

  if ( !string_to_datapath_id( str, &u64 ) ) {
    error( "Failed to translate a string (%s) to uint64_t.", str );
  }

  return u64;
}


static int
add_filter_entry_from_sqlite(void *NotUsed, int argc, char **argv, char **column) {
  UNUSED( NotUsed );
  UNUSED( argc );
  UNUSED( column );

  char *endp = NULL;
  uint16_t priority = ( uint16_t ) atoi( argv[ 0 ] );

  filter_match match;

  memset( &match, 0, sizeof( filter_match ) );

  match.ofp_match.wildcards = ( uint32_t ) strtoul( argv[ 1 ], &endp, 0 );
  match.ofp_match.in_port = ( uint16_t ) atoi( argv[ 2 ] );

  uint64_t mac_u64 = string_to_uint64( argv[ 3 ] );
  uint8_t mac[ ETH_ADDRLEN ];
  mac[ 0 ] = ( uint8_t ) ( ( mac_u64 >> 40 ) & 0xff );
  mac[ 1 ] = ( uint8_t ) ( ( mac_u64 >> 32 ) & 0xff );
  mac[ 2 ] = ( uint8_t ) ( ( mac_u64 >> 24 ) & 0xff );
  mac[ 3 ] = ( uint8_t ) ( ( mac_u64 >> 16 ) & 0xff );
  mac[ 4 ] = ( uint8_t ) ( ( mac_u64 >> 8 ) & 0xff );
  mac[ 5 ] = ( uint8_t ) ( mac_u64  & 0xff );
  memcpy( match.ofp_match.dl_src, mac, ETH_ADDRLEN );

  mac_u64 = string_to_uint64( argv[ 4 ] );
  mac[ 0 ] = ( uint8_t ) ( ( mac_u64 >> 40 ) & 0xff );
  mac[ 1 ] = ( uint8_t ) ( ( mac_u64 >> 32 ) & 0xff );
  mac[ 2 ] = ( uint8_t ) ( ( mac_u64 >> 24 ) & 0xff );
  mac[ 3 ] = ( uint8_t ) ( ( mac_u64 >> 16 ) & 0xff );
  mac[ 4 ] = ( uint8_t ) ( ( mac_u64 >> 8 ) & 0xff );
  mac[ 5 ] = ( uint8_t ) ( mac_u64  & 0xff );
  memcpy( match.ofp_match.dl_dst, mac, ETH_ADDRLEN );

  match.ofp_match.dl_vlan = ( uint16_t ) atoi( argv[ 5 ] );
  match.ofp_match.dl_vlan_pcp = ( uint8_t ) atoi( argv[ 6 ] );
  match.ofp_match.dl_type = ( uint16_t ) atoi( argv[ 7 ] );
  match.ofp_match.nw_tos = ( uint8_t ) atoi( argv[ 8 ] );
  match.ofp_match.nw_proto = ( uint8_t ) atoi( argv[ 9 ] );
  match.ofp_match.nw_src = ( uint32_t ) strtoul( argv[ 10 ], &endp, 0 );
  match.ofp_match.nw_dst = ( uint32_t ) strtoul( argv[ 11 ], &endp, 0 );
  match.ofp_match.tp_src = ( uint16_t ) atoi( argv[ 12 ] );
  match.ofp_match.tp_dst = ( uint16_t ) atoi( argv[ 13 ] );

  match.wildcards = ( uint32_t ) strtoul( argv[ 14 ], &endp, 0 );
  match.in_datapath_id = string_to_uint64( argv[ 15 ] );
  match.slice_number = ( uint16_t ) atoi( argv [ 16 ] );

  uint8_t action = ( uint8_t ) atoi( argv[ 17 ] );

  add_filter_entry( match, priority, action );

  return 0;
}


static void
load_filter_entries_from_sqlite( void *user_data ) {
  UNUSED( user_data );

  int ret;
  struct stat st;
  sqlite3 *db;

  memset( &st, 0, sizeof( struct stat ) );

  ret = stat( filter_db_file, &st );
  if ( ret < 0 ) {
    error( "Failed to stat %s (%s).", filter_db_file, strerror( errno ) );
    return;
  }
  if ( st.st_mtime == last_filter_db_mtime ) {
    debug( "Filter database is not changed." );
    return;
  }

  info( "Loading filter definitions." );

  last_filter_db_mtime = st.st_mtime;

  delete_filter_db();
  create_filter_db();

  ret = sqlite3_open( filter_db_file, &db );
  if ( ret ) {
    error( "Failed to load filter database (%s).", sqlite3_errmsg( db ) );
    sqlite3_close( db );
    return;
  }

  ret = sqlite3_exec( db, "select * from filter order by priority",
                      add_filter_entry_from_sqlite, 0, NULL );
  if ( ret != SQLITE_OK ) {
    error( "Failed to execute a SQL statement (%s).", sqlite3_errmsg( db ) );
    sqlite3_close( db );
    return;
  }

  sqlite3_close( db );

  return;
}


bool
init_filter( const char *file ) {
  if ( file == NULL || strlen( file ) == 0 ) {
    error( "Filter database must be specified." );
    return false;
  }
  if ( filter_db.hash != NULL || filter_db.list != NULL ) {
    error( "Filter database is already created." );
    return false;
  }

  strncpy( filter_db_file, file, sizeof( filter_db_file ) );

  create_filter_db();

  load_filter_entries_from_sqlite( NULL );

  add_periodic_event_callback( FILTER_DB_UPDATE_INTERVAL,
                               load_filter_entries_from_sqlite,
                               NULL );

  return true;
}


bool
finalize_filter() {
  delete_filter_db();
  memset( filter_db_file, '\0', sizeof( filter_db_file ) );

  return true;
}


int
filter( uint64_t in_datapath_id, uint16_t in_port, uint16_t slice_number, const buffer *frame ) {
  filter_match match;

  memset( &match, 0, sizeof( filter_match ) );

  set_match_from_packet( &match.ofp_match, in_port, 0, frame );

  match.wildcards = 0;
  match.in_datapath_id = in_datapath_id;
  match.slice_number = slice_number;

  filter_entry *entry = lookup_filter( match );

  if ( entry == NULL ) {
    return DENY;
  }

  return entry->action;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
