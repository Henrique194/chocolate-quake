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
// net_dgrm.c


#include "net_dgrm.h"
#include "cmd.h"
#include "console.h"
#include "keys.h"
#include "net_socket.h"
#include "net_poll.h"
#include "net_udp.h"
#include "screen.h"
#include "server.h"
#include "sys.h"
#include <SDL_net.h>

// This enables a simple IP banning mechanism
// #define BAN_TEST

#ifdef BAN_TEST
#if defined(_WIN32)
#include <windows.h>
#else
#define AF_INET 2           /* internet */
struct in_addr {
    union {
        struct {
            unsigned char s_b1, s_b2, s_b3, s_b4;
        } S_un_b;
        struct {
            unsigned short s_w1, s_w2;
        } S_un_w;
        unsigned long S_addr;
    } S_un;
};
#define s_addr  S_un.S_addr /* can be used for most tcp & ip code */
struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};
char* inet_ntoa(struct in_addr in);
unsigned long inet_addr(const char* cp);
#endif
#endif	// BAN_TEST


// Statistic Counters
static int packetsSent = 0;
static int packetsReSent = 0;
static int packetsReceived = 0;
static int receivedDuplicateCount = 0;
static int shortPacketCount = 0;
static int droppedDatagrams;

static int myDriverLevel;

struct {
    unsigned int length;
    unsigned int sequence;
    byte data[MAX_DATAGRAM];
} packetBuffer;

extern int m_return_state;
extern int m_state;
extern qboolean m_return_onerror;
extern char m_return_reason[32];


#ifdef PARANOID
char *StrAddr (IPaddress* addr)
{
	static char buf[34];
	byte *p = (byte *)addr;
	int n;

	for (n = 0; n < 16; n++)
		sprintf (buf + n * 2, "%02x", *p++);
	return buf;
}
#endif


#ifdef BAN_TEST
static unsigned long banAddr = 0x00000000;
static unsigned long banMask = 0xffffffff;

void NET_Ban_f(void) {
    char addrStr[32];
    char maskStr[32];
    void (*print)(char* fmt, ...);

    if (cmd_source == src_command) {
        if (!sv.active) {
            Cmd_ForwardToServer();
            return;
        }
        print = Con_Printf;
    } else {
        if (pr_global_struct->deathmatch) {
            return;
        }
        print = SV_ClientPrintf;
    }

    switch (Cmd_Argc()) {
        case 1:
            if (((struct in_addr*) &banAddr)->s_addr) {
                Q_strcpy(addrStr, inet_ntoa(*(struct in_addr*) &banAddr));
                Q_strcpy(maskStr, inet_ntoa(*(struct in_addr*) &banMask));
                print("Banning %s [%s]\n", addrStr, maskStr);
            } else {
                print("Banning not active\n");
            }
            break;
        case 2:
            if (Q_strcasecmp(Cmd_Argv(1), "off") == 0) {
                banAddr = 0x00000000;
            } else {
                banAddr = inet_addr(Cmd_Argv(1));
            }
            banMask = 0xffffffff;
            break;
        case 3:
            banAddr = inet_addr(Cmd_Argv(1));
            banMask = inet_addr(Cmd_Argv(2));
            break;
        default:
            print("BAN ip_address [mask]\n");
            break;
    }
}
#endif


int Datagram_SendMessage(qsocket_t* sock, sizebuf_t* data) {
    unsigned int packetLen;
    unsigned int dataLen;
    unsigned int eom;

#ifdef PARANOID
    if (data->cursize == 0) {
        Sys_Error("Datagram_SendMessage: zero length message\n");
    }
    if (data->cursize > NET_MAXMESSAGE) {
        Sys_Error("Datagram_SendMessage: message too big %u\n", data->cursize);
    }
    if (sock->canSend == false) {
        Sys_Error("SendMessage: called with canSend == false\n");
    }
#endif

    Q_memcpy(sock->sendMessage, data->data, data->cursize);
    sock->sendMessageLength = data->cursize;

    if (data->cursize <= MAX_DATAGRAM) {
        dataLen = data->cursize;
        eom = NETFLAG_EOM;
    } else {
        dataLen = MAX_DATAGRAM;
        eom = 0;
    }
    packetLen = NET_HEADERSIZE + dataLen;

    packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
    packetBuffer.sequence = BigLong(sock->sendSequence++);
    Q_memcpy(packetBuffer.data, sock->sendMessage, dataLen);

    sock->canSend = false;

    if (UDP_Write(sock->socket, (byte*) &packetBuffer, packetLen, &sock->addr) == -1) {
        return -1;
    }

    sock->lastSendTime = net_time;
    packetsSent++;
    return 1;
}


static int SendMessageNext(qsocket_t* sock) {
    unsigned int packetLen;
    unsigned int dataLen;
    unsigned int eom;

    if (sock->sendMessageLength <= MAX_DATAGRAM) {
        dataLen = sock->sendMessageLength;
        eom = NETFLAG_EOM;
    } else {
        dataLen = MAX_DATAGRAM;
        eom = 0;
    }
    packetLen = NET_HEADERSIZE + dataLen;

    packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
    packetBuffer.sequence = BigLong(sock->sendSequence++);
    Q_memcpy(packetBuffer.data, sock->sendMessage, dataLen);

    sock->sendNext = false;

    if (UDP_Write(sock->socket, (byte*) &packetBuffer, packetLen, &sock->addr) == -1) {
        return -1;
    }

    sock->lastSendTime = net_time;
    packetsSent++;
    return 1;
}


static int ReSendMessage(qsocket_t* sock) {
    unsigned int packetLen;
    unsigned int dataLen;
    unsigned int eom;

    if (sock->sendMessageLength <= MAX_DATAGRAM) {
        dataLen = sock->sendMessageLength;
        eom = NETFLAG_EOM;
    } else {
        dataLen = MAX_DATAGRAM;
        eom = 0;
    }
    packetLen = NET_HEADERSIZE + dataLen;

    packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
    packetBuffer.sequence = BigLong(sock->sendSequence - 1);
    Q_memcpy(packetBuffer.data, sock->sendMessage, dataLen);

    sock->sendNext = false;

    if (UDP_Write(sock->socket, (byte*) &packetBuffer, packetLen, &sock->addr) == -1) {
        return -1;
    }

    sock->lastSendTime = net_time;
    packetsReSent++;
    return 1;
}


qboolean Datagram_CanSendMessage(qsocket_t* sock) {
    if (sock->sendNext) {
        SendMessageNext(sock);
    }
    return sock->canSend;
}


qboolean Datagram_CanSendUnreliableMessage(qsocket_t* sock) {
    return true;
}


int Datagram_SendUnreliableMessage(qsocket_t* sock, sizebuf_t* data) {
#ifdef PARANOID
    if (data->cursize == 0) {
        Sys_Error("Datagram_SendUnreliableMessage: zero length message\n");
    }
    if (data->cursize > MAX_DATAGRAM) {
        Sys_Error("Datagram_SendUnreliableMessage: message too big %u\n", data->cursize);
    }
#endif

    const int packetLen = NET_HEADERSIZE + data->cursize;

    packetBuffer.length = BigLong(packetLen | NETFLAG_UNRELIABLE);
    packetBuffer.sequence = BigLong(sock->unreliableSendSequence++);
    Q_memcpy(packetBuffer.data, data->data, data->cursize);

    if (UDP_Write(sock->socket, (byte*) &packetBuffer, packetLen, &sock->addr) == -1) {
        return -1;
    }

    packetsSent++;
    return 1;
}


int Datagram_GetMessage(qsocket_t* sock) {
    unsigned int length;
    unsigned int flags;
    int ret = 0;
    IPaddress readaddr;
    unsigned int sequence;
    unsigned int count;

    if (!sock->canSend && (net_time - sock->lastSendTime) > 1.0) {
        ReSendMessage(sock);
    }

    while (true) {
        length = UDP_Read(sock->socket, (byte*) &packetBuffer, NET_DATAGRAMSIZE, &readaddr);
        if (length == 0) {
            break;
        }
        if (length == -1) {
            Con_Printf("Read error\n");
            return -1;
        }
        if (UDP_AddrCompare(&readaddr, &sock->addr) != 0) {
#ifdef PARANOID
            Con_DPrintf("Forged packet received\n");
            Con_DPrintf("Expected: %s\n", StrAddr(&sock->addr));
            Con_DPrintf("Received: %s\n", StrAddr(&readaddr));
#endif
            continue;
        }
        if (length < NET_HEADERSIZE) {
            shortPacketCount++;
            continue;
        }

        length = BigLong(packetBuffer.length);
        flags = length & (~NETFLAG_LENGTH_MASK);
        length &= NETFLAG_LENGTH_MASK;

        if (flags & NETFLAG_CTL) {
            continue;
        }

        sequence = BigLong(packetBuffer.sequence);
        packetsReceived++;

        if (flags & NETFLAG_UNRELIABLE) {
            if (sequence < sock->unreliableReceiveSequence) {
                Con_DPrintf("Got a stale datagram\n");
                ret = 0;
                break;
            }
            if (sequence != sock->unreliableReceiveSequence) {
                count = sequence - sock->unreliableReceiveSequence;
                droppedDatagrams += count;
                Con_DPrintf("Dropped %u datagram(s)\n", count);
            }
            sock->unreliableReceiveSequence = sequence + 1;

            length -= NET_HEADERSIZE;

            SZ_Clear(&net_message);
            SZ_Write(&net_message, packetBuffer.data, length);

            ret = 2;
            break;
        }

        if (flags & NETFLAG_ACK) {
            if (sequence != (sock->sendSequence - 1)) {
                Con_DPrintf("Stale ACK received\n");
                continue;
            }
            if (sequence == sock->ackSequence) {
                sock->ackSequence++;
                if (sock->ackSequence != sock->sendSequence) {
                    Con_DPrintf("ack sequencing error\n");
                }
            } else {
                Con_DPrintf("Duplicate ACK received\n");
                continue;
            }
            sock->sendMessageLength -= MAX_DATAGRAM;
            if (sock->sendMessageLength > 0) {
                Q_memcpy(sock->sendMessage, sock->sendMessage + MAX_DATAGRAM, sock->sendMessageLength);
                sock->sendNext = true;
            } else {
                sock->sendMessageLength = 0;
                sock->canSend = true;
            }
            continue;
        }

        if (flags & NETFLAG_DATA) {
            packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
            packetBuffer.sequence = BigLong(sequence);
            UDP_Write(sock->socket, (byte*) &packetBuffer, NET_HEADERSIZE, &readaddr);

            if (sequence != sock->receiveSequence) {
                receivedDuplicateCount++;
                continue;
            }
            sock->receiveSequence++;

            length -= NET_HEADERSIZE;

            if (flags & NETFLAG_EOM) {
                SZ_Clear(&net_message);
                SZ_Write(&net_message, sock->receiveMessage, sock->receiveMessageLength);
                SZ_Write(&net_message, packetBuffer.data, length);
                sock->receiveMessageLength = 0;
                ret = 1;
                break;
            }

            Q_memcpy(sock->receiveMessage + sock->receiveMessageLength, packetBuffer.data, length);
            sock->receiveMessageLength += length;
        }
    }

    if (sock->sendNext) {
        SendMessageNext(sock);
    }

    return ret;
}


static void NET_Stats_f(void) {
    if (Cmd_Argc() == 1) {
        Con_Printf("unreliable messages sent   = %i\n", unreliableMessagesSent);
        Con_Printf("unreliable messages recv   = %i\n", unreliableMessagesReceived);
        Con_Printf("reliable messages sent     = %i\n", messagesSent);
        Con_Printf("reliable messages received = %i\n", messagesReceived);
        Con_Printf("packetsSent                = %i\n", packetsSent);
        Con_Printf("packetsReSent              = %i\n", packetsReSent);
        Con_Printf("packetsReceived            = %i\n", packetsReceived);
        Con_Printf("receivedDuplicateCount     = %i\n", receivedDuplicateCount);
        Con_Printf("shortPacketCount           = %i\n", shortPacketCount);
        Con_Printf("droppedDatagrams           = %i\n", droppedDatagrams);
        return;
    }
    NET_PrintSocketStats(Cmd_Argv(1));
}


static qboolean testInProgress = false;
static int testPollCount;
static UDPsocket testSocket;

static void Test_Poll(void);
poll_procedure_t testPollProcedure = {NULL, 0.0, &Test_Poll};

static void Test_Poll(void) {
    IPaddress clientaddr;
    int control;
    int len;
    char name[32];
    char address[64];
    int colors;
    int frags;
    int connectTime;
    byte playerNumber;

    while (true) {
        len = UDP_Read(testSocket, net_message.data, net_message.maxsize, &clientaddr);
        if (len < sizeof(int)) {
            break;
        }

        net_message.cursize = len;

        MSG_BeginReading();
        control = BigLong(*((int*) net_message.data));
        MSG_ReadLong();
        if (control == -1) {
            break;
        }
        if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) {
            break;
        }
        if ((control & NETFLAG_LENGTH_MASK) != len) {
            break;
        }

        if (MSG_ReadByte() != CCREP_PLAYER_INFO) {
            Sys_Error("Unexpected repsonse to Player Info request\n");
        }

        playerNumber = MSG_ReadByte();
        Q_strcpy(name, MSG_ReadString());
        colors = MSG_ReadLong();
        frags = MSG_ReadLong();
        connectTime = MSG_ReadLong();
        Q_strcpy(address, MSG_ReadString());

        Con_Printf("%s\n  frags:%3i  colors:%u %u  time:%u\n  %s\n", name,
                   frags, colors >> 4, colors & 0x0f, connectTime / 60,
                   address);
    }

    testPollCount--;
    if (testPollCount) {
        NET_SchedulePollProcedure(&testPollProcedure, 0.1);
    } else {
        UDP_CloseSocket(testSocket);
        testInProgress = false;
    }
}

static void Test_f(void) {
    char* host;
    int n;
    int max = MAX_SCOREBOARD;
    IPaddress sendaddr;

    if (testInProgress) {
        return;
    }

    host = Cmd_Argv(1);

    if (host && hostCacheCount) {
        for (n = 0; n < hostCacheCount; n++) {
            if (Q_strcasecmp(host, hostcache[n].name) != 0) {
                continue;
            }
            if (hostcache[n].driver != myDriverLevel) {
                continue;
            }
            max = hostcache[n].maxusers;
            Q_memcpy(&sendaddr, &hostcache[n].addr, sizeof(IPaddress));
            break;
        }
        if (n < hostCacheCount) {
            goto JustDoIt;
        }
    }

    if (!UDP_IsInitialized()) {
        return;
    }
    // See if we can resolve the host name.
    if (UDP_GetAddrFromName(host, &sendaddr) == -1) {
        return;
    }

JustDoIt:
    testSocket = UDP_OpenSocket(0);
    if (testSocket == NULL) {
        return;
    }

    testInProgress = true;
    testPollCount = 20;

    for (n = 0; n < max; n++) {
        SZ_Clear(&net_message);
        // save space for the header, filled in later
        MSG_WriteLong(&net_message, 0);
        MSG_WriteByte(&net_message, CCREQ_PLAYER_INFO);
        MSG_WriteByte(&net_message, n);
        *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
        UDP_Write(testSocket, net_message.data, net_message.cursize, &sendaddr);
    }
    SZ_Clear(&net_message);
    NET_SchedulePollProcedure(&testPollProcedure, 0.1);
}


static qboolean test2InProgress = false;
static UDPsocket test2Socket;

static void Test2_Poll(void);
poll_procedure_t test2PollProcedure = {NULL, 0.0, &Test2_Poll};

static void Test2_Poll(void) {
    IPaddress clientaddr;
    int control;
    int len;
    char name[256];
    char value[256];

    name[0] = 0;

    len = UDP_Read(test2Socket, net_message.data, net_message.maxsize, &clientaddr);
    if (len < sizeof(int)) {
        goto Reschedule;
    }

    net_message.cursize = len;

    MSG_BeginReading();
    control = BigLong(*((int*) net_message.data));
    MSG_ReadLong();
    if (control == -1) {
        goto Error;
    }
    if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) {
        goto Error;
    }
    if ((control & NETFLAG_LENGTH_MASK) != len) {
        goto Error;
    }
    if (MSG_ReadByte() != CCREP_RULE_INFO) {
        goto Error;
    }

    Q_strcpy(name, MSG_ReadString());
    if (name[0] == 0) {
        goto Done;
    }
    Q_strcpy(value, MSG_ReadString());

    Con_Printf("%-16.16s  %-16.16s\n", name, value);

    SZ_Clear(&net_message);
    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
    MSG_WriteString(&net_message, name);
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
    UDP_Write(test2Socket, net_message.data, net_message.cursize, &clientaddr);
    SZ_Clear(&net_message);

Reschedule:
    NET_SchedulePollProcedure(&test2PollProcedure, 0.05);
    return;

Error:
    Con_Printf("Unexpected repsonse to Rule Info request\n");
Done:
    UDP_CloseSocket(test2Socket);
    test2InProgress = false;
}

static void Test2_f(void) {
    char* host;
    int n;
    IPaddress sendaddr;

    if (test2InProgress)
        return;

    host = Cmd_Argv(1);

    if (host && hostCacheCount) {
        for (n = 0; n < hostCacheCount; n++) {
            if (Q_strcasecmp(host, hostcache[n].name) != 0) {
                continue;
            }
            if (hostcache[n].driver != myDriverLevel) {
                continue;
            }
            Q_memcpy(&sendaddr, &hostcache[n].addr, sizeof(IPaddress));
            break;
        }
        if (n < hostCacheCount) {
            goto JustDoIt;
        }
    }

    if (!UDP_IsInitialized()) {
        return;
    }
    // See if we can resolve the host name.
    if (UDP_GetAddrFromName(host, &sendaddr) == -1) {
        return;
    }

JustDoIt:
    test2Socket = UDP_OpenSocket(0);
    if (test2Socket == NULL) {
        return;
    }

    test2InProgress = true;

    SZ_Clear(&net_message);
    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
    MSG_WriteString(&net_message, "");
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
    UDP_Write(test2Socket, net_message.data, net_message.cursize, &sendaddr);
    SZ_Clear(&net_message);
    NET_SchedulePollProcedure(&test2PollProcedure, 0.05);
}


int Datagram_Init(void) {
    myDriverLevel = net_driverlevel;
    Cmd_AddCommand("net_stats", NET_Stats_f);

    if (COM_CheckParm("-nolan")) {
        return -1;
    }

    UDP_Init();

#ifdef BAN_TEST
    Cmd_AddCommand("ban", NET_Ban_f);
#endif
    Cmd_AddCommand("test", Test_f);
    Cmd_AddCommand("test2", Test2_f);

    return 0;
}


void Datagram_Shutdown(void) {
    UDP_Shutdown();
}


void Datagram_Close(qsocket_t* sock) {
    UDP_CloseSocket(sock->socket);
}


void Datagram_Listen(qboolean state) {
    if (UDP_IsInitialized()) {
        UDP_Listen(state);
    }
}


#ifdef BAN_TEST
static qboolean NET_IsBanned(IPaddress* addr) {
    if (addr->sa_family != AF_INET) {
        return false;
    }
    unsigned long testAddr = ((struct sockaddr_in*) addr)->sin_addr.s_addr;
    return (testAddr & banMask) == banAddr;
}
#endif

static client_t* NET_FindActiveClient(const int client_num) {
    int active_num = -1;
    for (int i = 0; i < svs.maxclients; i++) {
        client_t* client = &svs.clients[i];
        if (!client->active) {
            continue;
        }
        active_num++;
        if (active_num == client_num) {
            return client;
        }
    }
    return NULL;
}

static cvar_t* NET_FindNextServerCvar(char* prev_cvar_name) {
    cvar_t* var;

    // First, find the search start location.
    if (*prev_cvar_name) {
        var = Cvar_FindVar(prev_cvar_name);
        if (!var) {
            return NULL;
        }
        var = var->next;
    } else {
        var = cvar_vars;
    }

    // Now search for the next server cvar.
    while (var) {
        if (var->server) {
            break;
        }
        var = var->next;
    }

    return var;
}

static void NET_REP_Reject(
    UDPsocket acceptsock,
    const IPaddress* addr,
    char* reason
) {
    SZ_Clear(&net_message);

    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREP_REJECT);
    MSG_WriteString(&net_message, reason);
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));

    UDP_Write(acceptsock, net_message.data, net_message.cursize, addr);

    SZ_Clear(&net_message);
}

static void NET_REP_Accept(
    UDPsocket acceptsock,
    const IPaddress* addr,
    UDPsocket newsock
) {
    IPaddress newaddr = UDP_GetSocketAddr(newsock);

    SZ_Clear(&net_message);

    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREP_ACCEPT);
    MSG_WriteLong(&net_message, UDP_GetSocketPort(&newaddr));
    // Write header.
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));

    UDP_Write(acceptsock, net_message.data, net_message.cursize, addr);

    SZ_Clear(&net_message);
}

static qsocket_t* NET_TryConnectClient(
    UDPsocket acceptsock,
    const IPaddress* addr
) {
    // Allocate a QSocket.
    qsocket_t* sock = NET_NewQSocket();
    if (!sock) {
        // No room. Try to let him know.
        NET_REP_Reject(acceptsock, addr, "Server is full.\n");
        return NULL;
    }

    // Allocate a network socket.
    UDPsocket newsock = UDP_OpenSocket(0);
    if (!newsock) {
        NET_FreeQSocket(sock);
        return NULL;
    }

    // Everything is allocated, just fill in the details.
    sock->socket = newsock;
    sock->addr = *addr;
    Q_strcpy(sock->address, UDP_AddrToString(addr));

    // Send him back the info about the server connection he has been allocated.
    NET_REP_Accept(acceptsock, addr, newsock);

    return sock;
}

static qboolean NET_IsAlreadyConnected(
    UDPsocket acceptsock,
    const IPaddress* addr
) {
    for (qsocket_t* s = net_activeSockets; s; s = s->next) {
        if (s->driver != net_driverlevel) {
            continue;
        }
        const int ret = UDP_AddrCompare(addr, &s->addr);
        if (ret < 0) {
            continue;
        }
        // Is this a duplicate connection request?
        if (ret == 0 && net_time - s->connecttime < 2.0) {
            // Yes, so send a duplicate reply.
            NET_REP_Accept(acceptsock, addr, s->socket);
            return true;
        }
        // It's somebody coming back in from a crash/disconnect,
        // so close the old qsocket and let their retry get them back in.
        NET_Close(s);
        return true;
    }
    return false;
}

static qsocket_t* NET_REP_Connect(UDPsocket acceptsock, IPaddress* addr) {
    if (Q_strcmp(MSG_ReadString(), "QUAKE") != 0) {
        return NULL;
    }
    if (MSG_ReadByte() != NET_PROTOCOL_VERSION) {
        NET_REP_Reject(acceptsock, addr, "Incompatible version.\n");
        return NULL;
    }
#ifdef BAN_TEST
    // check for a ban
    if (NET_IsBanned(addr)) {
        NET_REP_Reject(acceptsock, addr, "You have been banned.\n");
        return NULL;
    }
#endif
    if (NET_IsAlreadyConnected(acceptsock, addr)) {
        return NULL;
    }
    return NET_TryConnectClient(acceptsock, addr);
}

static void NET_REP_RuleInfo(UDPsocket acceptsock, const IPaddress* addr) {
    char* prev_cvar_name = MSG_ReadString();
    const cvar_t* var = NET_FindNextServerCvar(prev_cvar_name);

    SZ_Clear(&net_message);

    // Save space for the header, filled in later.
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREP_RULE_INFO);
    if (var) {
        MSG_WriteString(&net_message, var->name);
        MSG_WriteString(&net_message, var->string);
    }
    // Write header.
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));

    UDP_Write(acceptsock, net_message.data, net_message.cursize, addr);

    SZ_Clear(&net_message);
}

static void NET_REP_PlayerInfo(UDPsocket acceptsock, const IPaddress* addr) {
    const int playerNumber = MSG_ReadByte();

    client_t* client = NET_FindActiveClient(playerNumber);
    if (!client) {
        return;
    }

    SZ_Clear(&net_message);

    // Save space for the header, filled in later.
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREP_PLAYER_INFO);
    MSG_WriteByte(&net_message, playerNumber);
    MSG_WriteString(&net_message, client->name);
    MSG_WriteLong(&net_message, client->colors);
    MSG_WriteLong(&net_message, (int) client->edict->v.frags);
    MSG_WriteLong(&net_message, (int) (net_time - client->netconnection->connecttime));
    MSG_WriteString(&net_message, client->netconnection->address);
    // Write header.
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));

    UDP_Write(acceptsock, net_message.data, net_message.cursize, addr);

    SZ_Clear(&net_message);
}

static void NET_REP_ServerInfo(UDPsocket acceptsock, const IPaddress* addr) {
    if (Q_strcmp(MSG_ReadString(), "QUAKE") != 0) {
        return;
    }

    SZ_Clear(&net_message);

    // Save space for the header, filled in later.
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREP_SERVER_INFO);
    MSG_WriteString(&net_message, "");
    MSG_WriteString(&net_message, hostname.string);
    MSG_WriteString(&net_message, sv.name);
    MSG_WriteByte(&net_message, net_activeconnections);
    MSG_WriteByte(&net_message, svs.maxclients);
    MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
    // Write header.
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));

    UDP_Write(acceptsock, net_message.data, net_message.cursize, addr);

    SZ_Clear(&net_message);
}

static qsocket_t* NET_ControlResponse(UDPsocket acceptsock, IPaddress* addr) {
    const int command = MSG_ReadByte();
    switch (command) {
        case CCREQ_SERVER_INFO:
            NET_REP_ServerInfo(acceptsock, addr);
            return NULL;
        case CCREQ_PLAYER_INFO:
            NET_REP_PlayerInfo(acceptsock, addr);
            return NULL;
        case CCREQ_RULE_INFO:
            NET_REP_RuleInfo(acceptsock, addr);
            return NULL;
        case CCREQ_CONNECT:
            return NET_REP_Connect(acceptsock, addr);
        default:
            return NULL;
    }
}

static qboolean NET_CheckControlHeader(const int len) {
    const int control = BigLong(*((int*) net_message.data));
    MSG_ReadLong();
    if (control == -1) {
        return false;
    }
    if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) {
        return false;
    }
    if ((control & NETFLAG_LENGTH_MASK) != len) {
        return false;
    }
    return true;
}

qsocket_t* Datagram_CheckNewConnections(void) {
    if (!UDP_IsInitialized()) {
        return NULL;
    }

    UDPsocket acceptsock = UDP_GetAcceptSocket();
    if (acceptsock == NULL) {
        return NULL;
    }

    SZ_Clear(&net_message);

    IPaddress clientaddr;
    const int len = UDP_Read(acceptsock, net_message.data, net_message.maxsize, &clientaddr);
    if (len < sizeof(int)) {
        return NULL;
    }
    net_message.cursize = len;

    MSG_BeginReading();
    if (!NET_CheckControlHeader(len)) {
        return NULL;
    }
    return NET_ControlResponse(acceptsock, &clientaddr);
}


static void NET_REQ_ServerInfo(void) {
    UDPsocket control_sock = UDP_GetControlSocket();

    SZ_Clear(&net_message);

    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
    MSG_WriteString(&net_message, "QUAKE");
    MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
    // Write header.
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));

    UDP_Broadcast(control_sock, net_message.data, net_message.cursize);

    SZ_Clear(&net_message);
}

static void NET_ResolveNameConflict(hostcache_t* host) {
    for (int i = 0; i < hostCacheCount; i++) {
        if (&hostcache[i] == host) {
            continue;
        }
        if (Q_strcasecmp(host->name, hostcache[i].name) != 0) {
            continue;
        }
        i = Q_strlen(host->name);
        if (i < 15 && host->name[i - 1] > '8') {
            host->name[i] = '0';
            host->name[i + 1] = 0;
        } else {
            host->name[i - 1]++;
        }
        i = -1;
    }
}

static hostcache_t* NET_CacheNewHost(IPaddress* addr) {
    hostcache_t* host = &hostcache[hostCacheCount];
    hostCacheCount++;

    Q_strcpy(host->name, MSG_ReadString());
    Q_strcpy(host->map, MSG_ReadString());
    host->users = MSG_ReadByte();
    host->maxusers = MSG_ReadByte();
    if (MSG_ReadByte() != NET_PROTOCOL_VERSION) {
        Q_strcpy(host->cname, host->name);
        host->cname[14] = 0;
        Q_strcpy(host->name, "*");
        Q_strcat(host->name, host->cname);
    }
    Q_memcpy(&host->addr, addr, sizeof(IPaddress));
    host->driver = net_driverlevel;
    Q_strcpy(host->cname, UDP_AddrToString(addr));

    return host;
}

static qboolean NET_IsHostCached(const IPaddress* addr) {
    for (int n = 0; n < hostCacheCount; n++) {
        if (UDP_AddrCompare(addr, &hostcache[n].addr) == 0) {
            return true;
        }
    }
    return false;
}

static void NET_CacheServerHost(const IPaddress* client_addr) {
    IPaddress server_addr = *client_addr;
    UDP_GetAddrFromName(MSG_ReadString(), &server_addr);
    if (!NET_IsHostCached(&server_addr)) {
        hostcache_t* host = NET_CacheNewHost(&server_addr);
        NET_ResolveNameConflict(host);
    }
}

static void NET_ReadServerInfo(void) {
    int ret;
    IPaddress readaddr;

    UDPsocket control_sock = UDP_GetControlSocket();
    IPaddress my_addr = UDP_GetSocketAddr(control_sock);

    while ((ret = UDP_Read(control_sock, net_message.data, net_message.maxsize, &readaddr)) > 0) {
        if (ret < sizeof(int)) {
            continue;
        }
        net_message.cursize = ret;

        if (UDP_AddrCompare(&readaddr, &my_addr) >= 0) {
            // Don't answer our own query.
            continue;
        }
        if (hostCacheCount == HOSTCACHESIZE) {
            // Cache full.
            continue;
        }

        MSG_BeginReading();
        if (!NET_CheckControlHeader(ret)) {
            continue;
        }
        if (MSG_ReadByte() == CCREP_SERVER_INFO) {
            NET_CacheServerHost(&readaddr);
        }
    }
}

void Datagram_SearchForHosts(qboolean xmit) {
    if (!UDP_IsInitialized() || hostCacheCount == HOSTCACHESIZE) {
        return;
    }
    if (xmit) {
        NET_REQ_ServerInfo();
    }
    NET_ReadServerInfo();
}


static void NET_REQ_Connect(UDPsocket sock, const IPaddress* addr) {
    SZ_Clear(&net_message);

    // save space for the header, filled in later
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREQ_CONNECT);
    MSG_WriteString(&net_message, "QUAKE");
    MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
    // Write header.
    *((int*) net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));

    UDP_Write(sock, net_message.data, net_message.cursize, addr);

    SZ_Clear(&net_message);
}

static void NET_ConnectError(qsocket_t* sock) {
    if (m_return_onerror) {
        key_dest = key_menu;
        m_state = m_return_state;
        m_return_onerror = false;
    }
    if (sock) {
        UDP_CloseSocket(sock->socket);
        NET_FreeQSocket(sock);
    }
}

static void NET_ReportConnectError(qsocket_t* sock, char* reason) {
    Con_Printf("%s", reason);
    Q_strncpy(m_return_reason, reason, 31);
    NET_ConnectError(sock);
}

static qsocket_t* NET_HandleConnectResponse(
    qsocket_t* sock,
    IPaddress* sendaddr
) {
    const int command = MSG_ReadByte();
    switch (command) {
        case CCREP_REJECT:
            NET_ReportConnectError(sock, MSG_ReadString());
            return NULL;
        case CCREP_ACCEPT:
            Con_Printf("Connection accepted\n");
            Q_memcpy(&sock->addr, sendaddr, sizeof(*sendaddr));
            UDP_SetSocketPort(&sock->addr, MSG_ReadLong());
            UDP_GetNameFromAddr(sendaddr, sock->address);
            sock->lastMessageTime = SetNetTime();
            m_return_onerror = false;
            return sock;
        default:
            NET_ReportConnectError(sock, "Bad Response\n");
            return NULL;
    }
}

static int NET_WaitConnectResponse(
    double start_time,
    UDPsocket newsock,
    IPaddress* readaddr,
    const IPaddress* sendaddr
) {
    int ret;

    do {
        ret = UDP_Read(newsock, net_message.data, net_message.maxsize, readaddr);
        if (ret <= 0) {
            // No response.
            continue;
        }
        if (UDP_AddrCompare(readaddr, sendaddr) != 0) {
            // Not from the right place.
#ifdef PARANOID
            Con_Printf("wrong reply address\n");
            Con_Printf("Expected: %s\n", StrAddr(sendaddr));
            Con_Printf("Received: %s\n", StrAddr(readaddr));
            SCR_UpdateScreen();
#endif
            ret = 0;
            continue;
        }
        if (ret < sizeof(int)) {
            ret = 0;
            continue;
        }
        net_message.cursize = ret;
        MSG_BeginReading();
        if (!NET_CheckControlHeader(ret)) {
            ret = 0;
        }
    } while (ret == 0 && (SetNetTime() - start_time) < 2.5);

    return ret;
}

static int NET_TryConnectServer(
    UDPsocket newsock,
    IPaddress* readaddr,
    const IPaddress* sendaddr
) {
    Con_Printf("trying...\n");
    SCR_UpdateScreen();

    double start_time = net_time;
    int ret = 0;

    for (int reps = 0; reps < 3; reps++) {
        NET_REQ_Connect(newsock, sendaddr);
        ret = NET_WaitConnectResponse(start_time, newsock, readaddr, sendaddr);
        if (ret) {
            break;
        }
        Con_Printf("still trying...\n");
        SCR_UpdateScreen();
        start_time = SetNetTime();
    }

    return ret;
}

qsocket_t* Datagram_Connect(char* host) {
    if (!UDP_IsInitialized()) {
        return NULL;
    }

    IPaddress sendaddr;
    if (UDP_GetAddrFromName(host, &sendaddr) == -1) {
        // Cannot resolve the host name.
        return NULL;
    }

    UDPsocket newsock = UDP_OpenSocket(0);
    if (newsock == NULL) {
        return NULL;
    }

    qsocket_t* sock = NET_NewQSocket();
    if (!sock) {
        UDP_CloseSocket(newsock);
        NET_ConnectError(sock);
        return NULL;
    }
    sock->socket = newsock;

    IPaddress readaddr;
    const int ret = NET_TryConnectServer(newsock, &readaddr, &sendaddr);
    if (ret == 0) {
        NET_ReportConnectError(sock, "No Response\n");
        return NULL;
    }
    if (ret == -1) {
        NET_ReportConnectError(sock, "Network Error\n");
        return NULL;
    }
    return NET_HandleConnectResponse(sock, &sendaddr);
}
