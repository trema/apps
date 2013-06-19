/*
 * Author: TeYen Liu
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


#ifndef SIMPLE_RESTAPI_MANAGER_H
#define SIMPLE_RESTAPI_MANAGER_H

#include "trema.h"


typedef void ( *succeeded_handler )(
  uint64_t datapath_id,
  const buffer *original_message,
  void *user_data
);


typedef void ( *failed_handler )(
  uint64_t datapath_id,
  const buffer *original_message,
  void *user_data
);

/* define REST API Callback Function Pointer */
typedef char* ( *restapi_callback_func )( const struct mg_request_info *request_info, void *request_data );

bool start_restapi_manager();
bool add_restapi_url( char *url_str, restapi_callback_func restapi_callback );
bool delete_restapi_url();

bool init_restapi_manager();
bool finalize_restapi_manager();


#endif // SIMPLE_RESTAPI_MANAGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
