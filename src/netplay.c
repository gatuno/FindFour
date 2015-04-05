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

/* Para los sockets no-bloqueantes */
#include <unistd.h>
#include <fcntl.h>

#include "findfour.h"
#include "netplay.h"

int findfour_netinit (void) {
	struct sockaddr_storage bind_addr;
	struct sockaddr_in6 *ipv6;
	int fd;
	
	/* Crear, iniciar el socket */
	fd = socket (AF_INET6, SOCK_DGRAM, 0);
	
	if (fd < 0) {
		/* Mostrar la ventana de error */
		return -1;
	}
	
	ipv6 = (struct sockaddr_in6 *) &bind_addr;
	
	ipv6->sin6_family = AF_INET6;
	ipv6->sin6_port = htons (SERVER_PORT);
	ipv6->sin6_flowinfo = 0;
	memcpy (&ipv6->sin6_addr, &in6addr_any, sizeof (in6addr_any));
	
	/* Asociar el socket con el puerto */
	if (bind (fd, (struct sockaddr *) &bind_addr, sizeof (bind_addr)) < 0) {
		/* Mostrar ventana de error */
		
		return -1;
	}
	
	/* No utilizaré poll, sino llamadas no-bloqueantes */
	fcntl (fd, F_SETFL, O_NONBLOCK);
	
	return fd;
}

void unpack (FF_NET *net, char *buffer, size_t len) {
	memset (net, 0, sizeof (FF_NET));
	/* Las banderas son uint8, no tienen ningún problema */
	net->flags.flags = buffer[0];
	
	if (net->flags.syn && net->flags.trn) {
		net->syn_trn.inicio = buffer[1];
	} else if (net->flags.syn && net->flags.ack) {
		/* Copiar la parte del syn */
		net->syn_ack.syn.version = buffer[1];
		memcpy (&net->syn_ack.syn.nick, buffer + 2, NICK_SIZE);
		
		/* Copiar la parte del ack */
		net->syn_ack.ack.ack = buffer[2 + NICK_SIZE];
		/* La otra parte del ack no nos interesa en este caso */
	} else if (net->flags.syn) {
		net->syn.version = buffer[1];
		memcpy (&net->syn.nick, buffer + 2, NICK_SIZE);
	} else if (net->flags.ack && net->flags.trn) {
		net->trn_ack.ack.ack = buffer[1];
		net->trn_ack.ack.fila = buffer[2];
		
		net->trn_ack.trn.turno = buffer[3];
		net->trn_ack.trn.columna = buffer[4];
	} else if (net->flags.trn) {
		net->trn.turno = buffer[1];
		net->trn.columna = buffer[2];
	} else if (net->flags.ack) {
		net->ack.ack = buffer[1];
		net->ack.fila = buffer[2];
	}
}

int pack (FF_NET *net, char *buffer) {
	buffer[0] = net->flags.flags;
	
	if (net->flags.syn && net->flags.trn) {
		buffer[1] = net->syn_trn.inicio;
		return 2;
	} else if (net->flags.syn && net->flags.ack) {
		/* Copiar la parte del syn */
		buffer[1] = net->syn_ack.syn.version;
		memcpy (buffer + 2, &net->syn_ack.syn.nick, NICK_SIZE);
		
		/* Copiar la parte del ack */
		buffer[2 + NICK_SIZE] = net->syn_ack.ack.ack;
		/* La otra parte del ack no nos interesa en este caso */
		return 3 + NICK_SIZE;
	} else if (net->flags.syn) {
		buffer[1] = net->syn.version;
		memcpy (buffer + 2, &net->syn.nick, NICK_SIZE);
		return 2 + NICK_SIZE;
	} else if (net->flags.ack && net->flags.trn) {
		buffer[1] = net->trn_ack.ack.ack;
		buffer[2] = net->trn_ack.ack.fila;
		
		buffer[3] = net->trn_ack.trn.turno;
		buffer[4] = net->trn_ack.trn.columna;
		
		return 5;
	} else if (net->flags.trn) {
		buffer[1] = net->trn.turno;
		buffer[2] = net->trn.columna;
		
		return 3;
	} else if (net->flags.ack) {
		buffer[1] = net->ack.ack;
		buffer[2] = net->ack.fila;
		
		return 3;
	}
}

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

void enviar_syn (int fd, Juego *juego, char *nick) {
	FF_NET net;
	char buffer [256];
	int len;
	
	memset (&net, 0, sizeof (net));
	
	net.flags.syn = 1;
	net.syn.version = 0;
	memcpy (net.syn.nick, nick, NICK_SIZE);
	
	len = pack (&net, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->estado = NET_WAIT_SYN_ACK;
	
	printf ("Envié mi SYN al peer\n");
}

void enviar_syn_trn (int fd, Juego *juego) {
	FF_NET net;
	char buffer [256];
	int len;
	
	memset (&net, 0, sizeof (net));
	
	net.flags.syn = net.flags.trn = 1;
	net.syn_trn.inicio = juego->inicio;
	
	len = pack (&net, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->estado = NET_WAIT_SYN_TRN_ACK;
	
	printf ("Le envié quién empieza, ");
	if (juego->inicio == 0) {
		printf ("nosotros\n");
	} else {
		printf ("ellos\n");
	}
}

void enviar_syn_trn_ack (int fd, Juego *juego) {
	FF_NET net;
	char buffer [256];
	int len;
	
	memset (&net, 0, sizeof (net));
	
	net.flags.syn = net.flags.trn = net.flags.ack = 1;
	net.syn_trn.inicio = juego->inicio;
	
	len = pack (&net, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->estado = NET_READY;
	
	printf ("Respondimos con la confirmación de quién empieza, ");
	if (juego->inicio == 0) {
		printf ("nosotros\n");
	} else {
		printf ("ellos\n");
	}
}

void enviar_ack (int fd, Juego *juego, int fila) {
	FF_NET net;
	char buffer [256];
	int len;
	
	memset (&net, 0, sizeof (net));
	net.flags.ack = 1;
	net.ack.ack = juego->turno;
	net.ack.fila = fila;
	
	len = pack (&net, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	
	printf ("Envie el ack del turno: %i\n", net.ack.ack);
}

void enviar_ack_0 (int fd, Juego *juego) {
	FF_NET net;
	char buffer [256];
	int len;
	
	memset (&net, 0, sizeof (net));
	net.flags.ack = 1;
	net.ack.ack = 0;
	
	len = pack (&net, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	juego->estado = NET_WAIT_SYN_TRN;
	printf ("Envié ack 0 del syn que me envió el servidor\n");
}

void enviar_movimiento (int fd, Juego *juego, int col) {
	FF_NET net;
	char buffer [256];
	int len;
	
	memset (&net, 0, sizeof (net));
	net.flags.trn = 1;
	net.trn.turno = juego->turno;
	net.trn.columna = col;
	
	juego->turno++;
	
	len = pack (&net, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &juego->cliente, juego->tamsock);
	
	juego->estado = NET_WAIT_ACK;
	printf ("Envié mi movimiento al otro extremo\n");
}

void process_netevent (int fd) {
	char buffer [256];
	int len;
	FF_NET netmsg;
	Juego *ventana;
	int g, h;
	
	struct sockaddr_storage cliente;
	socklen_t tamsock;
	
	do {
		tamsock = sizeof (cliente);
		/* Intentar hacer otra lectura para procesamiento */
		len = recvfrom (fd, buffer, sizeof (buffer), 0, (struct sockaddr *) &cliente, &tamsock);
		
		if (len < 0) break;
		
		unpack (&netmsg, buffer, len);
		
		/* Buscar el juego que haga match por la dirección IP */
		for (ventana = primero; ventana != NULL; ventana = ventana->next) {
			if (sockaddr_cmp ((struct sockaddr *) &cliente, (struct sockaddr *) &(ventana->cliente)) == 0) {
				/* Evento para este juego */
				printf ("Estado: Antes del procesamiento: %i\n", ventana->estado);
				switch (ventana->estado) {
					case NET_WAIT_SYN_ACK:
						/* Esperamos el nick y ack del servidor */
						if (netmsg.flags.syn && netmsg.flags.ack && !netmsg.flags.trn) {
							printf ("Recibí SYN + ACK, copiando\n");
							/* Copiar el nick del servidor */
							memcpy (ventana->nick, netmsg.syn.nick, NICK_SIZE);
							enviar_ack_0 (fd, ventana);
						} else {
							printf ("No esperado\n");
						}
						break;
					case NET_WAIT_ACK_0 :
						/* Esperamos un ACK */
						if (!netmsg.flags.syn && netmsg.flags.ack && !netmsg.flags.trn && netmsg.ack.ack == 0) {
							/* Recibir y enviar quién empieza el baile */
							printf ("Recibí el ack 0\n");
							enviar_syn_trn (fd, ventana);
						} else {
							/* En caso contrario, no me gusta esta conexión, me envió algo no solicitado */
							/* FIXME: ¿Qué hago con esta conexión? */
							printf ("No esperado\n");
						}
						break;
					case NET_WAIT_SYN_TRN:
						if (netmsg.flags.syn && netmsg.flags.trn && !netmsg.flags.ack) {
							/* Recibí el turno, cambiar quién empieza */
							printf ("Recibí SYN + TRN\n");
							if (netmsg.syn_trn.inicio == 0) {
								/* Ellos empiezan */
								ventana->inicio = 1;
							} else {
								/* Nosotros empezamos */
								ventana->inicio = 0;
							}
							
							enviar_syn_trn_ack (fd, ventana);
						} else {
							printf ("No esperado\n");
						}
						break;
					case NET_WAIT_SYN_TRN_ACK:
						/* Esperamos su confirmación de inicio de turno invertida, para que quede claro que quién inicia */
						if (netmsg.flags.syn && netmsg.flags.ack && netmsg.flags.trn) {
							printf ("Recibí SYN + TRN + ACK\n");
							if (ventana->inicio == netmsg.syn_trn.inicio) {
								printf ("Problema, el otro extremo espera ser primero\n");
							} else {
								/* Todo bien, pasar a jugar */
								ventana->estado = NET_READY;
								printf ("Paso al juego\n");
							}
						}
						break;
					case NET_READY:
						if (netmsg.flags.trn && !netmsg.flags.ack && !netmsg.flags.syn) {
							if (ventana->turno % 2 == ventana->inicio) {
								/* Es nuestro turno y nos envian un movimiento, es un error. Repetir el último ack nuestro */
								printf ("Turno incorrecto\n");
							} else {
								/* Recibir el movimiento y responder con el ack */
								printf ("Recibe el número de turno: %i, mi turno es %i\n", netmsg.trn.turno, ventana->turno);
								if (netmsg.trn.turno == ventana->turno) {
									h = netmsg.trn.columna;
									
									if (ventana->tablero[5][h] != 0) {
										/* Tablero lleno, columna equivocada */
									} else {
										g = 5;
										while (g > 0 && ventana->tablero[g][h] != 0) g--;
										
										/* Poner la ficha en la posición [g][h] y avanzar turno */
										ventana->tablero[g][h] = (ventana->turno % 2) + 1;
										ventana->turno++;
										/* Enviar el ack */
										enviar_ack (fd, ventana, g);
									}
								} else {
									/* Número equivocado de turno */
								}
							}
						}
						break;
					case NET_WAIT_ACK:
						if (netmsg.flags.ack && !netmsg.flags.trn && !netmsg.flags.syn) {
							/* Llegó el ack de nuestro movimiento enviado */
							printf ("Recibí ACK\n");
							printf ("Recibe el número de turno: %i, mi turno es %i\n", netmsg.ack.ack, ventana->turno);
							if (netmsg.ack.ack == ventana->turno + 1) {
								ventana->estado = NET_READY;
								//ventana->turno++;
								/* TODO: Revisar la fila en la que cayó */
							} else {
								/* Confirmación de turno incorrecto */
								printf ("Confirmación de turno incorrecto\n");
							}
						} else {
							printf ("Paquete equivocado\n");
						}
						break;
				}
				
				printf ("Estado: post al procesamiento: %i\n", ventana->estado);
			}
		}
		
		if (netmsg.flags.syn == 1 && netmsg.flags.ack == 0 && netmsg.flags.trn == 0) {
			/* Nuevo juego entrante */
			ventana = crear_ventana ();
			
			/* Copiar la dirección IP del cliente */
			memcpy (&ventana->cliente, &cliente, tamsock);
			ventana->tamsock = tamsock;
			
			/* Copiar el nick del otro jugador */
			memcpy (ventana->nick, netmsg.syn.nick, NICK_SIZE);
			
			/* Preparar y enviar un mensaje de SYN + ACK */
			memset (&netmsg, 0, sizeof (netmsg));
			
			netmsg.flags.syn = netmsg.flags.ack = 1;
			netmsg.syn_ack.syn.version = 0;
			memcpy (netmsg.syn_ack.syn.nick, "Gatuno", strlen ("Gatuno") + 1);
			
			netmsg.syn_ack.ack.ack = 0;
			
			len = pack (&netmsg, buffer);
			
			printf ("Nueva conexión entrante, envié SYN + ACK\n");
			sendto (fd, buffer, len, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
			ventana->estado = NET_WAIT_ACK_0;
		}
	} while (1);
}


