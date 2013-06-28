/*
 * Monitoring example1
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
 * 51
*/

#include <assert.h>
#include <inttypes.h>
#include "trema.h"
#include "libmonitoring.h"


// =============================== send request of port loading ===============================
/** 
 * To request port loading info on monitoring application
 */
void
send_my_port_loading_request() {
  /* Send port loading request to monitoring app */
  send_port_loading_request( 0xe4, 1, NULL );
}
// =============================== send request of port loading ===============================


// =========================== handle reply from monitoring application ===========================
static void
handle_port_loading_reply( void *user_data, const port_loading *port_loading_info, uint64_t dpid, uint16_t port_no ){
    
  info( "[handle_port_loading_reply] %#016" PRIx64 ", port = %u, curr = %u, avg_rx_bytes = %u, avg_tx_bytes = %u, avg_rx_packets = %u, avg_tx_packets = %u, loading = %u", 
          dpid, port_no, 
          port_loading_info->curr, 
          port_loading_info->avg_rx_bytes,
          port_loading_info->avg_tx_bytes,
          port_loading_info->avg_rx_packets,
          port_loading_info->avg_tx_packets,
          port_loading_info->loading );
}
// =========================== handle reply from monitoring application ===========================


// =============================== handle alert from monitoring application ===============================
static void
alert_port_loading( void *user_data, const port_loading *port_loading_info, uint64_t dpid, uint16_t port_no ) {
  assert( user_data != NULL );
  assert( port_loading_info != NULL );
  
  UNUSED( user_data );
  
  info( "[alert_port_loading] dpid:%#" PRIx64 " port: %u, port_bit_rate = %u, port_feature_rate = %u, avg_rx_bytes = %u, loading = %u", 
          dpid, port_no, port_loading_info->port_bit_rate, port_loading_info->port_feature_rate, 
          port_loading_info->avg_rx_bytes, port_loading_info->loading );
}

static void
alert_flow_loading( void *user_data, const flow_loading *flow_loading_info, uint64_t dpid ) {
  assert( user_data != NULL );
  assert( flow_loading_info != NULL );
  
  info( "[alert_flow_loading] dpid:%#" PRIx64 " match = %s, flow_bit_rate = %u", dpid,
          get_static_match_str( &flow_loading_info->match ), flow_loading_info->flow_bit_rate );
}
// =============================== handle alert from monitoring application ===============================


int
main( int argc, char *argv[] ) {
  // Initialize the Trema world
  init_trema( &argc, &argv );

  // =============================== Monitoring Manager ===============================
  /* Setup a timer to send request for querying port loading */
  add_periodic_event_callback( 10, send_my_port_loading_request, NULL );
  /* Setup a handler for the reply of query port loading  */
  add_callback_port_loading_requested( handle_port_loading_reply, NULL );
  /* Subscribe the port and flow notification */
  add_callback_port_loading_notified( alert_port_loading, NULL );
  add_callback_flow_loading_notified( alert_flow_loading, NULL );
  
  init_monitoring();
  send_monitoring_subscribe_request();
  // =============================== Monitoring Manager ===============================
  
  // Main loop
  start_trema();

  // Finalize monitoring
  finalize_monitoring();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
