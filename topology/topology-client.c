/*
 * Author: Nick Karanatsios <nickkaranatsios@gmail.com>
 *
 * Copyright (C) 2008-2011 NEC Corporation
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


#include <inttypes.h>
#include "ruby.h"
#include "trema.h"
#include "libtopology.h"
#include "topology_service_interface_option_parser.h"


VALUE mTopologyClient;
static VALUE cTopologyPortStatus;
static VALUE cTopologyLinkStatus;


#if 0
static VALUE
topology_port_status_init( VALUE kclass ) {
  topology_port_status *_topology_port_status = xmalloc( sizeof( topology_port_status ) );
  return Data_Wrap_Struct( klass, 0, xfree, _topology_port_status );
}


static VALUE
topology_link_status_init( VALUE kclass ) {
  topology_link_status *_topology_link_status = xmalloc( sizeof( topology_link_status ) );
  return Data_Wrap_Struct( kclass, 0, xfree, _topology_link_status );
}
#endif


/*
 * Creates an instance of the class TopologyPortStatus.
 */
static VALUE
topology_port_status_alloc( VALUE kclass ) {
  topology_port_status *_topology_port_status = xmalloc( sizeof( topology_port_status ) );
  memset( _topology_port_status, 0, sizeof( topology_port_status ) );
  return Data_Wrap_Struct( kclass, 0, xfree, _topology_port_status );
}


/*
 * Creates an instance of the class TopologyLinkStatus.
 */
static VALUE
topology_link_status_alloc( VALUE kclass ) {
  topology_link_status *_topology_link_status = xmalloc( sizeof( topology_link_status ) );
  memset( _topology_link_status, 0, sizeof( topology_link_status ) );
  return Data_Wrap_Struct( kclass, 0, xfree, _topology_link_status );
}


/*
 * Topoloy port status notifier.
 */
static void
port_status_updated( void *user_data, const topology_port_status *status ) {
  VALUE topology = ( VALUE ) user_data;

  debug( "Port status updated: dpid:%#" PRIx64 ", port:%u, %s, %s",
         status->dpid, status->port_no,
         ( status->status == TD_PORT_UP ? "up" : "down" ),
         ( status->external == TD_PORT_EXTERNAL ? "external" : "internal or inactive" ) );
  VALUE message = rb_funcall( cTopologyPortStatus, rb_intern( "new" ), 0 );
  topology_port_status *tmp = NULL;
  Data_Get_Struct( message, topology_port_status, tmp );
  memcpy( tmp, status, sizeof( topology_port_status ) );
  rb_funcall( topology, rb_intern( "topology_notifier" ), 2, message, ID2SYM( rb_intern( "port_status" ) ) );
}


static void
port_status( void *user_data, size_t n_entries, const topology_port_status *s ) {
  UNUSED( user_data );
  UNUSED( n_entries );
  UNUSED( s );
  add_callback_port_status_updated( port_status_updated, user_data );
}


/*
 * Topology link status notifier.
 */
static void 
link_status_updated( void *user_data, const topology_link_status *status ) {
  VALUE topology = ( VALUE ) user_data;

  VALUE message = rb_funcall( cTopologyLinkStatus, rb_intern( "new" ), 0 );
  topology_link_status *tmp = NULL;
  Data_Get_Struct( message, topology_link_status, tmp );
  memcpy( tmp, status, sizeof( topology_link_status ) );
  rb_funcall( topology, rb_intern( "topology_notifier" ), 2, message, ID2SYM( rb_intern( "link_status" ) ) );
}


static void
link_status( void *user_data, size_t n_entries, const topology_link_status *status ) {
  UNUSED( status );
  UNUSED( n_entries );
  add_callback_link_status_updated( link_status_updated, user_data  );
}


static void
topology_subscribed( void *user_data ) {

  VALUE topology = ( VALUE ) user_data;
  UNUSED( topology );
  
  if ( rb_respond_to( topology, rb_intern( "subscribed" ) ) == Qfalse ) {
    get_all_port_status( port_status, user_data );
    get_all_link_status( link_status, user_data );
  } else { 
    rb_funcall( topology, rb_intern( "subscribed" ), 0 );
  }
}


static VALUE
init_topology_client( VALUE self, VALUE service_name ) {
  int argc = 1;
  char **argv = xmalloc( sizeof ( char * ) * ( uint32_t ) ( argc + 1 ) );
  argv[ 0 ] = StringValuePtr( service_name );
  argv[ 1 ] = NULL; 
  
  init_topology_service_interface_options( &argc, &argv );
  xfree( argv );
  init_libtopology( get_topology_service_interface_name() );
  
  subscribe_topology( topology_subscribed, ( void * ) self );
  return self;
}


static VALUE
finalize_topology_client( VALUE self ) {
  finalize_libtopology();
  return self;
}


static topology_port_status *
get_topology_port_status( VALUE self ) {
  topology_port_status *_topology_port_status;
  Data_Get_Struct( self, topology_port_status, _topology_port_status );
  return _topology_port_status;
}


/*
 * Datapath_id, the message originator of this topology port status notifier.
 */
static VALUE
topology_port_status_dpid( VALUE self ) {
  return ULL2NUM( get_topology_port_status( self )->dpid );
}


/*
 * The port number that its status changed.
 */
static VALUE
topology_port_status_port_no( VALUE self ) {
  return UINT2NUM( get_topology_port_status( self )->port_no );
}


/*
 * An up and down status of a port.
 */
static VALUE
topology_port_status_status( VALUE self ) {
  return UINT2NUM( get_topology_port_status( self )->status );
}


/*
 * External status whether another switch is connected to this port or not.
 */
static VALUE
topology_port_status_external( VALUE self ) {
  return UINT2NUM( get_topology_port_status( self )->external );
}


static topology_link_status *
get_topology_link_status( VALUE self ) {
  topology_link_status *_topology_link_status;
  Data_Get_Struct( self, topology_link_status, _topology_link_status );
  return _topology_link_status;
}


/*
 * The origin path identifier for this topology link status notifier.
 */
static VALUE
topology_link_status_from_dpid( VALUE self ) {
  return ULL2NUM( get_topology_link_status( self )->from_dpid );
}


/*
 * The destination path identifier for this topology link status notifier.
 */
static VALUE
topology_link_status_to_dpid( VALUE self ) {
  return ULL2NUM( get_topology_link_status( self )->to_dpid );
}


/*
 * The origin port for this topology link status notifier.
 */
static VALUE
topology_link_status_from_portno( VALUE self ) {
  return UINT2NUM( get_topology_link_status( self )->from_portno );
}


/*
 * The destination port for this topology link status notifier.
 */
static VALUE
topology_link_status_to_portno( VALUE self ) {
  return UINT2NUM( get_topology_link_status( self )->to_portno );
}


/*
 * An up/down status of the identified path.
 */
static VALUE
topology_link_status_status( VALUE self ) {
  return UINT2NUM( get_topology_link_status( self )->status );
}


void
Init_topology_client() {
  mTopologyClient = rb_define_module( "TopologyClient" );
  cTopologyPortStatus = rb_define_class_under( mTopologyClient, "TopologyPortStatus", rb_cObject );
  cTopologyLinkStatus = rb_define_class_under( mTopologyClient, "TopologyLinkStatus", rb_cObject );
  rb_define_alloc_func( cTopologyPortStatus, topology_port_status_alloc );
  rb_define_alloc_func( cTopologyLinkStatus, topology_link_status_alloc );
#if 0
  rb_define_method( cTopologyPortStatus, "initialize", topology_port_status_init, 0 );
  rb_define_method( cTopologyLinkStatus, "initialize", topology_link_status_init, 0 );
#endif
  rb_define_method( mTopologyClient, "init_topology_client", init_topology_client, 1 );
  rb_define_method( mTopologyClient, "finalize_topology_client", finalize_topology_client, 0 );
  rb_define_method( cTopologyPortStatus, "dpid", topology_port_status_dpid, 0 );
  rb_define_method( cTopologyPortStatus, "port_no", topology_port_status_port_no, 0 );
  rb_define_method( cTopologyPortStatus, "status", topology_port_status_status, 0 );
  rb_define_method( cTopologyPortStatus, "external", topology_port_status_external, 0 );

  rb_define_method( cTopologyLinkStatus, "from_dpid", topology_link_status_from_dpid, 0 );
  rb_define_method( cTopologyLinkStatus, "to_dpid", topology_link_status_to_dpid, 0 );
  rb_define_method( cTopologyLinkStatus, "from_portno", topology_link_status_from_portno, 0 );
  rb_define_method( cTopologyLinkStatus, "to_portno", topology_link_status_to_portno, 0 );
  rb_define_method( cTopologyLinkStatus, "status", topology_link_status_status, 0 );
  rb_require( "topology/topology-client" );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
