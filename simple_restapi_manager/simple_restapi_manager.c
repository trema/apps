/*
 * Author: TeYen Liu 
 *         teyen.liu@gmail.com
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


#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <jansson.h>
#include <regex.h>

#include "mongoose.h"
#include "librestapi.h"
#include "simple_restapi_manager.h"
#include "trema.h"


/* This variable is for putting the url mapping data into here */
url_mapping_table url_mapping_db = { NULL };
const char *options[] = {"listening_ports", "8080", NULL};

struct mg_context *ctx = NULL;
struct mg_callbacks callbacks;

/*_________________---------------------------__________________
  _________________   mongoose callback       __________________
  -----------------___________________________------------------
*/
static int *mongoose_callback( struct mg_connection *conn ) 
{
  const struct mg_request_info *request_info = mg_get_request_info(conn); 
   
    char *ret_strings = NULL;
    
    /* Iterate all the url_mapping data */
    hash_iterator iter;
    hash_entry *e;
    int reti;
    
    init_hash_iterator( url_mapping_db.url_mapping, &iter );
    while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
      url_mapping *url_mapping_data = ( url_mapping * ) e->value;
      
      /* Compare the string with regex */
      reti = regexec( &url_mapping_data->regex, request_info->uri , 0, NULL, 0 );
      
      if( !reti ){ /* Match */
        if( strcmp( request_info->request_method , url_mapping_data->method ) == 0 ) { //Match the REST API
          printf( "REST API found \n" );
          ret_strings = ( *url_mapping_data->restapi_requested_callback )( request_info, NULL );
        }
        else {
          ret_strings = "";
        }
      }
    }
    
    int ret_length = strlen( ret_strings );
    char content[ ret_length + 1024 ];
    int content_length = snprintf(content, sizeof(content),
                                  "Hello from mongoose! Remote port: %d, content:%s\n",
                                  request_info->remote_port, ret_strings );
    
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: %d\r\n"        // Always set Content-Length
              "\r\n"
              "%s",
              content_length, content);
    
    
    return 1;
}

bool start_restapi_manager() {
  /* Start mongoose web server */
  ctx = mg_start(&mongoose_callback, NULL, options);
  return true;
}

bool add_restapi_url( char *url_str, restapi_callback_func restapi_callback ){
  url_mapping *url_mapping_data = NULL;
  url_mapping_data = allocate_url_mapping();
  url_mapping_data->url_pattern_key = url_str;
  url_mapping_data->restapi_requested_callback = restapi_callback;
  compile_url_mapping( url_mapping_data, &url_mapping_db );
  return true;
}


bool delete_restapi_url(){
  // TBD
  return true; 
}


bool init_restapi_manager() {
    
  if ( ctx != NULL )
      return false; // has initialized already
 
  /* Prepare callbacks structure. 
   * We have only one callback, the rest are NULL.
   */
  memset( &callbacks, 0, sizeof( callbacks ) );
  callbacks.begin_request = mongoose_callback;
  
  /* Prepare the URL Mapping Data Structure */
  create_url_mapping_db( &url_mapping_db );
  initialize_url_mapping( &url_mapping_db );
  
  return true;
}

bool finalize_restapi_manager() {
  mg_stop(ctx);
  delete_url_mapping_db( &url_mapping_db );
  memset( &callbacks, 0, sizeof( callbacks ) );
}





/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
