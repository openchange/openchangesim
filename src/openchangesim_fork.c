/*
   OpenChangeSim Fork modele

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

#include <sys/types.h>
#include <sys/wait.h>

#include "src/openchangesim.h"

uint32_t openchangesim_fork_process_start(struct ocsim_context *ctx, const char *server)
{
	struct ocsim_server	*el;
	pid_t			pid;
	int			i;
	int			range;
	int			index = 0;

	el = configuration_validate_server(ctx, server);
	OCSIM_RETVAL_IF(!el, OCSIM_ERROR, OCSIM_INVALID_SERVER, NULL);

	range = el->range_end - el->range_start;

	ctx->pid = talloc_array(ctx, pid_t, range);

	index = el->range_start;
	for (i = 0; i < range; i++) {
		pid = fork();
		if (pid > 0) {
			ctx->pid[i] = pid;
		} else {
			if (pid == 0) {
				TALLOC_CTX	*mem_ctx;
				char		*profname;

				index += i;

				mem_ctx = talloc_named(NULL, 0, "fork");
				profname = talloc_asprintf(mem_ctx, PROFNAME_TEMPLATE_NB,
							   el->name, el->generic_user, index, el->realm);
				openchangesim_modules_run(ctx, profname);
				talloc_free(mem_ctx);
				exit (0);
			} else {
				DEBUG(0, ("Fork Problem detected"));
			}
		}
	}

	return OCSIM_SUCCESS;
}

uint32_t openchangesim_fork_process_end(struct ocsim_context *ctx, const char *server)
{
	struct ocsim_server	*el;
	int			i;
	int			range;
	int			status;

	el = configuration_validate_server(ctx, server);
	OCSIM_RETVAL_IF(!el, OCSIM_ERROR, OCSIM_INVALID_SERVER, NULL);

	range = el->range_end - el->range_start;

	for (i = 0; i < range; i++) {
		waitpid(ctx->pid[i], &status, 0);
		DEBUG(0, ("In father: pid %d finished\n", ctx->pid[i]));
		ctx->pid[i] = 0;
	}
	return OCSIM_SUCCESS;
}
