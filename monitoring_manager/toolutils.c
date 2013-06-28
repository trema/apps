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


#include <string.h>
#include "toolutils.h"

static char static_match_str[ 256 ];
static char static_actions_str[ 256 ]; 

char* get_static_match_str( struct ofp_match *match ) {
  memset( static_match_str, '\0', sizeof( static_match_str ) );                                                                                                                                
  match_to_string( match, static_match_str, sizeof( static_match_str ) );
  return static_match_str;
}

char* get_static_actions_str( struct ofp_flow_stats *stats ) {
  memset( static_actions_str, '\0', sizeof( static_actions_str ) );          
  uint16_t actions_length = (uint16_t) (stats->length - offsetof( struct ofp_flow_stats, actions ));
  if ( actions_length > 0 ) {
    actions_to_string( stats->actions, actions_length, static_actions_str, sizeof( static_actions_str ) );                                                                                     
  } 
  return static_actions_str;
}