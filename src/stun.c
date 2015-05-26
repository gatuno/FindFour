/*
 * stun.c
 * This file is part of Find Four
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
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
 * along with Find Four. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Para el manejo de red */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "stun.h"

void build_binding_request (StunMessage* msg, int change_port, int change_ip, int id) {
	int g, r;
	memset (msg, 0, sizeof (StunMessage));
	
	msg->msgHdr.msgType = BindRequestMsg;
	
	for (g = 0; g < 16; g = g + 4) {
		r = random ();
		msg->msgHdr.id[g+0]= r>>0;
		msg->msgHdr.id[g+1]= r>>8;
		msg->msgHdr.id[g+2]= r>>16;
		msg->msgHdr.id[g+3]= r>>24;
	}

	if (id != 0) {
		msg->msgHdr.id[0] = id; 
	}
	
	msg->hasChangeRequest = 1;
	msg->changeRequest.value = (change_ip ? ChangeIpFlag : 0) | (change_port ? ChangePortFlag : 0);
}

int encode_stun_message (const StunMessage *msg, char *buf, unsigned int buf_len) {
	uint16_t temp;
	uint32_t temp32;
	
	size_t pos = 0;
	size_t len;
	
	/* Primer byte, el tipo de mensaje */
	temp = htons (msg->msgHdr.msgType);
	memcpy (&buf[pos], &temp, sizeof (uint16_t));
	pos += sizeof (uint16_t);
	
	/* En el byte 2 va la longitud del mensaje */
	len = pos;
	pos += sizeof (uint16_t);
	
	memcpy (&buf[pos], &(msg->msgHdr.id), sizeof (msg->msgHdr.id));
	pos += sizeof (msg->msgHdr.id);
	
	if (msg->hasChangeRequest) {
		/* Codificar el change request */
		temp = htons (ChangeRequest);
		memcpy (&buf[pos], &temp, sizeof (uint16_t));
		pos += sizeof (uint16_t);
		
		temp = htons (4);
		memcpy (&buf[pos], &temp, sizeof (uint16_t));
		pos += sizeof (uint16_t);
		
		temp32 = htonl (msg->changeRequest.value);
		memcpy (&buf[pos], &temp32, sizeof (uint32_t));
		pos += sizeof (uint32_t);
	}
	
	/* Rellenar la longitud */
	temp = htons (pos - sizeof (msg->msgHdr));
	memcpy (&buf[len], &temp, sizeof (uint16_t));
	
	return pos;
}

void try_stun_binding (const char *server, int fd_socket) {
	char host[520];
	struct hostent *h;
	int port;
	char *sep;
	char *port_part;
	struct sockaddr_in stun_addr;
	char buffer[512];
	size_t len_msg;
	StunMessage stun_msg;
	
	strncpy (host, server, 520);
	host[519] = '\0';
	
	sep = strchr(host, ':');
	/* Tratar de ver si tiene puerto */
	if (sep == NULL) {
		port = STUN_PORT;
	} else {
		port_part = sep + 1;
		*sep = '\0';
		
		port = strtol (port_part, NULL, 10);
		
		if (port < 1 || port > 0xFFFF) port = STUN_PORT;
	}
		
	h = gethostbyname (host);
	
	if (h == NULL) {
		perror ("Falló la conversión de nombres");
		return;
	}
	
	build_binding_request (&stun_msg, 1, 1, 0x0c);
	
	len_msg = encode_stun_message (&stun_msg, buffer, sizeof (buffer));
	
	stun_addr.sin_family = h->h_addrtype;
	stun_addr.sin_port = htons (port);
	
	memcpy (&(stun_addr.sin_addr), h->h_addr, h->h_length);
	
	sendto (fd_socket, buffer, len_msg, 0, (struct sockaddr *) &stun_addr, sizeof (stun_addr));
	
	/* Cuando llegue la respuesta, guardar la IP */
}

