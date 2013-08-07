/*
 * Copyright (C) 2008-2013 NEC Corporation
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
#include "fdb.h"


typedef struct {
  uint8_t mac[ ETH_ADDRLEN ];
  uint32_t port_number;
  time_t updated_at;
} forwarding_entry;


bool
mac_to_string( const uint8_t *mac, char *str, size_t size ) {
  assert( size > 1 );
  int ret = snprintf( str, size, "%02x:%02x:%02x:%02x:%02x:%02x",
                      mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );
  if ( ret < 0 || ( size_t ) ret >= size ) {
    str[ 0 ] = '\0';
    return false;
  }
  return true;
}


static forwarding_entry *
allocate_forwarding_entry( const uint8_t *mac, uint32_t port_number ) {
  forwarding_entry *entry = xmalloc( sizeof( forwarding_entry ) );
  memcpy( entry->mac, mac, ETH_ADDRLEN );
  entry->port_number = port_number;
  entry->updated_at = time( NULL );

  return entry;
}


static void
update_forwarding_entry( forwarding_entry *entry, uint32_t port_number ) {
  entry->port_number = port_number;
  entry->updated_at = time( NULL );
}


static void
free_forwarding_entry( forwarding_entry *entry ) {
  if ( entry == NULL ) {
    return;
  }
  xfree( entry );
}


static bool
aged_forwarding_entry( forwarding_entry *entry, time_t time ) {
  return ( entry->updated_at < time );
}


hash_table *
create_fdb() {
  return create_hash( compare_mac, hash_mac );
}


uint32_t
lookup_fdb( hash_table *db, const uint8_t *mac, time_t *updated_at ) {
  uint32_t port_number = ENTRY_NOT_FOUND_IN_FDB;
  forwarding_entry *entry = lookup_hash_entry( db, mac );
  if ( entry != NULL && !aged_forwarding_entry( entry, time( NULL ) - MAX_AGE ) ) {
    port_number = entry->port_number;
    if ( updated_at != NULL ) {
      *updated_at = entry->updated_at;
    }
  }
  char mac_str[ MAC_STRING_LENGTH ];
  mac_to_string( mac, mac_str, sizeof( mac_str ) );
  debug( "lookup fdb: %s -> %u ( entry = %p )", mac_str, port_number, entry );
  return port_number;
}


static uint8_t mac_zero[ OFP_ETH_ALEN ] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };


static bool
valid_mac( const uint8_t *mac ) {
  if ( ( mac[ 0 ] & 1 ) == 1 ) { // check I/G bit
    return false;
  }
  if ( memcmp( mac, mac_zero, OFP_ETH_ALEN ) == 0 ) {
    return false;
  }
  return true;
}


void
update_fdb( hash_table *db, const uint8_t *mac, uint32_t port_number ) {
  char mac_str[ MAC_STRING_LENGTH ];
  mac_to_string( mac, mac_str, sizeof( mac_str ) );
  if ( !valid_mac( mac ) ) {
    warn( "learn fdb: invalid mac address ( mac address = %s, port_number = %u )", mac_str, port_number );
    return;
  }
  forwarding_entry *entry = lookup_hash_entry( db, mac );
  if ( entry == NULL ) {
    entry = allocate_forwarding_entry( mac, port_number );
    debug( "learn fdb: %s -> %u ( entry = %p )", mac_str, port_number, entry );
    insert_hash_entry( db, entry->mac, entry );
  }
  else if ( entry->port_number != port_number ) {
    uint32_t old_port_number = entry->port_number;
    debug( "learn fdb: %s -> %u ( old port number = %u, entry = %p )", mac_str, port_number, old_port_number, entry );
    update_forwarding_entry( entry, port_number );
  }
}


typedef struct {
  hash_table *db;

  uint32_t port_number;
  time_t time;
} fdb_select;


static void
delete_forwarding_entry( void *key, void *value, void *user_data ) {
  forwarding_entry *entry = value;
  fdb_select *fdb_select = user_data;

  if ( fdb_select->port_number == OFPP_ANY ||
       entry->port_number == fdb_select->port_number ||
       aged_forwarding_entry( entry, fdb_select->time ) ) {
    char mac_str[ MAC_STRING_LENGTH ];
    mac_to_string( entry->mac, mac_str, sizeof( mac_str ) );
    debug( "delete fdb: %s ( port number = %u, entry = %p )", mac_str, entry->port_number, entry );
    free_forwarding_entry( delete_hash_entry( fdb_select->db, key ) );
  }
}


void
delete_forwarding_entries_by_port_number( hash_table *db, uint32_t port_number ) {
  fdb_select fdb_select;
  memset( &fdb_select, 0, sizeof( fdb_select ) );
  fdb_select.db = db;
  fdb_select.port_number = port_number;
  foreach_hash( db, delete_forwarding_entry, &fdb_select );
}


void
delete_aged_forwarding_entries( hash_table *db ) {
  fdb_select fdb_select;
  memset( &fdb_select, 0, sizeof( fdb_select ) );
  fdb_select.db = db;
  fdb_select.time = time( NULL ) - MAX_AGE;
  foreach_hash( db, delete_forwarding_entry, &fdb_select );
}


void
delete_fdb( hash_table *db ) {
  hash_iterator iter;
  init_hash_iterator( db, &iter );
  hash_entry *hash;
  while ( ( hash = iterate_hash_next( &iter ) ) != NULL ) {
    free_forwarding_entry( hash->value );
  }
  delete_hash( db );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
