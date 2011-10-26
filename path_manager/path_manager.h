/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2008-2011 NEC Corporation
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


#ifndef PATH_MANAGER_H
#define PATH_MANAGER_H


#include <inttypes.h>
#include "trema.h"


#define PATH_SETUP_SERVICE_NAME "path_setup_service"


/**
 * Message type definition for path setup requests.
 */
enum {
  MESSENGER_PATH_SETUP_REQUEST = 0x9001,
};


typedef struct {
  uint64_t datapath_id;
  uint16_t in_port;
  uint16_t out_port;
} __attribute__( ( packed ) ) path_manager_hop;


typedef struct {
  struct ofp_match match;
  uint16_t n_hops;
  path_manager_hop hops[ 0 ];
} __attribute__( ( packed ) ) path_manager_path;



#endif // PATH_MANAGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
