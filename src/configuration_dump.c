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
		DEBUG(0, ("\t\t version\t\t= %-30d\n", el->version));
		DEBUG(0, ("\t\t address\t\t= %s\n", el->address));
		DEBUG(0, ("\t\t domain\t\t\t= %s\n", el->domain));
		DEBUG(0, ("\t\t realm\t\t\t= %s\n", el->realm));
		DEBUG(0, ("\t\t generic user\t\t= %s\n", el->generic_user ? el->generic_user :
			  "no user supplied (!!!WARNING!!!)"));
		if (el->range == true) {
			DEBUG(0, ("\t\t generic user range\t= from %d to %d\n",
				  el->range_start, el->range_end));
		} else {
			DEBUG(0, ("\t\t generic user range\t= none - single user\n"));
		}
		DEBUG(0, ("\t\t generic password\t= %s\n", 
			  el->generic_password ? el->generic_password : 
			  "no password supplied (!!!WARNING!!!)"));
		DEBUG(0, ("\t}\n\n"));
	}
	DEBUG(0, ("}\n"));

	return OCSIM_SUCCESS;
}


/**
   \details Dump available servers list from OpenChangeSim configuration file

   \param ctx pointer to the OpenChangeSim context

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
_PUBLIC_ int configuration_dump_servers_list(struct ocsim_context *ctx)
{
	struct ocsim_server	*el;
	uint32_t		user_nb = 0;

	/* Sanity checks */
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	OCSIM_RETVAL_IF(!ctx->servers, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);

	DEBUG(0, ("Available servers:\n"));
	for (el = ctx->servers; el->next; el = el->next) {
		if (el->range == false) {
			if (el->generic_user && el->generic_password) {
				DEBUG(0, ("\t* %s (1 user)\n", el->name));
			} else {
				DEBUG(0, ("\t* %s (0 user ... !!!WARNING!!!)\n", el->name));
			}
		} else {
			user_nb = el->range_end - el->range_start + 1; /*including 1st record */
			DEBUG(0, ("\t* %s (%d users)\n", el->name, user_nb));
		}
	}

	return OCSIM_SUCCESS;
}


_PUBLIC_ int configuration_dump_scenarios(struct ocsim_context *ctx)
{
	struct ocsim_scenario		*el;
	struct ocsim_scenario_case	*elc;
	struct ocsim_scenario_sendmail	*sendmail;
	int				i;
	int				j;

	/* Sanity checks */
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	OCSIM_RETVAL_IF(!ctx->scenarios, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);

	for (el = ctx->scenarios; el->next; el = el->next) {
		DEBUG(0, ("scenario %s {\n", el->name));
		DEBUG(0, ("\t repeat\t\t= %d\n\n", el->repeat));
		for (elc = el->cases, i = 0; elc; elc = elc->next, i++) {
			DEBUG(0, ("\t scenario case %d {\n", i));
			if (!strcasecmp(el->name, SENDMAIL_MODULE_NAME)) {
				sendmail = (struct ocsim_scenario_sendmail *) elc->private_data;
				switch (sendmail->body_type) {
				case OCSIM_BODY_NONE:
					DEBUG(0, ("\t\t body\t\t\t= GENERIC\n"));
					break;
				case OCSIM_BODY_UTF8_INLINE:
					DEBUG(0, ("\t\t body\t\t\t= INLINE UTF8\n"));
					DEBUG(0, ("\t\t content = %s\n", sendmail->body_inline));
					break;
				case OCSIM_BODY_HTML_INLINE:
					DEBUG(0, ("\t\t body\t\t\t= INLINE HTML\n"));
					DEBUG(0, ("\t\t content = %s\n", sendmail->body_inline));
					break;
				case OCSIM_BODY_UTF8_FILE:
					DEBUG(0, ("\t\t body\t\t\t= UTF8 FILE\n"));
					DEBUG(0, ("\t\t filename\t\t= %s\n", sendmail->body_file));
					break;
				case OCSIM_BODY_HTML_FILE:
					DEBUG(0, ("\t\t body\t\t\t= HTML FILE\n"));
					DEBUG(0, ("\t\t filename\t\t= %s\n", sendmail->body_file));
					break;
				case OCSIM_BODY_RTF_FILE:
					DEBUG(0, ("\t\t body\t\t\t= RTF FILE\n"));
					DEBUG(0, ("\t\t filename\t\t= %s\n", sendmail->body_file));
					break;
				}
				DEBUG(0, ("\t\t attachments\t\t= %d\n", sendmail->attachment_count));
				for (j = 0; j < sendmail->attachment_count; j++) {
					DEBUG(0, ("\t\t attachment\t\t= %s\n", sendmail->attachments[j]));
				}
			}
			DEBUG(0, ("\t };\n\n"));
		}
		DEBUG(0, ("}\n"));
	}

	return OCSIM_SUCCESS;
}


/**
   \details Ensure the specified server exists within the
   configuration, is valid for further processing and return a pointer
   on the server entry if it meets requirements.

   \param ctx pointer to the OpenChangeSim context
   \param server pointer to the server to use

   \return valid pointer on ocsim_server entry on success, otherwise NULL
 */
struct ocsim_server *configuration_validate_server(struct ocsim_context *ctx, 
						   const char *server)
{
	struct ocsim_server	*el;

	/* Sanity checks */
	if (!ctx || !ctx->servers || !server) {
		return NULL;
	}

	for (el = ctx->servers; el->next; el = el->next) {
		if (el->name && !strcmp(el->name, server)) {
			if (el->generic_user && el->generic_password) {
				return el;
			} else {
				return NULL;
			}
		}
	}

	return NULL;
}


/**
   \details Ensure the specified scenario exists within the
   configuration, is valid for further processing and return a pointer
   on the scenario entry.

   \param ctx pointer to the OpenChangeSim context
   \param name scenario name to validate

   \return valid pointer on ocsim_scenario entry on success, otherwise NULL
 */
struct ocsim_scenario	*configuration_validate_scenario(struct ocsim_context *ctx,
							 const char *scenario)
{
	struct ocsim_scenario	*el = NULL;

	/* Sanity checks */
	if (!ctx || !ctx->scenarios || !scenario) {
		return NULL;
	}

	for (el = ctx->scenarios; el->next; el = el->next) {
		if (el->name && !strcmp(el->name, scenario)) {
			return el;
		}
	}

	return NULL;
}
