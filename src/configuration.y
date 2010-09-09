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


%{

#include "src/openchangesim.h"

#define YYERROR_VERBOSE

int	ocsim_yylex(void *, void *);
void	yyerror(struct ocsim_context *, void *, char *);

%}

%pure_parser
%parse-param {struct ocsim_context *ctx}
%parse-param {void *scanner}
%lex-param {yyscan_t *scanner}
%name-prefix="ocsim_yy"

%union {
	char		*name;
	char		*var;
	uint32_t	integer;
	char		*charval;
	char		*ip_address;
}

%token	<name>		IDENTIFIER
%token	<name>		STRING
%token	<integer>	INTEGER
%token	<var>		VAR
%token	<ip_address>	IP_ADDRESS

%token	kw_INCLUDE
%token	kw_SERVER
%token	kw_SCENARIO
%token	kw_CASE
%token	kw_NAME
%token	kw_LOGFILE
%token	kw_VERSION
%token	kw_ADDRESS
%token	kw_DOMAIN
%token	kw_REALM
%token	kw_GENERIC_USER
%token	kw_GENERIC_USER_RANGE
%token	kw_GENERIC_PASSWORD
%token	kw_IP_RANGE
%token	kw_REPEAT
%token	kw_ATTACHMENT
%token	kw_FILE_UTF8
%token	kw_FILE_HTML
%token	kw_FILE_RTF
%token	kw_INLINE_UTF8
%token	kw_INLINE_HTML

%token	OBRACE
%token	EBRACE
%token	SEMICOLON
%token	COLON
%token	EQUAL
%token	MINUS

%start keywords

%%

keywords	: 
		{
			if (!ctx->server_el) {
				ctx->server_el = talloc_zero(ctx->servers, struct ocsim_server);
			}
			if (!ctx->scenario_el) {
				ctx->scenario_el = talloc_zero(ctx->scenarios, struct ocsim_generic_scenario);
				ctx->scenario_el->repeat = 0;
				ctx->scenario_el->case_el = NULL;
			}
			if (!ctx->case_el) {
				ctx->case_el = talloc_zero(ctx->scenario_el, 
							   struct ocsim_generic_scenario_case);
				ctx->case_el->body_file = NULL;
				ctx->case_el->body_inline = NULL;
				ctx->case_el->attachment_count = 0;
				ctx->case_el->attachments = talloc_array(ctx->case_el, char *, 2);
			}
		}
		| keywords kvalues
		;

kvalues		: include
		| set
		| server
		| scenario
		;

include		:
		kw_INCLUDE STRING
		{
			printf("include directive found: %s\n", $2);
		}
		;

set		:
		IDENTIFIER EQUAL STRING
		{
		}
		| IDENTIFIER EQUAL INTEGER
		{
		}
		| IDENTIFIER EQUAL IDENTIFIER
		{
		}
		| IDENTIFIER EQUAL VAR
		{
		}
		;

server		:
		kw_SERVER OBRACE server_contents EBRACE SEMICOLON
		{
			configuration_add_server(ctx, ctx->server_el);
			talloc_free(ctx->server_el);
			ctx->server_el = talloc_zero(ctx->servers, struct ocsim_server);
		}

server_contents	: | server_contents server_content
		{
		}
		;

server_content	: kw_NAME EQUAL IDENTIFIER SEMICOLON
		{
			ctx->server_el->name = talloc_strdup(ctx->server_el, $3);
		}
		| kw_NAME EQUAL STRING SEMICOLON
		{
			ctx->server_el->name = talloc_strdup(ctx->server_el, $3);
		}
		| kw_LOGFILE EQUAL STRING SEMICOLON
		{
			ctx->server_el->logfile = talloc_strdup(ctx->server_el, $3);
		}
		| kw_VERSION EQUAL INTEGER SEMICOLON
		{
			ctx->server_el->version = $3;
		}
		| kw_ADDRESS EQUAL IDENTIFIER SEMICOLON
		{
			ctx->server_el->address = talloc_strdup(ctx->server_el, $3);
		}
		| kw_ADDRESS EQUAL IP_ADDRESS SEMICOLON
		{
			ctx->server_el->address = talloc_strdup(ctx->server_el, $3);
		}
		| kw_DOMAIN EQUAL IDENTIFIER SEMICOLON
		{
			ctx->server_el->domain = talloc_strdup(ctx->server_el, $3);
		}
		| kw_DOMAIN EQUAL STRING SEMICOLON
		{
			ctx->server_el->domain = talloc_strdup(ctx->server_el, $3);
		}				
		| kw_REALM EQUAL IDENTIFIER SEMICOLON
		{
			ctx->server_el->realm = talloc_strdup(ctx->server_el, $3);
		}
		| kw_REALM EQUAL STRING SEMICOLON
		{
			ctx->server_el->realm = talloc_strdup(ctx->server_el, $3);
		}
		| kw_GENERIC_USER EQUAL IDENTIFIER SEMICOLON
		{
			ctx->server_el->generic_user = talloc_strdup(ctx->server_el, $3);
		}
		| kw_GENERIC_PASSWORD EQUAL STRING SEMICOLON
		{
			ctx->server_el->generic_password = talloc_strdup(ctx->server_el, $3);
		}
		| kw_GENERIC_USER_RANGE EQUAL INTEGER MINUS INTEGER SEMICOLON
		{
			if ($5 < $3) {
				printf("Invalid user range: start > end\n");
			} else if ($5 == $3) {
				ctx->server_el->range = false;
			} else {
				ctx->server_el->range = true;
				ctx->server_el->range_start = $3;
				ctx->server_el->range_end = $5;
			}
		}
		| kw_IP_RANGE EQUAL IP_ADDRESS MINUS IP_ADDRESS SEMICOLON
		{
			ctx->server_el->ip_start = configuration_get_ip(ctx->mem_ctx, $3);
			if (!ctx->server_el->ip_start) {
				yyerror(ctx, NULL, "Invalid IP address for start range");
			}
			ctx->server_el->ip_end   = configuration_get_ip(ctx->mem_ctx, $5);
			
			ctx->server_el->ip_number = configuration_get_ip_count(ctx->server_el->ip_start, 
									       ctx->server_el->ip_end);

		}
		;

scenario	:
		kw_SCENARIO OBRACE scenario_contents EBRACE SEMICOLON
		{
			configuration_add_scenario(ctx, ctx->scenario_el);
			talloc_free(ctx->scenario_el);
			ctx->scenario_el = talloc_zero(ctx->scenarios, struct ocsim_generic_scenario);
			ctx->scenario_el->repeat = 0;
		}

scenario_contents: | scenario_contents scenario_content
		{
		}
		;

scenario_content: kw_NAME EQUAL IDENTIFIER SEMICOLON
		{
			ctx->scenario_el->name = talloc_strdup(ctx->scenario_el, $3);
		}
		| kw_NAME EQUAL STRING SEMICOLON
		{
			ctx->scenario_el->name = talloc_strdup(ctx->scenario_el, $3);
		}
		| kw_REPEAT EQUAL INTEGER SEMICOLON
		{
			ctx->scenario_el->repeat = $3;
		}
		| scenario_case
		{
			configuration_add_generic_scenario_case(ctx->scenario_el, ctx->case_el);
			talloc_free(ctx->case_el);
			ctx->case_el = talloc_zero(ctx->scenario_el, struct ocsim_generic_scenario_case);
			ctx->case_el->body_type = 0;
			ctx->case_el->body_file = NULL;
			ctx->case_el->body_inline = NULL;
			ctx->case_el->attachment_count = 0;
			ctx->case_el->attachments = talloc_array(ctx->case_el, char *, 2);
		}
		;

scenario_case	: kw_CASE OBRACE scases EBRACE SEMICOLON
		{
		}

scases:		| scases scase		

scase		: kw_NAME EQUAL STRING SEMICOLON
		{
			if (!ctx->case_el->name) {
				ctx->case_el->name = talloc_strdup(ctx->case_el, $3);
			} else {
				printf("%s: %d\n", "name already specified for this case", ctx->lineno);
				exit (1);
			}
		}
		| kw_ATTACHMENT EQUAL STRING SEMICOLON
		{
		  ctx->case_el->attachments = talloc_realloc(ctx->case_el, ctx->case_el->attachments, char *,
							     ctx->case_el->attachment_count + 2);
		  ctx->case_el->attachments[ctx->case_el->attachment_count] = talloc_strdup(ctx->case_el->attachments, $3);
		  ctx->case_el->attachment_count += 1;
		}
		| kw_FILE_UTF8 EQUAL STRING SEMICOLON
		{
			if (ctx->case_el->body_type == OCSIM_BODY_NONE) {
				ctx->case_el->body_type = OCSIM_BODY_UTF8_FILE;
				ctx->case_el->body_file = talloc_strdup(ctx->case_el, $3);
			} else {
				printf("%s: %d\n", "body already specificed for this case", ctx->lineno);
				exit (1);
			}
		}
		| kw_FILE_HTML EQUAL STRING SEMICOLON
		{
			if (ctx->case_el->body_type == OCSIM_BODY_NONE) {
				ctx->case_el->body_type = OCSIM_BODY_HTML_FILE;
				ctx->case_el->body_file = talloc_strdup(ctx->case_el, $3);
			} else {
				printf("%s: %d\n", "body already specificed for this case", ctx->lineno);
				exit (1);
			}
		}
		| kw_FILE_RTF EQUAL STRING SEMICOLON
		{
			if (ctx->case_el->body_type == OCSIM_BODY_NONE) {
				ctx->case_el->body_type = OCSIM_BODY_RTF_FILE;
				ctx->case_el->body_file = talloc_strdup(ctx->case_el, $3);
			} else {
				printf("%s: %d\n", "body already specificed for this case", ctx->lineno);
				exit (1);
			}			
		}
		| kw_INLINE_UTF8 EQUAL STRING SEMICOLON
		{
			if (ctx->case_el->body_type == OCSIM_BODY_NONE) {
				ctx->case_el->body_type = OCSIM_BODY_UTF8_INLINE;
				ctx->case_el->body_inline = talloc_strdup(ctx->case_el, $3);
			} else {
				printf("%s: %d\n", "body already specificed for this case", ctx->lineno);
				exit (1);
			}						
		}
		| kw_INLINE_HTML EQUAL STRING SEMICOLON
		{
			if (ctx->case_el->body_type == OCSIM_BODY_NONE) {
				ctx->case_el->body_type = OCSIM_BODY_HTML_INLINE;
				ctx->case_el->body_inline = talloc_strdup(ctx->case_el, $3);
			} else {
				printf("%s: %d\n", "body already specificed for this case", ctx->lineno);
				exit (1);
			}						
		}

%%

void yyerror(struct ocsim_context *ctx, void *scanner, char *s)
{
	printf("%s: %d\n", s, ctx->lineno);
}
