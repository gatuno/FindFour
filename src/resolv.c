/*
 * resolv.c
 * This file is part of Find Four
 *
 * Copyright (C) 2016 - Félix Arreola Rodríguez
 *
 * Find Four is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Find Four is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Find Four; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

/* Para el manejo de red */
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netdb.h>
#endif
#include <sys/types.h>
#include <fcntl.h>

#include <signal.h>

#include "resolv.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MAX_ADDR_RESPONSE_LEN 1048576

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

/*
 * This is a convenience struct used to pass data to
 * the child resolver process.
 */
typedef struct {
	char hostname[512];
	int port;
} dns_params_t;

typedef struct _CallbackNode {
	char hostname[512];
	int port;
	ResolvCallback callback;
	struct _CallbackNode *next;
} CallbackNode;

/* Prototipos */
static void resolver_run (int child_out, int child_in);

/* Variables globales */
static pid_t forked_child;
static int child_in, child_out;
static CallbackNode *resolv_list = NULL;

static int use_non_fork_resolv = 0;

void init_resolver (void) {
#ifndef HAVE_WORKING_FORK
	use_non_fork_resolv = 1;
	
	return;
#else
	pid_t child;
	
	int proc_in[2], proc_out[2];
	
	/* Abrir 2 pipes de comunicación hacia el proceso hijo */
	if (pipe (proc_in) || pipe (proc_out)) {
		forked_child = 0;
		return;
	}
	
	/* "Go fork and multiply." --Tommy Caldwell (Emily's dad, not the climber) */
	forked_child = fork ();
	
	if (forked_child == 0) {
		/* Proceso hijo */
		close (proc_in[1]);
		close (proc_out[0]);
		
		resolver_run (proc_out[1], proc_in[0]);
	}
	
	close (proc_in[0]);
	close (proc_out[1]);
	
	if (forked_child == -1) {
		close (proc_in[1]);
		close (proc_out[0]);
		
		use_non_fork_resolv = 1;
		
		fprintf (stderr, "Error al crear proceso para resolución de nombres\n");
		forked_child = 0;
		return;
	}
	
	child_out = proc_out[0];
	child_in = proc_in[1];
	
	/* Aplicar no-bloqueante a la entrada del hijo */
#ifdef __MINGW32__
	u_long flags = 1;
	ioctlsocket (child_out, FIONBIO, &flags);
#else
	int flags;
	flags = fcntl (child_out, F_GETFL, 0);
	flags = flags | O_NONBLOCK;
	fcntl (child_out, F_SETFL, flags);
#endif
#endif
}

static void cancel_requests (void) {
	CallbackNode *node, *next;
	/* Cancelar todo lo pendiente */
	node = resolv_list;
	
	while (node != NULL) {
		next = node->next;
		
		node->callback (NULL);
		free (node);
		
		node = next;
	}
	
	resolv_list = NULL;
}

static void destroy_resolver (void) {
	if (forked_child > 0) {
		kill(forked_child, SIGKILL);
	}
	
	forked_child = 0;
	
	close (child_in);
	close (child_out);
	
	cancel_requests ();
}

/* Funciones del proceso hijo */
static void write_to_parent(int fd, const void *buf, size_t count) {
	ssize_t written;

	written = write(fd, buf, count);
	if (written < 0 || (size_t)written != count) {
		if (written < 0)
			fprintf(stderr, "dns[%d]: Error writing data to "
					"parent: %s\n", getpid(), strerror(errno));
		else
			fprintf(stderr, "dns[%d]: Error: Tried to write %zu bytes to parent but instead "
					"wrote %zd bytes\n",
					getpid(), count, written);
	}
}

static void resolver_run (int child_out, int child_in) {
	dns_params_t dns_params;
	const size_t zero = 0;
	int rc;
	struct addrinfo hints, *res, *tmp;
	char servname[20];
	char *hostname;
	
	while (1) {
		rc = read(child_in, &dns_params, sizeof(dns_params_t));
	
		if (rc == 0) {
			printf ("dns[%d]: Oops, father has gone, wait for me, wait...!\n", getpid());
			_exit(0);
		}
	
		if (rc < 0) {
			fprintf (stderr, "dns[%d]: Error: Could not read dns_params: %s\n", getpid(), strerror(errno));
			break;
		}
		
		if (dns_params.hostname[0] == '\0') {
			fprintf (stderr, "dns[%d]: Error: Parent requested resolution of an empty hostname (port = %d)!!!\n", getpid(), dns_params.port);
			_exit(1);
		}
		
		hostname = strdup(dns_params.hostname);
		
		snprintf(servname, sizeof(servname), "%d", dns_params.port);
		memset(&hints, 0, sizeof(hints));

		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags |= AI_ADDRCONFIG;
		
		rc = getaddrinfo(hostname, servname, &hints, &res);
		write_to_parent(child_out, &rc, sizeof(rc));
		if (rc != 0) {
			printf("dns[%d] Error: getaddrinfo returned %d\n",
					getpid(), rc);
			dns_params.hostname[0] = '\0';
			free(hostname);
			hostname = NULL;
			//break;
			continue;
		}
		tmp = res;
		while (res) {
			size_t ai_addrlen = res->ai_addrlen;
			write_to_parent(child_out, &ai_addrlen, sizeof(ai_addrlen));
			write_to_parent(child_out, res->ai_addr, res->ai_addrlen);
			res = res->ai_next;
		}
		freeaddrinfo(tmp);
		
		write_to_parent(child_out, &zero, sizeof(zero));
		dns_params.hostname[0] = '\0';

		free(hostname);
		hostname = NULL;
	}
	
	close (child_out);
	close (child_in);
	
	printf ("Child process exiting!\n");
	_exit (0);
}
/* Fin de las funciones del proceso hijo */

/* Resolución de nombres sin fork */
void resolv_non_fork (const char *hostname, int puerto, ResolvCallback callback) {
	struct addrinfo hints, *res;
	char servname[20];
	int rc;
	
	snprintf(servname, sizeof(servname), "%d", puerto);
	memset(&hints, 0, sizeof(hints));
	
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_ADDRCONFIG;
	
	rc = getaddrinfo(hostname, servname, &hints, &res);
	
	if (rc < 0) {
		callback (NULL);
		
		return;
	}
	
	callback (res);
	
	freeaddrinfo (res);
}

static int send_dns_request_to_child (const char *hostname, int puerto) {
	pid_t pid;
	dns_params_t dns_params;
	ssize_t rc;
	
	pid = waitpid (forked_child, NULL, WNOHANG);
	if (pid > 0) {
		fprintf (stderr, "dns: DNS child %d no longer exists\n", forked_child);
		forked_child = 0;
		destroy_resolver ();
	} else if (pid < 0) {
		fprintf (stderr, "dns: Wait for DNS child %d failed: %s\n",
				forked_child, strerror (errno));
		destroy_resolver ();
	}
	
	if (forked_child == 0) {
		/* El proceso hijo murió por alguna extraña razón, intentar volverlo a lanzar */
		init_resolver ();
	}
	
	/* Copy the hostname and port into a single data structure */
	strncpy(dns_params.hostname, hostname, sizeof(dns_params.hostname) - 1);
	dns_params.hostname[sizeof(dns_params.hostname) - 1] = '\0';
	dns_params.port = puerto;

	/* Send the data structure to the child */
	rc = write(child_in, &dns_params, sizeof(dns_params));
	if (rc < 0) {
		fprintf (stderr, "dns: Unable to write to DNS child %d: %s\n",
				forked_child, strerror(errno));
		destroy_resolver ();
		return FALSE;
	}
	if ((size_t)rc < sizeof(dns_params)) {
		fprintf (stderr, "dns: Tried to write %zu bytes to child but only wrote %zd\n",
				sizeof(dns_params), rc);
		destroy_resolver ();
		return FALSE;
	}
	
	return TRUE;
}

void do_query (const char *hostname, int puerto, ResolvCallback callback) {
	int res;
	CallbackNode *node, *pos;
	
	if (use_non_fork_resolv) {
		resolv_non_fork (hostname, puerto, callback);
		return;
	}
	res = send_dns_request_to_child (hostname, puerto);
	
	if (res == FALSE) {
		callback (NULL);
		return;
	}
	
	/* Agregar al final de la lista */
	node = malloc (sizeof (CallbackNode));
	
	strncpy (node->hostname, hostname, sizeof (node->hostname) - 1);
	node->hostname[sizeof(node->hostname) - 1] = '\0';
	node->callback = callback;
	node->port = puerto;
	node->next = NULL;
	
	if (resolv_list == NULL) {
		resolv_list = node;
	} else {
		pos = resolv_list;
		while (pos->next != NULL) {
			pos = pos->next;
		}
		pos->next = node;
	}
}

static void free_local_addrinfo (struct addrinfo *list) {
	struct addrinfo *next;
	
	while (list != NULL) {
		next = list->ai_next;
		
		free (list->ai_addr);
		free (list);
		
		list = next;
	}
}

void pending_query (void) {
	int rc, err;
	CallbackNode *next;
	struct sockaddr *addr = NULL;
	size_t addrlen;
	struct addrinfo *ip_list, *ip, *ip_next;
	
	/* Nada que vigilar */
	if (resolv_list == NULL) return;
	
	rc = read(child_out, &err, sizeof(err));
	if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return;
	} else if (rc < 0) {
		perror ("Error reading from resolver process");
		/* ¿Debería destruir la lista de consultas pendientes */
	} else if (rc == 0) {
		fprintf (stderr, "Resolver process exited without answering our request\n");
	} else if (rc == 4 && err != 0) {
		if (resolv_list == NULL) {
			fprintf (stderr, "Advertencia: Resolución no solicitada\n");
			return;
		}
		fprintf (stderr, "Error resolving %s:\n%s\n", resolv_list->hostname, gai_strerror (err));
		
		resolv_list->callback (NULL);
		
		/* Quitar de la lista */
		next = resolv_list->next;
		free (resolv_list);
		resolv_list = next;
	} else if (rc > 0) {
		/* Success! */
		ip_list = NULL;
		while (rc > 0) {
			rc = read(child_out, &addrlen, sizeof(addrlen));
			if (rc > 0 && addrlen > 0 && addrlen < MAX_ADDR_RESPONSE_LEN) {
				ip = malloc (sizeof (struct addrinfo));
				
				addr = malloc(addrlen);
				rc = read(child_out, addr, addrlen);
				if (ip_list == NULL) {
					ip_list = ip;
				} else {
					ip_next->ai_next = ip;
				}
				ip_next = ip;
				
				ip->ai_next = NULL;
				ip->ai_addrlen = addrlen;
				ip->ai_addr = addr;
			} else {
				break;
			}
		}
		/* Ejecutar la llamada */
		resolv_list->callback (ip_list);
		
		free_local_addrinfo (ip_list);
		/* Quitar de la lista */
		next = resolv_list->next;
		free (resolv_list);
		resolv_list = next;
	}
}

int analizador_hostname_puerto (const char *cadena, char *hostname, int *puerto, int default_port) {
	char *p, *port, *invalid;
	int g;
	
	if (cadena[0] == 0) {
		hostname[0] = 0;
		return FALSE;
	}
	
	*puerto = default_port;
	
	if (cadena[0] == '[') {
		/* Es una ip literal, buscar por otro ] de cierre */
		p = strchr (cadena, ']');
		
		if (p == NULL) {
			/* Error, no hay cierre */
			hostname[0] = 0;
			return FALSE;
		}
		strncpy (hostname, &cadena[1], p - cadena - 1);
		hostname [p - cadena - 1] = 0;
		p++;
		cadena = p;
	} else {
		/* Nombre de host directo */
		port = strchr (cadena, ':');
		
		if (port == NULL) {
			/* No hay puerto, sólo host directo */
			strcpy (hostname, cadena);
			
			return TRUE;
		} else {
			/* Copiar hasta antes del ":" */
			strncpy (hostname, cadena, port - cadena);
			hostname [port - cadena] = 0;
		}
		cadena = port;
	}
	
	/* Buscar por un posible ":" */
	if (cadena[0] == ':') {
		g = strtol (&cadena[1], &invalid, 10);
		
		if (invalid[0] != 0 || g > 65535) {
			/* Caracteres sobrantes */
			hostname[0] = 0;
			return FALSE;
		}
		
		*puerto = g;
	}
	
	return TRUE;
}
