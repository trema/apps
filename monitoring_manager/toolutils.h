/*
 * Monitoring
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef TOOLUTILS_H
#define	TOOLUTILS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "trema.h"

char* get_static_match_str( struct ofp_match *match );
char* get_static_actions_str( struct ofp_flow_stats *stats );

#ifdef	__cplusplus
}
#endif

#endif	/* TOOLUTILS_H */

