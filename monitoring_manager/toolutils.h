/* 
 * File:   toolutils.h
 * Author: liudanny
 *
 * Created on July 30, 2012, 9:50 AM
 */

#ifndef TOOLUTILS_H
#define	TOOLUTILS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "trema.h"

char* get_static_match_str( struct ofp_match *match );
char* get_static_actions_str( struct ofp_flow_stats *stats );

#ifdef	__cplusplus
}
#endif

#endif	/* TOOLUTILS_H */

