/*
 * Author: Yasunobu Chiba, Lei SUN
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
#include <inttypes.h>
#include <linux/limits.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "slice.h"
#include "port.h"
#include "filter.h"


#define SLICE_DB_UPDATE_INTERVAL 30

#define BINDING_AGING_INTERVAL 60
#define BINDING_TIMEOUT 3600
#define BINDING_ID_LENGTH 64

#define BINDING_TYPE_PORT 0x01
#define BINDING_TYPE_MAC 0x02
#define BINDING_TYPE_PORT_MAC 0x04

typedef struct {
  uint8_t type;
  uint64_t datapath_id;
  uint16_t port;
  uint16_t vid;
  uint8_t mac[ OFP_ETH_ALEN ];
  char id[ BINDING_ID_LENGTH ];
  uint16_t slice_number;
  bool dynamic;
  time_t updated_at;
} binding_entry;

#define SLICE_NAME_LENGTH 64

typedef struct {
  uint16_t number;
  char id[ SLICE_NAME_LENGTH ];
  uint16_t n_mac_slice_maps;
} slice_entry;

typedef struct {
  hash_table *slices;
  hash_table *port_slice_map;
  hash_table *mac_slice_map;
  hash_table *port_mac_slice_map;
  hash_table *port_slice_vid_map;
} slice_table;

static bool loose_mac_based_slicing = false;
static bool restrict_hosts_on_port = false;

static char slice_db_file[ PATH_MAX ];
static slice_table slice_db;
static time_t last_slice_db_mtime = 0;

static sliceable_switch *switch_instance = NULL;


static bool
compare_slice_entry( const void *x, const void *y ) {
  return ( memcmp( x, y, sizeof( uint16_t ) ) == 0 ) ? true : false;
}


static unsigned int
hash_slice_entry( const void *key ) {
  const uint16_t *hash = key;

  return ( unsigned int ) *hash;
}


static bool
compare_port_slice_entry( const void *x, const void *y ) {
  return ( memcmp( x, y, offsetof( binding_entry, mac ) ) == 0 ) ? true : false;
}


static unsigned int
hash_port_slice_entry( const void *key ) {
  unsigned int hash = 0;
  const binding_entry *entry = key;

  hash = ( unsigned int ) entry->datapath_id;
  hash += ( unsigned int ) entry->port;
  hash += ( unsigned int ) entry->vid;

  return hash;
}


static bool
compare_mac_slice_entry( const void *x, const void *y ) {
  const binding_entry *entry_x = x;
  const binding_entry *entry_y = y;

  bool ret = false;
  if ( entry_x->type == entry_y->type && memcmp( entry_x->mac, entry_y->mac, OFP_ETH_ALEN ) == 0 ) {
    ret = true;
  }

  return ret;
}


static unsigned int
hash_mac_slice_entry( const void *key ) {
  const binding_entry *entry = key;

  return hash_mac( entry->mac );
}


static bool
compare_port_mac_slice_entry( const void *x, const void *y ) {
  return ( memcmp( x, y, offsetof( binding_entry, id ) ) == 0 ) ? true : false;
}


static bool
compare_port_slice_vid_entry( const void *x, const void *y ) {
  const binding_entry *entry_x = x;
  const binding_entry *entry_y = y;

  if ( entry_x->datapath_id == entry_y->datapath_id &&
       entry_x->port == entry_y->port &&
       entry_x->slice_number == entry_y->slice_number ) {
    return true;
  }

  return false;
}


static unsigned int
hash_port_slice_vid_entry( const void *key ) {
  unsigned int hash = 0;
  const binding_entry *entry = key;

  hash = ( unsigned int ) entry->datapath_id;
  hash += ( unsigned int ) entry->port;
  hash += ( unsigned int ) entry->slice_number;

  return hash;
}


static bool
create_slice_db() {
  if ( slice_db.slices != NULL || slice_db.port_slice_map != NULL ||
       slice_db.mac_slice_map != NULL || slice_db.port_mac_slice_map != NULL ||
       slice_db.port_slice_vid_map != NULL ) {
    return false;
  }

  slice_db.slices = create_hash( compare_slice_entry, hash_slice_entry );
  slice_db.port_slice_map = create_hash( compare_port_slice_entry, hash_port_slice_entry );
  slice_db.mac_slice_map = create_hash( compare_mac_slice_entry, hash_mac_slice_entry );
  slice_db.port_mac_slice_map = create_hash( compare_port_mac_slice_entry, hash_mac_slice_entry );
  slice_db.port_slice_vid_map = create_hash( compare_port_slice_vid_entry, hash_port_slice_vid_entry );

  return true;
}


static bool
delete_slice_db() {
  if ( slice_db.slices == NULL || slice_db.port_slice_map == NULL ||
       slice_db.mac_slice_map == NULL || slice_db.port_mac_slice_map == NULL ||
       slice_db.port_slice_vid_map == NULL ) {
    return false;
  }

  hash_iterator iter;
  hash_entry *entry;

  init_hash_iterator( slice_db.slices, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    xfree( entry->value );
  }
  delete_hash( slice_db.slices );
  slice_db.slices = NULL;

  init_hash_iterator( slice_db.port_slice_map, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    if ( entry->value != NULL ) {
      xfree( entry->value );
      entry->value = NULL;
    }
  }
  delete_hash( slice_db.port_slice_map );
  slice_db.port_slice_map = NULL;

  delete_hash( slice_db.port_slice_vid_map );
  slice_db.port_slice_vid_map = NULL;

  init_hash_iterator( slice_db.mac_slice_map, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    if ( entry->value != NULL ) {
      xfree( entry->value );
      entry->value = NULL;
    }
  }
  delete_hash( slice_db.mac_slice_map );
  slice_db.mac_slice_map = NULL;

  init_hash_iterator( slice_db.port_mac_slice_map, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    if ( entry->value != NULL ) {
      xfree( entry->value );
      entry->value = NULL;
    }
  }
  delete_hash( slice_db.port_mac_slice_map );
  slice_db.port_mac_slice_map = NULL;

  return true;
}


static void
add_slice_entry( uint16_t number, const char *id ) {
  slice_entry *entry;

  entry = xmalloc( sizeof( slice_entry ) );

  entry->number = number;
  memset( entry->id, '\0', SLICE_NAME_LENGTH );
  memcpy( entry->id, id, SLICE_NAME_LENGTH - 1 );
  entry->n_mac_slice_maps = 0;

  if ( lookup_hash_entry( slice_db.slices, entry ) != NULL ) {
    xfree( entry );
    warn( "Slice entry is already registered ( number = %#x, id = %s ).", number, id );
    return;
  }

  insert_hash_entry( slice_db.slices, entry, entry );
}


static void
add_port_slice_binding( uint64_t datapath_id, uint16_t port, uint16_t vid, uint16_t slice_number, const char *id, bool dynamic ) {
  binding_entry *entry;

  entry = xmalloc( sizeof( binding_entry ) );

  memset( entry, 0, sizeof( binding_entry ) );
  entry->type = BINDING_TYPE_PORT;
  entry->datapath_id = datapath_id;
  entry->port = port;
  entry->vid = vid;
  entry->slice_number = slice_number;
  memset( entry->id, '\0', sizeof( entry->id ) );
  if ( id != NULL ) {
    memcpy( entry->id, id, sizeof( entry->id ) - 1 );
  }
  entry->dynamic = dynamic;
  entry->updated_at = time( NULL );

  info( "Adding a port-slice binding ( type = %#x, datapath_id = %#" PRIx64
        ", port = %#x, vid = %#x, slice_number = %#x, id = %s, dynamic = %d, updated_at = %u ).",
        entry->type, datapath_id, port, vid, slice_number, id, dynamic, entry->updated_at );

  if ( lookup_hash_entry( slice_db.port_slice_map, entry ) != NULL ) {
    xfree( entry );
    warn( "Port-slice entry is already registered ( datapath_id = %#" PRIx64
          ", port = %u, vid = %u, slice_number = %#x, dynamic = %d ).",
          datapath_id, port, vid, slice_number, dynamic );
    return;
  }

  insert_hash_entry( slice_db.port_slice_map, entry, entry );
  insert_hash_entry( slice_db.port_slice_vid_map, entry, entry );
}


static void
add_mac_slice_binding( const uint8_t *mac, uint16_t slice_number, const char *id ) {
  slice_entry *slice = lookup_hash_entry( slice_db.slices, &slice_number );
  if ( slice == NULL ) {
    error( "Invalid slice number ( #%x ).", slice_number );
    return;
  }

  binding_entry *entry = xmalloc( sizeof( binding_entry ) );

  memset( entry, 0, sizeof( binding_entry ) );
  entry->type = BINDING_TYPE_MAC;
  memcpy( entry->mac, mac, OFP_ETH_ALEN );
  entry->slice_number = slice_number;
  memset( entry->id, '\0', sizeof( entry->id ) );
  if ( id != NULL ) {
    memcpy( entry->id, id, sizeof( entry->id ) - 1 );
  }
  entry->dynamic = false;
  entry->updated_at = time( NULL );

  info( "Adding a mac-slice binding ( type = %#x, %02x:%02x:%02x:%02x:%02x:%02x, slice_number = %#x, id = %s, "
        "dynamic = %d, updated_at = %u ).",
        entry->type, mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ], slice_number, id,
        entry->dynamic, entry->updated_at );

  if ( lookup_hash_entry( slice_db.mac_slice_map, entry ) != NULL ) {
    xfree( entry );
    warn( "Mac-slice entry is already registered ( mac = %02x:%02x:%02x:%02x:%02x:%02x, slice_number = %#x, dynamic = %d ).",
          mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ], slice_number, entry->dynamic );
    return;
  }

  insert_hash_entry( slice_db.mac_slice_map, entry, entry );
  slice->n_mac_slice_maps++;
}


static void
add_port_mac_slice_binding( uint64_t datapath_id, uint16_t port, uint16_t vid, uint8_t *mac, uint16_t slice_number, const char *id ) {
  binding_entry *entry;

  entry = xmalloc( sizeof( binding_entry ) );

  memset( entry, 0, sizeof( binding_entry ) );
  entry->type = BINDING_TYPE_PORT_MAC;
  entry->datapath_id = datapath_id;
  entry->port = port;
  entry->vid = vid;
  memcpy( entry->mac, mac, OFP_ETH_ALEN );
  entry->slice_number = slice_number;
  memset( entry->id, '\0', sizeof( entry->id ) );
  if ( id != NULL ) {
    memcpy( entry->id, id, sizeof( entry->id ) - 1 );
  }
  entry->dynamic = false;
  entry->updated_at = time( NULL );

  info( "Adding a port_mac-slice binding ( type = %#x, datapath_id = %#" PRIx64 ",port = %#x, vid = %#x, "
        "mac = %02x:%02x:%02x:%02x:%02x:%02x:, slice_number = %#x, id = %s, dynamic = %d, updated_at = %u ).",
        entry->type, datapath_id, port, vid, mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ],
        slice_number, id, entry->dynamic, entry->updated_at );

  if ( lookup_hash_entry( slice_db.port_mac_slice_map, entry ) != NULL ) {
    xfree( entry );
    warn( "Port_mac-slice entry is already registered ( type = %#x, datapath_id = %#" PRIx64 ",port = %#x, vid = %#x, "
          "mac = %02x:%02x:%02x:%02x:%02x:%02x:, slice_number = %#x, id = %s, dynamic = %d ).",
          entry->type, datapath_id, port, vid, mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ],
          slice_number, id, entry->dynamic );
    return;
  }

  insert_hash_entry( slice_db.port_mac_slice_map, entry, entry );
}


static int
add_slice_entry_from_sqlite( void *NotUsed, int argc, char **argv, char **column ) {
  UNUSED( NotUsed );
  UNUSED( argc );
  UNUSED( column );

  uint16_t number = ( uint16_t ) atoi( argv[ 0 ] );

  add_slice_entry( number, argv[ 1 ] );

  return 0;
}


static int
add_binding_entry_from_sqlite( void *NotUsed, int argc, char **argv, char **column ) {
  UNUSED( NotUsed );
  UNUSED( argc );
  UNUSED( column );

  char *endp;

  uint8_t type = ( uint8_t ) atoi( argv[ 0 ] );

  switch ( type ) {
  case BINDING_TYPE_PORT:
  {
    uint64_t datapath_id = ( uint64_t ) strtoull( argv[ 1 ], &endp, 0 );
    uint16_t port = ( uint16_t ) atoi( argv[ 2 ] );
    uint16_t vid = ( uint16_t ) atoi( argv[ 3 ] );
    uint16_t slice_number = ( uint16_t ) atoi( argv[ 6 ] );
    char *id = argv[ 5 ];

    add_port_slice_binding( datapath_id, port, vid, slice_number, id, false );
  }
  break;

  case BINDING_TYPE_MAC:
  {
    uint8_t mac[ OFP_ETH_ALEN ];

    uint64_t mac_u64 = ( uint64_t ) strtoull( argv[ 4 ], &endp, 0 );

    mac[ 0 ] = ( uint8_t ) ( ( mac_u64 >> 40 ) & 0xff );
    mac[ 1 ] = ( uint8_t ) ( ( mac_u64 >> 32 ) & 0xff );
    mac[ 2 ] = ( uint8_t ) ( ( mac_u64 >> 24 ) & 0xff );
    mac[ 3 ] = ( uint8_t ) ( ( mac_u64 >> 16 ) & 0xff );
    mac[ 4 ] = ( uint8_t ) ( ( mac_u64 >> 8 ) & 0xff );
    mac[ 5 ] = ( uint8_t ) ( mac_u64 & 0xff );

    uint16_t slice_number = ( uint16_t ) atoi( argv[ 6 ] );
    char *id = argv[ 5 ];

    add_mac_slice_binding( mac, slice_number, id );
  }
  break;

  case BINDING_TYPE_PORT_MAC:
  {
    uint64_t datapath_id = ( uint64_t ) strtoull( argv[ 1 ], &endp, 0 );
    uint16_t port = ( uint16_t ) atoi( argv[ 2 ] );
    uint16_t vid = ( uint16_t ) atoi( argv[ 3 ] );
    uint16_t slice_number = ( uint16_t ) atoi( argv[ 6 ] );
    char *id = argv[ 5 ];

    uint8_t mac[ OFP_ETH_ALEN ];
    uint64_t mac_u64 = ( uint64_t ) strtoull( argv[ 4 ], &endp, 0 );

    mac[ 0 ] = ( uint8_t ) ( ( mac_u64 >> 40 ) & 0xff );
    mac[ 1 ] = ( uint8_t ) ( ( mac_u64 >> 32 ) & 0xff );
    mac[ 2 ] = ( uint8_t ) ( ( mac_u64 >> 24 ) & 0xff );
    mac[ 3 ] = ( uint8_t ) ( ( mac_u64 >> 16 ) & 0xff );
    mac[ 4 ] = ( uint8_t ) ( ( mac_u64 >> 8 ) & 0xff );
    mac[ 5 ] = ( uint8_t ) ( mac_u64 & 0xff );

    add_port_mac_slice_binding( datapath_id, port, vid, mac, slice_number, id );
  }
  break;

  default:
    error( "Undefined binding type ( type = %u ).", type );
    return -1;
  }

  return 0;
}


static void
age_dynamic_port_slice_bindings() {
  if ( slice_db.port_slice_map == NULL ) {
    return;
  }

  binding_entry *binding;
  hash_iterator iter;
  hash_entry *entry;

  init_hash_iterator( slice_db.port_slice_map, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    if ( entry->value != NULL ){
      binding = entry->value;
      if ( ( binding->dynamic == true ) &&
           ( ( binding->updated_at + BINDING_TIMEOUT ) < time( NULL ) ) ){
        info( "Deleting a port-slice binding ( type = %#x, datapath_id = %#" PRIx64
              ", port = %#x, vid = %#x, slice_number = %#x, id = %s, dynamic = %d, updated_at = %u ).",
              binding->type, binding->datapath_id, binding->port, binding->vid, binding->slice_number, binding->id,
              binding->dynamic, binding->updated_at );
        delete_hash_entry( slice_db.port_slice_map, entry->value );
        delete_hash_entry( slice_db.port_slice_vid_map, entry->value );
        xfree( entry->value );
      }
    }
  }
}


void
delete_dynamic_port_slice_bindings( uint64_t datapath_id, uint16_t port ) {
  if ( slice_db.port_slice_map == NULL ) {
    return;
  }

  binding_entry *binding;
  hash_iterator iter;
  hash_entry *entry;

  init_hash_iterator( slice_db.port_slice_map, &iter );
  while ( ( entry = iterate_hash_next( &iter ) ) != NULL ) {
    if ( entry->value != NULL ){
      binding = entry->value;
      if ( binding->dynamic == true &&
           binding->datapath_id == datapath_id && binding->port == port ) {
        info( "Deleting a port-slice binding ( type = %#x, datapath_id = %#" PRIx64
              ", port = %#x, vid = %#x, slice_number = %#x, id = %s, dynamic = %d, updated_at = %u ).",
              binding->type, binding->datapath_id, binding->port, binding->vid, binding->slice_number, binding->id,
              binding->dynamic, binding->updated_at );
        delete_hash_entry( slice_db.port_slice_map, entry->value );
        delete_hash_entry( slice_db.port_slice_vid_map, entry->value );
        xfree( entry->value );
      }
    }
  }
}


static void
delete_flows( switch_info *sw, void *user_data ) {
  struct ofp_match match;  
  memset( &match, 0, sizeof( struct ofp_match ) );
  match.wildcards = OFPFW_ALL;

  buffer *flow_mod = create_flow_mod( get_transaction_id(), match, get_cookie(),
                                      OFPFC_DELETE, 0, 0, 0, 0, OFPP_NONE, 0, NULL );

  send_openflow_message( sw->dpid, flow_mod );
  free_buffer( flow_mod );
}


static void
delete_all_flows(){
  if ( switch_instance == NULL ) {
    return;
  }

  foreach_switch( switch_instance->switches, delete_flows, NULL );
}


static void
load_slice_definitions_from_sqlite( void *user_data ) {
  UNUSED( user_data );

  int ret;
  struct stat st;
  sqlite3 *db;

  memset( &st, 0, sizeof( struct stat ) );

  ret = stat( slice_db_file, &st );
  if ( ret < 0 ) {
    error( "Failed to stat %s (%s).", slice_db_file, strerror( errno ) );
    return;
  }

  if ( st.st_mtime == last_slice_db_mtime ) {
    debug( "Slice database is not changed." );
    return;
  }

  info( "Loading slice definitions." );

  last_slice_db_mtime = st.st_mtime;

  delete_slice_db();

  // FIXME: delete affected entries only
  delete_all_flows();

  create_slice_db();

  ret = sqlite3_open( slice_db_file, &db );
  if ( ret ) {
    error( "Failed to load slice (%s).", sqlite3_errmsg( db ) );
    sqlite3_close( db );
    return;
  }

  ret = sqlite3_exec( db, "select * from slices",
                      add_slice_entry_from_sqlite, 0, NULL );
  if ( ret != SQLITE_OK ) {
    error( "Failed to execute a SQL statement (%s).", sqlite3_errmsg( db ) );
    sqlite3_close( db );
    return;
  }

  ret = sqlite3_exec( db, "select * from bindings",
                      add_binding_entry_from_sqlite, 0, NULL );
  if ( ret != SQLITE_OK ) {
    error( "Failed to execute a SQL statement (%s).", sqlite3_errmsg( db ) );
    sqlite3_close( db );
    return;
  }

  sqlite3_close( db );

  return;
}


bool
init_slice( const char *file, uint16_t mode , sliceable_switch *instance ) {
  if ( switch_instance != NULL ) {
    error( "slice is already initialized." );
    return false;
  }

  if ( file == NULL || strlen( file ) == 0 ){
    error( "slice database must be specified." );
    return false;
  }

  if ( instance == NULL ){
    error( "switch instance must be specified." );
    return false;
  }

  switch_instance = instance;

  memset( slice_db_file, '\0', sizeof( slice_db_file ) );
  strncpy( slice_db_file, file, sizeof( slice_db_file) );

  create_slice_db();

  load_slice_definitions_from_sqlite( NULL );

  add_periodic_event_callback( SLICE_DB_UPDATE_INTERVAL,
                               load_slice_definitions_from_sqlite,
                               NULL );

  add_periodic_event_callback( BINDING_AGING_INTERVAL,
                               age_dynamic_port_slice_bindings,
                               NULL );

  if ( mode & LOOSE_MAC_BASED_SLICING ) {
    loose_mac_based_slicing = true;
  }
  if ( mode & RESTRICT_HOSTS_ON_PORT ) {
    restrict_hosts_on_port = true;
  }

  return true;
}


bool
finalize_slice() {
  delete_slice_db();
  memset( slice_db_file, '\0', sizeof( slice_db_file ) );
  switch_instance = NULL;

  return true;
}


bool
get_port_vid( uint16_t slice_number, uint64_t datapath_id, uint16_t port, uint16_t *vid ) {
  binding_entry entry;
  memset( &entry, 0, sizeof( binding_entry ) );
  entry.datapath_id = datapath_id;
  entry.port = port;
  entry.slice_number = slice_number;

  binding_entry *found = lookup_hash_entry( slice_db.port_slice_vid_map, &entry );
  if ( found == NULL ) {
    return false;
  }

  *vid = found->vid;

  return true;
}


uint16_t
lookup_slice_by_mac( const uint8_t *mac ) {
  if ( mac == NULL ) {
    error( "MAC address is not specified." );
    return SLICE_NOT_FOUND;
  }

  binding_entry entry;
  memset( &entry, 0, sizeof( binding_entry ) );
  entry.type = BINDING_TYPE_MAC;
  memcpy( entry.mac, mac, OFP_ETH_ALEN );
  binding_entry *found = lookup_hash_entry( slice_db.mac_slice_map, &entry );
  if ( found != NULL ) {
    debug( "Slice found in mac-slice map ( slice_number = %#x )", found->slice_number );
    return found->slice_number;
  }

  debug( "No slice found." );
  return SLICE_NOT_FOUND;
}


bool
loose_mac_based_slicing_enabled() {
  return loose_mac_based_slicing;
}


static bool
restrict_hosts_on_port_enabled() {
  return restrict_hosts_on_port;
}


bool
mac_slice_maps_exist( uint16_t slice_number ) {
  slice_entry *found = lookup_hash_entry( slice_db.slices, &slice_number );
  if ( found == NULL ) {
    return false;
  }

  if ( found->n_mac_slice_maps == 0 ) {
    return false;
  }

  return true;
}


uint16_t
lookup_slice( uint64_t datapath_id, uint16_t port, uint16_t vid, const uint8_t *mac ) {
  void *found;
  uint16_t slice_number;
  binding_entry entry;

  memset( &entry, 0, sizeof( binding_entry ) );
  entry.datapath_id = datapath_id;
  entry.port = port;
  entry.vid = vid;

  if ( mac != NULL ) {
    memcpy( entry.mac, mac, OFP_ETH_ALEN );
    entry.type = BINDING_TYPE_MAC;
    found = lookup_hash_entry( slice_db.mac_slice_map, &entry );
    if ( found != NULL ) {
      slice_number = ( ( binding_entry * ) found )->slice_number;
      debug( "Slice found in mac-slice map ( slice_number = %#x ).", slice_number );
      if ( !loose_mac_based_slicing_enabled() ) {
        entry.type = BINDING_TYPE_PORT;
        found = lookup_hash_entry( slice_db.port_slice_map, &entry );
        if ( found != NULL ) {
          uint16_t port_slice_number = ( ( binding_entry * ) found )->slice_number;
          if ( slice_number == port_slice_number ) {
            ( ( binding_entry * ) found )->updated_at = time( NULL );
          }
        }
        else{
          char id[ BINDING_ID_LENGTH ];
          sprintf( id, "%012" PRIx64 ":%04x:%04x", datapath_id, port, vid );
          add_port_slice_binding( datapath_id, port, vid, slice_number, id, true );
        }
      }
      goto found;
    }

    if ( restrict_hosts_on_port_enabled() ) {
      entry.type = BINDING_TYPE_PORT_MAC;
      found = lookup_hash_entry( slice_db.port_mac_slice_map, &entry );
      if( found != NULL ) {
        slice_number = ( ( binding_entry * ) found )->slice_number;
        debug( "Slice found in port_mac-slice map ( slice_number = %#x ).", slice_number );
        goto found;
      }
    }
  }

  if ( !restrict_hosts_on_port_enabled() ) {
    entry.type = BINDING_TYPE_PORT;
    found = lookup_hash_entry( slice_db.port_slice_map, &entry );
    if( found != NULL ) {
      slice_number = ( ( binding_entry * ) found )->slice_number;
      debug( "Slice found in port-slice map ( slice_number = %#x ).", slice_number );
      goto found;
    }
  }

not_found:
  debug( "No slice found." );
  return SLICE_NOT_FOUND;

found:
  found = lookup_hash_entry( slice_db.slices, &slice_number );
  if ( found == NULL ) {
    goto not_found;
  }

  return slice_number;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
