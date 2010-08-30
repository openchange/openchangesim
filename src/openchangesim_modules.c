/*
   OpenChangeSim modules API

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

#include "src/openchangesim.h"

uint32_t openchangesim_register_modules(struct ocsim_context *ctx)
{
	uint32_t	ret;

	DEBUG(0, (DEBUG_FORMAT_STRING, "Initializing modules"));
	ret = module_fetchmail_init(ctx);
	ret = module_sendmail_init(ctx);

	return ret;
}

/**
   \details Register an openchangesim module
   
   \param ctx pointer to the openchangesim context
   \param module the module we want to add

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
uint32_t openchangesim_module_register(struct ocsim_context *ctx,
				       struct ocsim_module *module)
{
	struct ocsim_module	*el = NULL;

	/* Sanity checks */
	if (!ctx || !ctx->mem_ctx) return OCSIM_ERROR;
	if (!module) return OCSIM_ERROR;

	/* Ensure the name is not yet registered */
	for (el = ctx->modules; el; el = el->next) {
		if (el->name && !strcmp(el->name, module->name)) {
			DEBUG(0, (DEBUG_FORMAT_STRING_MODULE_ERR, module->name, "Module already loaded"));
			return OCSIM_ERROR;
		}
	}

	DLIST_ADD_END(ctx->modules, module, struct ocsim_module *);
	DEBUG(0, (DEBUG_FORMAT_STRING_MODULE, module->name, "Module loaded"));

	return OCSIM_SUCCESS;
}

struct ocsim_module *openchangesim_module_init(struct ocsim_context *ctx,
					       const char *name,
					       const char *description)
{
	struct ocsim_module	*module = NULL;

	/* Sanity checks */
	if (!ctx || !ctx->mem_ctx) return NULL;
	if (!name) return NULL;

	module = talloc_zero(ctx->mem_ctx, struct ocsim_module);
	module->name = talloc_strdup((TALLOC_CTX *)module, name);
	if (!description) {
		module->description = NULL;
	} else {
		module->description = talloc_strdup((TALLOC_CTX *)module, description);
	}

	return module;
}

uint32_t openchangesim_modules_run(struct ocsim_context *ctx, char *profname)
{
	int			i;
	TALLOC_CTX		*mem_ctx;
	enum MAPISTATUS		retval;
	struct mapi_session	*session = NULL;

	mem_ctx = talloc_named(NULL, 0, "openchangesim_modules_run");
	if (!mem_ctx) {
		DEBUG(0, ("No more memory available\n"));
		return OCSIM_ERROR;
	}

	retval = MapiLogonEx(&session, profname, NULL);

	for (i = 0; i < 3000; i++) {
		retval = module_fetchmail_run(ctx, session);
		if (retval) {
			DEBUG(0, ("Error in child %d: module fetchmail: 0x%x\n", getpid(), retval));
		}
		retval = module_sendmail_run(ctx, session);
		if (retval) {
			DEBUG(0, ("Error in child: module sendmail\n"));
		}
	}

	return OCSIM_SUCCESS;
}
