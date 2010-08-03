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

   \brief OpenChangeSim configuration file
 */

#include "src/openchangesim.h"

/**
   \details Dump server part of OpenChangeSim configuration

   \param ctx pointer to the OpenChangeSim context

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
_PUBLIC_ int configuration_dump_servers(struct ocsim_context *ctx)
{
	struct ocsim_server	*el;

	/* Sanity checks */
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	OCSIM_RETVAL_IF(!ctx->servers, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);

	DEBUG(0, ("server {\n"));
	for (el = ctx->servers; el->next; el = el->next) {
		DEBUG(0, ("\t%s {\n", el->name));
		DEBUG(0, ("\t\t version\t=\t%d\n", el->version));
		DEBUG(0, ("\t\t address\t=\t%s\n", el->address));
		DEBUG(0, ("\t\t domain\t=\t%s\n", el->domain));
		DEBUG(0, ("\t\t realm\t=\t%s\n", el->realm));
		DEBUG(0, ("\t}\n\n"));
	}
	DEBUG(0, ("}\n"));

	return OCSIM_SUCCESS;
}
