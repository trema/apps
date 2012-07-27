/*
 * Sliceable switch application.
 *
 * Author: Shuji Ishii, Yasunobu Chiba, Lei SUN
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


#ifndef SLICEABLE_SWITCH_H
#define SLICEABLE_SWITCH_H


#include "libpathresolver.h"


typedef struct {
  uint16_t idle_timeout;
  bool handle_arp_with_packetout;
  list_element *switches;
  hash_table *fdb;
  pathresolver *pathresolver;

  bool second_stage_down;
  bool last_stage_down;
} sliceable_switch;


#endif // SLICEABLE_SWITCH_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
