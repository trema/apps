/* 
 * File:   sflowcollector.h
 * Author: liudanny
 *
 * Created on February 4, 2013, 2:30 PM
 */

#ifndef SFLOWCOLLECTOR_H
#define	SFLOWCOLLECTOR_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/time.h>
#include <setjmp.h>
#include <regex.h>
#include "mongoose.h"
#include "trema.h"

    
typedef struct url_mapping {
    char *url_pattern_key;
    char method[5];
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

