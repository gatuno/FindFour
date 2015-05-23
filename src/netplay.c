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

/* Para SDL_GetTicks */
#include <SDL.h>

#include "findfour.h"
#include "netplay.h"

static int fd_socket;

int findfour_netinit (int puerto) {
	printf ("Estoy a la escucha en el puerto: %i\n", puerto);
	struct sockaddr_storage bind_addr;
	struct sockaddr_in6 *ipv6;
	
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
	
	/* Ningún error */
	return 0;
}

void conectar_con (Juego *juego, const char *nick, const char *ip, const int puerto) {
	uint16_t temp;
	
	printf ("Conectando al puerto: %i\n", puerto);
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
	
	/* Para conectar, hay que convertir la ip a sockaddr 
	 * FIXME: Hacer conversión con GetAddresInfo */
	struct sockaddr_in *ipv4;
	ipv4 = (struct sockaddr_in *) &juego->cliente;
	ipv4->sin_family = AF_INET;
	ipv4->sin_port = htons (puerto);
	inet_pton (AF_INET, "127.0.0.1", &ipv4->sin_addr);
	
	juego->tamsock = sizeof (juego->cliente);
	
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
	temp = htons (juego->seq++);
	printf ("Enviando ACK, el seq después: %i\n", juego->seq);
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
	temp = htons (juego->seq++);
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
	int g, h;
	int manejado;
	int len;
	Uint32 now_time;
	
	do {
		tamsock = sizeof (cliente);
		/* Intentar hacer otra lectura para procesamiento */
		len = recvfrom (fd_socket, buffer, sizeof (buffer), 0, (struct sockaddr *) &cliente, &tamsock);
		
		if (len < 0) break;
		
		if (unpack (&netmsg, buffer, len) < 0) continue;
		
		manejado = FALSE;
		/* Buscar el juego que haga match por el número de ack */
		for (ventana = primero; ventana != NULL; ventana = ventana->next) {
			if (netmsg.base.ack == ventana->seq) {
				manejado = TRUE;
				/* Coincide por número de secuencia */
				if (ventana->estado == NET_SYN_SENT && netmsg.base.flags == (FLAG_SYN | FLAG_ACK)) {
					/* Enviar el ack 0 */
					enviar_ack_0 (ventana, &netmsg);
					if (netmsg.syn_ack.initial == 0) {
						printf ("Ellos inician primero\n");
						ventana->inicio = 1;
					} else if (netmsg.syn_ack.initial == 255) {
						printf ("Nosotros empezamos\n");
						ventana->inicio = 0;
					} else {
						printf ("No sabemos quién empieza, esto es un problema\n");
					}
					ventana->estado = NET_READY;
					printf ("Listo para jugar, envié mi último ack\n");
				} else if (ventana->estado == NET_SYN_RECV && netmsg.base.flags == FLAG_ACK) {
					printf ("Listo para jugar\n");
					ventana->estado = NET_READY;
					ventana->ack = netmsg.ack.seq + 1;
				} else if ((ventana->estado == NET_SYN_RECV || ventana->estado == NET_READY) && netmsg.base.flags == FLAG_TRN) {
					ventana->estado = NET_READY;
					
					if (ventana->turno != netmsg.trn.turno) {
						printf ("Número de turno equivocado\n");
					} else if (ventana->turno % 2 == ventana->inicio) {
						printf ("Es nuestro turno, cerrar esta conexión\n");
					} else {
						h = netmsg.trn.col;
						if (ventana->tablero[0][h] != 0) {
							/* Tablero lleno, columna equivocada */
							/* FIXME: Detener esta partida */
							printf ("Columna llena, no deberias poner fichas ahí\n");
						} else {
							g = 5;
							while (g > 0 && ventana->tablero[g][h] != 0) g--;
						
							/* Poner la ficha en la posición [g][h] y avanzar turno */
							if (g != netmsg.trn.fila) {
								printf ("La ficha de mi lado cayó en la fila incorrecta\n");
							} else {
								ventana->tablero[g][h] = (ventana->turno % 2) + 1;
								ventana->turno++;
								enviar_trn_ack (ventana, &netmsg);
							}
						}
					}
				} else if (ventana->estado == NET_WAIT_TRN_ACK && netmsg.base.flags == FLAG_TRN) {
					/* Nos llegó un movimiento cuando estabamos esperando el ack */
					/* Como su número de ack coincide con nuestro último envio, estamos bien */
					ventana->estado = NET_READY;
					
					if (ventana->turno != netmsg.trn.turno) {
						printf ("Número de turno equivocado\n");
					} else if (ventana->turno % 2 == ventana->inicio) {
						printf ("Es nuestro turno, cerrar esta conexión\n");
					} else {
						h = netmsg.trn.col;
						if (ventana->tablero[0][h] != 0) {
							/* Tablero lleno, columna equivocada */
							/* FIXME: Detener esta partida */
							printf ("Columna llena, no deberias poner fichas ahí\n");
						} else {
							g = 5;
							while (g > 0 && ventana->tablero[g][h] != 0) g--;
						
							/* Poner la ficha en la posición [g][h] y avanzar turno */
							if (g != netmsg.trn.fila) {
								printf ("La ficha de mi lado cayó en la fila incorrecta\n");
							} else {
								ventana->tablero[g][h] = (ventana->turno % 2) + 1;
								ventana->turno++;
								enviar_trn_ack (ventana, &netmsg);
							}
						}
					}
				} else if (ventana->estado == NET_WAIT_TRN_ACK && netmsg.base.flags == (FLAG_TRN | FLAG_ACK)) {
					/* Es una confirmación de mi movimiento */
					/* Nada que revisar */
					ventana->estado = NET_READY;
				}
			} else if (netmsg.base.ack == (ventana->seq - 1)) {
				manejado = TRUE;
				/* Coincide por número de secuencia, pero es una solicitud de repetición */
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				printf ("Una repetición de paquete, responderemos con el último paquete que nosotros enviamos\n");
			}
		}
		
		if (!manejado) {
			if (netmsg.base.flags == FLAG_SYN) {
				/* Conexión inicial entrante */
				printf ("Nueva conexión entrante\n");
				ventana = crear_ventana ();
			
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
	ventana = primero;
	now_time = SDL_GetTicks();
	
	while (ventana != NULL) {
		if (ventana->retry >= 5) {
			/* Demasiados intentos */
			next = ventana->next;
			eliminar_ventana (ventana);
			
			/* TODO: Intentar enviar un último mensaje de FIN */
			ventana = next;
			continue;
		}
		
		if (ventana->estado == NET_READY) {
			if (ventana->retry == 0 && now_time > ventana->last_response + NET_READY_TIMER) {
				/* Reenviar mi último paquete para saber si ellos están vivos */
				sendto (fd_socket, ventana->buffer_send, ventana->len_send, 0, (struct sockaddr *) &ventana->cliente, ventana->tamsock);
				ventana->last_response = SDL_GetTicks ();
				ventana->retry++;
			} else if (ventana->retry > 0 && now_time > ventana->last_response + NET_CONN_TIMER) {
				/* Reenviar de forma más continua */
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
			}
		}
		
		ventana = ventana->next;
	}
}
