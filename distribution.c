/*  This file is part of "mirrorer" (TODO: find better title)
 *  Copyright (C) 2003 Bernhard R. Link
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <config.h>

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <zlib.h>
#include <db.h>
#include "error.h"
#include "mprintf.h"
#include "chunks.h"
#include "sources.h"
#include "md5sum.h"
#include "dirs.h"
#include "names.h"
#include "distribution.h"

static void distribution_free(struct distribution *distribution) {
	if( distribution) {
		free(distribution->codename);
		free(distribution->suite);
		free(distribution->version);
		free(distribution->origin);
		free(distribution->label);
		free(distribution->description);
		strlist_done(&distribution->architectures);
		strlist_done(&distribution->components);
		free(distribution);
	}
}

static retvalue distribution_parse(struct distribution **distribution,const char *chunk) {
	struct distribution *r;
	retvalue ret;

	assert( chunk !=NULL && distribution != NULL );
	
	r = calloc(1,sizeof(struct distribution));
	if( !r )
		return RET_ERROR_OOM;

#define checkret		 if(!RET_IS_OK(ret)) { \
					distribution_free(r); \
					return ret; \
				}
	ret = chunk_getvalue(chunk,"Codename",&r->codename);
	checkret;
	ret = chunk_getvalue(chunk,"Suite",&r->suite);
	checkret;
	ret = chunk_getvalue(chunk,"Version",&r->version);
	checkret;
	ret = chunk_getvalue(chunk,"Origin",&r->origin);
	checkret;
	ret = chunk_getvalue(chunk,"Label",&r->label);
	checkret;
	ret = chunk_getvalue(chunk,"Description",&r->description);
	checkret;
	ret = chunk_getwordlist(chunk,"Architectures",&r->architectures);
	checkret;
	ret = chunk_getwordlist(chunk,"Components",&r->components);
	checkret;

	*distribution = r;
	return RET_OK;
}

struct distribution_filter {int count; char **dists; };

static retvalue distribution_parse_and_filter(struct distribution **distribution,const char *chunk,struct distribution_filter filter) {
	retvalue result;
	int i;
	
	result = distribution_parse(distribution,chunk);
	if( RET_IS_OK(result) ) {
		if( filter.count > 0 ) {
			i = filter.count;
			while( i-- > 0 && strcmp((filter.dists)[i],(*distribution)->codename) != 0 ) {
			}
			if( i < 0 ) {
				distribution_free(*distribution);
				*distribution = NULL;
				return RET_NOTHING;
			}
		}
	}
	return result;
}
	

/* call <sourceaction> for each source part of <distribution> and <binaction> for each binary part of it. */
retvalue distribution_foreach_part(const struct distribution *distribution,distribution_each_source_action sourceaction,distribution_each_binary_action binaction,void *data,int force) {
	retvalue result,r;
	int i,j;
	const char *arch,*comp;

	result = RET_NOTHING;
	for( i = 0 ; i < distribution->components.count ; i++ ) {
		comp = distribution->components.values[i];
		for( j = 0 ; j < distribution->architectures.count ; j++ ) {
			arch = distribution->architectures.values[j];
			if( strcmp(arch,"source") != 0 ) {
				r = binaction(data,comp,arch);
				RET_UPDATE(result,r);
				if( RET_WAS_ERROR(r) && force <= 0 )
					return result;
			}
			
		}
		r = sourceaction(data,comp);
		RET_UPDATE(result,r);
		if( RET_WAS_ERROR(r) && force <= 0 )
			return result;
	}
	return result;
}

struct dist_mydata {struct distribution_filter filter; distributionaction *action; void *data;};

static retvalue processdistribution(void *d,const char *chunk) {
	struct dist_mydata *mydata = d;
	retvalue result;
	struct distribution *distribution;

	result = distribution_parse_and_filter(&distribution,chunk,mydata->filter);
	if( RET_IS_OK(result) ){

		result = mydata->action(mydata->data,chunk,distribution);
		distribution_free(distribution);
	}

	return result;
}

retvalue distribution_foreach(const char *conf,int argc,char *argv[],distributionaction action,void *data,int force) {
	retvalue result;
	char *fn;
	struct dist_mydata mydata;

	mydata.filter.count = argc;
	mydata.filter.dists = argv;
	mydata.data = data;
	mydata.action = action;
	
	fn = calc_dirconcat(conf,"distributions");
	if( !fn ) 
		return RET_ERROR_OOM;
	
	result = chunk_foreach(fn,processdistribution,&mydata,force);

	free(fn);
	return result;
}