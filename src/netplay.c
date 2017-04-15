/*
 * netplay.c
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>

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

/**
 * IPV6_ADD_MEMBERSHIP/IPV6_DROP_MEMBERSHIP may not be defined on OSX
 */
#ifdef MACOSX
#	ifndef IPV6_ADD_MEMBERSHIP
#		define IPV6_ADD_MEMBERSHIP     IPV6_JOIN_GROUP
#		define IPV6_DROP_MEMBERSHIP    IPV6_LEAVE_GROUP
#	endif
#endif

/* Para los sockets no-bloqueantes */
#include <unistd.h>
#include <fcntl.h>

/* Para SDL_GetTicks */
#include <SDL.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_GETIFADDRS
#include <ifaddrs.h>
#include <net/if.h>
#endif

#include "findfour.h"
#include "juego.h"
#include "netplay.h"
#include "stun.h"
#include "message.h"
#include "resolv.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MULTICAST_IPV4_GROUP "224.0.0.133"
#define MULTICAST_IPV6_GROUP "FF02::224:0:0:133"

/* Nuestro socket de red */
#ifdef __MINGW32__
static SOCKET fd_socket6, fd_socket4;
#else
static int fd_socket6, fd_socket4;
#endif

/* El sockaddr para la dirección IPv4 y IPv6 multicast */
struct sockaddr_in mcast_addr;
struct sockaddr_in6 mcast_addr6;

Uint32 multicast_timer;

void conectar_juego (Juego *juego, const char *nick);

/* Función de Kazuho Oku
 * https://gist.github.com/kazuho/45eae4f92257daceb73e
 */
int sockaddr_cmp (struct sockaddr *x, struct sockaddr *y) {
#define CMP(a, b) if (a != b) return a < b ? -1 : 1

	CMP(x->sa_family, y->sa_family);

	if (x->sa_family == AF_INET) {
		struct sockaddr_in *xin = (void*)x, *yin = (void*)y;
		CMP(ntohl(xin->sin_addr.s_addr), ntohl(yin->sin_addr.s_addr));
		CMP(ntohs(xin->sin_port), ntohs(yin->sin_port));
	} else if (x->sa_family == AF_INET6) {
		struct sockaddr_in6 *xin6 = (void*)x, *yin6 = (void*)y;
		int r = memcmp (xin6->sin6_addr.s6_addr, yin6->sin6_addr.s6_addr, sizeof(xin6->sin6_addr.s6_addr));
		if (r != 0) return r;
		CMP(ntohs(xin6->sin6_port), ntohs(yin6->sin6_port));
		CMP(xin6->sin6_flowinfo, yin6->sin6_flowinfo);
		CMP(xin6->sin6_scope_id, yin6->sin6_scope_id);
	} else {
		return -1; /* Familia desconocida */
	}

#undef CMP
	return 0;
}

#ifdef __MINGW32__
int findfour_try_netinit4 (int puerto) {
	struct addrinfo hints, *resultados;
	struct ip_mreq mcast_req;
	char buff_p[10];
	unsigned char g;
	int fd;
	
	fd = socket (AF_INET, SOCK_DGRAM, 0);
	
	if (fd == INVALID_SOCKET) {
		printf ("Error al crear socket AF_INET\n");
		return -1;
	}
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		printf ("WSA Error: %i\n", WSAGetLastError());
		close (fd);
		return -1;
	}
	
	/* Asociar el socket con el puerto */
	if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) < 0) {
		/* Mostrar ventana de error */
		printf ("WSA Error: %i\n", WSAGetLastError());
		printf ("Error de bind\n");
		return -1;
	}
	
	freeaddrinfo (resultados);
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	u_long flags = 1;
	ioctlsocket (fd, FIONBIO, &flags);
	
	/* Hacer join a los grupos multicast */
	/* Primero join al IPv4 */
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons (puerto);
	mcast_addr.sin_addr.s_addr = inet_addr (MULTICAST_IPV4_GROUP);
	
	mcast_req.imr_multiaddr.s_addr = inet_addr (MULTICAST_IPV4_GROUP);
	mcast_req.imr_interface.s_addr = INADDR_ANY;
	
	if (setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mcast_req, sizeof(mcast_req)) < 0) {
		printf ("WSA Error: %i\n", WSAGetLastError());
		printf ("Error al hacer ADD_MEMBERSHIP IPv4 Multicast\n");
	}
	
	g = 0;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_LOOP, &g, sizeof(g));
	g = 1;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_TTL, &g, sizeof(g));
	
	return fd;
}

int findfour_try_netinit6 (int puerto) {
	struct addrinfo hints, *resultados;
	struct ipv6_mreq mcast_req6;
	unsigned int h;
	char buff_p[10];
	int fd;
	
	fd = socket (AF_INET6, SOCK_DGRAM, 0);
	
	if (fd == INVALID_SOCKET) {
		printf ("Error al crear socket AF_INET6\n");
		return -1;
	}
	
	h = 1;
	setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&h, sizeof(h));
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET6;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		perror ("Getaddrinfo IPv6");
		close (fd);
		return -1;
	}
	
	/* Asociar el socket con el puerto */
	if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) < 0) {
		/* Mostrar ventana de error */
		printf ("WSA Error: %i\n", WSAGetLastError());
		printf ("Error de bind\n");
		return -1;
	}
	
	freeaddrinfo (resultados);
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	u_long flags = 1;
	ioctlsocket (fd, FIONBIO, &flags);
	
	/* Intentar el join al grupo IPv6 */
	mcast_addr6.sin6_family = AF_INET6;
	mcast_addr6.sin6_port = htons (puerto);
	mcast_addr6.sin6_flowinfo = 0;
	mcast_addr6.sin6_scope_id = 0; /* Cualquier interfaz */
	
	inet_pton (AF_INET6, MULTICAST_IPV6_GROUP, &mcast_addr6.sin6_addr);
	memcpy (&mcast_req6.ipv6mr_multiaddr, &(mcast_addr6.sin6_addr), sizeof (struct in6_addr));
	mcast_req6.ipv6mr_interface = 0;
	
	if (setsockopt (fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&mcast_req6, sizeof(mcast_req6)) < 0) {
		printf ("WSA Error: %i\n", WSAGetLastError());
		printf ("Error al hacer IPV6_ADD_MEMBERSHIP IPv6 Multicast\n");
	}
	
	h = 0;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *)&h, sizeof (h));
	h = 64;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *)&h, sizeof (h));
	
	return fd;
}

int findfour_netinit (int puerto) {
	struct addrinfo hints, *resultados;
	struct ip_mreq mcast_req;
	struct ipv6_mreq mcast_req6;
	unsigned char g;
	unsigned int h;
	char buff_p[10];
	
	WSADATA wsaData;
	int nRet = WSAStartup( MAKEWORD(2,2), &wsaData );
	if (nRet == SOCKET_ERROR ) {
		fprintf (stderr, "Failed to init Winsock library\n");
		return -1;
	}
	
	fd_socket4 = findfour_try_netinit4 (puerto);
	
	fd_socket6 = findfour_try_netinit6 (puerto);
	
	if (fd_socket4 < 0 && fd_socket6 < 0) {
		/* Mostrar la ventana de error */
		/* Cerrar el socket que se haya podido abrir */
		if (fd_socket4 >= 0) close (fd_socket4);
		if (fd_socket6 >= 0) close (fd_socket6);
		return -1;
	}
	
	/* Intentar el binding request */
	try_stun_binding ("stun.ekiga.net");
	
	enviar_broadcast_game (nick_global);
	multicast_timer = SDL_GetTicks ();
	
	/* Ningún error */
	return 0;
}

void findfour_netclose (void) {
	/* Enviar el multicast de retiro de partida */
	enviar_end_broadcast_game ();
	
	/* Y cerrar el socket */
	if (fd_socket4 >= 0) closesocket (fd_socket4);
	if (fd_socket6 >= 0) closesocket (fd_socket6);
	
	WSACleanup();
}

SOCKET findfour_get_socket4 (void) {
	return fd_socket4;
}
#else

int findfour_try_netinit4 (int puerto) {
	struct addrinfo hints, *resultados, *ressave;
	struct ip_mreq mcast_req;
	char buff_p[10];
	unsigned char g;
	int fd;
	
	fd = socket (AF_INET, SOCK_DGRAM, 0);
	
	if (fd < 0) {
		perror ("Socket AF_INET");
		return -1;
	}
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		perror ("Getaddrinfo IPv4");
		close (fd);
		return -1;
	}
	
	ressave = resultados;
	/* Recorrer los resultados hasta que uno haga bind */
	while (resultados != NULL) {
		if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) >= 0) {
			/* Por fin un bind */
			break;
		}
		resultados = resultados->ai_next;
	}
	
	freeaddrinfo (ressave);
	if (resultados == NULL) {
		fprintf (stderr, "Ningún bind hizo efecto\n");
		close (fd);
		return -1;
	}
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	int flags;
	flags = fcntl (fd, F_GETFL, 0);
	flags = flags | O_NONBLOCK;
	fcntl (fd, F_SETFL, flags);
	
	inet_pton (AF_INET, MULTICAST_IPV4_GROUP, &mcast_addr.sin_addr.s_addr);
	memset (&mcast_req, 0, sizeof (mcast_req));
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons (puerto);
	/* Hacer join a los grupos multicast */
#if HAVE_GETIFADDRS
	/* Listar todas las interfaces para hacer join en todas */
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	int rc;
	
	rc = getifaddrs (&ifap);
	
	if (rc == 0) { /* Sin error */
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr->sa_family == AF_INET &&
				(ifa->ifa_flags & IFF_LOOPBACK) == 0 && /* No queremos las interfaces Loopback */
				(ifa->ifa_flags & IFF_MULTICAST)) { /* Y solo las que soportan multicast */
				sa = (struct sockaddr_in *) ifa->ifa_addr;
				
				mcast_req.imr_multiaddr.s_addr = mcast_addr.sin_addr.s_addr;
				mcast_req.imr_interface.s_addr = sa->sin_addr.s_addr;
				
				if (setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast_req, sizeof(mcast_req)) < 0) {
					perror ("Error al hacer ADD_MEMBERSHIP IPv4 Multicast");
				}
			}
		}
		freeifaddrs(ifap);
	} else {
#endif
	/* Join a la interfaz 0.0.0.0 */
	mcast_req.imr_multiaddr.s_addr = mcast_addr.sin_addr.s_addr;
	mcast_req.imr_interface.s_addr = INADDR_ANY;
	
	if (setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast_req, sizeof(mcast_req)) < 0) {
		perror ("Error al hacer ADD_MEMBERSHIP IPv4 Multicast");
	}
#if HAVE_GETIFADDRS
	}
#endif
	
	g = 0;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_LOOP, &g, sizeof(g));
	g = 1;
	setsockopt (fd, IPPROTO_IP, IP_MULTICAST_TTL, &g, sizeof(g));
	
	return fd;
}

int findfour_try_netinit6 (int puerto) {
	struct addrinfo hints, *resultados, *ressave;
	struct ipv6_mreq mcast_req6;
	unsigned int h;
	char buff_p[10];
	int fd;
	
	fd = socket (AF_INET6, SOCK_DGRAM, 0);
	
	if (fd < 0) {
		perror ("Socket AF_INET6");
		return -1;
	}
	
	h = 1;
	setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&h, sizeof(h));
	
	/* Intentar hacer el bind, pero por medio de getaddrinfo */
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET6;
	
	sprintf (buff_p, "%i", puerto);
	
	if (getaddrinfo (NULL, buff_p, &hints, &resultados) < 0) {
		perror ("Getaddrinfo IPv6");
		close (fd);
		return -1;
	}
	
	ressave = resultados;
	/* Recorrer los resultados hasta que uno haga bind */
	while (resultados != NULL) {
		if (bind (fd, resultados->ai_addr, resultados->ai_addrlen) >= 0) {
			/* Por fin un bind */
			break;
		}
		resultados = resultados->ai_next;
	}
	
	freeaddrinfo (ressave);
	if (resultados == NULL) {
		fprintf (stderr, "Ningún bind hizo efecto v6\n");
		close (fd);
		return -1;
	}
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	fcntl (fd, F_SETFL, O_NONBLOCK);
	
	/* Intentar el join al grupo IPv6 */
	inet_pton (AF_INET6, MULTICAST_IPV6_GROUP, &mcast_addr6.sin6_addr);
	mcast_addr6.sin6_family = AF_INET6;
	mcast_addr6.sin6_port = htons (puerto);
	mcast_addr6.sin6_flowinfo = 0;
	mcast_addr6.sin6_scope_id = 0;
	
#if HAVE_GETIFADDRS
	/* Listar todas las interfaces para hacer join en todas */
	struct ifaddrs *ifap, *ifa;
	int rc;
	rc = getifaddrs (&ifap);
	
	if (rc == 0) { /* Sin error */
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr->sa_family == AF_INET6 &&
				(ifa->ifa_flags & IFF_LOOPBACK) == 0 && /* No queremos las interfaces Loopback */
				(ifa->ifa_flags & IFF_MULTICAST)) { /* Y solo las que soportan multicast */
				memset (&mcast_req6, 0, sizeof (mcast_req6));
				
				memcpy (&mcast_req6.ipv6mr_multiaddr, &(mcast_addr6.sin6_addr), sizeof (struct in6_addr));
				mcast_req6.ipv6mr_interface = if_nametoindex (ifa->ifa_name);
				
				if (mcast_req6.ipv6mr_interface != 0) {
					if (setsockopt (fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mcast_req6, sizeof(mcast_req6)) < 0) {
						perror ("Error al hacer IPV6_ADD_MEMBERSHIP IPv6 Multicast");
					}
				}
			}
		}
		freeifaddrs(ifap);
	} else {
#endif
	memcpy (&mcast_req6.ipv6mr_multiaddr, &(mcast_addr6.sin6_addr), sizeof (struct in6_addr));
	mcast_req6.ipv6mr_interface = 0; /* Cualquier interfaz */
	
	if (setsockopt (fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mcast_req6, sizeof(mcast_req6)) < 0) {
		perror ("Error al hacer IPV6_ADD_MEMBERSHIP IPv6 Multicast");
	}
#if HAVE_GETIFADDRS
	}
#endif
	
	h = 0;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &h, sizeof (h));
	h = 64;
	setsockopt (fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &h, sizeof (h));
	
	return fd;
}

int findfour_netinit (int puerto) {
	/* Crear, iniciar el socket de IPv4 */
	fd_socket4 = findfour_try_netinit4 (puerto);
	
	fd_socket6 = findfour_try_netinit6 (puerto);
	
	if (fd_socket4 < 0 && fd_socket6 < 0) {
		/* Mostrar la ventana de error */
		/* Cerrar el socket que se haya podido abrir */
		if (fd_socket4 >= 0) close (fd_socket4);
		if (fd_socket6 >= 0) close (fd_socket6);
		return -1;
	}
	
	/* Intentar el binding request */
	try_stun_binding ("stun.ekiga.net");
	
	enviar_broadcast_game (nick_global);
	multicast_timer = SDL_GetTicks ();
	
	/* Ningún error */
	return 0;
}

void findfour_netclose (void) {
	/* Enviar el multicast de retiro de partida */
	enviar_end_broadcast_game ();
	
	/* Cerrar los sockets */
	if (fd_socket4 >= 0) close (fd_socket4);
	if (fd_socket6 >= 0) close (fd_socket6);
}

int findfour_get_socket4 (void) {
	return fd_socket4;
}
#endif

/* TODO: Función que ya debería estar obsoleta */
void conectar_con (Juego *juego, const char *nick, const char *ip, const int puerto) {
	int n;
	struct addrinfo hints, *res;
	char buffer_port[10];
	
	memset (&hints, 0, sizeof (hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	sprintf (buffer_port, "%i", puerto);
	
	/* Antes de intentar la conexión, hacer la resolución de nombres */
	n = getaddrinfo (ip, buffer_port, &hints, &res);
	
	if (n < 0 || res == NULL) {
		/* Lanzar mensaje de error */
		eliminar_juego (juego);
		return;
	}
	
	/* Tomar y copiar el primer resultado */
	memcpy (&juego->peer, res->ai_addr, res->ai_addrlen);
	juego->peer_socklen = res->ai_addrlen;
	
	/* Sortear el turno inicial */
	juego->inicio = RANDOM(2);
	
	conectar_juego (juego, nick);
}

void conectar_con_sockaddr (Juego *juego, const char *nick, struct sockaddr *peer, socklen_t peer_socklen) {
	memcpy (&juego->peer, peer, peer_socklen);
	juego->peer_socklen = peer_socklen;
	
	/* Sortear el turno inicial */
	juego->inicio = RANDOM(2);
	
	conectar_juego (juego, nick);
}

void conectar_juego (Juego *juego, const char *nick) {
	char buffer_send[128];
	uint16_t temp;
	
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_SYN;
	
	/* Número local */
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	/* Número remoto */
	buffer_send[6] = buffer_send[7] = 0;
	
	/* El nick local */
	strncpy (&buffer_send[8], nick, sizeof (char) * NICK_SIZE);
	buffer_send[7 + NICK_SIZE] = '\0';
	
	buffer_send[8 + NICK_SIZE] = (juego->inicio == 1 ? 255 : 0);
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 9 + NICK_SIZE, 0, (struct sockaddr *) &juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié un SYN inicial. Mi local port: %i\n", juego->local);
	juego->estado = NET_SYN_SENT;
}

void enviar_res_syn (Juego *juego, const char *nick) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_RES_SYN;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	/* El nick local */
	strncpy (&buffer_send[8], nick, sizeof (char) * NICK_SIZE);
	buffer_send[7 + NICK_SIZE] = '\0';
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 8 + NICK_SIZE, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envie un RES_SYN. Estoy listo para jugar. Mi local prot: %i\n", juego->local);
	
	juego->estado = NET_READY;
}

void enviar_movimiento (Juego *juego, int turno, int col, int fila) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_TRN;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	buffer_send[8] = turno;
	buffer_send[9] = col;
	buffer_send[10] = fila;
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 11, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié un movimiento.\n");
	
	juego->estado = NET_WAIT_ACK;
}

void enviar_mov_ack (Juego *juego) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_TRN_ACK;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	buffer_send[8] = juego->turno;
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 9, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié una confirmación de movimiento.\n");
}

void enviar_mov_ack_finish (Juego *juego, int reason) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_TRN_ACK_GAME;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	buffer_send[8] = juego->turno;
	
	buffer_send[9] = reason;
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 10, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié una confirmación de movimiento con ganador.\n");
}

void enviar_fin (Juego *juego) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_FIN;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	buffer_send[8] = juego->last_fin;
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 9, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié un petición de FIN.\n");
	juego->estado = NET_WAIT_CLOSING;
}

void enviar_fin_ack (Juego *juego) {
	char buffer_send[128];
	uint16_t temp;
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_FIN_ACK;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 8, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié una confirmación de FIN.\n");
	juego->estado = NET_CLOSED;
}

void enviar_keep_alive (Juego *juego) {
	char buffer_send[128];
	uint16_t temp;
	
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_KEEP_ALIVE;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 8, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Envié keep alive para ver si sigue vivo.\n");
}

void enviar_keep_alive_ack (Juego *juego) {
	char buffer_send[128];
	uint16_t temp;
	
	/* Rellenar con la firma del protocolo FF */
	buffer_send[0] = buffer_send[1] = 'F';
	
	/* Poner el campo de la versión */
	buffer_send[2] = 2;
	
	/* El campo de tipo */
	buffer_send[3] = TYPE_KEEP_ALIVE_ACK;
	
	temp = htons (juego->local);
	memcpy (&buffer_send[4], &temp, sizeof (temp));
	
	temp = htons (juego->remote);
	memcpy (&buffer_send[6], &temp, sizeof (temp));
	
	sendto ((juego->peer.ss_family == AF_INET ? fd_socket4 : fd_socket6), buffer_send, 8, 0, (struct sockaddr *)&juego->peer, juego->peer_socklen);
	juego->last_response = SDL_GetTicks ();
	
	//printf ("Respondí con un Keep Alive ACK\n");
}

int unpack (FFMessageNet *msg, char *buffer, size_t len) {
	uint16_t temp;
	
	/* Vaciar la estructura */
	memset (msg, 0, sizeof (FFMessageNet));
	
	if (len < 4) return -1;
	
	if (buffer[0] != 'F' || buffer[1] != 'F') {
		printf ("Protocol Mismatch!\n");
		
		return -1;
	}
	
	if (buffer[2] != 2) {
		printf ("Version mismatch. Expecting 2\n");
		
		return -1;
	}
	
	msg->type = buffer[3];
	
	if (msg->type == TYPE_SYN) {
		if (len < (9 + NICK_SIZE)) return -1; /* Oops, tamaño incorrecto */
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* No hay puerto remoto, aún */
		
		/* Copiar el nick */
		strncpy (msg->nick, &buffer[8], sizeof (char) * NICK_SIZE);
		msg->nick[NICK_SIZE - 1] = 0;
		
		/* Copiar quién empieza */
		msg->initial = buffer[8 + NICK_SIZE];
		
		/* Validar el nick */
		if (!is_utf8 (msg->nick)) {
			strncpy (msg->nick, "--NICK--", sizeof (char) * NICK_SIZE);
		}
	} else if (msg->type == TYPE_RES_SYN) {
		if (len < (8 + NICK_SIZE)) return -1; /* Oops, tamaño incorrecto */
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		/* Copiar el nick */
		strncpy (msg->nick, &buffer[8], sizeof (char) * NICK_SIZE);
		msg->nick[NICK_SIZE - 1] = 0;
		
		/* Copiar quién empieza */
		msg->initial = buffer[8 + NICK_SIZE];
		/* Validar el nick */
		if (!is_utf8 (msg->nick)) {
			strncpy (msg->nick, "--NICK--", sizeof (char) * NICK_SIZE);
		}
	} else if (msg->type == TYPE_TRN) {
		if (len < 11) return -1;
		
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		msg->turno = buffer[8];
		msg->col = buffer[9];
		msg->fila = buffer[10];
	} else if (msg->type == TYPE_TRN_ACK) {
		if (len < 9) return -1;
		
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		msg->turn_ack = buffer[8];
	} else if (msg->type == TYPE_TRN_ACK_GAME) {
		if (len < 10) return -1;
		
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		msg->turn_ack = buffer[8];
		msg->win = buffer[9];
	} else if (msg->type == TYPE_FIN) {
		if (len < 9) return -1;
		
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
		
		msg->fin = buffer[8];
	} else if (msg->type == TYPE_FIN_ACK) {
		if (len < 8) return -1;
		
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
	} else if (msg->type == TYPE_MCAST_ANNOUNCE) {
		if (len < 4 + NICK_SIZE) return -1;
		
		/* Copiar el nick */
		strncpy (msg->nick, &buffer[4], sizeof (char) * NICK_SIZE);
		msg->nick[NICK_SIZE - 1] = 0;
		
		/* Validar el nick */
		if (!is_utf8 (msg->nick)) {
			strncpy (msg->nick, "--NICK--", sizeof (char) * NICK_SIZE);
		}
	} else if (msg->type == TYPE_MCAST_FIN) {
		/* Ningún dato extra */
	} else if (msg->type == TYPE_KEEP_ALIVE) {
		if (len < 8) return -1;
		
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
	} else if (msg->type == TYPE_KEEP_ALIVE_ACK) {
		if (len < 8) return -1;
		
		/* Copiar el puerto local */
		memcpy (&temp, &buffer[4], sizeof (temp));
		msg->local = ntohs (temp);
		
		/* Copiar el puerto remoto */
		memcpy (&temp, &buffer[6], sizeof (temp));
		msg->remote = ntohs (temp);
	} else {
		return -1;
	}
	
	return 0;
}

void check_for_retry (void) {
	Uint32 now_time;
	Juego *ventana, *next;
	
	now_time = SDL_GetTicks();
	ventana = (Juego *) get_first_window ();
	
	while (ventana != NULL) {
		/* Omitir esta ventana porque no es de tipo juego */
		if (ventana->ventana.tipo != WINDOW_GAME) {
			ventana = (Juego *) ventana->ventana.next;
			continue;
		}
		if (ventana->retry >= 5) {
			if (ventana->estado == NET_WAIT_CLOSING || ventana->estado == NET_SYN_SENT) {
				/* Demasiados intentos */
				/* Caso especial, enviamos un SYN y nunca llegó,
				 * ni siquiera intentamos enviar el fin */
				next = (Juego *) ventana->ventana.next;
				eliminar_juego (ventana);
				ventana = next;
			} else {
				/* Intentar enviar un último mensaje de FIN */
				ventana->last_fin = NET_DISCONNECT_NETERROR;
				ventana->retry = 0;
				enviar_fin (ventana);
				ventana->ventana.mostrar = FALSE;
			}
			continue;
		}
		if (now_time > ventana->last_response + NET_CONN_TIMER) {
			//printf ("Reenviando por timer: %i\n", ventana->estado);
			if (ventana->estado == NET_SYN_SENT) {
				//printf ("Reenviando SYN inicial timer\n");
				conectar_juego (ventana, nick_global);
				ventana->retry++;
			} else if (ventana->estado == NET_WAIT_ACK || ventana->estado == NET_WAIT_WINNER) {
				//printf ("Reenviando TRN por timer\n");
				enviar_movimiento (ventana, ventana->turno - 1, ventana->last_col, ventana->last_fila);
				ventana->retry++;
			} else if (ventana->estado == NET_WAIT_CLOSING) {
				//printf ("Reenviando FIN por timer\n");
				enviar_fin (ventana);
				ventana->retry++;
			} else if (ventana->estado == NET_READY && ventana->turno % 2 != ventana->inicio) {
				//printf ("Enviando Keep Alive\n");
				enviar_keep_alive (ventana);
				ventana->retry++;
			}
		}
		ventana = (Juego *) ventana->ventana.next;
	}
	
	/* Enviar el anuncio de juego multicast */
	if (now_time > multicast_timer + NET_MCAST_TIMER) {
		enviar_broadcast_game (nick_global);
		multicast_timer = now_time;
		buddy_list_mcast_clean (now_time);
	}
}

int do_read (void *buffer, size_t buffer_len, struct sockaddr *src_addr, socklen_t *addrlen) {
	int len;
	if (fd_socket4 >= 0) {
		/* Intentar una lectura del socket de IPv4 */
		len = recvfrom (fd_socket4, buffer, buffer_len, 0, src_addr, addrlen);
		
		if (len >= 0) return len;
	}
	
	if (fd_socket6 >= 0) {
		/* Intentar una lectura del socket de IPv6 */
		len = recvfrom (fd_socket6, buffer, buffer_len, 0, src_addr, addrlen);
		
		if (len >= 0) return len;
	}
	
	return -1;
}

void process_netevent (void) {
	char buffer [256];
	Juego *ventana, *next;
	FFMessageNet message;
	struct sockaddr_storage peer;
	socklen_t peer_socklen;
	int len;
	int manejado;

#ifdef HAVE_WORKING_FORK
	pending_query ();
#endif
	
	do {
		peer_socklen = sizeof (peer);
		
		/* Intentar hacer otra lectura para procesamiento */
		len = do_read (buffer, sizeof (buffer), (struct sockaddr *) &peer, &peer_socklen);
		
		if (len < 0) break;
		
		/* Detectar si este es un paquete STUN */
		if (buffer[0] == 0x01) {
			parse_stun_message (buffer, len);
			continue;
		}
		
		if (unpack (&message, buffer, len) < 0) {
			printf ("Recibí un paquete mal estructurado\n");
			continue;
		}
		
		if (message.type == TYPE_MCAST_ANNOUNCE) {
			/* Multicast de anuncio de partida de red */
			buddy_list_mcast_add (message.nick, &peer, peer_socklen);
			continue;
		} else if (message.type == TYPE_MCAST_FIN) {
			/* Multicast de eliminación de partida */
			buddy_list_mcast_remove (&peer, peer_socklen);
			continue;
		}
		
		/* Si es una conexión inicial (SYN), verificar que ésta no sea una repetición de algo que ya hayamos enviado */
		if (message.type == TYPE_SYN) {
			ventana = (Juego *) get_first_window ();
			
			manejado = FALSE;
			while (ventana != NULL) {
				if (ventana->ventana.tipo != WINDOW_GAME) {
					ventana = (Juego *) ventana->ventana.next;
					continue;
				}
				
				if (message.local == ventana->remote && sockaddr_cmp ((struct sockaddr *) &ventana->peer, (struct sockaddr *) &peer) == 0) {
					/* Coincidencia por puerto remoto y dirección, es una repetición */
					ventana->retry++;
					enviar_res_syn (ventana, nick_global);
					manejado = TRUE;
					break;
				}
				ventana = (Juego *) ventana->ventana.next;
			}
			
			if (manejado) continue;
			
			/* Si no fué manejado es conexión nueva */
			//printf ("Nueva conexión entrante\n");
			ventana = crear_juego (FALSE);
		
			/* Copiar la dirección IP del peer */
			memcpy (&ventana->peer, &peer, peer_socklen);
			ventana->peer_socklen = peer_socklen;
			
			/* Copiar el puerto remoto */
			ventana->remote = message.local;
			
			/* Copiar quién empieza */
			if (message.initial == 0) {
				ventana->inicio = 1;
			} else if (message.initial == 255) {
				ventana->inicio = 0;
			}
			
			recibir_nick (ventana, message.nick);
			
			enviar_res_syn (ventana, nick_global);
			
			ventana->estado = NET_READY;
			continue;
		}
		
		/* Buscar el puerto local que coincide con el mensaje */
		ventana = (Juego *) get_first_window ();
		while (ventana != NULL) {
			if (ventana->ventana.tipo != WINDOW_GAME || message.remote != ventana->local) {
				ventana = (Juego *) ventana->ventana.next;
				continue;
			}
			
			if (message.type == TYPE_FIN) {
				if (message.fin == NET_USER_QUIT && ventana->nick_remoto[0] != 0) {
					message_add (MSG_NORMAL, "Ok", "%s ha cerrado la partida", ventana->nick_remoto);
				} else {
					message_add (MSG_ERROR, "Ok", "La partida se ha cerrado\nErr: %i", message.fin);
				}
				enviar_fin_ack (ventana);
				
				/* Eliminar esta ventana */
				eliminar_juego (ventana);
			} else if (message.type == TYPE_KEEP_ALIVE) {
				/* Keep alive en cualquier momento se responde con Keep Alive ACK */
				enviar_keep_alive_ack (ventana);
			} else if ((message.type == TYPE_RES_SYN && ventana->estado != NET_SYN_SENT) ||
			    (message.type == TYPE_TRN && (ventana->estado != NET_READY && ventana->estado != NET_WAIT_ACK)) ||
			    (message.type == TYPE_TRN_ACK && ventana->estado != NET_WAIT_ACK) ||
			    (message.type == TYPE_TRN_ACK_GAME && ventana->estado != NET_WAIT_WINNER) ||
			    (message.type == TYPE_FIN_ACK && ventana->estado != NET_WAIT_CLOSING) ||
			    (message.type == TYPE_KEEP_ALIVE_ACK && ventana->estado != NET_READY)) {
				printf ("Paquete en el momento incorrecto\n");
			} else if (message.type == TYPE_RES_SYN) {
				/* Copiar el nick del otro jugador */
				recibir_nick (ventana, message.nick);
				
				/* Copiar el puerto remoto */
				ventana->remote = message.local;
				
				//printf ("Recibí RES SYN. Su puerto remoto es: %i\n", ventana->remote);
				ventana->estado = NET_READY;
			} else if (message.type == TYPE_TRN) {
				/* Si estaba en el estado WAIT_ACK, y recibo un movimiento,
				 * eso confirma el turno que estaba esperando y pasamos a recibir el movimiento */
				if (ventana->estado == NET_WAIT_ACK && message.turno == ventana->turno) {
					//printf ("Movimiento de turno cuando esperaba confirmación\n");
					ventana->estado = NET_READY;
				}
				
				/* Recibir el movimiento */
				recibir_movimiento (ventana, message.turno, message.col, message.fila);
			} else if (message.type == TYPE_TRN_ACK) {
				/* Verificar que el turno confirmado sea el local */
				
				if (message.turn_ack == ventana->turno) {
					ventana->estado = NET_READY;
				} else {
					//printf ("FIXME: ???\n");
				}
			} else if (message.type == TYPE_TRN_ACK_GAME) {
				/* Última confirmación de turno */
				ventana->estado = NET_CLOSED;
			} else if (message.type == TYPE_FIN_ACK) {
				/* La última confirmación que necesitaba */
				/* Eliminar esta ventana */
				eliminar_juego (ventana);
			} else if (message.type == TYPE_KEEP_ALIVE_ACK) {
				ventana->retry = 0;
			}
			break;
		}
	} while (1);
	
	check_for_retry ();
}

void enviar_broadcast_game (char *nick) {
	char buffer[32];
	
	buffer[0] = buffer[1] = 'F';
	buffer[2] = 2; /* Versión del protocolo */
	
	buffer[3] = TYPE_MCAST_ANNOUNCE;
	
	strncpy (&(buffer[4]), nick, sizeof (char) * NICK_SIZE);
	
	buffer[4 + NICK_SIZE - 1] = '\0';
	
	/* Enviar a IPv4 y IPv6 */
	sendto (fd_socket4, buffer, 4 + NICK_SIZE, 0, (struct sockaddr *) &mcast_addr, sizeof (mcast_addr));
	sendto (fd_socket6, buffer, 4 + NICK_SIZE, 0, (struct sockaddr *) &mcast_addr6, sizeof (mcast_addr6));
}

void enviar_end_broadcast_game (void) {
	char buffer[4];
	
	buffer[0] = buffer[1] = 'F';
	buffer[2] = 2; /* Versión del protocolo */
	
	buffer[3] = TYPE_MCAST_FIN;
	
	/* Enviar a IPv4 y IPv6 */
	sendto (fd_socket4, buffer, 4, 0, (struct sockaddr *) &mcast_addr, sizeof (mcast_addr));
	sendto (fd_socket6, buffer, 4, 0, (struct sockaddr *) &mcast_addr6, sizeof (mcast_addr6));
}

