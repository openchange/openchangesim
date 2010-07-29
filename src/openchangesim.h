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

#ifndef	__OPENCHANGESIM_H__
#define	__OPENCHANGESIM_H__

#include <libmapi/libmapi.h>
#include <popt.h>
#include "src/version.h"

#define	DEFAULT_PROFDB	"%s/.openchange/profiles.ldb"

extern struct poptOption popt_openchange_version[];

#define	POPT_OPENCHANGE_VERSION { NULL, 0, POPT_ARG_INCLUDE_TABLE, popt_openchange_version, 0, "Common openchange options:", NULL },

#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS		extern "C" {
#define __END_DECLS		}
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

#ifndef	_PUBLIC_
#define	_PUBLIC_
#endif

__BEGIN_DECLS
__END_DECLS

#endif	/* !__OPENCHANGESIM_H__ */
