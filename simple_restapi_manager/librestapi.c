/*
 * Author: TeYen Liu
 *
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

#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <math.h>
#include "librestapi.h"


/**
 * This is default REST API callback function
 * @param request_info
 * @param request_data
 * @return 
 */
static char* 
process_default_callback( const struct mg_request_info *request_info, void *request_data ) {
  return "OK";
}

/* ===================== Starts with manage url_mapping ======================== */
static bool
compare_url_mapping( const void *url1, const void *url2 ) {
  // compare with char string
  if ( strcmp( url1, url2 ) ) {
    return true;
  }
  return false;
}

static unsigned int
hash_url_mapping( const void *key ) {
  // the key of paths_entry is match
  return hash_core( key, ( int ) sizeof( url_mapping ) );  // offsetof( paths_entry, number_of_multipath )
}

url_mapping *
lookup_url_mapping_by_match( url_mapping_table *mapping_db, char *url_key ) {
  return lookup_hash_entry( mapping_db->url_mapping, url_key );
}

url_mapping *
allocate_url_mapping( void ) {
  url_mapping *ret = malloc( sizeof( url_mapping ) );
  memset( ret->url_pattern_key, '\0', sizeof( ret->url_pattern_key ) );
  memset( ret->method, '\0', sizeof( ret->method ) );
  ret->restapi_requested_callback = NULL;
  
  return ret;
}

bool
add_url_mapping( char *url_key, url_mapping_table *mapping_db, url_mapping *mapping_data ) {
  if ( lookup_url_mapping_by_match( mapping_db, url_key ) != NULL ) {
    error( "Duplicated Paths Entry with Match" );
    return false;
  }
  insert_hash_entry( mapping_db->url_mapping, url_key, mapping_data );

  return true;
}

bool
delete_url_mapping( url_mapping_table *mapping_db, url_mapping *mapping_data ) {
  info( "[delete_url_mapping] runs ..." );
  url_mapping *deleted = delete_hash_entry( mapping_db->url_mapping, mapping_data->url_pattern_key );
  if ( deleted == NULL ) {
    return false;
  } else {
    free( deleted );
  }
  info( "[delete_url_mapping] a url_mapping is deleted" );
  return true;
}

bool
create_url_mapping_db( url_mapping_table *mapping_db ) {
  mapping_db->url_mapping = create_hash( compare_url_mapping, hash_url_mapping );
  return true;
}

bool
delete_url_mapping_db( url_mapping_table *mapping_db ) {
  hash_iterator iter;
  hash_entry *e;

  init_hash_iterator( mapping_db->url_mapping, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    void *value = delete_hash_entry( mapping_db->url_mapping, e->key );
    /* Need to free the element of hops */
    free( e->key );
    delete_url_mapping( mapping_db, value );
  }
  delete_hash( mapping_db->url_mapping );
  mapping_db->url_mapping = NULL;

  return true;
}

void
compile_url_mapping( url_mapping *url_mapping_data, url_mapping_table *mapping_db ){
  /* Compile the regular expression */
  int reti = regcomp( &url_mapping_data->regex, url_mapping_data->url_pattern_key, 0 );
  if ( reti ) { 
    error( "Cannot compile regex: %s\n", url_mapping_data->url_pattern_key ); 
  };
  
  /* After compiling regular expression, it needs to be added in url mapping db */
  add_url_mapping( url_mapping_data->url_pattern_key, mapping_db, url_mapping_data );
}

bool
initialize_url_mapping( url_mapping_table *mapping_db ) {
  url_mapping *url_mapping_data = NULL;
  
  /* Agent with GET Method */
  url_mapping_data = allocate_url_mapping();
  
  /* Add a default url and REST API callback function */
  strcpy( url_mapping_data->url_pattern_key, "^/testing$" );
  strcpy( url_mapping_data->method, "GET" );
  url_mapping_data->restapi_requested_callback = process_default_callback;
  compile_url_mapping( url_mapping_data, mapping_db );
  
  return true;
}
/* ===================== End with manage url_mapping ======================== */








