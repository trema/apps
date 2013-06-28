/*
 * Monitoring
 *
 * Author: TeYen Liu
 *
 * Copyright (C) 2013 TeYen Liu
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


#include <sys/types.h>
#include <unistd.h>
#include "monitoring_interface.h"
#include "trema.h"


static uint64_t flow_entry_group_index = 0;


buffer *create_monitoring_port_loading_request( uint64_t datapath_id, uint16_t port_no ) {
  size_t length = sizeof( monitoring_port_loading_request );
  buffer *buf = alloc_buffer_with_length( length );
  monitoring_port_loading_request *request = append_front_buffer( buf, length );

  request->datapath_id = datapath_id;
  request->port_no = port_no;

  return buf;
}

buffer *create_monitoring_port_loading_reply( uint64_t datapath_id, uint16_t port_no, 
        port_loading *port_loading_info ) {
  size_t length = sizeof( monitoring_port_loading_reply );
  buffer *buf = alloc_buffer_with_length( length );
  monitoring_port_loading_reply *reply = append_front_buffer( buf, length );

  reply->datapath_id = datapath_id;
  reply->port_no = port_no;
  reply->port_loading_info = *port_loading_info;

  return buf;
}


buffer *create_monitoring_port_loading_notification( uint64_t datapath_id, uint16_t port_no, 
        port_loading *port_loading_info, uint16_t port_notification_percentage_cond ) {
    
  size_t length = sizeof( monitoring_port_loading_notification );
  buffer *buf = alloc_buffer_with_length( length );
  monitoring_port_loading_notification *notification = append_front_buffer( buf, length );
  notification->datapath_id = datapath_id;
  notification->port_no = port_no;
  notification->port_loading_info = *port_loading_info;
  notification->port_notification_percentage_condition = port_notification_percentage_cond;

  return buf;
}

buffer *create_monitoring_flow_loading_notification( uint64_t datapath_id, 
        flow_loading *flow_loading_info, uint64_t flow_notification_bit_rate_cond ) {
    
  size_t length = sizeof( monitoring_flow_loading_notification );
  buffer *buf = alloc_buffer_with_length( length );
  monitoring_flow_loading_notification *notification = append_front_buffer( buf, length );
  notification->datapath_id = datapath_id;
  notification->flow_loading_info = *flow_loading_info;
  notification->flow_notification_bit_rate_condition = flow_notification_bit_rate_cond;

  return buf;
}


buffer *create_monitoring_subscribe_request( const char *name ) {
  size_t length = sizeof( monitoring_subscribe_request );
  buffer *buf = alloc_buffer_with_length( length );
  monitoring_subscribe_request *request = append_front_buffer( buf, length );
  memcpy( request->owner, name, sizeof( request->owner ) );
  request->owner[ sizeof( request->owner ) - 1 ] = '\0';
  //request->flow_notification_bit_rate_condition = flow_bit_rate_cond;
  //request->port_notification_percentage_condition = port_percentage_cond;
  
  return buf;
}

buffer *create_monitoring_unsubscribe_request( const char *name ) {
  size_t length = sizeof( monitoring_unsubscribe_request );
  buffer *buf = alloc_buffer_with_length( length );
  monitoring_unsubscribe_request *request = append_front_buffer( buf, length );
  memcpy( request->owner, name, sizeof( request->owner ) );
  request->owner[ sizeof( request->owner ) - 1 ] = '\0';
}

buffer *create_monitoring_subscribe_or_unsubscribe_reply( uint16_t result ) {
  size_t length = sizeof( monitoring_subscribe_or_unsubscribe_reply );
  buffer *buf = alloc_buffer_with_length( length );
  monitoring_subscribe_or_unsubscribe_reply *notification = append_front_buffer( buf, length );
  notification->result = result;

  return buf;
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
