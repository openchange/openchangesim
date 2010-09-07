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

/**
   \file openchangesim_public.c

   \brief OpenChangeSim public API
 */

#include "src/openchangesim.h"

/* The following public definitions come from flex/bison */
int	ocsim_yylex_init(void *);
int	ocsim_yylex_init_extra(struct ocsim_context *, void *);
void	ocsim_yyset_in(FILE *, void *);
int	ocsim_yylex_destroy(void *);
int	ocsim_yyparse(struct ocsim_context *, void *);

int			error_flag;


/**
   \details Initialize OpenChangeSim context
   
   \param mem_ctx pointer to the memory context

   \return Allocated ocsim_context structure on success, otherwise
   NULL
 */
_PUBLIC_ struct ocsim_context *openchangesim_init(TALLOC_CTX *mem_ctx)
{
	struct ocsim_context	*ctx;

	ctx = talloc_zero(mem_ctx, struct ocsim_context);
	OCSIM_RETVAL_IF(!ctx, NULL, OCSIM_MEMORY_ERROR, NULL);

	ctx->mem_ctx = mem_ctx;
	ctx->lineno = 1;

	ctx->servers = talloc_zero(mem_ctx, struct ocsim_server);
	OCSIM_RETVAL_IF(!ctx->servers, NULL, OCSIM_MEMORY_ERROR, NULL);

	ctx->scenarios = talloc_zero(mem_ctx, struct ocsim_scenario);
	OCSIM_RETVAL_IF(!ctx->scenarios, NULL, OCSIM_MEMORY_ERROR, NULL);

	ctx->options = talloc_zero(mem_ctx, struct ocsim_var);
	OCSIM_RETVAL_IF(!ctx->options, NULL, OCSIM_MEMORY_ERROR, NULL);

	return ctx;
}


/**
   \details Uninitialize OpenChangeSim context

   Uninitialize the OpenChangeSim context and release memory.

   \param ctx pointer to the OpenChangeSim context

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR

   \sa openchangesim_init
 */
_PUBLIC_ int openchangesim_release(struct ocsim_context *ctx)
{
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);

	talloc_free(ctx);
	ctx = NULL;

	return OCSIM_SUCCESS;
}


/**
   \details Parse OpenChangeSim configuration file

   \param ctx pointer to the OpenChangeSim context
   \param filename pointer to the OpenChangeSim configuration file

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
_PUBLIC_ int openchangesim_parse_config(struct ocsim_context *ctx, const char *filename)
{
	TALLOC_CTX	*mem_ctx;
	int		ret;
	void		*scanner;

	/* Sanity Checks */
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);

	mem_ctx = talloc_named(NULL, 0, "openchangesim_parse_config");

	if (!filename) {
		filename = talloc_asprintf(mem_ctx, DEFAULT_CONFFILE, getenv("HOME"));
	}

	/* Open configuration file */
	ctx->fp = fopen(filename, "r");
	if (!ctx->fp) {
		DEBUG(0, ("Invalid Filename: %s\n", filename));
		talloc_free(mem_ctx);
		return OCSIM_ERROR;
	}

	ret = ocsim_yylex_init(&scanner);
	OCSIM_RETVAL_IF(ret, OCSIM_ERROR, OCSIM_MEMORY_ERROR, NULL);
	ret = ocsim_yylex_init_extra(ctx, &scanner);
	OCSIM_RETVAL_IF(ret, OCSIM_ERROR, OCSIM_MEMORY_ERROR, NULL);
	ocsim_yyset_in(ctx->fp, scanner);
	ret = ocsim_yyparse(ctx, scanner);
	OCSIM_RETVAL_IF(ret, OCSIM_ERROR, OCSIM_CONF_ERROR, NULL);
	ocsim_yylex_destroy(scanner);

	talloc_free(mem_ctx);

	return ret;
}

/**
   \details Debug function for OpenChangeSim

   \param ctx pointer to the OpenChangeSim context
   \param format format string

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
_PUBLIC_ int openchangesim_do_debug(struct ocsim_context *ctx, const char *format, ...)
{
	va_list ap;
	char	*s = NULL;
	int	ret;

	va_start(ap, format);
	ret = vasprintf(&s, format, ap);
	va_end(ap);

	if (ret == -1) {
		printf("%s:%d: [Debug dump failure]\n", ctx->filename, ctx->lineno);
		fflush(0);
		return OCSIM_ERROR;
	}
	if (ctx) {
		printf("%s:%d: %s\n", ctx->filename, ctx->lineno, s);
		fflush(0);
	} else {
		printf("%s\n", s);
		fflush(0);
	}
	free(s);

	return OCSIM_SUCCESS;
}
