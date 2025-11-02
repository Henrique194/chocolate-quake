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
// net_socket.c


#include "net_socket.h"
#include "console.h"
#include "server.h"
#include "sys.h"
#include "zone.h"


qsocket_t* net_activeSockets = NULL;
static qsocket_t* net_freeSockets = NULL;

static cvar_t net_messagetimeout = {"net_messagetimeout", "300"};


/*
================================================================================

SOCKET ALLOCATION

================================================================================
*/

static void NET_AddToList(qsocket_t** list, qsocket_t* sock) {
    sock->next = *list;
    *list = sock;
}

static void NET_RemoveFromList(qsocket_t** list, const qsocket_t* sock) {
    if (sock == *list) {
        *list = (*list)->next;
        return;
    }
    for (qsocket_t* s = *list; s; s = s->next) {
        if (s->next == sock) {
            s->next = sock->next;
            return;
        }
    }
    Sys_Error("NET_RemoveFromList: socket not in list\n");
}

static qsocket_t* NET_GetFreeSocket(void) {
    if (net_freeSockets == NULL) {
        return NULL;
    }
    if (net_activeconnections >= svs.maxclients) {
        return NULL;
    }
    // Get one from free list.
    qsocket_t* sock = net_freeSockets;
    NET_RemoveFromList(&net_freeSockets, sock);
    NET_AddToList(&net_activeSockets, sock);
    return sock;
}

qsocket_t* NET_NewQSocket(void) {
    qsocket_t* sock = NET_GetFreeSocket();
    if (!sock) {
        return NULL;
    }
    sock->driver = net_driverlevel;
    sock->driverdata = NULL;
    Q_strcpy(sock->address, "UNSET ADDRESS");
    sock->disconnected = false;
    sock->sendNext = false;
    sock->canSend = true;
    sock->connecttime = net_time;
    sock->lastMessageTime = net_time;
    sock->socket = 0;
    sock->ackSequence = 0;
    sock->sendSequence = 0;
    sock->unreliableSendSequence = 0;
    sock->sendMessageLength = 0;
    sock->receiveSequence = 0;
    sock->unreliableReceiveSequence = 0;
    sock->receiveMessageLength = 0;
    return sock;
}

void NET_FreeQSocket(qsocket_t* sock) {
    NET_RemoveFromList(&net_activeSockets, sock);
    NET_AddToList(&net_freeSockets, sock);
    sock->disconnected = true;
}

//==============================================================================


/*
================================================================================

SOCKET PROPERTIES

================================================================================
*/

const char* NET_GetSocketAddr(const qsocket_t* sock) {
    return sock->address;
}

double NET_GetSocketConnectTime(const qsocket_t* sock) {
    return sock->connecttime;
}

qboolean NET_IsSocketDisconnected(const qsocket_t* sock) {
    return sock->disconnected;
}

//==============================================================================


/*
================================================================================

SOCKET STATS

================================================================================
*/

static void NET_PrintStats(const qsocket_t* s) {
    Con_Printf("canSend = %4u   \n", s->canSend);
    Con_Printf("sendSeq = %4u   ", s->sendSequence);
    Con_Printf("recvSeq = %4u   \n", s->receiveSequence);
    Con_Printf("\n");
}

static qsocket_t* NET_GetSocketByAddr(const char* addr) {
    qsocket_t* s;
    for (s = net_activeSockets; s; s = s->next) {
        if (Q_strcasecmp(addr, s->address) == 0) {
            return s;
        }
    }
    for (s = net_freeSockets; s; s = s->next) {
        if (Q_strcasecmp(addr, s->address) == 0) {
            return s;
        }
    }
    return NULL;
}

static void NET_PrintAllSockets(void) {
    const qsocket_t* s;
    for (s = net_activeSockets; s; s = s->next) {
        NET_PrintStats(s);
    }
    for (s = net_freeSockets; s; s = s->next) {
        NET_PrintStats(s);
    }
}

void NET_PrintSocketStats(const char* addr) {
    if (Q_strcmp(addr, "*") == 0) {
        NET_PrintAllSockets();
        return;
    }
    const qsocket_t* s = NET_GetSocketByAddr(addr);
    if (s) {
        NET_PrintStats(s);
    }
}

//==============================================================================


/*
================================================================================

SOCKET IO

================================================================================
*/

static const net_driver_t* NET_GetDriver(const qsocket_t* sock) {
    const net_driver_t* driver = &net_drivers[sock->driver];
    // Make sure net time is updated before using driver.
    SetNetTime();
    return driver;
}

static qboolean NET_HasTimedOut(const qsocket_t* sock) {
    if (!sock->driver) {
        // Local client has no lag.
        return false;
    }
    const double lag = net_time - sock->lastMessageTime;
    return lag > net_messagetimeout.value;
}

static void NET_UpdateConnection(qsocket_t* sock, const int ret) {
    if (ret == 0 && NET_HasTimedOut(sock)) {
        NET_Close(sock);
        return;
    }
    if (!sock->driver || ret <= 0) {
        return;
    }
    sock->lastMessageTime = net_time;
    if (ret == 1) {
        messagesReceived++;
    } else if (ret == 2) {
        unreliableMessagesReceived++;
    }
}

int NET_GetSocketMessage(qsocket_t* sock) {
    if (sock->disconnected) {
        Con_Printf("NET_GetSocketMessage: disconnected socket\n");
        return -1;
    }
    const net_driver_t* driver = NET_GetDriver(sock);
    const int ret = driver->QGetMessage(sock);
    NET_UpdateConnection(sock, ret);
    return ret;
}

qboolean NET_CanSocketSendMessage(qsocket_t* sock) {
    if (sock->disconnected) {
        return false;
    }
    const net_driver_t* driver = NET_GetDriver(sock);
    return driver->CanSendMessage(sock);
}

int NET_SendSocketMessage(qsocket_t* sock, sizebuf_t* data) {
    if (sock->disconnected) {
        Con_Printf("NET_SendSocketMessage: disconnected socket\n");
        return -1;
    }
    const net_driver_t* driver = NET_GetDriver(sock);
    const int ret = driver->QSendMessage(sock, data);
    if (ret == 1 && sock->driver) {
        messagesSent++;
    }
    return ret;
}

int NET_SendSocketUnreliableMessage(qsocket_t* sock, sizebuf_t* data) {
    if (sock->disconnected) {
        Con_Printf("NET_SendSocketUnreliableMessage: disconnected socket\n");
        return -1;
    }
    const net_driver_t* driver = NET_GetDriver(sock);
    const int ret = driver->SendUnreliableMessage(sock, data);
    if (ret == 1 && sock->driver) {
        unreliableMessagesSent++;
    }
    return ret;
}

void NET_Close(qsocket_t* sock) {
    if (!sock) {
        return;
    }
    if (sock->disconnected) {
        return;
    }
    const net_driver_t* driver = NET_GetDriver(sock);
    driver->Close(sock);
    NET_FreeQSocket(sock);
}

//==============================================================================


/*
================================================================================

INITIALIZATION AND SHUTDOWN

================================================================================
*/

void NET_InitSockets(void) {
    int num_sockets = svs.maxclientslimit;
    if (cls.state != ca_dedicated) {
        // One more socket for local client.
        num_sockets++;
    }
    for (int i = 0; i < num_sockets; i++) {
        qsocket_t* sock = Hunk_AllocName(sizeof(*sock), "qsocket");
        sock->disconnected = true;
        NET_AddToList(&net_freeSockets, sock);
    }

    Cvar_RegisterVariable(&net_messagetimeout);
}

void NET_CloseSockets(void) {
    for (qsocket_t* s = net_activeSockets; s; s = s->next) {
        NET_Close(s);
    }
}

//==============================================================================
