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


#ifndef MONITORING_INTERFACE_H
#define MONITORING_INTERFACE_H


#include <inttypes.h>
#include "trema.h"


#define MONITORING_SERVICE "monitoring_service"


enum {
  MONITORING_PORT_LOADING_REQUEST = 0xD000,
  MONITORING_PORT_LOADING_REPLY,
  MONITORING_PORT_LOADING_NOTIFICATION,
  MONITORING_FLOW_LOADING_NOTIFICATION,
  MONITORING_SUBSCRIBE_REQUEST,
  MONITORING_UNSUBSCRIBE_REQUEST,
  MONITORING_SUBSCRIBE_OR_UNSUBSCRIBE_REPLY,
};

enum {
  SUCCEEDED,
  FAILED,
  SWITCH_ERROR,
  UNDEFINED_ERROR = 255,
};

typedef struct {
  uint16_t avg_rx_packets;
  uint16_t avg_tx_packets;
  uint16_t avg_rx_bytes;
  uint16_t avg_tx_bytes;
  
  uint32_t curr;          /* Current features. */
  uint32_t advertised;    /* Features being advertised by the port. */
  uint32_t supported;     /* Features supported by the port. */
  uint32_t peer;          /* Features advertised by peer. */
  
  uint64_t port_feature_rate; /* the port rate that controller chooses to calculate loading */
  uint64_t port_bit_rate;
  uint16_t loading;       /* Loading Percentage */
} __attribute__( ( packed ) ) port_loading;

typedef struct {
  uint64_t flow_bit_rate;               // bytes * 8 / duration_sec
  struct ofp_match match;
  bool big_flow_flag;
} __attribute__( ( packed ) ) flow_loading;

typedef struct {
  uint64_t datapath_id;
  uint16_t port_no;
} __attribute__( ( packed ) ) monitoring_port_loading_request;

typedef struct {
  uint64_t datapath_id;
  uint16_t port_no;
  port_loading port_loading_info;
} __attribute__( ( packed ) ) monitoring_port_loading_reply;

typedef struct {
  uint64_t datapath_id;
  uint16_t port_no;
  port_loading port_loading_info;
  uint16_t port_notification_percentage_condition;  /* Loading Percentage */
} __attribute__( ( packed ) ) monitoring_port_loading_notification;

typedef struct {
  uint64_t datapath_id;
  flow_loading flow_loading_info;
  uint64_t flow_notification_bit_rate_condition;
} __attribute__( ( packed ) ) monitoring_flow_loading_notification;

typedef struct {
  char owner[ MESSENGER_SERVICE_NAME_LENGTH ];
} __attribute__( ( packed ) ) monitoring_subscribe_request;

typedef struct {
  char owner[ MESSENGER_SERVICE_NAME_LENGTH ];
} __attribute__( ( packed ) ) monitoring_unsubscribe_request;

typedef struct {
    uint64_t result;
} __attribute__( ( packed ) ) monitoring_subscribe_or_unsubscribe_reply;


enum {
  TIMEOUT,
  MANUALLY_REQUESTED,
};

buffer *create_monitoring_port_loading_request( uint64_t datapath_id, uint16_t port_no);
buffer *create_monitoring_port_loading_reply( uint64_t datapath_id, uint16_t port_no, 
        port_loading *port_loading_info );
buffer *create_monitoring_port_loading_notification( uint64_t datapath_id, uint16_t port_no, 
        port_loading *port_loading_info, uint16_t port_notification_percentage_cond );
buffer *create_monitoring_flow_loading_notification( uint64_t datapath_id, 
        flow_loading *flow_loading_info, uint64_t flow_notification_bit_rate_cond );
buffer *create_monitoring_subscribe_request( const char *name );
buffer *create_monitoring_unsubscribe_request( const char *name );
buffer *create_monitoring_subscribe_or_unsubscribe_reply( uint16_t result );


#endif // FLOW_MANAGER_INTERFACE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
