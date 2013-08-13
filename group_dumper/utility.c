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


#include "utility.h"


static bool
bucket_to_string( const struct ofp_bucket *bucket, uint16_t bucket_length, char *str, size_t str_length ) {
  uint16_t offset = offsetof( struct ofp_bucket, actions );
  const struct ofp_action_header *actions = ( const struct ofp_action_header * ) ( ( const char * ) bucket + offset );
  uint16_t actions_len = ( uint16_t ) ( bucket->len - offset );
  char act_str[ 1024 ];
  act_str[ 0 ] = '\0';
  if ( actions_len > 0 ) {
    bool ret = actions_to_string( actions, actions_len, act_str, sizeof( act_str ) );
    if ( ret == false ) {
      return false;
    }
  }
  int ret = snprintf( str, str_length, "weight=%u watch_port=%u watch_group=%u actions=[%s]",
                      bucket->weight, bucket->watch_port,
                      bucket->watch_group, act_str );
  if ( ( ret >= ( int ) str_length ) || ( ret < 0 ) ) {
    return false;
  }
  return true;
}


bool
buckets_to_string( const struct ofp_bucket *buckets, uint16_t buckets_length, char *str, size_t str_length ) {
  bool ret = true;
  size_t offset = 0;
  while ( ( buckets_length - offset ) >= sizeof( struct ofp_bucket ) ) {
    size_t current_str_length = strlen( str );
    size_t remaining_str_length = str_length - current_str_length;
    if ( current_str_length > 0 ) {
      if ( remaining_str_length > 2 ) {
        snprintf( str + current_str_length, remaining_str_length, ", " );
        remaining_str_length -= 2;
        current_str_length += 2;
      }
      else {
        ret = false;
        break;
      }
    }
    char *p = str + current_str_length;
    const struct ofp_bucket *bucket = ( const struct ofp_bucket * ) ( ( const char * ) buckets + offset );
    ret = bucket_to_string( bucket, bucket->len, p, remaining_str_length );
    if ( ret == false ) {
      break;
    }
    offset += bucket->len;
  }

  str[ str_length - 1 ] = '\0';

  return ret;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
