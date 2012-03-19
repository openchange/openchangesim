/*
   OpenChangeSim Fork model

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
extern struct ocsim_signal_context sig_ctx;
static struct ocsim_context *c = NULL;

static void sigchild_hdl(int signal, siginfo_t *siginfo, void *contex)
{
	pid_t pid;
	unsigned i;

	while ((pid = waitpid(-1, NULL, WNOHANG)) >0) {
		for (i=0; i < c->childs; i++) {
			if (c->pid[i] == pid) {
				c->active_childs--;
				if (siginfo->si_code != CLD_EXITED) {
				fprintf(stdout,
					"Process %ld exited with signal %d !!!\n",
					(long)siginfo->si_pid,
					siginfo->si_status);
				}
				break;
			}
		}
	}
}

uint32_t openchangesim_fork_process_start(struct ocsim_context *ctx, struct mapi_context *mapi_ctx, const char *server)
{
	struct ocsim_server	*el;
	pid_t			pid;
	int			i;
	int			range;
	int			index = 0;
	struct sigaction act;

	memset (&act, '\0', sizeof(act));
	act.sa_sigaction = &sigchild_hdl;

	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGCHLD, &act, NULL) < 0) {
		perror ("sigaction");
		return OCSIM_ERROR;
	}

	el = configuration_validate_server(ctx, server);
	OCSIM_RETVAL_IF(!el, OCSIM_ERROR, OCSIM_INVALID_SERVER, NULL);

	range = el->range_end - el->range_start;

	ctx->pid = talloc_array(ctx, pid_t, range);

	ctx->childs = 0;
	ctx->active_childs = 0;
	c = ctx;

	index = el->range_start;
	for (i = 0; i < range; i++) {
		pid = fork();
		if (pid > 0) {
			ctx->childs++;
			ctx->active_childs++;
			ctx->pid[i] = pid;
		} else {
			if (pid == 0) {
				TALLOC_CTX	*mem_ctx;
				char		*profname;

				/* Mark interfaces deregistered in the child so that we don't try to
				 * deregister them multiple times
				 */
				sig_ctx.interface_deregistered = true;
				index += i;

				mem_ctx = talloc_named(NULL, 0, "fork");
				profname = talloc_asprintf(mem_ctx, PROFNAME_TEMPLATE_NB,
							   el->name, el->generic_user, index, el->realm);
				openchangesim_modules_run(ctx, mapi_ctx, profname);
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
	int			range;

	el = configuration_validate_server(ctx, server);
	OCSIM_RETVAL_IF(!el, OCSIM_ERROR, OCSIM_INVALID_SERVER, NULL);

	range = el->range_end - el->range_start;

	while(ctx->active_childs);
	return OCSIM_SUCCESS;
}
