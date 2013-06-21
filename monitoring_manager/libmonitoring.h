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


#ifndef LIBPATH_H
#define LIBPATH_H


#include "trema.h"
#include "monitoring_interface.h"

bool add_callback_port_loading_requested (
  void ( *callback )( void *, const port_loading *, uint64_t, uint16_t ), void *param );
bool add_callback_port_loading_notified (
  void ( *callback )( void *, const port_loading *, uint64_t, uint16_t ), void *param );
bool add_callback_flow_loading_notified (
  void ( *callback )( void *, const flow_loading *, uint64_t ), void *param );

bool send_monitoring_subscribe_request();
bool send_monitoring_unsubscribe_request();
void subscribe_or_unsubscribe_reply_completed( uint16_t result);

bool send_port_loading_request( uint64_t datapathid, uint16_t port_no, 
        void *user_data );
void port_loading_reply_completed( uint64_t datapath_id, uint64_t port_no, 
        port_loading *port_loading_info, void *user_data );
void port_loading_notification_completed( uint16_t tag __attribute__((unused)),
        void *data, size_t len __attribute__((unused)) );
void flow_loading_notification_completed( uint16_t tag __attribute__((unused)),
        void *data, size_t len __attribute__((unused)) );
bool init_monitoring( void );
bool finalize_monitoring( void );


#endif // LIBPATH_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
