/*
   OpenChangeSim Configuration File

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

/**
   \file configuration_dump.c

   \brief OpenChangeSim configuration API
 */

#include "src/openchangesim.h"

/**
   \details Add a server parsed from configuration file to the list of
   available servers

   \param ctx pointer to the openchangesim context
   \param server pointer to the current server record to be added

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
_PUBLIC_ int configuration_add_server(struct ocsim_context *ctx, 
				      struct ocsim_server *server)
{
	struct ocsim_server	*el;

	/* Sanity checks */
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	OCSIM_RETVAL_IF(!server, OCSIM_ERROR, OCSIM_INVALID_PARAMETER, NULL);

	el = talloc_zero(ctx->mem_ctx, struct ocsim_server);
	el->name = talloc_strdup(el, server->name);
	el->version = server->version;
	el->address = talloc_strdup(el, server->address);
	el->domain = talloc_strdup(el, server->domain);
	el->realm = talloc_strdup(el, server->realm);
	el->generic_user = talloc_strdup(el, server->generic_user);
	el->generic_password = talloc_strdup(el, server->generic_password);
	el->range = server->range;
	if (el->range == true) {
		el->range_start = server->range_start;
		el->range_end = server->range_end;
	} else {
		el->range_start = 0;
		el->range_end = 0;
	}

	DLIST_ADD_END(ctx->servers, el, struct ocsim_server *);

	return OCSIM_SUCCESS;
}
