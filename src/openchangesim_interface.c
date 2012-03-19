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

#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>

#include "src/openchangesim.h"

#ifndef	TUNSETGROUP
#define	TUNSETGROUP	_IOW('T', 206, int)
#endif


/**
   \details Retrieve the next available IP address in the available pool

   \param el pointer to the openchangesim server context
   \param status boolean value defining whether it is the first call
   of not

 */
void openchangesim_interface_get_next_ip(struct ocsim_server *el, bool status)
{
	if (status == true) {
		el->ip_current[0] = el->ip_start[0];
		el->ip_current[1] = el->ip_start[1];
		el->ip_current[2] = el->ip_start[2];
		el->ip_current[3] = el->ip_start[3];
		el->ip_used = 0;
	} else {
		/* We are not in the same subnet */
		if (el->ip_current[2] < el->ip_end[2]) {
			el->ip_current[3] += 1;
			if (el->ip_current[3] == 255) {
				el->ip_current[3] = 0;
				el->ip_current[2] += 1;
			}
		} else if (el->ip_current[2] == el->ip_end[2]) {
			if (el->ip_current[3] < el->ip_end[3]) {
				el->ip_current[3] += 1;
			} else {
				DEBUG(0, (DEBUG_FORMAT_STRING_ERR, DEBUG_ERR_OUT_OF_ADDRESS));
				exit (1);
			}
		}
		el->ip_used += 1;
	}
}

/**
   \details Create a virtual tap interface and assign an IPv4 IP
   address

   \param mem_ctx pointer to the memory context
   \param tap_number the tap interface number to use
   \param ip_addr the IP address to assign to the interface

   \return 0 on success, otherwise -1
 */
int openchangesim_create_interface_tap(TALLOC_CTX *mem_ctx, 
				       int * _tap_fd,
				       const char *ip_addr)
{
	struct ifreq		ifr;
	struct addrinfo		*result = NULL;
	char			*tap = "";
	char			*name;
	char			*file = "/dev/net/tun";
	uid_t			owner = -1;
	int			tap_fd;
	int			s;
	*_tap_fd = -1;

	if ((tap_fd = open(file, O_RDWR)) < 0) {
		fprintf(stderr, "Failed to open '%s' : ", file);
		perror("");
		return -1;
	}

	memset(&ifr.ifr_addr, 0, sizeof (ifr.ifr_addr));
	ifr.ifr_addr.sa_family = AF_INET;
	
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI /* | IFF_UP | IFF_RUNNING */;
	strncpy(ifr.ifr_name, tap, sizeof (ifr.ifr_name) - 1);
	if (ioctl(tap_fd, TUNSETIFF, (void *) &ifr) < 0) {
		fprintf(stderr, "ioctl failed\n");
		perror("TUNSETIFF");
		return -1;
	}
	name = talloc_strdup(mem_ctx, ifr.ifr_name);

	owner = geteuid();
	if (owner != -1) {
		if (ioctl(tap_fd, TUNSETOWNER, owner) < 0) {
			perror("TUNSETOWNER");
			return -1;
		}
	}

	if (ioctl(tap_fd, TUNSETPERSIST, 1) < 0) {
		perror("enabling TUNSETPERSIST");
		return -1;
	}

	memset(&ifr, 0, sizeof (ifr));
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	getaddrinfo(ip_addr, "0", NULL, &result);
	ifr.ifr_addr.sa_family = result->ai_addr->sa_family;
	memcpy(ifr.ifr_addr.sa_data, result->ai_addr->sa_data, sizeof (ifr.ifr_addr.sa_data));
	freeaddrinfo(result);
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (ioctl(s, SIOCSIFADDR, (void *) &ifr) < 0) {
		perror("SIOCSIFADDR");

		DEBUG(0, ("Deleting interface"));
		if (ioctl(tap_fd, TUNSETPERSIST, 0) < 0) {
		  perror("TUNSETPERSIST");
		  return OCSIM_ERROR;
		}
		return OCSIM_ERROR;
	}

	*_tap_fd = tap_fd;
	return OCSIM_SUCCESS;
}


/**
   \details Delete a virtual interface

   \param mem_ctx pointer to the memory context
   \param tap_number tap interface number to delete

   \return 0 on success, otherwise -1
 */
int openchangesim_delete_interface_tap(TALLOC_CTX *mem_ctx,
				       int tap_fd)
{
	if (tap_fd == 0) {
		return OCSIM_SUCCESS;
	}

	if (ioctl(tap_fd, TUNSETPERSIST, 0) < 0) {
		perror("TUNSETPERSIST");
		return OCSIM_ERROR;
	}
	
	return OCSIM_SUCCESS;
}

int openchangesim_delete_interfaces(struct ocsim_context *ctx, 
				    const char *server)
{
	FILE			*f;
	struct ocsim_server	*el;
	int			i;
	int			ret;
	char			*logstr;

	el = configuration_validate_server(ctx, server);
	if (!el) return OCSIM_ERROR;

	f = fdopen(STDOUT_FILENO, "r+");

	for (i = 0; i <= el->ip_used; i++) {
		ret = openchangesim_delete_interface_tap(ctx->mem_ctx, el->interfaces_fd[i]);
		logstr = talloc_asprintf(ctx->mem_ctx,
				"[*] Interface %d (fd %d) deleted\n",
				i, el->interfaces_fd[i]);
		openchangesim_printlog(f, logstr);
		talloc_free(logstr);
	}
	logstr = talloc_asprintf(ctx->mem_ctx, "[*] All %d virtual interfaces pending physical deletion\n",
				 el->ip_used+1);
	openchangesim_printlog(f, logstr);
	talloc_free(logstr);
	printf("\n");

	return OCSIM_SUCCESS;
}
