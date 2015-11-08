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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

/* Para los sockets no-bloqueantes */
#include <unistd.h>
#include <fcntl.h>

/* Para SDL_GetTicks */
#include <SDL.h>

#include "findfour.h"
#include "juego.h"
#include "netplay.h"
#include "stun.h"

#define MULTICAST_IPV4_GROUP "224.0.0.133"
#define MULTICAST_IPV6_GROUP "FF02::224:0:0:133"

/* Nuestro socket de red */
static int fd_socket;

/* El sockaddr para la dirección IPv4 y IPv6 multicast */
struct sockaddr_in mcast_addr;
struct sockaddr_in6 mcast_addr6;

Uint32 multicast_timer;

/* Prototipos locales */
void enviar_end_broadcast_game (void);

int findfour_netinit (int puerto) {
	struct sockaddr_storage bind_addr;
	struct sockaddr_in6 *ipv6;
	struct ip_mreq mcast_req;
	struct ipv6_mreq mcast_req6;
	int g;
	
	printf ("Estoy a la escucha en el puerto: %i\n", puerto);
	/* Crear, iniciar el socket */
	fd_socket = socket (AF_INET6, SOCK_DGRAM, 0);
	
	if (fd_socket < 0) {
		/* Mostrar la ventana de error */
		return -1;
	}
	
	ipv6 = (struct sockaddr_in6 *) &bind_addr;
	
	ipv6->sin6_family = AF_INET6;
	ipv6->sin6_port = htons (puerto);
	ipv6->sin6_flowinfo = 0;
	memcpy (&ipv6->sin6_addr, &in6addr_any, sizeof (in6addr_any));
	
	/* Asociar el socket con el puerto */
	if (bind (fd_socket, (struct sockaddr *) &bind_addr, sizeof (bind_addr)) < 0) {
		/* Mostrar ventana de error */
		
		return -1;
	}
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	fcntl (fd_socket, F_SETFL, O_NONBLOCK);
	
	/* Intentar el binding request */
	try_stun_binding ("stun.ekiga.net", fd_socket);
	
	/* Hacer join a los grupos multicast */
	g = 0;
	setsockopt (fd_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &g, sizeof(g));
	
	g = 1;
	setsockopt (fd_socket, IPPROTO_IP, IP_MULTICAST_TTL, &g, sizeof(g));
	
	/* Primero join al IPv4 */
	mcast_addr.sin_family = AF_INET;
	mcast_addr.sin_port = htons (puerto);
	
	inet_aton (MULTICAST_IPV4_GROUP, &mcast_addr.sin_addr.s_addr);
	mcast_req.imr_multiaddr.s_addr = mcast_addr.sin_addr.s_addr;
	mcast_req.imr_interface.s_addr = INADDR_ANY;
	
	if (setsockopt (fd_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast_req, sizeof(mcast_req)) < 0) {
		perror ("Error al hacer ADD_MEMBERSHIP IPv4 Multicast");
	}
	
	/* Intentar el join al grupo IPv6 */
	mcast_addr6.sin6_family = AF_INET6;
	mcast_addr6.sin6_port = htons (puerto);
	mcast_addr6.sin6_flowinfo = 0;
	mcast_addr6.sin6_scope_id = 0; /* Cualquier interfaz */
	
	g = 0;
	setsockopt (fd_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &g, sizeof (g));
	
	g = 64;
	setsockopt (fd_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &g, sizeof (g));
	
	inet_pton (AF_INET6, MULTICAST_IPV6_GROUP, &mcast_addr6.sin6_addr);
	memcpy (&mcast_req6.ipv6mr_multiaddr, &(mcast_addr6.sin6_addr), sizeof (struct in6_addr));
	mcast_req6.ipv6mr_interface = 0;
	
	if (setsockopt (fd_socket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mcast_req6, sizeof(mcast_req6)) < 0) {
		perror ("Error al hacer IPV6_ADD_MEMBERSHIP IPv6 Multicast");
	}
	
	enviar_broadcast_game (nick);
	multicast_timer = SDL_GetTicks ();
	
	/* Ningún error */
	return 0;
}

void findfour_netclose (void) {
	/* Enviar el multicast de retiro de partida */
	
	enviar_end_broadcast_game ();
	/* Y cerrar el socket */
	
	close (fd_socket);
}

void conectar_con (Juego *juego, const char *nick, const char *ip, const int puerto) {
	uint16_t temp;
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
	memcpy (&juego->cliente, res->ai_addr, res->ai_addrlen);
	
	/* Generar un número de secuencia aleatorio */
	while (juego->seq == 0) {
		juego->seq = RANDOM (65535);
	}
	
	/* Rellenar con la firma del protocolo FF */
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	
	/* Rellenar los bytes */
	juego->buffer_send[2] = FLAG_SYN;
	printf ("Conexión de salida, seq antes: %i\n", juego->seq);
	temp = htons (juego->seq++);
	printf ("Conexión de salida, seq después: %i\n", juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = 0; /* Ack 0 inicial */
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	juego->buffer_send[7] = 1; /* Versión del protocolo */
	strncpy (&juego->buffer_send[8], nick, sizeof (char) * NICK_SIZE);
	juego->buffer_send[7 + NICK_SIZE] = '\0';
	juego->len_send = 8 + NICK_SIZE;
	
	juego->tamsock = sizeof (juego->cliente);
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un SYN inicial. Mi seq: %i\n", juego->seq);
	juego->estado = NET_SYN_SENT;
}

void conectar_con_sockaddr (Juego *juego, const char *nick, struct sockaddr *destino, socklen_t tamsock) {
	uint16_t temp;
	
	/* Generar un número de secuencia aleatorio */
	while (juego->seq == 0) {
		juego->seq = RANDOM (65535);
	}
	
	/* Rellenar con la firma del protocolo FF */
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	
	/* Rellenar los bytes */
	juego->buffer_send[2] = FLAG_SYN;
	printf ("Conexión de salida, seq antes: %i\n", juego->seq);
	temp = htons (juego->seq++);
	printf ("Conexión de salida, seq después: %i\n", juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = 0; /* Ack 0 inicial */
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	juego->buffer_send[7] = 1; /* Versión del protocolo */
	strncpy (&juego->buffer_send[8], nick, sizeof (char) * NICK_SIZE);
	juego->buffer_send[7 + NICK_SIZE] = '\0';
	juego->len_send = 8 + NICK_SIZE;
	
	/* Copiar el sockaddr */
	memcpy (&juego->cliente, destino, tamsock);
	
	juego->tamsock = tamsock;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un SYN inicial. Mi seq: %i\n", juego->seq);
	juego->estado = NET_SYN_SENT;
}

void enviar_syn_ack (Juego *juego, FF_NET *recv, const char *nick) {
	uint16_t temp;
	
	printf ("Enviando SYN + ACK, el ack antes: %i\n", recv->base.seq);
	juego->ack = recv->base.seq + 1;
	printf ("Enviando SYN + ACK, el ack después: %i\n", juego->ack);
	while (juego->seq == 0) {
		juego->seq = RANDOM (65535);
	}
	
	/* Rellenar con la firma del protocolo FF */
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	
	/* Sortear quién empieza primero */
	juego->inicio = RANDOM (2);
	
	/* Rellenar los bytes */
	juego->buffer_send[2] = FLAG_SYN | FLAG_ACK;
	printf ("Enviando SYN + ACK, el seq antes: %i\n", juego->seq);
	temp = htons (juego->seq++);
	printf ("Enviando SYN + ACK, el seq después: %i\n", juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	juego->buffer_send[7] = 1; /* Versión del protocolo */
	strncpy (&juego->buffer_send[8], nick, sizeof (char) * NICK_SIZE);
	juego->buffer_send[7 + NICK_SIZE] = '\0';
	juego->buffer_send[8 + NICK_SIZE] = (juego->inicio == 0 ? 0 : 255);
	juego->len_send = 9 + NICK_SIZE;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un SYN + ACK. mi seq: %i, y el ack: %i\n", juego->seq, juego->ack);
}

void enviar_ack_0 (Juego *juego, FF_NET *recv) {
	uint16_t temp;
	
	printf ("Enviando ACK, el ack antes: %i\n", recv->base.seq);
	juego->ack = recv->base.seq + 1;
	printf ("Enviando ACK, el ack después: %i\n", juego->ack);
	
	/* Rellenar con la firma del protocolo FF */
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	
	juego->buffer_send[2] = FLAG_ACK;
	printf ("Enviando ACK, el seq antes: %i\n", juego->seq);
	temp = htons (juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	juego->len_send = 7;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un ACK. mi seq: %i, y el ack: %i\n", juego->seq, juego->ack);
}

void enviar_movimiento (Juego *juego, int col, int fila) {
	uint16_t temp;
	
	/* Rellenar con la firma del protocolo FF */
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	
	juego->buffer_send[2] = FLAG_TRN;
	temp = htons (juego->seq++);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	
	juego->buffer_send[7] = juego->turno++;
	juego->buffer_send[8] = col;
	juego->buffer_send[9] = fila;
	juego->len_send = 10;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un movimiento. Turno: %i, mi seq: %i y el ack: %i\n", juego->turno - 1, juego->seq, juego->ack);
	juego->estado = NET_WAIT_TRN_ACK;
}

void enviar_trn_ack (Juego *juego, FF_NET *recv) {
	uint16_t temp;
	
	juego->ack = recv->base.seq + 1;
	
	/* Rellenar con la firma del protocolo FF */
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	
	juego->buffer_send[2] = FLAG_TRN | FLAG_ACK;
	temp = htons (juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	
	juego->buffer_send[7] = juego->turno;
	juego->len_send = 8;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié mi confirmación de un movimiento. Turno: %i, mi seq: %i y el ack: %i\n", juego->turno - 1, juego->seq, juego->ack);
}

void enviar_keep_alive (Juego *juego) {
	uint16_t temp;
	
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	juego->buffer_send[2] = FLAG_ALV;
	temp = htons (juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	
	juego->len_send = 7;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	//juego->retry = 0;
	printf ("Envié un Keep Alive para ver si el otro sigue vivo, mi Seq: %i y el ack: %i\n", juego->seq, juego->ack);
}

void enviar_keep_alive_ack (Juego *juego, FF_NET *recv) {
	uint16_t temp;
	
	juego->ack = recv->base.seq;
	
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	juego->buffer_send[2] = FLAG_ALV | FLAG_ACK;
	temp = htons (juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	
	juego->len_send = 7;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un Keep Alive ACK, sigo vivo. mi Seq: %i y el ack: %i\n", juego->seq, juego->ack);
}

void enviar_fin (Juego *juego, FF_NET *recv, int razon) {
	uint16_t temp;
	printf ("Razon para terminar: %i\n", razon);
	if (recv != NULL) juego->ack = recv->base.seq + 1;
	
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	juego->buffer_send[2] = FLAG_FIN;
	temp = htons (juego->seq++);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	
	/* Empaquetar la razon de la desconexión */
	juego->buffer_send[7] = razon;
	
	juego->len_send = 8;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	
	if (razon == NET_DISCONNECT_YOUWIN || razon == NET_DISCONNECT_YOULOST) {
		juego->estado = NET_WAIT_WINNER_ACK;
	} else {
		juego->estado = NET_WAIT_CLOSING;
		/* Ocultar la ventana */
		juego->ventana.mostrar = FALSE;
	}
	
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un FIN. mi Seq: %i y el ack: %i\n", juego->seq, juego->ack);
}

void enviar_fin_ack (Juego *juego, FF_NET *recv) {
	uint16_t temp;
	
	juego->ack = recv->base.seq + 1;
	
	juego->buffer_send[0] = juego->buffer_send[1] = 'F';
	juego->buffer_send[2] = FLAG_FIN | FLAG_ACK;
	temp = htons (juego->seq);
	memcpy (&juego->buffer_send[3], &temp, sizeof (temp));
	temp = htons (juego->ack);
	memcpy (&juego->buffer_send[5], &temp, sizeof (temp));
	
	juego->len_send = 7;
	
	sendto (fd_socket, juego->buffer_send, juego->len_send, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	
	juego->last_response = SDL_GetTicks ();
	juego->retry = 0;
	printf ("Envié un FIN ACK. mi Seq: %i y el ack: %i\n", juego->seq, juego->ack);
}

void enviar_broadcast_game (char *nick) {
	char buffer[32];
	FF_broadcast_game bgame;
	
	buffer[0] = buffer[1] = 'F';
	buffer[2] = FLAG_MCG;
	
	buffer[3] = 1; /* Versión del protocolo */
	
	strncpy (&(buffer[4]), nick, sizeof (char) * NICK_SIZE);
	
	buffer[4 + NICK_SIZE - 1] = '\0';
	
	/* Enviar a IPv4 y IPv6 */
	sendto (fd_socket, buffer, 4 + NICK_SIZE, 0, (struct sockaddr *) &mcast_addr, sizeof (mcast_addr));
	
	sendto (fd_socket, buffer, 4 + NICK_SIZE, 0, (struct sockaddr *) &mcast_addr6, sizeof (mcast_addr6));
}

void enviar_end_broadcast_game (void) {
	char buffer[4];
	FF_broadcast_game bgame;
	
	buffer[0] = buffer[1] = 'F';
	buffer[2] = FLAG_MCG | FLAG_FIN;
	
	/* Enviar a IPv4 y IPv6 */
	sendto (fd_socket, buffer, 3, 0, (struct sockaddr *) &mcast_addr, sizeof (mcast_addr));
	
	sendto (fd_socket, buffer, 3, 0, (struct sockaddr *) &mcast_addr6, sizeof (mcast_addr6));
}

int unpack (FF_NET *net, char *buffer, size_t len) {
	uint16_t temp;
	memset (net, 0, sizeof (FF_NET));
	
	/* Mínimo son 7 bytes, firma del protocolo FF y Flags
	 * Número de secuencia y número de ack */
	if (len < 3) return -1;
	
	if (buffer[0] != 'F' || buffer[1] != 'F') {
		printf ("Protocol Mismatch. Expecting Find Four (FF)\n");
		
		return -1;
	}
	
	/* Las banderas son uint8, no tienen ningún problema */
	net->base.flags = buffer[2];
	
	memcpy (&temp, &buffer[3], sizeof (temp));
	net->base.seq = ntohs (temp);
	memcpy (&temp, &buffer[5], sizeof (temp));
	net->base.ack = ntohs (temp);
	
	if (net->base.flags & FLAG_SYN) {
		/* Contiene el nick y la versión del protocolo */
		printf ("SYN Package len: %i\n", len);
		net->syn.version = buffer[7];
		strncpy (net->syn.nick, &buffer[8], sizeof (char) * NICK_SIZE);
		
		if (net->base.flags & FLAG_ACK) {
			printf ("SYN + ACK Package len: %i\n", len);
			/* Este paquete tiene adicionalmente turno inicial */
			net->syn_ack.initial = buffer[8 + NICK_SIZE];
		}
	} else if (net->base.flags == FLAG_TRN) {
		printf ("TRN Package len: %i\n", len);
		net->trn.turno = buffer[7];
		net->trn.col = buffer[8];
		net->trn.fila = buffer[9];
	} else if (net->base.flags == FLAG_FIN) {
		printf ("FIN Package len: %i\n", len);
		net->fin.fin = buffer[7];
	} else if (net->base.flags == FLAG_MCG) {
		printf ("MCG Package len: %i\n", len);
		net->bgame.version = buffer[3];
		strncpy (net->bgame.nick, &buffer[4], sizeof (char) * NICK_SIZE);
	}
	
	/* Paquete completo */
	return 0;
}

void process_netevent (void) {
	char buffer [256];
	FF_NET netmsg;
	Juego *ventana, *next;
	struct sockaddr_storage cliente;
	socklen_t tamsock;
	int g;
	int manejado;
	int len;
	Uint32 now_time;
	uint16_t temp;
	
	do {
		tamsock = sizeof (cliente);
		/* Intentar hacer otra lectura para procesamiento */
		len = recvfrom (fd_socket, buffer, sizeof (buffer), 0, (struct sockaddr *) &cliente, &tamsock);
		
		if (len < 0) break;
		
		if (unpack (&netmsg, buffer, len) < 0) continue;
		
		if (netmsg.base.flags == FLAG_MCG) {
			/* Multicast de anuncio de partida de red */
			buddy_list_mcast_add (netmsg.bgame.nick, &cliente, tamsock);
			continue;
		} else if (netmsg.base.flags == (FLAG_MCG | FLAG_FIN)) {
			/* Multicast de eliminación de partida */
			buddy_list_mcast_remove (&cliente, tamsock);
			continue;
		}
		
		manejado = FALSE;
		/* Buscar el juego que haga match por el número de ack */
		ventana = (Juego *) primero;
		while (ventana != NULL) {
			/* Omitir esta ventana porque no es de tipo juego */
			if (ventana->ventana.tipo != WINDOW_GAME) {
				ventana = (Juego *) ventana->ventana.next;
				continue;
			}
			if (netmsg.base.ack == ventana->seq) {
				manejado = TRUE;
				/* Coincide por número de secuencia */
				if (netmsg.base.flags == FLAG_FIN) {
					/* Un fin en cualquier momento es fin */
					if (ventana->estado == NET_WAIT_WINNER) {
						enviar_fin_ack (ventana, &netmsg);
						ventana->estado = NET_CLOSED;
					} else {
						enviar_fin_ack (ventana, &netmsg);
						next = (Juego *) ventana->ventana.next;
						eliminar_juego (ventana);
						ventana = next;
						continue;
					}
				} else if (ventana->estado == NET_SYN_SENT && netmsg.base.flags == (FLAG_SYN | FLAG_ACK)) {
					
					/* Revisar quién empieza */
					if (netmsg.syn_ack.initial != 0 && netmsg.syn_ack.initial != 255) {
						printf ("No sabemos quién empieza, esto es un problema\n");
						enviar_fin (ventana, &netmsg, NET_DISCONNECT_UNKNOWN_START);
					} else {
						/* Enviar el ack 0 sólo si el turno inicial es válido */
						enviar_ack_0 (ventana, &netmsg);
						if (netmsg.syn_ack.initial == 0) {
							printf ("Ellos inician primero\n");
							ventana->inicio = 1;
						} else if (netmsg.syn_ack.initial == 255) {
							printf ("Nosotros empezamos\n");
							ventana->inicio = 0;
						}
						ventana->estado = NET_READY;
						printf ("Listo para jugar, envié mi último ack\n");
					}
				} else if (ventana->estado == NET_SYN_RECV && netmsg.base.flags == FLAG_ACK) {
					printf ("Listo para jugar\n");
					ventana->estado = NET_READY;
					//ventana->ack = netmsg.ack.seq + 1;
				} else if ((ventana->estado == NET_SYN_RECV || ventana->estado == NET_READY) && netmsg.base.flags == FLAG_TRN) {
					ventana->estado = NET_READY;
					
					if (recibir_movimiento (ventana, netmsg.trn.turno, netmsg.trn.col, netmsg.trn.fila, &g) == 0) {
						/* Como no hubo errores, enviar el ack */
						enviar_trn_ack (ventana, &netmsg);
						buscar_ganador (ventana);
					} else {
						/* Hubo errores, enviar un fin */
						enviar_fin (ventana, &netmsg, g);
					}
				} else if (ventana->estado == NET_WAIT_TRN_ACK && netmsg.base.flags == FLAG_TRN) {
					/* Nos llegó un movimiento cuando estabamos esperando el ack */
					/* Como su número de ack coincide con nuestro último envio, estamos bien */
					ventana->estado = NET_READY;
					
					if (recibir_movimiento (ventana, netmsg.trn.turno, netmsg.trn.col, netmsg.trn.fila, &g) == 0) {
						/* Como no hubo errores, enviar el ack */
						enviar_trn_ack (ventana, &netmsg);
						buscar_ganador (ventana);
					} else {
						/* Hubo errores, enviar un fin */
						enviar_fin (ventana, &netmsg, g);
					}
				} else if (ventana->estado == NET_WAIT_TRN_ACK && netmsg.base.flags == (FLAG_TRN | FLAG_ACK)) {
					/* Es una confirmación de mi movimiento */
					/* Nada que revisar */
					/* Tomar su número de seq */
					//ventana->ack = netmsg.base.seq + 1;
					ventana->estado = NET_READY;
					buscar_ganador (ventana);
				} else if (ventana->estado == NET_READY && netmsg.base.flags == (FLAG_ALV | FLAG_ACK)) {
					/* Una confirmación de keep alive */
					printf ("Recibí un Keep alive ACK\n");
					//ventana->ack = netmsg.base.seq + 1;
					ventana->retry = 0;
					ventana->last_response = SDL_GetTicks ();
				} else if (ventana->estado == NET_READY && netmsg.base.flags == FLAG_ALV) {
					/* Un keep alive */
					
					enviar_keep_alive_ack (ventana, &netmsg);
					ventana->retry = 0;
					ventana->last_response = SDL_GetTicks ();
					printf ("Recibí un keep alive, respondí con un Keep alive ACK\n");
				} else if (ventana->estado == NET_WAIT_CLOSING && netmsg.base.flags == (FLAG_FIN | FLAG_ACK)) {
					/* El ACK para nuestro FIN, ya cerramos y bye */
					next = (Juego *) ventana->ventana.next;
					eliminar_juego (ventana);
					ventana = next;
					printf ("Recibí un FIN + ACK, esto está totalmente cerrado\n");
					continue;
				} else if (ventana->estado == NET_WAIT_WINNER_ACK && netmsg.base.flags == (FLAG_FIN | FLAG_ACK)) {
					/* EL ACK de fin de que el juego está completo */
					ventana->estado = NET_CLOSED;
				}
			} else if (netmsg.base.ack == (ventana->seq - 1)) {
				manejado = TRUE;
				/* Coincide por número de secuencia, pero es una solicitud de repetición */
				
				/* Reenviar, pero modificar el ack con su número de secuencia como ack */
				//temp = htons (netmsg.base.seq + 1);
				//memcpy (&ventana->buffer_send[5], &temp, sizeof (temp));
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				printf ("Una repetición de paquete, responderemos con el último paquete que nosotros enviamos\n");
			}
			
			ventana = (Juego *) ventana->ventana.next;
		}
		
		if (!manejado) {
			if (netmsg.base.flags == FLAG_SYN) {
				/* Conexión inicial entrante */
				printf ("Nueva conexión entrante\n");
				ventana = crear_juego ();
			
				/* Copiar la dirección IP del cliente */
				memcpy (&ventana->cliente, &cliente, tamsock);
				ventana->tamsock = tamsock;
				ventana->estado = NET_SYN_RECV;
				
				enviar_syn_ack (ventana, &netmsg, "Gatuno Server");
			} else {
				printf ("Paquete desconocido\n");
			}
		}
	} while (1);
	
	/* Después de procesar los eventos de entrada,
	 * revisar los timers, para reenviar eventos */
	ventana = (Juego *) primero;
	now_time = SDL_GetTicks();
	
	while (ventana != NULL) {
		/* Omitir esta ventana porque no es de tipo juego */
		if (ventana->ventana.tipo != WINDOW_GAME) {
			ventana = (Juego *) ventana->ventana.next;
			continue;
		}
		if (ventana->retry >= 5) {
			if (ventana->estado == NET_WAIT_CLOSING) {
				/* Demasiados intentos */
				next = (Juego *) ventana->ventana.next;
				eliminar_juego (ventana);
				ventana = next;
			} else if (ventana->estado == NET_SYN_SENT) {
				/* Caso especial, enviamos un SYN y nunca llegó,
				 * ni siquiera intentamos enviar el fin */
				next = (Juego *) ventana->ventana.next;
				eliminar_juego (ventana);
				ventana = next;
			} else {
				/* Intentar enviar un último mensaje de FIN */
				enviar_fin (ventana, NULL, NET_DISCONNECT_NETERROR);
			}
			continue;
		}
		
		/* Enviar un keep alive sólo si NO es nuestro turno */
		if (ventana->estado == NET_READY && (ventana->turno % 2) != ventana->inicio) {
			if (ventana->retry == 0 && now_time > ventana->last_response + NET_READY_TIMER) {
				/* Enviar un Keep Alive */
				enviar_keep_alive (ventana);
				ventana->retry = 1;
			} else if (ventana->retry > 0 && now_time > ventana->last_response + NET_CONN_TIMER) {
				/* Reenviar el keep alive forma más continua */
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			}
		} else if (now_time > ventana->last_response + NET_CONN_TIMER) {
			//printf ("Reenviando por timer: %i\n", ventana->estado);
			if (ventana->estado == NET_SYN_SENT) {
				printf ("Reenviando SYN inicial timer\n");
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			} else if (ventana->estado == NET_SYN_RECV) {
				printf ("Reenviando SYN + ACK por timer\n");
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			} else if (ventana->estado == NET_WAIT_TRN_ACK) {
				printf ("Reenviando TRN por timer\n");
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			} else if (ventana->estado == NET_WAIT_CLOSING) {
				printf ("Reenviando FIN por timer\n");
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			} else if (ventana->estado == NET_WAIT_WINNER_ACK) {
				printf ("Reenviando FIN de ganador por timer\n");
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			} else if (ventana->estado == NET_WAIT_WINNER) {
				printf ("Reenviando TRN + ACK esperando a que llegue el FIN\n");
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			}
		}
		
		ventana = (Juego *) ventana->ventana.next;
	}
	
	/* Enviar el anuncio de juego multicast */
	if (now_time > multicast_timer + NET_MCAST_TIMER) {
		enviar_broadcast_game (nick);
		multicast_timer = now_time;
	}
}
