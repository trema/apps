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


#ifndef FDB_H
#define FDB_H

#include "trema.h"


#define MAX_AGE 300
#define ENTRY_NOT_FOUND_IN_FDB OFPP_ALL
#define MAC_STRING_LENGTH 18


bool mac_to_string( const uint8_t *mac, char *str, size_t size );
hash_table *create_fdb( void );
uint32_t lookup_fdb( hash_table *db, const uint8_t *mac, time_t *updated_at );
void update_fdb( hash_table *db, const uint8_t *mac, uint32_t port_number );
void delete_forwarding_entries_by_port_number( hash_table *db, uint32_t port_number );
void delete_aged_forwarding_entries( hash_table *db );
void delete_fdb( hash_table *db );


#endif // FDB_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
