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

#include "trema.h"
#include "datapath_db.h"


typedef struct {
  uint64_t id;
  hash_table *fdb;
  void ( *delete_fdb )( hash_table *fdb );
} datapath_entry;


static datapath_entry *
allocate_datapath_entry( uint64_t datapath_id, hash_table *fdb, void delete_fdb( hash_table *fdb ) ) {
  datapath_entry *entry = xmalloc( sizeof( datapath_entry ) );
  entry->id = datapath_id;
  entry->fdb = fdb;
  entry->delete_fdb = delete_fdb;
  return entry;
}


static void
free_datapath_entry( datapath_entry *entry ) {
  ( *entry->delete_fdb )( entry->fdb );
  xfree( entry );
}


hash_table *
create_datapath_db( void ) {
  return create_hash( compare_datapath_id, hash_datapath_id );
}


void
insert_datapath_entry( hash_table *db, uint64_t datapath_id, hash_table *fdb, void delete_fdb( hash_table *fdb ) ) {
    datapath_entry *entry = allocate_datapath_entry( datapath_id, fdb, delete_fdb );
    datapath_entry *old_datapath_entry = insert_hash_entry( db, &entry->id, entry );
    if ( old_datapath_entry != NULL ) {
      warn( "duplicated switch ( datapath_id = %#" PRIx64 ", entry = %p )", datapath_id, old_datapath_entry );
      free_datapath_entry( old_datapath_entry );
    }
    debug( "insert switch: %#" PRIx64 " ( entry = %p )", datapath_id, entry );
}


hash_table *
lookup_datapath_entry( hash_table *db, uint64_t datapath_id ) {
  datapath_entry *entry = lookup_hash_entry( db, &datapath_id );
  debug( "lookup switch: %#" PRIx64 " ( entry = %p )", datapath_id, entry );
  return entry != NULL ? entry->fdb : NULL;
}


void
delete_datapath_entry( hash_table *db, uint64_t datapath_id ) {
  datapath_entry *entry = delete_hash_entry( db, &datapath_id );
  free_datapath_entry( entry );
  debug( "delete switch: %#" PRIx64 " ( entry = %p )", datapath_id, entry );
}


void
delete_datapath_db( hash_table *db ) {
  hash_iterator iter;
  init_hash_iterator( db, &iter );
  hash_entry *hash;
  while ( ( hash = iterate_hash_next( &iter ) ) != NULL ) {
    free_datapath_entry( hash->value );
  }
  delete_hash( db );
}


void
foreach_datapath_db( hash_table *db, void function( hash_table *fdb ) ) {
  hash_iterator iter;
  init_hash_iterator( db, &iter );
  hash_entry *hash;
  while ( ( hash = iterate_hash_next( &iter ) ) != NULL ) {
    datapath_entry *entry = hash->value;
    function( entry->fdb );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
