/*
 * stun.h
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Find Four. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __STUN_H__
#define __STUN_H__

#include <stdint.h>

#define BindRequestMsg               0x0001
#define BindResponseMsg              0x0101
#define BindErrorResponseMsg         0x0111
#define SharedSecretRequestMsg       0x0002
#define SharedSecretResponseMsg      0x0102
#define SharedSecretErrorResponseMsg 0x0112

/// define a structure to hold a stun address
#define IPv4Family 0x01
#define IPv6Family 0x02

#define ChangeIpFlag   0x04
#define ChangePortFlag 0x02

// define  stun attribute
#define MappedAddress      0x0001
#define ResponseAddress    0x0002
#define ChangeRequest      0x0003
#define SourceAddress      0x0004
#define ChangedAddress     0x0005
#define Username           0x0006
#define Password           0x0007
#define MessageIntegrity   0x0008
#define ErrorCode          0x0009
#define UnknownAttribute   0x000A
#define ReflectedFrom      0x000B
#define XorMappedAddress   0x8020
#define XorOnly            0x0021
#define ServerName         0x8022
#define SecondaryAddress   0x8050

#define STUN_MAX_STRING 256

#define STUN_PORT 3478

typedef struct {
	uint16_t msgType;
	uint16_t msgLength;
	uint8_t id[16];
} StunMsgHdr;

typedef struct {
	uint16_t type;
	uint16_t length;
} StunAtrHdr;

typedef struct {
	uint8_t pad;
	uint8_t family;
	uint16_t port;
	uint32_t addr;
} StunAtrAddress4;

typedef struct {
	uint32_t value;
} StunAtrChangeRequest;

typedef struct {
	char hash[20];
} StunAtrIntegrity;

typedef struct {
	uint16_t pad; // all 0
	uint8_t errorClass;
	uint8_t number;
	char reason[STUN_MAX_STRING];
	uint16_t sizeReason;
} StunAtrError;

typedef struct {
	StunMsgHdr msgHdr;

	int hasMappedAddress;
	StunAtrAddress4 mappedAddress;

	int hasResponseAddress;
	StunAtrAddress4 responseAddress;

	int hasChangeRequest;
	StunAtrChangeRequest changeRequest;

	int hasSourceAddress;
	StunAtrAddress4 sourceAddress;

	int hasChangedAddress;
	StunAtrAddress4 changedAddress;

	int hasMessageIntegrity;
	StunAtrIntegrity messageIntegrity;

	int hasErrorCode;
	StunAtrError errorCode;

	int hasReflectedFrom;
	StunAtrAddress4 reflectedFrom;

	int hasXorMappedAddress;
	StunAtrAddress4 xorMappedAddress;

	int xorOnly;

	int hasSecondaryAddress;
	StunAtrAddress4 secondaryAddress;
} StunMessage;

void try_stun_binding (const char *server);
void parse_stun_message (const char *buffer, int buffer_len);

#endif /* __STUN_H__ */

