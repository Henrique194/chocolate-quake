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
// net_socket.h


#ifndef __NET_SOCKET__
#define __NET_SOCKET__

#include "quakedef.h"
#include "net.h"
#include <SDL_net.h>

typedef struct qsocket_s {
    struct qsocket_s* next;
    double connecttime;
    double lastMessageTime;
    double lastSendTime;

    qboolean disconnected;
    qboolean canSend;
    qboolean sendNext;

    int driver;
    UDPsocket socket;
    void* driverdata;

    unsigned int ackSequence;
    unsigned int sendSequence;
    unsigned int unreliableSendSequence;
    int sendMessageLength;
    byte sendMessage[NET_MAXMESSAGE];

    unsigned int receiveSequence;
    unsigned int unreliableReceiveSequence;
    int receiveMessageLength;
    byte receiveMessage[NET_MAXMESSAGE];

    IPaddress addr;
    char address[NET_NAMELEN];
} qsocket_t;


extern qsocket_t* net_activeSockets;

//
// Called by drivers when a new communications endpoint is required.
// The sequence and buffer fields will be filled in properly.
//
qsocket_t* NET_NewQSocket(void);

void NET_FreeQSocket(qsocket_t* sock);

qboolean NET_IsSocketDisconnected(const qsocket_t* sock);

void NET_PrintSocketStats(const char* addr);

int NET_GetSocketMessage(qsocket_t* sock);

qboolean NET_CanSocketSendMessage(qsocket_t* sock);

int NET_SendSocketMessage(qsocket_t* sock, sizebuf_t* data);

int NET_SendSocketUnreliableMessage(qsocket_t* sock, sizebuf_t* data);

void NET_InitSockets(void);

void NET_CloseSockets(void);

#endif
