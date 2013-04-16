/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012-2013 NEC Corporation
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
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include "transaction_manager.h"
#include "trema.h"


#define ADD_TIMESPEC( _a, _b, _return )                       \
  do {                                                        \
    ( _return )->tv_sec = ( _a )->tv_sec + ( _b )->tv_sec;    \
    ( _return )->tv_nsec = ( _a )->tv_nsec + ( _b )->tv_nsec; \
    if ( ( _return )->tv_nsec >= 1000000000 ) {               \
      ( _return )->tv_sec++;                                  \
      ( _return )->tv_nsec -= 1000000000;                     \
    }                                                         \
  }                                                           \
  while ( 0 )

#define TIMESPEC_LESS_THAN( _a, _b )                          \
  ( ( ( _a )->tv_sec == ( _b )->tv_sec ) ?                    \
    ( ( _a )->tv_nsec < ( _b )->tv_nsec ) :                   \
    ( ( _a )->tv_sec < ( _b )->tv_sec ) )


typedef struct {
  hash_table *xid;
  hash_table *barrier_xid;
} transaction_table;

typedef struct {
  uint32_t xid;
  uint32_t barrier_xid;
  uint32_t original_xid;
  uint64_t datapath_id;
  buffer *message;
  succeeded_handler succeeded_callback;
  void *succeeded_user_data;
  failed_handler failed_callback;
  void *failed_user_data;
  struct timespec expires_at;
  bool completed;
  bool error_received;
  bool timeout;
} transaction_entry;


static transaction_table *transaction_db = NULL;

static const struct timespec TRANSACTION_TIMEOUT = { 2, 0 };
static const struct timespec TRANSACTION_AGING_INTERVAL = { 0, 250000000 };
static const struct timespec TRANSACTION_END_MARGIN = { 1, 0 };


static unsigned int
hash_transaction_id( const void *key ) {
  return ( unsigned int ) *( const uint32_t * ) key;
}


static bool
compare_transaction_id( const void *x, const void *y ) {
  const transaction_entry *entry_x = x;
  const transaction_entry *entry_y = y;

  return ( entry_x->xid == entry_y->xid ) ? true : false;
}


static transaction_entry *
alloc_transaction_entry() {
  debug( "Allocating a transaction entry." );

  transaction_entry *entry = xmalloc( sizeof( transaction_entry ) );
  memset( entry, 0, sizeof( transaction_entry ) );

  debug( "A transaction entry is allocated ( %p ).", entry );

  return entry;
}


static void
dump_transaction_entry( void dump_function( const char *format, ... ), const transaction_entry *entry ) {
  assert( entry != NULL );
  assert( dump_function != NULL );

  dump_function( "Transaction entry %p: xid = %#x, barrier_xid = %#x, original_xid = %#x, datapath_id = %#" PRIx64 ", message = %p, "
                 "message data = %p, message length = %u, succeeded_callback = %p, succeeded_user_data = %p, failed_callback =%p, "
                 "failed_user_data = %p, expires_at = %d.%09d, completed = %d, error_received = %d, timeout = %d.",
                 entry, entry->xid, entry->barrier_xid, entry->original_xid, entry->datapath_id, entry->message,
                 entry->message != NULL ? entry->message->data : NULL, entry->message != NULL ? entry->message->length : 0,
                 entry->succeeded_callback, entry->succeeded_user_data, entry->failed_callback, entry->failed_user_data,
                 ( int ) entry->expires_at.tv_sec, ( int ) entry->expires_at.tv_nsec, entry->completed,
                 entry->error_received, entry->timeout );
}


static void
free_transaction_entry( transaction_entry *entry ) {
  debug( "Freeing a transaction entry ( %p ).", entry );

  assert( entry != NULL );

  dump_transaction_entry( debug, entry );

  if ( entry->message != NULL ) {
    debug( "Freeing a message associated with a transaction ( transaction_entry = %p, message = %p, data = %p, length = %zu ).",
           entry, entry->message, entry->message->data, entry->message->length );
    free_buffer( entry->message );
  }
  xfree( entry );

  debug( "A transaction entry is freed ( %p ).", entry );
}


static void
create_transaction_db() {
  debug( "Creating transaction database." );

  assert( transaction_db == NULL );

  transaction_db = xmalloc( sizeof( transaction_table ) );
  transaction_db->xid = create_hash( compare_transaction_id, hash_transaction_id );
  assert( transaction_db->xid != NULL );
  transaction_db->barrier_xid = create_hash( compare_transaction_id, hash_transaction_id );
  assert( transaction_db->barrier_xid != NULL );

  debug( "Transaction database is created ( db = %p, xid = %p, barrier_xid = %p ).", transaction_db, transaction_db->xid, transaction_db->barrier_xid );
}


static void
delete_transaction_db() {
  debug( "Deleting transaction database ( db = %p, xid = %p, barrier_xid = %p ).", transaction_db, transaction_db->xid, transaction_db->barrier_xid );

  assert( transaction_db != NULL );
  assert( transaction_db->xid != NULL );
  assert( transaction_db->barrier_xid != NULL );

  hash_entry *e;
  hash_iterator iter;
  init_hash_iterator( transaction_db->xid, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    if ( e->value != NULL ) {
      free_transaction_entry( e->value );
    }
  }
  delete_hash( transaction_db->xid );
  transaction_db->xid = NULL;
  delete_hash( transaction_db->barrier_xid );
  transaction_db->barrier_xid = NULL;
  xfree( transaction_db );

  debug( "Transaction database is deleted ( %p ).", transaction_db );

  transaction_db = NULL;
}


static transaction_entry *
lookup_transaction_entry_by_xid( uint32_t transaction_id ) {
  debug( "Looking up a transaction entry by transaction id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );

  assert( transaction_db != NULL );
  assert( transaction_db->xid != NULL );

  transaction_entry *entry = lookup_hash_entry( transaction_db->xid, &transaction_id );
  if ( entry != NULL ) {
    debug( "A transaction entry found." );
    dump_transaction_entry( debug, entry );
  }
  else {
    debug( "Failed to lookup a transaction entry by transaction id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );
  }

  return entry;
}


static transaction_entry *
lookup_transaction_entry_by_barrier_xid( uint32_t transaction_id ) {
  debug( "Looking up a transaction entry by barrier transaction id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );

  assert( transaction_db != NULL );
  assert( transaction_db->barrier_xid != NULL );

  transaction_entry *entry = lookup_hash_entry( transaction_db->barrier_xid, &transaction_id );
  if ( entry != NULL ) {
    debug( "A transaction entry found." );
    dump_transaction_entry( debug, entry );
  }
  else {
    debug( "Failed to lookup a transaction entry by barrier transaction id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );
  }

  return entry;
}


static transaction_entry *
delete_transaction_entry_by_xid( uint32_t transaction_id ) {
  debug( "Deleting a transaction entry by transaction id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );

  assert( transaction_db != NULL );
  assert( transaction_db->xid != NULL );
  assert( transaction_db->barrier_xid != NULL );

  transaction_entry *deleted = delete_hash_entry( transaction_db->xid, &transaction_id );
  if ( deleted != NULL ) {
    debug( "A transaction entry is deleted by transaction_id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );
    debug( "Deleting a transaction entry by barrier transaction id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );
    delete_hash_entry( transaction_db->barrier_xid, &deleted->barrier_xid );
  }
  else {
    debug( "Failed to delete a transaction entry by transaction_id ( transaction_db = %p, transaction_id = %#x ).", transaction_db, transaction_id );
  }

  return deleted;
}


static bool
get_monotonic_time( struct timespec *ts ) {
  debug( "Retrieving monotonic time ( ts = %p ).", ts );
  
  assert( ts != NULL );

  int ret = clock_gettime( CLOCK_MONOTONIC, ts );
  if ( ret != 0 ) {
    char buf[ 256 ];
    memset( buf, '\0', sizeof( buf ) );
    char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
    critical( "Failed to get monotonic time ( errno = %s [%d] ).", error_string, errno );
    return false;
  }

  debug( "Current monotonic time is %d.%09d.", ( int ) ts->tv_sec, ( int ) ts->tv_nsec );

  return true;
}


static bool
add_transaction_entry( transaction_entry *entry ) {
  debug( "Adding a transaction entry ( entry = %p, transaction_db = %p ).", entry, transaction_db );

  assert( entry != NULL );
  assert( transaction_db != NULL );
  assert( transaction_db->xid != NULL );
  assert( transaction_db->barrier_xid != NULL );

  bool ret = get_monotonic_time( &entry->expires_at );
  if ( !ret ) {
    return false;
  }
  ADD_TIMESPEC( &entry->expires_at, &TRANSACTION_TIMEOUT, &entry->expires_at );

  dump_transaction_entry( debug, entry );

  if ( lookup_transaction_entry_by_xid( entry->xid ) == NULL ) {
    void *duplicated = insert_hash_entry( transaction_db->xid, &entry->xid, entry );
    assert( duplicated == NULL );
  }
  else {
    error( "Duplicated transaction entry found ( entry = %p, xid = %#x ).", entry, entry->xid );
    dump_transaction_entry( error, entry );
    return false;
  }

  if ( lookup_transaction_entry_by_barrier_xid( entry->barrier_xid ) == NULL ) {
    void *duplicated = insert_hash_entry( transaction_db->barrier_xid, &entry->barrier_xid, entry );
    assert( duplicated == NULL );
  }
  else {
    error( "Duplicated transaction entry found ( entry = %p, barrier_xid = %#x ).", entry, entry->barrier_xid );
    dump_transaction_entry( error, entry );
    delete_transaction_entry_by_xid( entry->xid );
    return false;
  }

  debug( "A transaction is added." );
  dump_transaction_entry( debug, entry );

  return true;
}


static void
age_transaction_entries( void *user_data ) {
  UNUSED( user_data );

  debug( "Aging transaction entries ( %p ).", transaction_db );

  assert( transaction_db != NULL );
  assert( transaction_db->xid != NULL );

  struct timespec now;
  bool ret = get_monotonic_time( &now );
  if ( !ret ) {
    return;
  }

  hash_entry *e;
  hash_iterator iter;
  init_hash_iterator( transaction_db->xid, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    transaction_entry *entry = e->value;
    if( entry == NULL ) {
      continue;
    }

    debug( "Checking an entry ( %p ).", entry );
    dump_transaction_entry( debug, entry );

    if ( TIMESPEC_LESS_THAN( &entry->expires_at, &now ) ) {
      struct ofp_header *header = entry->message->data;
      header->xid = htonl( entry->original_xid );
      if ( entry->completed ) {
        if ( !entry->error_received && entry->succeeded_callback != NULL ) {
          debug( "Calling succeeded callback ( %p ).", entry->succeeded_callback );
          entry->succeeded_callback( entry->datapath_id, entry->message, entry->succeeded_user_data );
        }
        else if ( entry->error_received && entry->failed_callback != NULL ) {
          debug( "Calling failed callback ( %p ).", entry->failed_callback );
          entry->failed_callback( entry->datapath_id, entry->message, entry->failed_user_data );
        }
      }
      else {
        if ( !entry->timeout ) {
          debug( "Transaction timeout. Wait for final marin to elapse." );
          // Wait for a moment after deleting all messages in send queue
          delete_openflow_messages( entry->datapath_id );
          entry->expires_at = now;
          ADD_TIMESPEC( &entry->expires_at, &TRANSACTION_END_MARGIN, &entry->expires_at );
          entry->timeout = true;
          continue;
        }
        else {
          warn( "Transaction timeout ( xid = %#x, barrier_xid = %#x, original_xid = %#x, expires_at = %d.%09d ).",
                entry->xid, entry->barrier_xid, entry->original_xid,
                ( int ) entry->expires_at.tv_sec, ( int ) entry->expires_at.tv_nsec );
          if ( entry->failed_callback != NULL ) {
            entry->failed_callback( entry->datapath_id, entry->message, entry->failed_user_data );
          }
        }
      }
      delete_transaction_entry_by_xid( entry->xid );
      free_transaction_entry( entry );
    }
  }

  debug( "Aging completed ( %p ).", transaction_db );
}


bool
execute_transaction( uint64_t datapath_id, const buffer *message,
                     succeeded_handler succeeded_callback, void *succeeded_user_data,
                     failed_handler failed_callback, void *failed_user_data ) {
  assert( message != NULL );
  assert( message->length >= sizeof( struct ofp_header ) );

  transaction_entry *entry = alloc_transaction_entry();

  entry->datapath_id = datapath_id;
  entry->message = duplicate_buffer( message );
  entry->xid = get_transaction_id();
  entry->barrier_xid = get_transaction_id();
  struct ofp_header *header = entry->message->data;
  entry->original_xid = ntohl( header->xid );
  header->xid = htonl( entry->xid ); // Override xid for keeping uniqueness
  entry->completed = false;
  entry->error_received = false;
  entry->timeout = false;

  debug( "Executing a transaction ( xid = %#x, barrier_xid = %#x, original_xid = %#x ).",
         entry->xid, entry->barrier_xid, entry->original_xid );

  bool ret = send_openflow_message( datapath_id, entry->message );
  if ( !ret ) {
    // FIXME: send queue may be full. We may need to retry for sending the message.
    error( "Failed to send an OpenFlow message ( datapath_id = %#" PRIx64 ", transaction_id = %#x ).",
           datapath_id, entry->original_xid );
    free_transaction_entry( entry );
    return false;
  }

  buffer *barrier = create_barrier_request( entry->barrier_xid );
  ret = send_openflow_message( datapath_id, barrier );
  free_buffer( barrier );
  if ( !ret ) {
    error( "Failed to send a barrier request ( datapath_id = %#" PRIx64 ", transaction_id = %#x ).",
           datapath_id, entry->barrier_xid );
    free_transaction_entry( entry );
    return false;
  }

  entry->succeeded_callback = succeeded_callback;
  entry->succeeded_user_data = succeeded_user_data;
  entry->failed_callback = failed_callback;
  entry->failed_user_data = failed_user_data;

  ret = add_transaction_entry( entry );
  if ( !ret ) {
    error( "Failed to save transaction entry ( %p ).", entry );
    dump_transaction_entry( error, entry );
    free_transaction_entry( entry );
    return false;
  }

  debug( "OpenFlow messages are put into a send queue and a transaction entry is saved." );

  return true;
}


static void
handle_barrier_reply( uint64_t datapath_id, uint32_t transaction_id, void *user_data ) {
  UNUSED( user_data );

  debug( "Barrier reply from switch %#" PRIx64 " for %#x received.", datapath_id, transaction_id );

  transaction_entry *entry = lookup_transaction_entry_by_barrier_xid( transaction_id );
  if ( entry == NULL ) {
    warn( "A barrier reply for untracked transaction received ( transaction_id = %#x, datapath_id = %#" PRIx64 " ).",
          transaction_id, datapath_id );
    return;
  }

  entry->completed = true;
  bool ret = get_monotonic_time( &entry->expires_at );
  if ( !ret ) {
    return;
  }
  ADD_TIMESPEC( &entry->expires_at, &TRANSACTION_END_MARGIN, &entry->expires_at );

  debug( "Set transaction timeout to %d.%09d.", ( int ) entry->expires_at.tv_sec, ( int ) entry->expires_at.tv_nsec );
}


static void
handle_error( uint64_t datapath_id, uint32_t transaction_id, uint16_t type, uint16_t code,
              const buffer *data, void *user_data ) {
  UNUSED( user_data );
  UNUSED( data );

  warn( "Error ( type = %#x, code = %#x ) from switch %#" PRIx64 " for %#x received.",
        type, code, datapath_id, transaction_id );

  transaction_entry *entry = lookup_transaction_entry_by_xid( transaction_id );
  if ( entry == NULL ) {
    entry = lookup_transaction_entry_by_barrier_xid( transaction_id );
    if ( entry == NULL ) {
      warn( "An error for untracked transaction received "
            "( type = %#x, code = %#x, transaction_id = %#x, datapath_id = %#" PRIx64 " ).",
            type, code, transaction_id, datapath_id );
      return;
    }
    entry->completed = true;
  }

  entry->error_received = true;
  bool ret = get_monotonic_time( &entry->expires_at );
  if ( !ret ) {
    return;
  }
  ADD_TIMESPEC( &entry->expires_at, &TRANSACTION_END_MARGIN, &entry->expires_at );

  debug( "Set transaction timeout to %d.%09d.", ( int ) entry->expires_at.tv_sec, ( int ) entry->expires_at.tv_nsec );
}


bool
init_transaction_manager() {
  debug( "Initializing transaction manager." );

  create_transaction_db();
  set_barrier_reply_handler( handle_barrier_reply, NULL );
  set_error_handler( handle_error, NULL );

  struct itimerspec interval;
  interval.it_interval = TRANSACTION_AGING_INTERVAL;
  interval.it_value = TRANSACTION_AGING_INTERVAL;
  add_timer_event_callback( &interval, age_transaction_entries, NULL );

  debug( "Initialization completed." );

  return true;
}


bool
finalize_transaction_manager() {
  debug( "Finalizing transaction manager." );

  delete_transaction_db();
  delete_timer_event( age_transaction_entries, NULL );

  debug( "Finalization completed." );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
