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


#ifndef FLOW_ENTRY_H
#define FLOW_ENTRY_H

#include "trema.h"


#define INPUT_TABLE_ID 0
#define OUTPUT_TABLE_ID 3


void insert_output_flow_entry( uint8_t mac[ OFP_ETH_ALEN ], uint64_t datapath_id, uint32_t port_number );
void delete_output_flow_entry( uint8_t mac[ OFP_ETH_ALEN ], uint64_t datapath_id, uint32_t port_number );
void delete_output_flow_entries_by_outport( uint64_t datapath_id, uint32_t port_number );
void insert_input_flow_entry( uint8_t mac[ OFP_ETH_ALEN ], uint64_t datapath_id, uint32_t port_number );
void delete_input_flow_entry( uint8_t mac[ OFP_ETH_ALEN ], uint64_t datapath_id, uint32_t port_number );
void delete_input_flow_entries_by_inport( uint64_t datapath_id, uint32_t port_number );
void insert_table_miss_flow_entry( uint64_t datapath_id, uint8_t table_id );


#endif // FLOW_ENTRY_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
