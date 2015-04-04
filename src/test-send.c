/*
 * test-send.c
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
#include <stdio.h>
#include <string.h>

#include "findfour.h"
Juego *primero, *ultimo;

#include "netplay.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int pack (FF_NET *net, char *buffer);
void unpack (FF_NET *net, char *buffer, size_t len);

int main (int argc, char *argv[]) {
	int fd;
	struct sockaddr_in bind_addr, destino;
	char buffer[256];
	int len;
	int inicio;
	
	fd = socket (AF_INET, SOCK_DGRAM, 0);
	
	/* Asociar un puerto */
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons (3301);
	bind_addr.sin_addr.s_addr = INADDR_ANY;
	
	if (bind (fd, (struct sockaddr *) &bind_addr, sizeof (bind_addr)) < 0) {
		perror ("Error al hacer bind");
		return 1;
	}
	
	FF_NET netmsg;
	
	memset (&netmsg, 0, sizeof (netmsg));
	
	netmsg.flags.syn = 1;
	memcpy (netmsg.syn.nick, "Prueba", strlen ("Prueba") + 1);
	
	/* Armar el destino */
	destino.sin_family = AF_INET;
	destino.sin_port = htons (3300);
	
	inet_aton ("127.0.0.1", &destino.sin_addr);
	
	/* Enviar el paquete */
	len = pack (&netmsg, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &destino, sizeof (destino));
	
	/* Esperar a que llegue una respuesta, si es que llega */
	len = recvfrom (fd, buffer, len, 0, NULL, NULL);
	
	unpack (&netmsg, buffer, len);
	
	printf ("Banderas: ");
	if (netmsg.flags.syn) {
		printf ("SYN, ");
	}
	if (netmsg.flags.ack) {
		printf ("ACK, ");
	}
	if (netmsg.flags.trn) {
		printf ("TRN, ");
	}
	printf ("\n");
	
	if (netmsg.flags.syn && netmsg.flags.ack) {
		printf ("Es lo que esperabamos, Versión: %i, Nick: %s\n", netmsg.syn.version, netmsg.syn.nick);
	} else {
		printf ("Advertencia, no es lo que esperabamos\n");
	}
	
	/* Enviar la confirmación antes de sus datos */
	memset (&netmsg, 0, sizeof (netmsg));
	
	netmsg.flags.ack = 1;
	netmsg.ack.ack = 0; /* Para confirmar su syn */
	
	len = pack (&netmsg, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &destino, sizeof (destino));
	
	
	/* Esperar a que llegue una respuesta, si es que llega */
	len = recvfrom (fd, buffer, len, 0, NULL, NULL);
	
	unpack (&netmsg, buffer, len);
	
	printf ("Banderas: ");
	if (netmsg.flags.syn) {
		printf ("SYN, ");
	}
	if (netmsg.flags.ack) {
		printf ("ACK, ");
	}
	if (netmsg.flags.trn) {
		printf ("TRN, ");
	}
	printf ("\n");
	
	if (netmsg.flags.syn && netmsg.flags.trn) {
		printf ("Es lo que esperabamos, ");
		if (netmsg.syn_trn.inicio == 0) {
			/* Ellos empiezan */
			inicio = 1;
			printf ("ellos empiezan\n");
		} else {
			/* Nosotros empezamos */
			inicio = 0;
			printf ("nosotros empezamos\n");
		}
	} else {
		printf ("No es lo que esperabamos\n");
	}
	
	/* Enviar nuestra confirmación de quién empieza el turno */
	memset (&netmsg, 0, sizeof (netmsg));
	netmsg.flags.ack = netmsg.flags.trn = netmsg.flags.syn = 1;
	netmsg.syn_trn.inicio = inicio;
	
	len = pack (&netmsg, buffer);
	
	sendto (fd, buffer, len, 0, (struct sockaddr *) &destino, sizeof (destino));
	
	close (fd);
	return 0;
}
