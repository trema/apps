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

#ifndef MONITORING_H
#define	MONITORING_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "trema.h"
#include "openflow.h"
#include "hash_table.h"
    
    
typedef struct flow_info {
  uint64_t dpid;
  uint32_t prev_duration_sec;
  uint64_t prev_byte_count;
  uint64_t flow_bit_rate;               // bytes * 8 / duration_sec
  struct ofp_flow_stats flow_stats;
  uint16_t big_flow_number_times;       // count the number of times for defining big flow
  bool big_flow_flag;
  
} flow_info;
    
typedef struct port_info {
  uint64_t dpid;
  uint16_t port_no;
  
  bool external_link;
  bool switch_to_switch_link;
  bool switch_to_switch_reverse_link;
  
  /* For dynamic routing purpose */
  uint64_t prev_rx_packets;     /* Number of received packets. */
  uint64_t prev_tx_packets;     /* Number of transmitted packets. */
  uint64_t prev_rx_bytes;       /* Number of received bytes. */
  uint64_t prev_tx_bytes;       /* Number of transmitted bytes. */
  
  uint64_t curr_rx_packets;     /* Number of received packets. */
  uint64_t curr_tx_packets;     /* Number of transmitted packets. */
  uint64_t curr_rx_bytes;       /* Number of received bytes. */
  uint64_t curr_tx_bytes;       /* Number of transmitted bytes. */
  
  uint64_t avg_rx_packets;      /* Number of avg received packets. */
  uint64_t avg_tx_packets;      /* Number of avg transmitted packets. */
  uint64_t avg_rx_bytes;        /* Number of avg received bytes. */
  uint64_t avg_tx_bytes;        /* Number of avg transmitted bytes. */
  
  uint64_t port_feature_rate; /* the port rate that controller chooses to calculate loading */
  uint64_t port_bit_rate;
  uint16_t loading;           /* Loading Percentage */

  
  /* The feature of physical port */
  struct ofp_phy_port phy_port;
  
} port_info;


typedef struct switch_info {
  uint64_t dpid;
  list_element *ports; // list of port_info
  list_element *flows; // list of flow_info which is over threshold
} switch_info;


typedef struct monitoring_switch {
  list_element *switches;
  list_element *subscribers;
  /* monitoring configuration */
  // bandwidth loading percentage, for instance, 90 -> 90%
  uint64_t flow_bit_rate_conditon;
  uint16_t flow_times_condition;
  uint64_t port_setting_feature_rate;
  uint16_t port_percentage_condition;
} monitoring_switch;


#ifdef	__cplusplus
}
#endif

#endif	/* MONITORING_H */

