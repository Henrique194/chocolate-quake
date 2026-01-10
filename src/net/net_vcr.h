/*
 * Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) Henrique Barateli, <henriquejb194@gmail.com>, et al.
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
// net_vcr.h


#ifndef __NET_VCR__
#define __NET_VCR__

#include "quakedef.h"
#include "net.h"

#define VCR_OP_CONNECT        1
#define VCR_OP_GETMESSAGE     2
#define VCR_OP_SENDMESSAGE    3
#define VCR_OP_CANSENDMESSAGE 4
#define VCR_MAX_MESSAGE       4

i32 VCR_Init(void);
void VCR_Listen(qboolean state);
void VCR_SearchForHosts(qboolean xmit);
qsocket_t* VCR_Connect(char* host);
qsocket_t* VCR_CheckNewConnections(void);
i32 VCR_GetMessage(qsocket_t* sock);
i32 VCR_SendMessage(qsocket_t* sock, sizebuf_t* data);
qboolean VCR_CanSendMessage(qsocket_t* sock);
void VCR_Close(qsocket_t* sock);
void VCR_Shutdown(void);

#endif
