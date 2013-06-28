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

#ifndef UTILMONITORING_H
#define	UTILMONITORING_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "trema.h"
#include "libtopology.h"
#include "monitoring.h"

int do_ofp_match_cmp( const struct ofp_match *lhs, 
        const struct ofp_match *rhs );
list_element *create_subscribers( list_element **subscribers );
void delete_subscribers( list_element **subscribers );
char *lookup_subscriber( list_element *subscribers, const char *owner );
void add_subscriber( list_element **subscribers, const char *owner );
void delete_subscriber( list_element **subscribers, const char *owner );

void delete_flow( list_element **switches, flow_info *delete_flow );
void add_flow( list_element **switches, uint64_t dpid, struct ofp_flow_stats *flow_stats );
void update_flow( flow_info *flow, struct ofp_flow_stats *flow_stats );
flow_info *lookup_flow( list_element *switches, uint64_t dpid, struct ofp_match *match );

void delete_port( list_element **switches, port_info *delete_port );
void add_port( list_element **switches, uint64_t dpid, uint16_t port_no, 
        const char *name, uint8_t external, uint64_t port_setting_feature_rate );
void update_port( port_info *port, uint8_t external );
void delete_all_ports( list_element **switches );
port_info *lookup_port( list_element *switches, uint64_t dpid, uint16_t port_no );
void init_ports( list_element **switches, size_t n_entries, 
        const topology_port_status *s, uint64_t port_setting_feature_rate );

void dump_phy_port(uint64_t datapath_id, struct ofp_phy_port *phy_port);
void dump_flow_stats( uint64_t datapath_id, struct ofp_flow_stats *stats );
void dump_port_stats( uint64_t datapath_id, struct ofp_port_stats *stats );
uint32_t pickup_the_support_data( struct ofp_phy_port *port );
uint64_t calculate_port_rate_support(struct ofp_phy_port *port, 
        uint64_t port_setting_feature_rate );
void update_port_stats( void *user_data, uint64_t datapath_id, port_info *port, 
        struct ofp_port_stats *stats, int time_period );
void update_flow_stats( void *user_data, uint64_t datapath_id, flow_info *flow, 
        struct ofp_flow_stats *stats );

#ifdef	__cplusplus
}
#endif

#endif	/* UTILMONITORING_H */

