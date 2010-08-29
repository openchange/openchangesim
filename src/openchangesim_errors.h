/*
   OpenChangeSim

   OpenChange Project

   Copyright (C) Julien Kerihuel 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef	__OPENCHANGESIM_ERRORS_H__
#define	__OPENCHANGESIM_ERRORS_H__

#define	OCSIM_SUCCESS	0
#define	OCSIM_ERROR	-1

#define	OCSIM_WARN(c,x) (openchangesim_do_debug(c, x))

#define	OCSIM_RETVAL_CONF_IF(x, c, msg, mem_ctx)  	      	\
do {								\
	if (x) {						\
		openchangesim_do_debug(c, "%s", msg);		\
		if (mem_ctx) {					\
			talloc_free(mem_ctx);			\
		}						\
		return OCSIM_ERROR;    				\
	}							\
} while (0);

#define	OCSIM_RETVAL_IF(x, t, msg, mem_ctx)			\
do {								\
	if (x) {						\
		if (msg) {					\
			DEBUG(0, ("%s\n", msg));		\
		}						\
		if (mem_ctx) {					\
			talloc_free(mem_ctx);			\
		}						\
		return t;					\
	}							\
} while (0);

#define	OCSIM_INITIALIZED		"OpenChangeSim context has already been initialized"
#define	OCSIM_NOT_INITIALIZED		"OpenChangeSim context has not been initialized"
#define	OCSIM_INVALID_PARAMETER		"Invalid parameter"
#define	OCSIM_INVALID_SERVER		"Invalid server supplied"
#define	OCSIM_MEMORY_ERROR		"Not enough resources for memory allocation"
#define	OCSIM_CONF_ERROR		"Error in configuration file"

#endif /* ! __OPENCHANGESIM_ERRORS_H__ */
