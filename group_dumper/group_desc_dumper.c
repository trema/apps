/*
 * Copyright (C) 2013 NEC Corporation
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

#ifndef TREMA_EDGE
#error "not supported"
#endif


#include "trema.h"
#include "utility.h"


static int n_switches = 0;


static void
handle_list_switches_reply( const list_element *switches, void *user_data ) {
  UNUSED( user_data );

  buffer *stats_request = create_group_desc_multipart_request( get_transaction_id(), 0 );

  n_switches = 0;
  const list_element *element;
  for ( element = switches; element != NULL; element = element->next ) {
    uint64_t datapath_id = *( uint64_t * ) element->data;
    send_openflow_message( datapath_id, stats_request );
    n_switches++;
  }

  free_buffer( stats_request );
}


static void
dump_group_desc( uint64_t datapath_id, struct ofp_group_desc *group_desc ) {
  char buckets_str[ 2048 ];
  memset( buckets_str, '\0', sizeof( buckets_str ) );

  struct ofp_bucket *buckets = ( struct ofp_bucket * ) ( ( char * ) group_desc + offsetof( struct ofp_group_desc, buckets ) );
  uint16_t buckets_len = ( uint16_t ) ( group_desc->length - ( offsetof( struct ofp_group_desc, buckets ) ) );
  if ( buckets_len > 0 ) {
   buckets_to_string( buckets, buckets_len, buckets_str, sizeof( buckets_str ) );
  }
  else {
    int ret = snprintf( buckets_str, sizeof( buckets_str ), "no bucket" );
    if ( ( ret >= ( int ) sizeof( buckets_str ) ) || ( ret < 0 ) ) {
      return;
    }
  }

  info( "[%#016" PRIx64 "] type = %u, group_id = %u, buckets = [%s]",
        datapath_id, group_desc->type, group_desc->group_id, buckets_str );
}


static bool
type_is_group_desc( uint16_t type ) {
  return type == OFPMP_GROUP_DESC;
}


static bool
more_requests( uint16_t flags ) {
  return ( flags & OFPMPF_REPLY_MORE ) == OFPMPF_REPLY_MORE;
}


static void
handle_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                    uint16_t type, uint16_t flags, const buffer *data,
                    void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( user_data );

  if ( !type_is_group_desc( type ) ) {
    return;
  }

  if ( data != NULL ) {
    size_t offset = 0;
    while ( ( data->length - offset ) >= sizeof( struct ofp_group_desc ) ) {
      struct ofp_group_desc *group_desc = ( struct ofp_group_desc * ) ( ( char * ) data->data + offset );
      dump_group_desc( datapath_id, group_desc );
      offset += group_desc->length;
    }
  }

  if ( !more_requests( flags ) ) {
    if ( --n_switches <= 0 ) {
      stop_trema();
    }
  }
}


static void
timed_out( void *user_data ) {
  UNUSED( user_data );

  error( "timed out." );

  stop_trema();
}


int
main( int argc, char *argv[] ) {
  // Initialize the Trema world
  init_trema( &argc, &argv );

  // Set event handlers
  set_list_switches_reply_handler( handle_list_switches_reply );
  set_multipart_reply_handler( handle_stats_reply, NULL );

  // Ask switch manager to obtain the list of switches
  send_list_switches_request( NULL );

  // Set a timer callback
  add_periodic_event_callback( 10, timed_out, NULL );

  // Main loop
  start_trema();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
