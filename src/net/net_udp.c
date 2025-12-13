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
// net_udp.c


#include "net_udp.h"
#include "console.h"
#include "cvar.h"
#include "net.h"
#include "sys.h"
#include <SDL_net.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/errno.h>
#include <sys/param.h>
#include <unistd.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif


static qboolean initialized = false;

static IPaddress my_addr = {0};
static IPaddress broadcast_addr = {0};

static UDPsocket accept_sock = NULL;
static UDPsocket control_sock = NULL;
static UDPsocket broadcast_sock = NULL;


static qboolean UDP_IsLocalAddr(const IPaddress* addr) {
    return addr->host == 0 || SDLNet_Read32(&addr->host) == INADDR_LOOPBACK;
}

static void UDP_FindLocalAddr(void) {
    IPaddress addrs[10] = {0};
    const int count = SDL_arraysize(addrs);
    SDLNet_GetLocalAddresses(addrs, count);
    for (int i = 0; i < count; i++) {
        if (!UDP_IsLocalAddr(&addrs[i])) {
            my_addr = addrs[i];
            return;
        }
    }
    Sys_Error("UDP_FindLocalAddr: Could determine local address.");
}

qboolean UDP_IsInitialized(void) {
    return initialized;
}

UDPsocket UDP_GetControlSocket(void) {
    return control_sock;
}

UDPsocket UDP_GetAcceptSocket(void) {
    return accept_sock;
}

void UDP_Init(void) {
    if (COM_CheckParm("-noudp")) {
        return;
    }
    if (SDLNet_Init() == -1) {
        printf("UDP Initialization failed: %s\n", SDLNet_GetError());
        return;
    }

    // Determine my name and address.
    UDP_FindLocalAddr();

    // if the quake hostname isn't set, set it to the machine name
    if (Q_strcmp(hostname.string, "UNNAMED") == 0) {
        char buff[MAXHOSTNAMELEN];
        gethostname(buff, MAXHOSTNAMELEN);
        buff[15] = 0;
        Cvar_Set("hostname", buff);
    }

    if ((control_sock = UDP_OpenSocket(0)) == NULL) {
        Sys_Error("UDP_Init: Unable to open control socket\n");
    }

    broadcast_addr.host = INADDR_BROADCAST;
    broadcast_addr.port = SDLNet_Read16(&net_hostport);

    IPaddress control_addr = UDP_GetSocketAddr(control_sock);
    Q_strcpy(my_tcpip_address, UDP_AddrToString(&control_addr));
    char* colon = Q_strrchr(my_tcpip_address, ':');
    if (colon) {
        *colon = 0;
    }

    Con_Printf("UDP Initialized\n");
    tcpipAvailable = true;
    initialized = true;
}

void UDP_Shutdown(void) {
    UDP_Listen(false);
    UDP_CloseSocket(control_sock);
    initialized = false;
}

static void UDP_DisableListening(void) {
    if (accept_sock == NULL) {
        return;
    }
    UDP_CloseSocket(accept_sock);
    accept_sock = NULL;
}

static void UDP_EnableListening(void) {
    if (accept_sock != NULL) {
        return;
    }
    if ((accept_sock = UDP_OpenSocket(net_hostport)) == NULL) {
        Sys_Error("UDP_Listen: Unable to open accept socket\n");
    }
}

void UDP_Listen(qboolean state) {
    if (state) {
        UDP_EnableListening();
        return;
    }
    UDP_DisableListening();
}

UDPsocket UDP_OpenSocket(int port) {
    return SDLNet_UDP_Open((Uint16) port);
}

void UDP_CloseSocket(UDPsocket socket) {
    if (socket == broadcast_sock) {
        broadcast_sock = NULL;
    }
    SDLNet_UDP_Close(socket);
}

int UDP_Read(UDPsocket socket, byte* buf, int len, IPaddress* addr) {
    UDPpacket packet = {0};
    packet.data = buf;
    packet.maxlen = len;
    const int ret = SDLNet_UDP_Recv(socket, &packet);
    if (ret == -1 && (errno == EWOULDBLOCK || errno == ECONNREFUSED)) {
        return 0;
    }
    if (ret == -1) {
        // Is this necessary?
        return -1;
    }
    *addr = packet.address;
    return packet.len;
}

static int UDP_MakeSocketBroadcastCapable(UDPsocket socket) {
    if (broadcast_sock != NULL) {
        Sys_Error("Attempted to use multiple broadcasts sockets\n");
    }
    broadcast_sock = socket;
    return 0;
}

int UDP_Broadcast(UDPsocket socket, byte* buf, int len) {
    if (socket != broadcast_sock) {
        UDP_MakeSocketBroadcastCapable(socket);
    }
    return UDP_Write(socket, buf, len, &broadcast_addr);
}

int UDP_Write(UDPsocket socket, byte* buf, int len, const IPaddress* addr) {
    UDPpacket packet;
    packet.data = buf;
    packet.len = len;
    packet.address = *addr;
    if (!SDLNet_UDP_Send(socket, -1, &packet)) {
        return -1;
    }
    return 1;
}

char* UDP_AddrToString(const IPaddress* addr) {
    static char buffer[22];
    const Uint32 host = SDLNet_Read32(&addr->host);
    const Uint16 port = SDLNet_Read16(&addr->port);
    sprintf(buffer, "%d.%d.%d.%d:%d",
        (host >> 24) & 0xff,
        (host >> 16) & 0xff,
        (host >> 8) & 0xff,
        host & 0xff,
        port
    );
    return buffer;
}

IPaddress UDP_GetSocketAddr(UDPsocket socket) {
    IPaddress addr = {0};
    const IPaddress* sock_addr = SDLNet_UDP_GetPeerAddress(socket, -1);
    addr.host = sock_addr->host;
    addr.port = sock_addr->port;
    if (UDP_IsLocalAddr(sock_addr)) {
        addr.host = my_addr.host;
    }
    return addr;
}

void UDP_GetNameFromAddr(const IPaddress* addr, char* name) {
    char* host_name = (char*) SDLNet_ResolveIP(addr);
    if (host_name) {
        Q_strncpy(name, host_name, NET_NAMELEN - 1);
        return;
    }
    Q_strcpy(name, UDP_AddrToString(addr));
}

int UDP_GetAddrFromName(char* name, IPaddress* addr) {
    static char host[NET_NAMELEN];
    if (!name || *name == 0) {
        return -1;
    }
    Q_strncpy(host, name, NET_NAMELEN - 1);
    char* colon = Q_strrchr(host, ':');
    Uint16 port;
    if (colon) {
        *colon = 0;
        port = (Uint16) Q_atoi(colon + 1);
    } else {
        port = (Uint16) net_hostport;
    }
    return SDLNet_ResolveHost(addr, host, port);
}

int UDP_AddrCompare(const IPaddress* addr1, const IPaddress* addr2) {
    if (addr1->host != addr2->host) {
        return -1;
    }
    if (addr1->port == addr2->port) {
        return 0;
    }
    return 1;
}

int UDP_GetSocketPort(const IPaddress* addr) {
    return SDLNet_Read16(&addr->port);
}


void UDP_SetSocketPort(IPaddress* addr, int port) {
    SDLNet_Write16(port, &addr->port);
}
