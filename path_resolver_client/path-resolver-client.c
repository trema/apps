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


#include "ruby.h"
#include "trema.h"
#include "libpathresolver.h"


VALUE mPathResolverClient;
/*
 * A class that describes a hop a path segment resolved by the path resolver
 * library.
 */
VALUE cPathResolverHop;
/*
 * A client handle to the path resolver library.
 */
static pathresolver *handle;


/*
 * Creates and initializes the path resolver library. Saves a handle a user
 * can use to interact with the path resolver library.
 *
 * @return [self] receiver
 */
static VALUE
init_path_resolver_client( VALUE self ) {
  handle = create_pathresolver();
  return self;
}


/*
 * Releases the handle acquired previously by the initialization call.
 *
 */
static VALUE
finalize_path_resolver_client( VALUE self ) {
  delete_pathresolver( handle );
  return self;
}


#if 0
static VALUE
pathresolver_hop_init( VALUE kclass ) {
  pathresolver_hop *_pathresolver_hop = xmalloc( sizeof( pathresolver_hop ) );
  return Data_Wrap_Struct( klass, 0, xfree, _pathresolver_hop );
}
#endif


static VALUE
pathresolver_hop_alloc( VALUE kclass ) {
  pathresolver_hop *_pathresolver_hop = xmalloc( sizeof( pathresolver_hop ) );
  memset( _pathresolver_hop, 0, sizeof( pathresolver_hop ) );
  return Data_Wrap_Struct( kclass, 0, xfree, _pathresolver_hop );
}


static pathresolver_hop *
get_pathresolver_hop( VALUE self ) {
  pathresolver_hop *_pathresolver_hop;
  Data_Get_Struct( self, pathresolver_hop, _pathresolver_hop );
  return _pathresolver_hop;
}


/*
 * @return [Number] dpid the datapath identifier of this hop.
 */
static VALUE
pathresolver_hop_dpid( VALUE self ) {
  return ULL2NUM( get_pathresolver_hop( self )->dpid );
}


/*
 * @return [Number] in_port_no the input port number of this hop.
 */
static VALUE
pathresolver_hop_in_port_no( VALUE self ) {
  return UINT2NUM( get_pathresolver_hop( self )->in_port_no );
}


/*
 * @return [Number] out_port_no the output port number of this hop.
 */
static VALUE
pathresolver_hop_out_port_no( VALUE self ) {
  return UINT2NUM( get_pathresolver_hop( self )->out_port_no );
}


/*
 * Resolves a path to route a packet given by an origin(in_dpid/in_port)
 * and a destination(out_dpid, out_port).
 *
 * @example
 *   path_resolve in_dpid, in_port, out_dpid, out_port
 *
 * @param [Number] in_dpid
 *   the datapath id of the originator.
 *
 * @param [Number] in_port
 *   the input port from where the packet in is received.
 *
 * @param [Number] out_dpid
 *   the destination identifier.
 *
 * @param [Number] out_port
 *   the destination output port.
 *
 * @return [Array<PathResolverHop>]
 *   an array of PathResolverHop objects for each hop determined.
 *
 * @return [NilClass] nil if path determination failed.
 */
static VALUE
path_resolve( VALUE self, VALUE in_dpid, VALUE in_port, VALUE out_dpid, VALUE out_port  ) {
  UNUSED( self );

  if ( handle != NULL ) {
    dlist_element *hops = resolve_path( handle, NUM2ULL( in_dpid ), ( uint16_t ) NUM2UINT( in_port ), NUM2ULL( out_dpid ), ( uint16_t ) NUM2UINT( out_port ) );
    if ( hops != NULL ) {
      VALUE pathresolver_hops_arr = rb_ary_new();
      for ( dlist_element *e = hops; e != NULL; e = e->next ) {
        VALUE message = rb_funcall( cPathResolverHop, rb_intern( "new" ), 0 );
        pathresolver_hop *tmp = NULL;
        Data_Get_Struct( message, pathresolver_hop, tmp );
        memcpy( tmp, e->data,  sizeof( pathresolver_hop ) );
        rb_ary_push( pathresolver_hops_arr, message );
      }
      return pathresolver_hops_arr;
    }
    else {
      return Qnil;
    }
  }
  return Qnil;
}


/*
 * Is path resolver library initialialized?
 *
 * @return [Boolean] true if a valid handle otherwise false.
 */
static VALUE
is_handle( VALUE self ) {
  UNUSED( self );
  return handle != NULL ? Qtrue : Qfalse;
}


/*
 * Updates the status of a node based on the given topology link status message.
 *
 * @param [topology_link_status] message
 *   the message that describes the topology link status.
 *
 * @return [self] receiver
 */
static VALUE
update_path( VALUE self, VALUE message ) {
  topology_link_status *_topology_link_status;
  Data_Get_Struct( message, topology_link_status, _topology_link_status );
  update_topology( handle, _topology_link_status );
  return self;
}


void
Init_path_resolver_client() {
  mPathResolverClient = rb_define_module( "PathResolverClient" );
  cPathResolverHop = rb_define_class_under( mPathResolverClient, "HopInfo", rb_cObject );
  rb_define_alloc_func( cPathResolverHop, pathresolver_hop_alloc );
#if 0
  rb_define_method( cPathResolverHop "initialize", pathresolver_hop_init, 0 );
#endif
  rb_define_method( cPathResolverHop, "dpid", pathresolver_hop_dpid, 0 );
  rb_define_method( cPathResolverHop, "in_port_no", pathresolver_hop_in_port_no, 0 );
  rb_define_method( cPathResolverHop, "out_port_no", pathresolver_hop_out_port_no, 0 );

  rb_define_method( mPathResolverClient, "init_path_resolver_client", init_path_resolver_client, 0 );
  rb_define_method( mPathResolverClient, "finalize_path_resolver_client", finalize_path_resolver_client, 0 );
  rb_define_method( mPathResolverClient, "path_resolve", path_resolve, 4 );
  rb_define_method( mPathResolverClient, "handle?", is_handle, 0 );
  rb_define_method( mPathResolverClient, "update_path", update_path, 1 );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
