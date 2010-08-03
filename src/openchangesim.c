/*
   OpenChangeSim main application file

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

int main(int argc, const char *argv[])
{
	TALLOC_CTX		*mem_ctx;
	struct ocsim_context	*ctx;
	int			ret;
	enum MAPISTATUS		retval;
	poptContext		pc;
	int			opt;
	bool			opt_dumpdata = false;
	bool			opt_confcheck = false;
	bool			opt_confdump = false;
	const char		*opt_profdb = NULL;
	const char		*opt_debug = NULL;
	const char		*opt_conf_file = NULL;

	enum { OPT_PROFILE_DB=1000, OPT_DEBUG, OPT_DUMPDATA, OPT_VERSION,
	       OPT_CONFIG, OPT_CONFCHECK, OPT_CONFDUMP };

	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "database", 'f', POPT_ARG_STRING, NULL, OPT_PROFILE_DB, "set the profile database path", NULL },
		{ "debuglevel", 'd', POPT_ARG_STRING, NULL, OPT_DEBUG, "set debug level", NULL },
		{ "dump-data", 0, POPT_ARG_NONE, NULL, OPT_DUMPDATA, "dump hexadecimal data", NULL },
		{ "version", 'V', POPT_ARG_NONE, NULL, OPT_VERSION, "Print version ", NULL },
		{ "config", 'c', POPT_ARG_STRING, NULL, OPT_CONFIG, "Specify configuration file", NULL },
		{ "confcheck", 0, POPT_ARG_NONE, NULL, OPT_CONFCHECK, "Check/Validate configuration", NULL },
		{ "confdump", 0, POPT_ARG_NONE, NULL, OPT_CONFDUMP, "Dump configuration", NULL },
		{ NULL, 0, 0, NULL, 0, NULL, NULL }
	};

	mem_ctx = talloc_named(NULL, 0, "openchangesim");

	/* Step 1. Process command line options */
	pc = poptGetContext("openchangesim", argc, argv, long_options, 0);
	while ((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		case OPT_PROFILE_DB:
			opt_profdb = poptGetOptArg(pc);
			break;
		case OPT_DEBUG:
			opt_debug = poptGetOptArg(pc);
			break;
		case OPT_DUMPDATA:
			opt_dumpdata = true;
			break;
		case OPT_VERSION:
			printf("Version %s\n", OPENCHANGESIM_VERSION_STRING);
			exit (0);
		case OPT_CONFIG:
			opt_conf_file = poptGetOptArg(pc);
			break;
		case OPT_CONFCHECK:
			opt_confcheck = true;
			break;
		case OPT_CONFDUMP:
			opt_confdump = true;
			break;
		}
	}

	/* OpenChangeSim initialization */
	ctx = openchangesim_init(mem_ctx);
	ret = openchangesim_parse_config(ctx, opt_conf_file);

	/* confcheck work case */
	if (opt_confcheck) {
		if (ret) {
			DEBUG(0, ("Configuration File NOT OK!\n"));
		} else {
			DEBUG(0, ("Configuration OK\n"));
		}
		openchangesim_release(ctx);
		exit (0);
	}

	/* confdump work case */
	if (opt_confdump) {
		configuration_dump_servers(ctx);
		openchangesim_release(ctx);
		exit (0);
	}

	/* Step 2. Sanity check on options */
	if (!opt_profdb) {
		opt_profdb = talloc_asprintf(mem_ctx, DEFAULT_PROFDB, getenv("HOME"));
	}

	/* Step 3. Initialize MAPI subsystem */
	retval = MAPIInitialize(opt_profdb);
	if (retval != MAPI_E_SUCCESS) {
		mapi_errstr("MAPIInitialize", GetLastError());
		exit (1);
	}

	/* Step 4. Set debug options */
	SetMAPIDumpData(opt_dumpdata);
	if (opt_debug) {
		SetMAPIDebugLevel(atoi(opt_debug));
	}

	/* Uninitialize MAPI subsystem */
	poptFreeContext(pc);
	MAPIUninitialize();
	talloc_free(mem_ctx);

	return 0;
}
