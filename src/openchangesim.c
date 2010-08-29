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

void openchangesim_printlog(FILE *logfp, char *s)
{
	FPUTS("\r", logfp);
	FPUTS(s, logfp);
	fflush(logfp);
}

uint32_t callback(struct SRowSet *rowset, void *private)
{
	uint32_t		i;
	struct SPropValue	*lpProp;
	const char		*username = (const char *)private;

	for (i = 0; i < rowset->cRows; i++) {
		lpProp = get_SPropValue_SRow(&(rowset->aRow[i]), PR_DISPLAY_NAME);
		if (lpProp && lpProp->value.lpszA && !strcmp(username, lpProp->value.lpszA)) {
			return i;
		}
	}

	return 0;
}

/**
   \details Duplicate a profile in the database

   This operation will only work in an environment where mailboxes
   have been created using a template: generic username and password,
   same storage and administrative group.

   \param mem_ctx pointer to the memory context
   \param ref_username pointer to the profile name to duplicate
   \param username of the new profile
   \param el pointer to the server element

   \return MAPI_E_SUCCESS on success, otherwise MAPI error
 */
enum MAPISTATUS openchangesim_DuplicateProfile(TALLOC_CTX *mem_ctx,
					       char *profname_src,
					       struct ocsim_server *el)
{
	FILE			*f;
	int			i;
	char			*profname_dst;
	char			*username_dst;
	enum MAPISTATUS		retval;
	struct mapi_profile	profile;
	uint32_t		profile_nb = el->range_end - el->range_start;
	char			*logstr;
	char			*ip_address;

	f = fdopen(STDOUT_FILENO, "r+");

	for (i = el->range_start + 1; i != el->range_end; i++) {
		openchangesim_interface_get_next_ip(el, false);

		profname_dst = talloc_asprintf(mem_ctx, PROFNAME_TEMPLATE_NB,
					       el->name, el->generic_user, i, el->realm);

		retval = OpenProfile(&profile, profname_dst, NULL);
		if (retval != MAPI_E_SUCCESS) {
			username_dst = talloc_asprintf(mem_ctx, PROFNAME_USER, el->generic_user, i);
			retval = DuplicateProfile(profname_src, profname_dst, username_dst);
			if (retval) {
				talloc_free(profname_dst);
				talloc_free(username_dst);
				return retval;
			}
			ip_address = talloc_asprintf(mem_ctx, "%d.%d.%d.%d", el->ip_current[0], 
						     el->ip_current[1], el->ip_current[2], el->ip_current[3]);
			mapi_profile_add_string_attr(profname_dst, "localaddress", ip_address);
			if (openchangesim_create_interface_tap(mem_ctx, el->ip_used, ip_address) < 0) {
				exit (1);
			}
			talloc_free(ip_address);
			talloc_free(username_dst);
		}

		logstr = talloc_asprintf(mem_ctx, "[*] Creating profile %d/%d: %d.%d.%d.%d %s", 
					 i, profile_nb, el->ip_current[0], el->ip_current[1], 
					 el->ip_current[2], el->ip_current[3], profname_dst);
		openchangesim_printlog(f, logstr);
		talloc_free(logstr);
		talloc_free(profname_dst);
	}

	logstr = talloc_asprintf(mem_ctx, "[*] %d User profiles ready %200s\n", profile_nb, "");
	openchangesim_printlog(f, logstr);
	talloc_free(logstr);
	printf("\n");

	return MAPI_E_SUCCESS;
}


/**
   \details Create a MAPI profile in the database

   \param mem_ctx pointer to the memory context
   \param el pointer to the server element
   \param profname pointer to the profile name string
   \param username pointer to the username string to use along with
   the profile

   \return MAPI_E_SUCCESS on success, otherwise MAPI error
 */
enum MAPISTATUS openchangesim_CreateProfile(TALLOC_CTX *mem_ctx, 
					    struct ocsim_server *el, 
					    char *profname,
					    const char *username)
{
	enum MAPISTATUS		retval;
	struct mapi_session	*session = NULL;
	char			*exchange_version_str;
	const char		*locale;
	uint32_t		cpid;
	uint32_t		lcid;
	char			*cpid_str;
	char			*lcid_str;
	char			hostname[256];
	const char		*workstation = NULL;

	retval = CreateProfile(profname, username, el->generic_password, 0);
	if (retval != MAPI_E_SUCCESS) {
		mapi_errstr("CreateProfile", GetLastError());
		return retval;
	}

	mapi_profile_add_string_attr(profname, "binding", el->address);

	gethostname(hostname, sizeof(hostname) - 1);
	hostname[sizeof(hostname) - 1] = 0;
	workstation = hostname;
	mapi_profile_add_string_attr(profname, "workstation", workstation);

	mapi_profile_add_string_attr(profname, "domain", el->domain);
	mapi_profile_add_string_attr(profname, "realm", el->realm);
	
	/* WARNING: make this generic */
	exchange_version_str = talloc_asprintf(mem_ctx, "%d", 1);
	mapi_profile_add_string_attr(profname, "exchange_version", exchange_version_str);
	talloc_free(exchange_version_str);
	mapi_profile_add_string_attr(profname, "seal", "false");
	
	locale = mapi_get_system_locale();
	cpid = mapi_get_cpid_from_locale(locale);
	lcid = mapi_get_lcid_from_locale(locale);
	
	cpid_str = talloc_asprintf(mem_ctx, "%d", cpid);
	lcid_str = talloc_asprintf(mem_ctx, "0x%.4x", lcid);
	
	mapi_profile_add_string_attr(profname, "codepage", cpid_str);
	mapi_profile_add_string_attr(profname, "language", lcid_str);
	mapi_profile_add_string_attr(profname, "method", lcid_str);
	
	talloc_free(cpid_str);
	talloc_free(lcid_str);

	retval = MapiLogonProvider(&session, profname, el->generic_password, PROVIDER_ID_NSPI);
	if (retval != MAPI_E_SUCCESS) {
		mapi_errstr("MapiLogonProvider", GetLastError());
		printf("Deleting profile\n");
		if ((retval = DeleteProfile(profname)) != MAPI_E_SUCCESS) {
			mapi_errstr("DeleteProfile", GetLastError());
		}
		return retval;
	}

	retval = ProcessNetworkProfile(session, username, (mapi_profile_callback_t) callback, 
				       username);
	if (retval != MAPI_E_SUCCESS && retval != 0x1) {
		mapi_errstr("ProcessNetworkProfile", GetLastError());
		printf("Deleting profile\n");
		if ((retval = DeleteProfile(profname)) != MAPI_E_SUCCESS) {
			mapi_errstr("DeleteProfile", GetLastError());
		}
		return retval;
	}

	printf("[*] Reference profile %s completed and added to database\n", profname);

	return MAPI_E_SUCCESS;
}

/**
   \details Perform all MAPI profiles operations required for
   openchangesim to work properly

   \param ctx pointer to the OpenChangeSim context
   \param server the server to use for openchangesim tests

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
int openchangesim_profile(struct ocsim_context *ctx, const char *server)
{
	enum MAPISTATUS		retval;
	struct ocsim_server	*el;
	char			*profname;
	struct mapi_profile	profile;
	char			*ip_address;
	char			*username;

	/* Sanity checks */
	OCSIM_RETVAL_IF(!ctx, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	OCSIM_RETVAL_IF(!ctx->servers, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	OCSIM_RETVAL_IF(!server, OCSIM_ERROR, OCSIM_NOT_INITIALIZED, NULL);
	
	el = configuration_validate_server(ctx, server);
	OCSIM_RETVAL_IF(!el, OCSIM_ERROR, OCSIM_INVALID_SERVER, NULL);

	/* Check if profiles already exist */
	switch (el->range) {
	case false:
		profname = talloc_asprintf(ctx->mem_ctx, PROFNAME_TEMPLATE, 
					   el->name, el->generic_user, el->realm);
		retval = OpenProfile(&profile, profname, NULL);
		if (retval != MAPI_E_SUCCESS) {
			if (el->range_start > 0) {
				username = talloc_asprintf(ctx->mem_ctx, PROFNAME_USER, 
							   el->generic_user, el->range_start);
			} else {
				username = talloc_strdup(ctx->mem_ctx, el->generic_user);
			}
			retval = openchangesim_CreateProfile(ctx->mem_ctx, el, profname, username);
			talloc_free(username);
		}
		talloc_free(profname);
		break;
	case true:
		/* Create first profile */
		openchangesim_interface_get_next_ip(el, true);
		ip_address = talloc_asprintf(ctx->mem_ctx, "%d.%d.%d.%d", el->ip_current[0],
					     el->ip_current[1], el->ip_current[2], el->ip_current[3]);
		profname = talloc_asprintf(ctx->mem_ctx, PROFNAME_TEMPLATE_NB, el->name,
					   el->generic_user, el->range_start, el->realm);
		retval = OpenProfile(&profile, profname, NULL);
		if (retval != MAPI_E_SUCCESS) {
			username = talloc_asprintf(ctx->mem_ctx, PROFNAME_USER,
						   el->generic_user, el->range_start);
			retval = openchangesim_CreateProfile(ctx->mem_ctx, el,
							     profname, username);
			mapi_profile_add_string_attr(profname, "localaddress", ip_address);
			talloc_free(username);
		}
		if (openchangesim_create_interface_tap(ctx->mem_ctx, el->ip_used, ip_address) < 0) {
			exit (1);
		}
		talloc_free(ip_address);

		/* Duplicate other profiles */
		openchangesim_DuplicateProfile(ctx->mem_ctx, profname, el);
		talloc_free(profname);
		break;
	}

	return OCSIM_SUCCESS;
}

static int check_range_status(struct ocsim_context *ctx, const char *server)
{
	struct ocsim_server	*el;
	uint32_t		range;

	el = configuration_validate_server(ctx, server);
	if (!el) {
		DEBUG(0, (HELP_FORMAT_STRING, HELP_SERVER_INVALID));
		exit (1);
	}

	range = el->range_end - el->range_start;

	if (!range && !el->ip_number) {
		return 0;
	}

	if (range && el->ip_number) {
		if (range > el->ip_number) return -1;

		return 0;
	}

	return -1;
}

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
	bool			opt_server_list = false;
	const char		*opt_profdb = NULL;
	const char		*opt_debug = NULL;
	const char		*opt_conf_file = NULL;
	const char		*opt_server = NULL;

	enum { OPT_PROFILE_DB=1000, OPT_DEBUG, OPT_DUMPDATA, OPT_VERSION,
	       OPT_CONFIG, OPT_CONFCHECK, OPT_CONFDUMP, OPT_SERVER_LIST, 
	       OPT_SERVER };

	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "database", 'f', POPT_ARG_STRING, NULL, OPT_PROFILE_DB, "set the profile database path", NULL },
		{ "debuglevel", 'd', POPT_ARG_STRING, NULL, OPT_DEBUG, "set debug level", NULL },
		{ "dump-data", 0, POPT_ARG_NONE, NULL, OPT_DUMPDATA, "dump hexadecimal data", NULL },
		{ "version", 'V', POPT_ARG_NONE, NULL, OPT_VERSION, "Print version ", NULL },
		{ "config", 'c', POPT_ARG_STRING, NULL, OPT_CONFIG, "Specify configuration file", NULL },
		{ "confcheck", 0, POPT_ARG_NONE, NULL, OPT_CONFCHECK, "Check/Validate configuration", NULL },
		{ "confdump", 0, POPT_ARG_NONE, NULL, OPT_CONFDUMP, "Dump configuration", NULL },
		{ "server-list", 0, POPT_ARG_NONE, NULL, OPT_SERVER_LIST, "List available servers", NULL },
		{ "server", 0, POPT_ARG_STRING, NULL, OPT_SERVER, "Select server to use for openchangesim", NULL },
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
		case OPT_SERVER_LIST:
			opt_server_list = true;
			break;
		case OPT_SERVER:
			opt_server = poptGetOptArg(pc);
			break;
		default:
			DEBUG(0, ("Invalid option\n"));
			exit (1);
		}
	}

	/* OpenChangeSim initialization */
	ctx = openchangesim_init(mem_ctx);
	ret = openchangesim_parse_config(ctx, opt_conf_file);

	/* confcheck work case */
	if (opt_confcheck) {
		if (ret) {
			DEBUG(0, (DEBUG_FORMAT_STRING_WARN, DEBUG_CONF_FILE_KO));
		} else {
			DEBUG(0, (DEBUG_FORMAT_STRING, DEBUG_CONF_FILE_OK));
		}
		openchangesim_release(ctx);
		exit (0);
	}

	/* list available servers */
	if (opt_server_list) {
		configuration_dump_servers_list(ctx);
		openchangesim_release(ctx);
		exit (0);
	}

	/* confdump work case */
	if (opt_confdump) {
		configuration_dump_servers(ctx);
		openchangesim_release(ctx);
		exit (0);
	}

	if (!opt_server) {
		DEBUG(0, (HELP_FORMAT_STRING, HELP_SERVER_OPTION));
		configuration_dump_servers_list(ctx);
		openchangesim_release(ctx);
		exit (0);
	}

	/* Ensure IP address range is >= number of users requested */
	if (check_range_status(ctx, opt_server) < 0) {
		DEBUG(0, (HELP_FORMAT_STRING, HELP_IP_USER_RANGE));
		openchangesim_release(ctx);
		exit (0);
	}

	/* Step 2. Sanity check on options */
	if (!opt_profdb) {
		char		*conf_path;
		struct stat	sb;

		/* Ensure the directories exists, otherwise create them */
		conf_path = talloc_asprintf(mem_ctx, DEFAULT_PROFPATH_BASE, getenv("HOME"));
		ret = stat(conf_path, &sb);
		if (ret == -1) {
			mkdir(conf_path, 0700);
		}
		talloc_free(conf_path);

		conf_path = talloc_asprintf(mem_ctx, DEFAULT_PROFPATH, getenv("HOME"));
		ret = stat(conf_path, &sb);
		if (ret == -1) {
			mkdir(conf_path, 0700);
		}
		talloc_free(conf_path);

		/* Ensure the profile database exist, otherwise create it */
		opt_profdb = talloc_asprintf(mem_ctx, DEFAULT_PROFDB, getenv("HOME"));
		if (access(opt_profdb, F_OK)) {
			retval = CreateProfileStore(opt_profdb, mapi_profile_get_ldif_path());
			if (retval != MAPI_E_SUCCESS) {
				mapi_errstr("CreateProfileStore", GetLastError());
				exit (1);
			}
			DEBUG(0, (DEBUG_FORMAT_STRING, DEBUG_PROFDB_CREATED));
		}
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

	/* Step 4. Perform profile operations */
	ret = openchangesim_profile(ctx, opt_server);
	if (ret == OCSIM_ERROR) {
		goto end;
	}

	/* Last OpenChangeSim Step: Delete virtual interfaces */
	ret = openchangesim_delete_interfaces(ctx, opt_server);
	if (ret == OCSIM_ERROR) {
		DEBUG(0, ("Error while deleting the interfaces\n"));
		goto end;
	}

	/* Uninitialize MAPI subsystem */
end:
	poptFreeContext(pc);
	MAPIUninitialize();
	talloc_free(mem_ctx);

	return 0;
}
