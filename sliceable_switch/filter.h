/*
 * Packet filter functions.
 *
 * Author: Yasunobu Chiba
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


#ifndef FILTER_H
#define FILTER_H


#include "trema.h"


enum {
  ALLOW = 0,
  DENY,
  LOCAL,
};


bool init_filter( const char *file );
bool finalize_filter();
int filter( uint64_t in_datapath_id, uint16_t in_port, uint16_t slice_number, const buffer *packet );


#endif // FILTER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
