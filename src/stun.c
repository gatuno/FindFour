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
#ifdef __MINGW32__
#	include <winsock2.h>
#	include <ws2tcpip.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netdb.h>
#endif
#include <sys/types.h>

#include "stun.h"
#include "message.h"

void build_binding_request (StunMessage* msg, int change_port, int change_ip, int id) {
	int g, r;
	memset (msg, 0, sizeof (StunMessage));
	
	msg->msgHdr.msgType = BindRequestMsg;
	
	for (g = 0; g < 16; g = g + 4) {
		r = rand ();
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

int stunParseAttr (const char *buffer, int buffer_len, StunAtrAddress4 *result) {
	uint16_t nport;
	uint32_t naddr;
	
	if (buffer_len != 8) {
		printf ("Atributo con longitud equivocada\n");
		return 0;
	}
	result->pad = *buffer++;
	result->family = *buffer++;
	
	if (result->family == IPv4Family) {
		memcpy (&nport, buffer, 2);
		buffer += 2;
		result->port = ntohs (nport);
		
		memcpy (&naddr, buffer, 4);
		buffer += 4;
		result->addr = ntohl (naddr);
		return 1;
	} else if (result->family == IPv6Family) {
		printf ("IPv6 no soportado\n");
		return 0;
	} else {
		printf ("Familia no soportada\n");
	}
	
	return 0;
}

void parse_stun_message (const char *buffer, int buffer_len) {
	StunMessage stun_msg;
	const char *body;
	int size;
	StunAtrHdr *attr;
	int attrLen;
	int attrType;
	char ip_addr[20];
	
	memset (&stun_msg, 0, sizeof (stun_msg));
	
	if (sizeof (StunMsgHdr) > buffer_len) {
		printf ("Bad STUN Message\n");
		return;
	}
	
	memcpy (&stun_msg.msgHdr, buffer, sizeof (StunMsgHdr));
	stun_msg.msgHdr.msgType = ntohs (stun_msg.msgHdr.msgType);
	stun_msg.msgHdr.msgLength = ntohs (stun_msg.msgHdr.msgLength);
	
	if (stun_msg.msgHdr.msgLength + sizeof (StunMsgHdr) != buffer_len) {
		printf ("Wrong Stun len\n");
		return;
	}
	
	body = buffer + sizeof (StunMsgHdr);
	size = stun_msg.msgHdr.msgLength;
	
	while (size > 0) {
		attr = (StunAtrHdr *) body;
		
		attrLen = ntohs (attr->length);
		attrType = ntohs (attr->type);
		
		if (attrLen + 4 > size) {
			printf ("Atributo es más largo que el mensaje\n");
			return;
		}
		
		body += 4;
		size -= 4;
		
		switch (attrType) {
			case MappedAddress:
				if (stunParseAttr (body, attrLen, &stun_msg.mappedAddress)) {
					stun_msg.hasMappedAddress = 1;
				} else {
					return;
				}
				break;
			case ResponseAddress:
				if (stunParseAttr (body, attrLen, &stun_msg.responseAddress)) {
					stun_msg.hasMappedAddress = 1;
				} else {
					return;
				}
				break;
			case SourceAddress:
				if (stunParseAttr (body, attrLen, &stun_msg.sourceAddress)) {
					stun_msg.hasMappedAddress = 1;
				} else {
					return;
				}
				break;
			case ChangedAddress:
				if (stunParseAttr (body, attrLen, &stun_msg.changedAddress)) {
					stun_msg.hasMappedAddress = 1;
				} else {
					return;
				}
				break;
			case XorOnly:
				stun_msg.xorOnly = 1;
				break;
			
		}
		
		body += attrLen;
		size -= attrLen;
	}
	
	struct in_addr sin_addr;
	
	/* Si el mensaje se pudo analizar, decidir cuál mensaje es */
	if (stun_msg.msgHdr.msgType == 0x0101 && stun_msg.msgHdr.id[0] == 0x03 && stun_msg.hasMappedAddress) {
		sin_addr.s_addr = htonl (stun_msg.mappedAddress.addr);
#ifdef __MINGW32__
		strcpy (ip_addr, inet_ntoa (sin_addr));
#else
		inet_ntop (AF_INET, &sin_addr, ip_addr, sizeof (ip_addr));
#endif
		message_add (MSG_NORMAL, "Ok", "¡Felicidades!\nSe ha detectado que puedes jugar en linea\nTu dirección IP es: %s\n", ip_addr);
	}
}

void try_stun_binding_sockaddr (struct sockaddr_in *server) {
	struct sockaddr_in stun_addr;
	char buffer[512];
	size_t len_msg;
	StunMessage stun_msg;
	
	int fd_socket;
	
	fd_socket = findfour_get_socket4 ();
	if (fd_socket < 0) return;
	
	stun_addr.sin_family = AF_INET;
	stun_addr.sin_addr.s_addr = server->sin_addr.s_addr;
	stun_addr.sin_port = server->sin_port;
	
	/* Enviar un same-port, same-address */
	build_binding_request (&stun_msg, 0, 0, 0x01);
	len_msg = encode_stun_message (&stun_msg, buffer, sizeof (buffer));
	sendto (fd_socket, buffer, len_msg, 0, (struct sockaddr *) &stun_addr, sizeof (stun_addr));
	
	/* Enviar un different-port, same-address */
	build_binding_request (&stun_msg, 1, 0, 0x02);
	len_msg = encode_stun_message (&stun_msg, buffer, sizeof (buffer));
	sendto (fd_socket, buffer, len_msg, 0, (struct sockaddr *) &stun_addr, sizeof (stun_addr));
	
	/* Enviar un different-port, different-address */
	build_binding_request (&stun_msg, 1, 1, 0x03);
	len_msg = encode_stun_message (&stun_msg, buffer, sizeof (buffer));
	sendto (fd_socket, buffer, len_msg, 0, (struct sockaddr *) &stun_addr, sizeof (stun_addr));
	
	/* Cuando llegue la respuesta, guardar la IP */
}

void late_stun_connect (const struct addrinfo *res) {
	/* Localizar el primer resultado AF_INET*/
	while (res != NULL) {
		if (res->ai_addr->sa_family == AF_INET) {
			try_stun_binding_sockaddr ((struct sockaddr_in *)res->ai_addr);
			break;
		}
		res = res->ai_next;
	}
}

void try_stun_binding (const char *server) {
	const char *hostname;
	int puerto;
	int valido;
	
	hostname = strdup (server);
	
	valido = analizador_hostname_puerto (server, hostname, &puerto, STUN_PORT);
	
	if (valido) {
		/* Ejecutar la resolución de nombres primero, conectar después */
		do_query (hostname, puerto, late_stun_connect);
	} else {
		/* Mandar un mensaje de error */
	}
}

