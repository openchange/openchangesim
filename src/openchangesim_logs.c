/*
   OpenChangeSim logs API

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

struct ocsim_log *openchangesim_log_init(TALLOC_CTX *mem_ctx)
{
	struct ocsim_log	*log = NULL;

	/* Sanity checks */
	if (!mem_ctx) return NULL;

	log = talloc_zero(mem_ctx, struct ocsim_log);
	memset(&log->tv_start, 0, sizeof (struct timeval));
	memset(&log->tv_end, 0, sizeof (struct timeval));

	openlog(OPENCHANGESIM_LOGNAME, LOG_PID, LOG_USER);

	return log;
}

void openchangesim_log_start(struct ocsim_log *log)
{
	/* Sanity checks */
	if (!log) return;

	gettimeofday(&log->tv_start, NULL);

	return;
}

void openchangesim_log_end(struct ocsim_log *log,
			   char *scenario, 
			   char *case_name,
			   const char *clientIP) 
{
	uint64_t	sec;
	uint64_t	usec;

	gettimeofday(&log->tv_end, NULL);
	sec = log->tv_end.tv_sec - log->tv_start.tv_sec;
	if ((log->tv_end.tv_usec - log->tv_start.tv_usec) < 0) {
		sec -= 1;
		usec = log->tv_end.tv_usec + log->tv_start.tv_usec;
		while (usec > 1000000) {
			usec -= 1000000;
			sec += 1;
		}
	} else {
		usec = log->tv_end.tv_usec - log->tv_start.tv_usec;
	}

	syslog(LOG_INFO, "%s: %s \"%s\": %ld seconds %ld microseconds", scenario, clientIP, 
	       case_name, (long int) sec, (long int) usec);

	memset(&log->tv_start, 0, sizeof (struct timeval));
	memset(&log->tv_end, 0, sizeof (struct timeval));

	return;
}

void openchangesim_log_close(struct ocsim_log *log)
{
	talloc_free(log);
	closelog();

	return;
}
