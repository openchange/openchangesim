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

#ifndef	__OPENCHANGESIM_H__
#define	__OPENCHANGESIM_H__

#include <libmapi/libmapi.h>
#include <popt.h>
#include "src/version.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

#define	DEFAULT_PROFPATH_BASE	"%s/.openchange"
#define	DEFAULT_PROFPATH	"%s/.openchange/openchangesim"
#define	DEFAULT_PROFDB		"%s/.openchange/openchangesim/profiles.ldb"
#define	DEFAULT_CONFFILE	"%s/.openchange/openchangesim/openchangesim.conf"
#define	PROFNAME_TEMPLATE	"%s/%s@%s" /* server\username@realm */
#define	PROFNAME_TEMPLATE_NB	"%s/%s%d@%s" /* server\username1@realm */
#define	PROFNAME_USER		"%s%d"

/**
   DEBUG messages
 */
#define	DEBUG_FORMAT_STRING		"[*] %s\n"
#define	DEBUG_FORMAT_STRING_MODULE	"\t[*] %-20s: %s\n"
#define	DEBUG_FORMAT_STRING_MODULE_ERR	"\t[*] [ERROR] %-20s: %s\n"
#define	DEBUG_FORMAT_STRING_ERR		"[ERROR] %s\n"
#define	DEBUG_FORMAT_STRING_WARN	"[WARN] %s\n"
#define	DEBUG_PROFDB_CREATED		"Profile database created"
#define	DEBUG_CONF_FILE_KO		"Configuration file not OK!"
#define	DEBUG_CONF_FILE_OK		"Configuration file OK"
#define	DEBUG_ERR_OUT_OF_ADDRESS	"No more available IP address left"
#define	DEBUG_ERR_DUPLICATE		"Duplicate scenario"
#define	DEBUG_ERR_MISSING_NAME		"A scenario defined in the configuration file is missing the required name parameter"
#define	DEBUG_ERR_INVALID_NAME		"A scenario name defined in the configuration file doesn't exist"


/**
   HELP messages
 */
#define	HELP_FORMAT_STRING	"Help: %s\n"
#define	HELP_SERVER_OPTION	"You need to specify one server using --server option"
#define	HELP_SERVER_INVALID	"Invalid server specified"
#define	HELP_IP_USER_RANGE	"Your IP range is insufficient given the generic user range"

/**
   Common template strings
 */
#define	DFLT_SUBJECT_PREFIX	"[OPENCHANGESIM]"

/**
   Module names
 */
#define	SENDMAIL_MODULE_NAME	"sendmail"
#define	FETCHMAIL_MODULE_NAME	"fetchmail"

#define	MAX_READ_SIZE	0x1000

#define FPUTS(s, f) fprintf((f), "%s", (s))

extern struct poptOption popt_openchange_version[];

#define	POPT_OPENCHANGE_VERSION { NULL, 0, POPT_ARG_INCLUDE_TABLE, popt_openchange_version, 0, "Common openchange options:", NULL },

struct ocsim_log
{
	FILE			*fp;
	struct timeval		tv_start;
	struct timeval		tv_end;
};

struct ocsim_var
{
	const char		*name;
	const void		*value;
	struct ocsim_var	*prev;
	struct ocsim_var	*next;
};

struct ocsim_server
{
	const char		*name;
	const char		*address;
	const char		*domain;
	const char		*realm;
	uint32_t		version;
	const char		*generic_user;
	const char		*generic_password;
	bool			range;
	uint32_t		range_start;
	uint32_t		range_end;
	uint8_t			*ip_start;
	uint8_t			*ip_end;
	uint8_t			ip_current[4];
	uint32_t		ip_number;
	uint32_t		ip_used;
	
	struct ocsim_var	*vars;
	struct ocsim_server	*prev;
	struct ocsim_server	*next;
};

enum ocsim_scenario_body_type {
	OCSIM_BODY_NONE = 0,
	OCSIM_BODY_UTF8_INLINE,
	OCSIM_BODY_HTML_INLINE,
	OCSIM_BODY_UTF8_FILE,
	OCSIM_BODY_HTML_FILE,
	OCSIM_BODY_RTF_FILE
};

/**
   sendmail scenario can control body and attachments within its cases
 */
struct ocsim_scenario_sendmail
{
	enum ocsim_scenario_body_type	body_type;
	char				*body_file;
	char				*body_inline;
	uint32_t			attachment_count;
	char				**attachments;
};

struct ocsim_scenario_case
{
	char				*name;
	void				*private_data;
	struct ocsim_scenario_case	*prev;
	struct ocsim_scenario_case	*next;
};

struct ocsim_scenario
{
	const char			*name;
	uint32_t			repeat;
	struct ocsim_scenario_case	*cases;
	struct ocsim_scenario		*prev;
	struct ocsim_scenario		*next;
};

/**
   Generic scenario structure for parser. Superset of all available
   parameters. This structure is converted into ocsim_scenarion and
   additional (scenario's specific parameters) casted into
   private_data.
 */
struct ocsim_generic_scenario_case
{
	const char				*name;
	enum ocsim_scenario_body_type		body_type;
	char					*body_file;
	char					*body_inline;
	uint32_t				attachment_count;
	char					**attachments;
	struct ocsim_generic_scenario_case	*prev;
	struct ocsim_generic_scenario_case	*next;
};

struct ocsim_generic_scenario
{
	const char				*name;
	uint32_t				repeat;
	struct ocsim_generic_scenario_case	*case_el;
};

struct ocsim_module
{
	struct ocsim_module		*prev;		/* !< Pointer to the previous module */
	struct ocsim_module		*next;		/* !< Pointer to the next module */
	char				*name;		/* !< The name of the test suite */
	char				*description;	/* !< Description of the module */
	struct ocsim_scenario		*scenario;	/* !< The associated scenario */
	struct ocsim_scenario_case	*cases;		/*!< execution cases for the module */

	uint32_t			(*run)(TALLOC_CTX *, struct ocsim_scenario_case *, struct mapi_session *);
	uint32_t			(*set_ref_count)(struct ocsim_module *, int);
	uint32_t			(*get_ref_count)(struct ocsim_module *);
};

struct ocsim_context
{
	TALLOC_CTX		*mem_ctx;
	/* lexer internal data */
	struct ocsim_server			*server_el;
	struct ocsim_generic_scenario		*scenario_el;
	struct ocsim_generic_scenario_case	*case_el;
	unsigned int				lineno;
	int					result;
	/* ocsim */
	struct ocsim_server			*servers;
	struct ocsim_scenario			*scenarios;
	struct ocsim_var			*options;
	struct ocsim_module			*modules;
	/* context */
	FILE					*fp;
	const char				*filename;
	FILE					*logfp;
	pid_t					*pid;
};

#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS		extern "C" {
#define __END_DECLS		}
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

#ifndef	_PUBLIC_
#define	_PUBLIC_
#endif

__BEGIN_DECLS

/* The following public definitions come from src/openchangesim_public.c */
struct ocsim_context *openchangesim_init(TALLOC_CTX *);
int openchangesim_release(struct ocsim_context *);
int openchangesim_parse_config(struct ocsim_context *, const char *);
int openchangesim_do_debug(struct ocsim_context *, const char *, ...);

/* The following public definitions come from src/configuration_api.c */
int configuration_add_server(struct ocsim_context *, struct ocsim_server *);
int configuration_add_scenario(struct ocsim_context *, struct ocsim_generic_scenario *);
int configuration_add_generic_scenario_case(struct ocsim_generic_scenario *, struct ocsim_generic_scenario_case *);
uint8_t *configuration_get_ip(TALLOC_CTX *, const char *);
uint32_t configuration_get_ip_count(uint8_t *, uint8_t *);

/* The following public definitions come from src/configuration_dump.c */
int configuration_dump_servers(struct ocsim_context *);
int configuration_dump_servers_list(struct ocsim_context *);
int configuration_dump_scenarios(struct ocsim_context *);
struct ocsim_server *configuration_validate_server(struct ocsim_context *, const char *);
struct ocsim_scenario *configuration_validate_scenario(struct ocsim_context *, const char *);

/* The following public definitions come from src/openchangesim.c */
void openchangesim_printlog(FILE *, char *);
int openchangesim_profile(struct ocsim_context *, const char *);
enum MAPISTATUS openchangesim_DuplicateProfile(TALLOC_CTX *, char *, struct ocsim_server *);
enum MAPISTATUS openchangesim_CreateProfile(TALLOC_CTX *, struct ocsim_server *, char *, const char *);
uint32_t callback(struct SRowSet *, void *);

/* The following public definitions come from src/openchangesim_interface.c */
void openchangesim_interface_get_next_ip(struct ocsim_server *, bool);
int openchangesim_create_interface_tap(TALLOC_CTX *, uint32_t, char *);
int openchangesim_delete_interface_tap(TALLOC_CTX *, uint32_t);
int openchangesim_delete_interfaces(struct ocsim_context *, const char *);

/* The following public definitions come from src/openchangesim_fork.c */
uint32_t openchangesim_fork_process_start(struct ocsim_context *, const char *);
uint32_t openchangesim_fork_process_end(struct ocsim_context *, const char *);

/* The following public definitions come from src/openchangesim_modules.c */
uint32_t openchangesim_register_modules(struct ocsim_context *);
uint32_t openchangesim_module_register(struct ocsim_context *, struct ocsim_module *);
struct ocsim_module *openchangesim_module_init(struct ocsim_context *, const char *, const char *);
bool openchangesim_module_ref_count(struct ocsim_context *);
uint32_t module_get_ref_count(struct ocsim_module *);
uint32_t module_set_ref_count(struct ocsim_module *, int);
struct ocsim_scenario *module_get_scenario(struct ocsim_context *, const char *);
struct ocsim_scenario_case *module_get_scenario_data(struct ocsim_context *, const char *);
uint32_t openchangesim_modules_run(struct ocsim_context *, char *);

/* The following public definitions come from src/openchangesim_logs.c */
struct ocsim_log *openchangesim_log_init(char *);
uint32_t openchangesim_log_scenario_start(struct ocsim_log *);
uint32_t openchangesim_log_scenario_end(struct ocsim_log *);
uint32_t openchangesim_log_scenario_commit(struct ocsim_log *);
uint32_t openchangesim_log_close(struct ocsim_log *);

/* The following public definitions come from src/modules/module_fetchmail.c */
uint32_t module_fetchmail_init(struct ocsim_context *);

/* The following public definitions come from src/modules/module_sendmail.c */
uint32_t module_sendmail_init(struct ocsim_context *);

/* The following public definitions come from src/modules/module_cleanup.c */
uint32_t module_cleanup_init(struct ocsim_context *);
uint32_t module_cleanup_run(TALLOC_CTX *, struct mapi_session *);

void error_message (struct ocsim_context *, const char *, ...) __attribute__ ((format (printf, 2, 3)));

__END_DECLS

extern int error_flag;

#include "src/openchangesim_errors.h"

#endif	/* !__OPENCHANGESIM_H__ */
