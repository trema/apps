/*
 * Slicing functions for creating virtual layer 2 domains.
 *
 * Author: Yasunobu Chiba, Lei SUN
 *
 * Copyright (C) 2008-2012 NEC Corporation
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


#ifndef SLICE_H
#define SLICE_H


#include <linux/limits.h>
#include "trema.h"
#include "sliceable_switch.h"


#define SLICE_NOT_FOUND 0xffff
#define VLAN_NONE 0xffff

#define LOOSE_MAC_BASED_SLICING 0x0001
#define RESTRICT_HOSTS_ON_PORT 0x0002


bool init_slice( const char *file, uint16_t mode, sliceable_switch *sliceable_switch );
bool finalize_slice();
uint16_t lookup_slice( uint64_t datapath_id, uint16_t port, uint16_t vid, const uint8_t *mac );
void delete_dynamic_port_slice_bindings( uint64_t datapath_id, uint16_t port );
bool get_port_vid( uint16_t slice, uint64_t datapath_id, uint16_t port, uint16_t *vid );
uint16_t lookup_slice_by_mac( const uint8_t *mac );
bool loose_mac_based_slicing_enabled();
bool mac_slice_maps_exist( uint16_t slice_number );


#endif // SLICE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
