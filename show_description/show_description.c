/*
 * Author: SUGYO Kazushi
 *
 * Copyright (C) 2012-2013 NEC Corporation
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


#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include "trema.h"
#include "timer.h"


typedef struct {
  uint64_t datapath_id;
#ifdef TREMA_EDGE
  struct ofp_desc desc_stats;
  list_element *port_desc;
#else
  struct ofp_desc_stats desc_stats;
#endif
} desc_entry;


typedef struct {
  hash_table *db;
  uint count;
} show_desc;


static void
handle_list_switches_reply( const list_element *switches, void *user_data ) {
  show_desc *show_desc = user_data;

#ifdef TREMA_EDGE
  buffer *desc_stats_request = create_desc_multipart_request( get_transaction_id(), 0 );
#else
  buffer *desc_stats_request = create_desc_stats_request( get_transaction_id(), 0 );
#endif

  const list_element *element;
  for ( element = switches; element != NULL; element = element->next ) {
    uint64_t datapath_id = * ( uint64_t *) element->data;
    send_openflow_message( datapath_id, desc_stats_request );
    show_desc->count++;
  }

  free_buffer( desc_stats_request );
}


static bool
type_is_desc_stats( uint16_t type ) {
#ifdef TREMA_EDGE
  return type == OFPMP_DESC;
#else
  return type == OFPST_DESC;
#endif
}

#ifdef TREMA_EDGE
static bool
type_is_port_desc( uint16_t type ) {
  return type == OFPMP_PORT_DESC;
}


static bool
more_requests( uint16_t flags ) {
  return ( flags & OFPMPF_REPLY_MORE ) == OFPMPF_REPLY_MORE;
}
#endif


#ifdef TREMA_EDGE
#define SIZE_OF_OFP_DESC sizeof( struct ofp_desc )
#else
#define SIZE_OF_OFP_DESC sizeof( struct ofp_desc_stats )
#endif


static void
handle_desc_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                        uint16_t type, uint16_t flags, const buffer *data,
                       void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( type );
  UNUSED( flags );
  show_desc *show_desc = user_data;

  if ( data->length < SIZE_OF_OFP_DESC ) {
    error( "invalid data" );

    stop_trema();
    return;
  }
  desc_entry *entry = xmalloc( sizeof( desc_entry ) );
  entry->datapath_id = datapath_id;
#ifdef TREMA_EDGE
  create_list( &entry->port_desc );
#endif
  memcpy( &entry->desc_stats, data->data, SIZE_OF_OFP_DESC );
  insert_hash_entry( show_desc->db, &entry->datapath_id, entry );

#ifdef TREMA_EDGE
  buffer *port_desc_request = create_port_desc_multipart_request( get_transaction_id(), 0 );
  send_openflow_message( datapath_id, port_desc_request );
  free_buffer( port_desc_request );
#else
  buffer *features_request = create_features_request( get_transaction_id() );
  send_openflow_message( datapath_id, features_request );
  free_buffer( features_request );
#endif
}


#ifdef TREMA_EDGE
static void
handle_port_desc_reply( uint64_t datapath_id, uint32_t transaction_id,
                        uint16_t type, uint16_t flags, const buffer *data,
                       void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( type );
  UNUSED( flags );
  show_desc *show_desc = user_data;

  desc_entry *desc = lookup_hash_entry( show_desc->db, &datapath_id );
  if ( desc == NULL ) {
    return;
  }
  append_to_tail( &desc->port_desc, duplicate_buffer( data ) );

  if ( more_requests( flags ) ) {
    return;
  }

  struct ofp_desc *desc_stats = &desc->desc_stats;
  info( "Manufacturer description: %s", desc_stats->mfr_desc );
  info( "Hardware description: %s", desc_stats->hw_desc );
  info( "Software description: %s", desc_stats->sw_desc );
  info( "Serial number: %s", desc_stats->serial_num );
  info( "Human readable description of datapath: %s", desc_stats->dp_desc );
  info( "Datapath ID: %#" PRIx64, datapath_id );
  for ( list_element *e = desc->port_desc; e != NULL; e = e->next ) {
    buffer *data = e->data;
    struct ofp_port *port = ( struct ofp_port  * ) data->data;
    size_t length = data->length;
    while ( length >= sizeof( struct ofp_port ) ) {
      const char *fake_output = "";
      if ( port->port_no == OFPP_LOCAL ) {
	fake_output = ":Local";
      }
      else if ( port->port_no == 0 || port->port_no > OFPP_MAX ) {
	fake_output = ":Invalid port";
      }
      const char *state = "(Port up)";
      if ( ( port->config & OFPPC_PORT_DOWN ) == OFPPC_PORT_DOWN &&
	   ( port->state & OFPPS_LINK_DOWN ) == OFPPS_LINK_DOWN ) {
	state = "(Port down)";
      }
      else if ( ( port->config & OFPPC_PORT_DOWN ) == OFPPC_PORT_DOWN ) {
	state = "(Port is administratively down)";
      }
      else if ( ( port->state & OFPPS_LINK_DOWN ) == OFPPS_LINK_DOWN ) {
	state = "(No physical link present)";
      }
      info( "Port no: %u(0x%x%s)%s", port->port_no, port->port_no, fake_output, state );
      info(
	"  Hardware address: %02x:%02x:%02x:%02x:%02x:%02x",
	port->hw_addr[ 0 ],
	port->hw_addr[ 1 ],
	port->hw_addr[ 2 ],
	port->hw_addr[ 3 ],
	port->hw_addr[ 4 ],
	port->hw_addr[ 5 ]
      );
      info( "  Port name: %s", port->name );
      length -= ( uint16_t ) sizeof( struct ofp_port );
      port++;
    }
    free_buffer( e->data );
  }
  delete_list( desc->port_desc );
  delete_hash_entry( show_desc->db, &datapath_id );
  show_desc->count--;
  xfree( desc );

  if ( show_desc->count == 0 ) {
    stop_trema();
  }
}
#endif


static void
handle_stats_reply( uint64_t datapath_id, uint32_t transaction_id,
                    uint16_t type, uint16_t flags, const buffer *data,
                    void *user_data ) {
  if ( type_is_desc_stats( type ) ) {
    handle_desc_stats_reply( datapath_id, transaction_id, type, flags, data, user_data );
  }
#ifdef TREMA_EDGE
  else if ( type_is_port_desc( type ) ) {
    handle_port_desc_reply( datapath_id, transaction_id, type, flags, data, user_data );
  }
#endif
}


#ifndef TREMA_EDGE
static void
handle_features_reply( uint64_t datapath_id, uint32_t transaction_id, uint32_t n_buffers, uint8_t n_tables, uint32_t capabilities, uint32_t actions, const list_element *phy_ports, void *user_data) {
  UNUSED( transaction_id );
  UNUSED( n_buffers );
  UNUSED( n_tables );
  UNUSED( capabilities );
  UNUSED( actions );
  show_desc *show_desc = user_data;
  desc_entry *desc = lookup_hash_entry( show_desc->db, &datapath_id );
  if ( desc == NULL ) {
    return;
  }
  struct ofp_desc_stats *desc_stats = &desc->desc_stats;

  info( "Manufacturer description: %s", desc_stats->mfr_desc );
  info( "Hardware description: %s", desc_stats->hw_desc );
  info( "Software description: %s", desc_stats->sw_desc );
  info( "Serial number: %s", desc_stats->serial_num );
  info( "Human readable description of datapath: %s", desc_stats->dp_desc );
  info( "Datapath ID: %#" PRIx64, datapath_id );
  list_element ports_list;
  memcpy( &ports_list, phy_ports, sizeof( list_element ) );
  for ( list_element *port = &ports_list; port != NULL; port = port->next ) {
    struct ofp_phy_port *phy_port = port->data;
    const char *fake_output = "";
    if ( phy_port->port_no == OFPP_LOCAL ) {
      fake_output = ":Local";
    }
    else if ( phy_port->port_no == 0 || phy_port->port_no > OFPP_MAX ) {
      fake_output = ":Invalid port";
    }
    const char *state = "(Port up)";
    if ( ( phy_port->config & OFPPC_PORT_DOWN ) == OFPPC_PORT_DOWN &&
         ( phy_port->state & OFPPS_LINK_DOWN ) == OFPPS_LINK_DOWN ) {
      state = "(Port down)";
    }
    else if ( ( phy_port->config & OFPPC_PORT_DOWN ) == OFPPC_PORT_DOWN ) {
      state = "(Port is administratively down)";
    }
    else if ( ( phy_port->state & OFPPS_LINK_DOWN ) == OFPPS_LINK_DOWN ) {
      state = "(No physical link present)";
    }
    info( "Port no: %u(0x%x%s)%s", phy_port->port_no, phy_port->port_no, fake_output, state );
    info(
      "  Hardware address: %02x:%02x:%02x:%02x:%02x:%02x",
      phy_port->hw_addr[ 0 ],
      phy_port->hw_addr[ 1 ],
      phy_port->hw_addr[ 2 ],
      phy_port->hw_addr[ 3 ],
      phy_port->hw_addr[ 4 ],
      phy_port->hw_addr[ 5 ]
    );
    info( "  Port name: %s", phy_port->name );
  }

  delete_hash_entry( show_desc->db, &datapath_id );
  show_desc->count--;
  xfree( desc );

  if ( show_desc->count == 0 ) {
    stop_trema();
  }
}
#endif


static void
timed_out( void *user_data ) {
  UNUSED( user_data );

  error( "timed out." );

  stop_trema();
}


int
main( int argc, char *argv[] ) {
  init_trema( &argc, &argv );
  show_desc show_desc;
  show_desc.db = create_hash( compare_datapath_id, hash_datapath_id );
  show_desc.count = 0;

  set_list_switches_reply_handler( handle_list_switches_reply );
#ifdef TREMA_EDGE
  set_multipart_reply_handler( handle_stats_reply, &show_desc );
#else
  set_stats_reply_handler( handle_stats_reply, &show_desc );
  set_features_reply_handler( handle_features_reply, &show_desc );
#endif
  add_periodic_event_callback( 10, timed_out, NULL );

  send_list_switches_request( &show_desc );


  start_trema();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
