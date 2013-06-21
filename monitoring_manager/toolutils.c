
#include <string.h>
#include "toolutils.h"

static char static_match_str[ 256 ];
static char static_actions_str[ 256 ]; 

char* get_static_match_str( struct ofp_match *match ) {
  memset( static_match_str, '\0', sizeof( static_match_str ) );                                                                                                                                
  match_to_string( match, static_match_str, sizeof( static_match_str ) );
  return static_match_str;
}

char* get_static_actions_str( struct ofp_flow_stats *stats ) {
  memset( static_actions_str, '\0', sizeof( static_actions_str ) );          
  uint16_t actions_length = (uint16_t) (stats->length - offsetof( struct ofp_flow_stats, actions ));
  if ( actions_length > 0 ) {
    actions_to_string( stats->actions, actions_length, static_actions_str, sizeof( static_actions_str ) );                                                                                     
  } 
  return static_actions_str;
}