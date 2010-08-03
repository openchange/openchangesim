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

#define	DEFAULT_PROFDB		"%s/.openchange/openchangesim/profiles.ldb"
#define	DEFAULT_CONFFILE	"%s/.openchange/openchangesim/openchangesim.conf"

extern struct poptOption popt_openchange_version[];

#define	POPT_OPENCHANGE_VERSION { NULL, 0, POPT_ARG_INCLUDE_TABLE, popt_openchange_version, 0, "Common openchange options:", NULL },

struct ocsim_var
{
	const char		*name;
	const void		*value;
	struct ocsim_var	*prev;
	struct ocsim_var	*next;
};

struct ocsim_server
{
	const char		*name;
	const char		*address;
	const char		*domain;
	const char		*realm;
	uint32_t		version;
	struct ocsim_var	*vars;
	struct ocsim_server	*prev;
	struct ocsim_server	*next;
};

struct ocsim_context
{
	TALLOC_CTX		*mem_ctx;
	/* lexer internal data */
	struct ocsim_server	*server_el;
	unsigned int		lineno;
	int			result;
	/* ocsim */
	struct ocsim_server	*servers;
	struct ocsim_var	*options;
	/* context */
	FILE			*fp;
	const char		*filename;
};

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

/* The following public definitions come from src/openchangesim_public.c */
struct ocsim_context *openchangesim_init(TALLOC_CTX *);
int openchangesim_release(struct ocsim_context *);
int openchangesim_parse_config(struct ocsim_context *, const char *);
int openchangesim_do_debug(struct ocsim_context *, const char *, ...);

/* The following public definitions come from src/configuration_api.c */
int configuration_add_server(struct ocsim_context *, struct ocsim_server *);

/* The following public definitions come from src/configuration_dump.c */
int configuration_dump_servers(struct ocsim_context *);

void error_message (struct ocsim_context *, const char *, ...) __attribute__ ((format (printf, 2, 3)));

__END_DECLS

extern int error_flag;

#include "src/openchangesim_errors.h"

#endif	/* !__OPENCHANGESIM_H__ */
