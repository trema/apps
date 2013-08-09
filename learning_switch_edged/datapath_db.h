/*
 * Copyright (C) 2008-2013 NEC Corporation
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


#ifndef DATAPATH_DB_H
#define DATAPATH_DB_H

#include "trema.h"


hash_table *create_datapath_db( void );
void insert_datapath_entry( hash_table *db, uint64_t datapath_id, void *user_data, void delete_user_data( void *user_data ) );
void *lookup_datapath_entry( hash_table *db, uint64_t datapath_id );
void delete_datapath_entry( hash_table *db, uint64_t datapath_id );
void delete_datapath_db( hash_table *db );
void foreach_datapath_db( hash_table *db, void function( void *user_data ) );

#endif // DATAPATH_DB_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
