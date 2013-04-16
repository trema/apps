/*
 * Author: Yasunobu Chiba
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


#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H


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


bool execute_transaction( uint64_t datapath_id, const buffer *message,
                          succeeded_handler succeeded_callback, void *succeeded_user_data,
                          failed_handler failed_callback, void *failed_user_data );
bool init_transaction_manager();
bool finalize_transaction_manager();


#endif // TRANSACTION_MANAGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
