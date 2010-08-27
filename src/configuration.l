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

#include <stdarg.h>
#include <ctype.h>

#include "src/openchangesim.h"
#include "src/configuration.tab.h"

#define	YY_EXTRA_TYPE	struct ocsim_context *
#define	YY_NO_UNPUT

/*
 * Prototypes
 */
int	yylex(YYSTYPE *, void *);
int	yyget_lineno(void *);
FILE	*yyget_in(void *);
FILE	*yyget_out(void *);
int	yyget_leng(void *);
char	*yyget_text(void *);
void	yyset_lineno(int, void *);
void	yyset_extra(YY_EXTRA_TYPE, void *);
void	yyset_in (FILE *, void *);
void	yyset_out (FILE *, void *);
int	yyget_debug(void *);
void	yyset_debug(int, void *);
int	yylex_destroy(void *);
void	yyset_column(int, void *);
int	yyget_column(void *);

static void unterminated(struct ocsim_context *, const char *, unsigned int);

%}

%option	reentrant
%option	bison-bridge
%option	prefix="ocsim_yy"
%option	noyywrap

%%
.include		{ return kw_INCLUDE; }
server			{ return kw_SERVER; }
scenario		{ return kw_SCENARIO; }
name			{ return kw_NAME; }
version			{ return kw_VERSION; }
address			{ return kw_ADDRESS; }
domain			{ return kw_DOMAIN; }
realm			{ return kw_REALM; }
generic_user		{ return kw_GENERIC_USER; }
generic_user_range	{ return kw_GENERIC_USER_RANGE; }
generic_password	{ return kw_GENERIC_PASSWORD; }
\{			{ return OBRACE; }
\}			{ return EBRACE; }
;			{ return SEMICOLON; }
:			{ return COLON; }
=			{ return EQUAL; }
-			{ return MINUS; }
\/\*			{ 
			    struct ocsim_context *ctx = yyextra;
			    int c, start_lineno = ctx->lineno;
			    int level = 1;
			    int seen_star = 0;
			    int seen_slash = 0;
			    while((c = input(yyscanner)) != EOF) {
				if(c == '/') {
				    if(seen_star) {
					if(--level == 0)
					    break;
					seen_star = 0;
					continue;
				    }
				    seen_slash = 1;
				    continue;
				}
				if(seen_star && c == '/') {
				    if(--level == 0)
					break;
				    seen_star = 0;
				    continue;
				}
				if(c == '*') {
				    if(seen_slash) {
					level++;
					seen_star = seen_slash = 0;
					continue;
				    } 
				    seen_star = 1;
				    continue;
				}
				seen_star = seen_slash = 0;
				if(c == '\n') {
				    ctx->lineno++;
				    continue;
				}
			    }
			    if(c == EOF)
				unterminated(ctx, "comment", start_lineno);
			}
"\""			{ 
			    struct ocsim_context *ctx = yyextra;
			    int start_lineno = ctx->lineno;
			    int c, c2;
			    char buf[0x4000];
			    char *p = buf;
			    int f = 0;
			    int skip_ws = 0;
			    
			    while((c = input(yyscanner)) != EOF) {
				if(isspace(c) && skip_ws) {
				    if(c == '\n')
					ctx->lineno++;
				    continue;
				}
				skip_ws = 0;

				if (c == '\\') {
					c2 = c;
					c = input(yyscanner);
					if (c == '"') { 
						*p++ = c;
						c = input(yyscanner);
					} else {
						*p++ = c2;
					}
				}

				if(c == '"') {
					if(f) {
						*p++ = '"';
						f = 0;
					} else {
						f = 1;
					}
					continue;
				}
				if(f == 1) {
					unput(c);
					break;
				}
				if(c == '\n') {
				    ctx->lineno++;
				    while(p > buf && isspace((unsigned char)p[-1]))
					p--;
				    skip_ws = 1;
				    continue;
				}
				*p++ = c;
			    }
			    if(c == EOF)
				unterminated(ctx, "string", start_lineno);
			    *p++ = '\0';
			    yylval->name = buf;
			    return STRING; 
			}
\$[-A-Za-z0-9_]+      	{ char *y = yytext + 1;
			  yylval->var = strdup((const char *)y);
			  return VAR;
			}
[A-Za-z][-A-Za-z0-9_.]*	{
			  yylval->name = strdup((const char *)yytext);
			  return IDENTIFIER;
			}
([0-9]){1,3}+"."([0-9]){1,3}+"."([0-9]){1,3}+"."([0-9]){1,3} {
				yylval->ip_address = strdup((const char *)yytext);
				return IP_ADDRESS;
			}
[0-9]+			{
				char *y = yytext;
				yylval->integer = atoi((const char *)y);
				return INTEGER;
			}
[ \t]+			;
\n		{
		struct ocsim_context *ctx = yyextra;
		 ctx->lineno++; 
		}
.      		{ 
		struct ocsim_context *ctx = yyextra;
		error_message(ctx, "Ignoring char(%c)\n", *yytext); 
		}
%%

void error_message(struct ocsim_context *ctx, const char *format, ...)
{
	va_list	args;

	va_start(args, format);
	fprintf(stderr, "ERROR: %s:%d: ", ctx->filename, ctx->lineno);
	vfprintf(stderr, format, args);
	va_end(args);
	error_flag++;
	fflush(0);
}

static void unterminated(struct ocsim_context *ctx, const char *type, unsigned int start_lineno)
{
    error_message(ctx, "unterminated %s, possibly started on line %d\n", type, start_lineno);
    fflush(0);
}