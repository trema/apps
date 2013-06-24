/*
 * Monitoring
 *
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


#include <assert.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>
#include "toolutils.h"
#include "libmonitoring.h"
#include "toolutils.h"

/* Callback functions */
static void ( *port_loading_requested_callback )( void *, 
        const port_loading *, uint64_t, uint16_t, void *user_data ) = NULL;
static void *port_loading_requested_callback_param = NULL;

static void ( *port_loading_notified_callback )( void *, 
        const port_loading *, uint64_t, uint16_t ) = NULL;
static void *port_loading_notified_callback_param = NULL;

static void ( *flow_loading_notified_callback )( void *, 
        const flow_loading *, uint64_t ) = NULL;

static void *flow_loading_notified_callback_param = NULL;

/* service name for message */
static char service_name[ MESSENGER_SERVICE_NAME_LENGTH ];

static void
set_service_name( void ) {
  snprintf( service_name, sizeof( service_name ), "libmonitoring.%u", getpid() );
  service_name[ sizeof( service_name ) - 1 ] = '\0';
  info( "set_service_name = %s", service_name );
}


static const char *
get_service_name( void ) {
  return service_name;
}

bool add_callback_port_loading_requested (
  void ( *callback )( void *, const port_loading *, uint64_t, uint16_t ), void *param ) {

  port_loading_requested_callback = callback;
  port_loading_requested_callback_param = param;

  return true;
}

bool add_callback_port_loading_notified (
  void ( *callback )( void *, const port_loading *, uint64_t, uint16_t ), void *param ) {

  port_loading_notified_callback = callback;
  port_loading_notified_callback_param = param;

  return true;
}

bool add_callback_flow_loading_notified (
  void ( *callback )( void *, const flow_loading *, uint64_t ), void *param ) {

  flow_loading_notified_callback = callback;
  flow_loading_notified_callback_param = param;

  return true;
}

bool send_monitoring_subscribe_request() {
  buffer *request = create_monitoring_subscribe_request( get_service_name() );

  bool ret = send_request_message( MONITORING_SERVICE, // To
                                   get_service_name(), // From
                                   MONITORING_SUBSCRIBE_REQUEST, // Tag
                                   request->data, request->length, NULL );
  info( "[send_monitoring_subscribe_request] ( tag = %#x, data = %p, length = %u ).", 
          MONITORING_SUBSCRIBE_REQUEST, request->data, request->length );
  free_buffer( request );
  return ret;
}


bool send_monitoring_unsubscribe_request() {
  buffer *request = create_monitoring_unsubscribe_request( get_service_name() );

  bool ret = send_request_message( MONITORING_SERVICE, // To
                                   get_service_name(), // From
                                   MONITORING_UNSUBSCRIBE_REQUEST, // Tag
                                   request->data, request->length, NULL );
  info( "[send_monitoring_unsubscribe_request] ( tag = %#x, data = %p, length = %u ).", 
          MONITORING_UNSUBSCRIBE_REQUEST, request->data, request->length );
  free_buffer( request );
  return ret;   
}


void subscribe_or_unsubscribe_reply_completed( uint16_t result ) {
    info( "[subscribe_or_unsubscribe_reply_completed] result = %u", result );
}


bool
send_port_loading_request( uint64_t datapathid, uint16_t port_no, void *user_data ) {
  /* user_data is port_info */
  
  buffer *request = create_monitoring_port_loading_request( datapathid, port_no );

  bool ret = send_request_message( MONITORING_SERVICE, // To
                                   get_service_name(), // From
                                   MONITORING_PORT_LOADING_REQUEST, // Tag
                                   request->data, request->length, user_data );
  
  debug( "[send_port_loading_request] ( tag = %#x, data = %p, length = %u ).", 
          MONITORING_PORT_LOADING_REQUEST, request->data, request->length );
  free_buffer( request );
  return ret;
}

void 
port_loading_reply_completed( uint64_t datapath_id, uint64_t port_no, 
        port_loading *port_loading_info, void *user_data ) {
    
    //port_info *p = user_data;
    
    // run callback function
    ( *port_loading_requested_callback )( port_loading_requested_callback_param, 
          &port_loading_info, datapath_id, port_no, user_data );
}

void 
port_loading_notification_completed( uint16_t tag __attribute__((unused)),
                               void *data,
                               size_t len __attribute__((unused)) ) {
  
  if ( port_loading_notified_callback == NULL ) {
    debug( "%s: callback is NULL", __FUNCTION__ );
    return;     
  }
  monitoring_port_loading_notification *notification = data;
  port_loading port_loading_info = notification->port_loading_info;
  
  // (re)build port db
  ( *port_loading_notified_callback )( port_loading_notified_callback_param, 
          &port_loading_info, notification->datapath_id, notification->port_no );
    
  debug( "[port_loading_notification_completed]" );
  debug( "[%#016" PRIx64 "] port = %u, port_bit_rate = %u, port_feature_rate = %u, avg_rx_bytes = %u, loading = %u", 
          notification->datapath_id, notification->port_no, port_loading_info.port_bit_rate, 
          port_loading_info.port_feature_rate, port_loading_info.avg_rx_bytes, port_loading_info.loading );
}

void 
flow_loading_notification_completed( uint16_t tag __attribute__((unused)),
                               void *data,
                               size_t len __attribute__((unused)) ) {
  
  if ( flow_loading_notified_callback == NULL ) {
    debug( "%s: callback is NULL", __FUNCTION__ );
    return;     
  }
  monitoring_flow_loading_notification *notification = data;
  flow_loading flow_loading_info = notification->flow_loading_info;
  
  // (re)build port db
  ( *flow_loading_notified_callback )( flow_loading_notified_callback_param, 
          &flow_loading_info, notification->datapath_id );
  
  debug( "[flow_loading_notification_completed]" );
  debug( "[%#016" PRIx64 "] match = %s, flow_bit_rate = %u, big_flow_flag = %s", 
          notification->datapath_id, 
          get_static_match_str( &flow_loading_info.match ), 
          flow_loading_info.flow_bit_rate, 
          flow_loading_info.big_flow_flag == false ? "FALSE" : "TRUE" );
}

static void
handle_reply( uint16_t tag, void *data, size_t length, void *user_data ) {
  switch ( tag ) {
    case MONITORING_PORT_LOADING_REPLY:
    {
      if ( length != sizeof( monitoring_port_loading_reply ) ) {
        error( "Invalid setup reply ( length = %u ).", length );
        return;
      }
      monitoring_port_loading_reply *reply = data;
      port_loading_reply_completed( reply->datapath_id, reply->port_no, 
              &reply->port_loading_info, user_data );
    }
    break;
    default:
    {
      error( "Undefined reply tag ( tag = %#x, length = %u ).", tag, length );
    }
    return;
  }
}

static void
handle_notification( uint16_t tag, void *data, size_t length ) {
  switch ( tag ) {
      case MONITORING_PORT_LOADING_NOTIFICATION:
      {
        if ( length != sizeof( monitoring_port_loading_notification ) ) {
          error( "Invalid port loading notification ( length = %u ).", length );
          return;
        }
        port_loading_notification_completed( tag, data, length );
      }
      break;
      case MONITORING_FLOW_LOADING_NOTIFICATION:
      {
        if ( length != sizeof( monitoring_flow_loading_notification ) ) {
          error( "Invalid flow loading notification ( length = %u ).", length );
          return;
        }
        flow_loading_notification_completed( tag, data, length );
      }
      break;
      default:
      {
        error( "Undefined notification tag ( tag = %#x, length = %u ).", tag, length );
      }
      return;
  }
}

bool
init_monitoring( void ) {
  set_service_name();
  add_message_received_callback( get_service_name(), handle_notification );
  add_message_replied_callback( get_service_name(), handle_reply );
  return true;
}

bool
finalize_monitoring( void ) {
  delete_message_received_callback( get_service_name(), handle_notification );
  delete_message_replied_callback( get_service_name(), handle_reply );
  return true;
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
