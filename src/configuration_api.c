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

	el->ip_start = talloc_steal(el, server->ip_start);
	el->ip_end = talloc_steal(el, server->ip_end);

	el->ip_number = server->ip_number;

	DLIST_ADD_END(ctx->servers, el, struct ocsim_server *);

	return OCSIM_SUCCESS;
}


/**
   \details Add a scenario parsed from configuration file to the list
   of available scenarios

   \param ctx pointer to the openchangesim context
   \param gscenario pointer to the current generic scenario record to
   be added

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
_PUBLIC_ int configuration_add_scenario(struct ocsim_context *ctx,
					struct ocsim_generic_scenario *gscenario)
{
	struct ocsim_scenario	*el;
	int			i;

	/* Sanity checks */
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	OCSIM_RETVAL_IF(!gscenario, OCSIM_ERROR, OCSIM_INVALID_PARAMETER, NULL);

	if (!gscenario->name) {
		DEBUG(0, (DEBUG_FORMAT_STRING_ERR, DEBUG_ERR_MISSING_NAME));
		return OCSIM_ERROR;
	}

	/* Ensure the scenario has not already been added */
	if (configuration_validate_scenario(ctx, gscenario->name)) {
		DEBUG(0, (DEBUG_FORMAT_STRING_MODULE_ERR, gscenario->name, DEBUG_ERR_DUPLICATE));
		return OCSIM_ERROR;
	}
	

	if (!strcasecmp(gscenario->name, SENDMAIL_MODULE_NAME)) {
		struct ocsim_scenario_sendmail		*sendmail;
		struct ocsim_scenario_case		*element;
		struct ocsim_generic_scenario_case	*elm;

		el = talloc_zero(ctx->mem_ctx, struct ocsim_scenario);
		el->cases = NULL;
		el->name = talloc_strdup(el, gscenario->name);
		el->repeat = gscenario->repeat;

		for (elm = gscenario->case_el; elm; elm = elm->next) {
			element = talloc_zero(el, struct ocsim_scenario_case);
			sendmail = talloc_zero(element, struct ocsim_scenario_sendmail);
			
			sendmail->body_type = elm->body_type;
			switch (sendmail->body_type) {
			case OCSIM_BODY_NONE:
				sendmail->body_inline = NULL;
				sendmail->body_file = NULL;
				break;
			case OCSIM_BODY_UTF8_INLINE:
			case OCSIM_BODY_HTML_INLINE:
				sendmail->body_inline = talloc_strdup(sendmail, elm->body_inline);
				break;
			case OCSIM_BODY_UTF8_FILE:
			case OCSIM_BODY_HTML_FILE:
			case OCSIM_BODY_RTF_FILE:
				sendmail->body_file = talloc_strdup(sendmail, elm->body_file);
				break;
			}
			sendmail->attachment_count = elm->attachment_count;
			sendmail->attachments = talloc_array(sendmail, char *, sendmail->attachment_count + 2);
			for (i = 0; i < sendmail->attachment_count; i++) {
				sendmail->attachments[i] = talloc_strdup(sendmail->attachments, elm->attachments[i]);
			}
			element->private_data = (void *) sendmail;
			DLIST_ADD_END(el->cases, element, struct ocsim_scenario_case);
		}		

		DLIST_ADD_END(ctx->scenarios, el, struct ocsim_scenario *);

	} else if (!strcasecmp(gscenario->name, FETCHMAIL_MODULE_NAME)) {
		el = talloc_zero(ctx->mem_ctx, struct ocsim_scenario);
		el->cases = NULL;
		el->name = talloc_strdup(el, gscenario->name);
		el->repeat = gscenario->repeat;

		DLIST_ADD_END(ctx->scenarios, el, struct ocsim_scenario *);
	}

	return OCSIM_SUCCESS;
}


_PUBLIC_ int configuration_add_generic_scenario_case(struct ocsim_generic_scenario *gscenario,
						     struct ocsim_generic_scenario_case *gcase)
{
	struct ocsim_generic_scenario_case	*el;
	int					i;

	el = talloc_zero(gscenario, struct ocsim_generic_scenario_case);
	el->body_type = gcase->body_type;

	switch (el->body_type) {
	case OCSIM_BODY_NONE:
		el->body_inline = NULL;
		el->body_file = NULL;
		break;
	case OCSIM_BODY_UTF8_INLINE:
	case OCSIM_BODY_HTML_INLINE:
		el->body_inline = talloc_strdup(el, gcase->body_inline);
		break;
	case OCSIM_BODY_UTF8_FILE:
	case OCSIM_BODY_HTML_FILE:
	case OCSIM_BODY_RTF_FILE:
		el->body_file = talloc_strdup(el, gcase->body_file);
		break;
	}

	el->attachment_count = gcase->attachment_count;
	el->attachments = talloc_array(el, char *, gcase->attachment_count + 2);
	for (i = 0; i < el->attachment_count; i++) {
		el->attachments[i] = talloc_strdup(el->attachments, gcase->attachments[i]);
	}

	DLIST_ADD_END(gscenario->case_el, el, struct ocsim_generic_scenario_case);

	return OCSIM_SUCCESS;
}

/**
   \details Split an IP address represented as a string into an array
   of uint8_t

   \param mem_ctx pointer to the memory context
   \param ip_address the ip address to analyze

   \return pointer to the uint8_t array on success, otherwise NULL
*/
uint8_t	*configuration_get_ip(TALLOC_CTX *mem_ctx, const char *ip_address)
{
	int		i;
	int		index;
	int		pos;
	char		*tmp;
	uint8_t		*ip;

	ip = talloc_array(mem_ctx, uint8_t, 4);
	for (i = 0, pos = 0, index = 0; i < strlen(ip_address); i++) {
		if (ip_address[i] == '.') {
			tmp = talloc_strndup(mem_ctx, &ip_address[pos], i - pos);
			ip[index] = atoi(tmp);
			if (ip[index] >= 255) {
				ip = NULL;
				goto end;
			}
			pos = i + 1;
			index++;
			talloc_free(tmp);
		}
		if (index == 3) {
			break;
		}
	}
	tmp = talloc_strdup(mem_ctx, &ip_address[pos]);
	ip[index] = atoi(tmp);
end:
	talloc_free(tmp);

	return ip;
}

/**
   \details Calculate the available number of IP within the start-end
   range.

   \param start pointer to the uint8 array with starting IP address
   \param end pointer to the uint8 array with ending IP address

   \return number of available IP address
 */
uint32_t configuration_get_ip_count(uint8_t *start, uint8_t *end)
{
	int	diff;
	int	count = 0;

	/* sanity checks */
	if (start[0] > end[0]) return 0;
	if (start[1] > end[1]) return 0;

	/* X.X.X.X 
	   | | | |_ 255
	   | | |___ 65.025
	   | |_____ 16.581.375
	   |_______ 4.228.250.625

	   The tool will only supports a maximum of 5.000 concurrent
	   clients. So we assume (for convenience purposes) that we
	   will have a class B network max.
	 */
	diff = end[2] - start[2];
	if (diff) {
		/* This is a class B */
		count = 254 * (diff + 1);
		count -= 254 - start[3];
		count -= end[3] + 1;
		
		return count;
	}

	/* This is a class C */
	diff = end[3] - start[3];
	if (diff) {
		return diff + 1;
	}

	return 0;
}
