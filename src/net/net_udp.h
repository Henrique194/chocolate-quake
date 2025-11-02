/*
 * Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) 2025 Henrique Barateli <henriquejb194@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
// net_udp.h


#ifndef __NET_UDP__
#define __NET_UDP__

#include "quakedef.h"
#include <SDL_net.h>

qboolean UDP_IsInitialized(void);
UDPsocket UDP_GetControlSocket(void);
UDPsocket UDP_GetAcceptSocket(void);
void UDP_Init(void);
void UDP_Shutdown(void);
void UDP_Listen(qboolean state);
UDPsocket UDP_OpenSocket(int port);
void UDP_CloseSocket(UDPsocket socket);
int UDP_Read(UDPsocket socket, byte* buf, int len, IPaddress* addr);
int UDP_Write(UDPsocket socket, byte* buf, int len, const IPaddress* addr);
int UDP_Broadcast(UDPsocket socket, byte* buf, int len);
char* UDP_AddrToString(const IPaddress* addr);
IPaddress UDP_GetSocketAddr(UDPsocket socket);
void UDP_GetNameFromAddr(const IPaddress* addr, char* name);
int UDP_GetAddrFromName(char* name, IPaddress* addr);
int UDP_AddrCompare(const IPaddress* addr1, const IPaddress* addr2);
int UDP_GetSocketPort(const IPaddress* addr);
void UDP_SetSocketPort(IPaddress* addr, int port);

#endif
