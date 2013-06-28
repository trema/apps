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

#ifndef SFLOWCOLLECTOR_H
#define	SFLOWCOLLECTOR_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <regex.h>
#include "mongoose.h"
#include "trema.h"

#define URL_PATTERN_LEN 1024
#define METHOD_LEN 5
    
typedef struct url_mapping {
    char url_pattern_key[URL_PATTERN_LEN];
    char method[METHOD_LEN];
    regex_t regex;
    char* ( *restapi_requested_callback )( const struct mg_request_info *request_info, void *request_data );
} url_mapping;

typedef struct {
  hash_table *url_mapping;
} url_mapping_table;


url_mapping *lookup_url_mapping_by_match( url_mapping_table *mapping_db, char *url_key );
url_mapping *allocate_url_mapping( void );
bool add_url_mapping( char *url_key, url_mapping_table *mapping_db, url_mapping *mapping_data );
bool delete_url_mapping( url_mapping_table *mapping_db, url_mapping *mapping_data );
bool create_url_mapping_db( url_mapping_table *mapping_db );
bool delete_url_mapping_db( url_mapping_table *mapping_db );
void compile_url_mapping( url_mapping *url_mapping_data, url_mapping_table *mapping_db );
bool initialize_url_mapping( url_mapping_table *mapping_db );

#ifdef	__cplusplus
}
#endif

#endif	/* SFLOWCOLLECTOR_H */

