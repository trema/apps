/*
 * Author: Yasunobu Chiba
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
#include "authenticator.h"
#include "trema.h"


/*
 * A Wrapper class to authentication function. 
 */
VALUE cAuthenticator;
static uint8_t initialized = 0;


/*
 * Authenticates a given Ethernet address against a list of known addresses.
 *
 * @param [Mac] mac an Ethernet address represented as an instance of the 
 *   class Trema::Mac.
 *
 * @return [Boolean] true if Ethernet address is found otherwise false.
 */
static VALUE
rb_authenticate_mac( VALUE self, VALUE mac ) {
  UNUSED( self );

  VALUE mac_arr = rb_funcall( mac, rb_intern( "to_short" ), 0 );
  int i;
  uint8_t haddr[ OFP_ETH_ALEN ];
  for ( i = 0; i < RARRAY_LEN( mac_arr ); i++ ) {
    haddr[ i ] = ( uint8_t ) ( NUM2INT( RARRAY_PTR( mac_arr )[ i ] ) );
  }
  if ( !authenticate( haddr ) ) {
    return Qfalse;
  }
  return Qtrue;
}


/*
 * Calls the authenticator initializer to load an authorized host SQLite 
 * database file into memory. The table name is authorized_host with
 * two fields mac and description. The mac is an unsigned bigint whilst
 * description is a text field. if the SQLite database file can not be 
 * loaded it tries continously to do so but no error is returned.
 *
 * @param [String] file 
 *   the authorized host database file.
 */
static VALUE
rb_init_authenticator( VALUE self, VALUE file ) {
  if ( !initialized ) {
    ( void )init_authenticator( StringValuePtr( file ) );
    initialized = 1;
  }
  else {
    info( "Authenticator already initialized" );
  }
  return self;
}


/*
 * Releases memory occuppied previously from the loading of the SQLite database
 * file. After this call initializer can be invoked again if desired.
 *
 */
static VALUE
rb_finalize_authenticator( VALUE self ) {
  if ( finalize_authenticator() ) {
    initialized = 0;
  }
  return self;
}


void
Init_authenticator() {
  rb_require( "singleton" );
  cAuthenticator = rb_define_class( "Authenticator", rb_cObject );
  // singleton so only one authenticator object can be created.
  rb_funcall( rb_const_get( rb_cObject, rb_intern( "Singleton") ), rb_intern( "included" ), 1, cAuthenticator );
  rb_define_method( cAuthenticator, "init", rb_init_authenticator, 1 );
  rb_define_method( cAuthenticator, "finalize", rb_finalize_authenticator, 0 );
  rb_define_method( cAuthenticator, "authenticate_mac", rb_authenticate_mac, 1 );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
