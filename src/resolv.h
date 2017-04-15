/*
 * resolv.h
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

#ifndef __RESOLV_H__
#define __RESOLV_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef void (*ResolvCallback) (const char *hostname, int puerto, const struct addrinfo *res, int error, const char *err_str);

void pending_query (void);
void do_query (const char *hostname, int puerto, ResolvCallback callback);
void init_resolver (void);
void destroy_resolver (void);
int analizador_hostname_puerto (const char *cadena, char *hostname, int *puerto, int default_port);

#endif /* __RESOLV_H__ */

