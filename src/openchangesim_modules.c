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
	ret = module_sendmail_init(ctx);
	ret = module_fetchmail_init(ctx);

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

bool openchangesim_module_ref_count(struct ocsim_context *ctx)
{
	struct ocsim_module	*el = NULL;

	if (!ctx || !ctx->modules) return true;

	for (el = ctx->modules; el; el = el->next) {
		if (el->get_ref_count(el) > 0) {
			return true;
		}
	}

	return false;
}

/**
   \details Retrieve the counter (repeat) value for the module

   \param module pointer to the OpenChangeSim module

   \return repeat value on success, otherwise OCSIM_ERROR
 */
uint32_t module_get_ref_count(struct ocsim_module *module)
{
	struct ocsim_scenario	*scenario;

	if (!module) return OCSIM_ERROR;

	scenario = (struct ocsim_scenario *)module->scenario;
	if (!scenario) return OCSIM_ERROR;

	return scenario->repeat;
}

/**
   \details Retrieve the counter (repeat) value for the module

   \param module pointer to the OpenChangeSim module
   \param incr the incrementer value to use

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
uint32_t module_set_ref_count(struct ocsim_module *module, int incr)
{
	struct ocsim_scenario	*scenario;

	if (!module) return OCSIM_ERROR;

	scenario = (struct ocsim_scenario *)module->scenario;
	if (!scenario) return OCSIM_ERROR;

	scenario->repeat += incr;
	if (scenario->repeat < 0) {
		scenario->repeat = 0;
	}

	return OCSIM_SUCCESS;
}

struct ocsim_scenario *module_get_scenario(struct ocsim_context *ctx, const char *name)
{
	struct ocsim_scenario	*el;

	for (el = ctx->scenarios; el; el = el->next) {
		if (el->name && !strncasecmp(el->name, name, strlen(name))) {
			return el;
		}
	}

	return NULL;
}

struct ocsim_scenario_case *module_get_scenario_data(struct ocsim_context *ctx, const char *name)
{
	struct ocsim_scenario	*el;

	for (el = ctx->scenarios; el; el = el->next) {
		if (el->name && !strncasecmp(el->name, name, strlen(name))) {
			return el->cases;
		}
	}

	return NULL;
}

uint32_t openchangesim_modules_run(struct ocsim_context *ctx, struct mapi_context *mapi_ctx, char *profname)
{
	TALLOC_CTX		*mem_ctx;
	struct mapi_session	*session;
	struct ocsim_module	*el = NULL;
	enum MAPISTATUS 	retval;

	mem_ctx = talloc_named(NULL, 0, "openchangesim_modules_run");
	session = talloc_zero(mem_ctx, struct mapi_session);
	if (!mem_ctx) {
		DEBUG(0, ("No more memory available\n"));
		return OCSIM_ERROR;
	}

	do {
		for (el = ctx->modules; el; el = el->next) {
			retval = MapiLogonEx(mapi_ctx, &session, profname, NULL);
			if (retval) {
				openchangesim_log_string("Opening session for %s failed", profname);
				return OCSIM_ERROR;
			}
			if (el->get_ref_count(el) > 0) {
				el->run(ctx, el->cases, session);
				el->set_ref_count(el, -1);
			}
		}
	} while (openchangesim_module_ref_count(ctx) == true);

	retval = MapiLogonEx(mapi_ctx, &session, profname, NULL);
	if (retval) {
		openchangesim_log_string("Opening session for %s failed", profname);
		return OCSIM_ERROR;
	}

	module_cleanup_run(ctx, session);
	talloc_free(mem_ctx);

	return OCSIM_SUCCESS;
}
