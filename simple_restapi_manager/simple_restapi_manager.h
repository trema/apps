/*
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


#ifndef SIMPLE_RESTAPI_MANAGER_H
#define SIMPLE_RESTAPI_MANAGER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "mongoose.h"

/* define REST API Callback Function Pointer */
typedef char* ( *restapi_callback_func )( const struct mg_request_info *request_info, void *request_data );

bool start_restapi_manager();
bool add_restapi_url( const char *url_str, const char *method, restapi_callback_func restapi_callback );
bool delete_restapi_url();

bool init_restapi_manager();
bool finalize_restapi_manager();


#ifdef	__cplusplus
}
#endif

#endif // SIMPLE_RESTAPI_MANAGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
