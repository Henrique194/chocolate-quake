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
// net_main.c


#include "net.h"
#include "client.h"
#include "cmd.h"
#include "console.h"
#include "net_poll.h"
#include "net_socket.h"
#include "net_vcr.h"
#include "server.h"
#include "sys.h"


qboolean serialAvailable = false;
qboolean ipxAvailable = false;
qboolean tcpipAvailable = false;

int net_hostport;
int DEFAULTnet_hostport = 26000;

char my_ipx_address[NET_NAMELEN];
char my_tcpip_address[NET_NAMELEN];

void (*GetComPortConfig)(int portNumber, int* port, int* irq, int* baud,
                         qboolean* useModem);
void (*SetComPortConfig)(int portNumber, int port, int irq, int baud,
                         qboolean useModem);
void (*GetModemConfig)(int portNumber, char* dialType, char* clear, char* init,
                       char* hangup);
void (*SetModemConfig)(int portNumber, char* dialType, char* clear, char* init,
                       char* hangup);

static qboolean listening = false;


sizebuf_t net_message;
int net_activeconnections = 0;

int messagesSent = 0;
int messagesReceived = 0;
int unreliableMessagesSent = 0;
int unreliableMessagesReceived = 0;

cvar_t hostname = {"hostname", "UNNAMED"};

qboolean configRestored = false;
cvar_t config_com_port = {"_config_com_port", "0x3f8", true};
cvar_t config_com_irq = {"_config_com_irq", "4", true};
cvar_t config_com_baud = {"_config_com_baud", "57600", true};
cvar_t config_com_modem = {"_config_com_modem", "1", true};
cvar_t config_modem_dialtype = {"_config_modem_dialtype", "T", true};
cvar_t config_modem_clear = {"_config_modem_clear", "ATZ", true};
cvar_t config_modem_init = {"_config_modem_init", "", true};
cvar_t config_modem_hangup = {"_config_modem_hangup", "AT H", true};

int vcrFile = -1;
qboolean recording = false;

// macro to make the code more readable
#define dfunc net_drivers[net_driverlevel]

int net_driverlevel;


double net_time;

double SetNetTime(void) {
    net_time = Sys_FloatTime();
    return net_time;
}


static void NET_Listen_f(void) {
    if (Cmd_Argc() != 2) {
        Con_Printf("\"listen\" is \"%u\"\n", listening ? 1 : 0);
        return;
    }

    listening = Q_atoi(Cmd_Argv(1)) ? true : false;

    for (net_driverlevel = 0; net_driverlevel < net_numdrivers;
         net_driverlevel++) {
        if (net_drivers[net_driverlevel].initialized == false)
            continue;
        dfunc.Listen(listening);
    }
}


static void MaxPlayers_f(void) {
    int n;

    if (Cmd_Argc() != 2) {
        Con_Printf("\"maxplayers\" is \"%u\"\n", svs.maxclients);
        return;
    }

    if (sv.active) {
        Con_Printf(
            "maxplayers can not be changed while a server is running.\n");
        return;
    }

    n = Q_atoi(Cmd_Argv(1));
    if (n < 1)
        n = 1;
    if (n > svs.maxclientslimit) {
        n = svs.maxclientslimit;
        Con_Printf("\"maxplayers\" set to \"%u\"\n", n);
    }

    if ((n == 1) && listening)
        Cbuf_AddText("listen 0\n");

    if ((n > 1) && (!listening))
        Cbuf_AddText("listen 1\n");

    svs.maxclients = n;
    if (n == 1)
        Cvar_Set("deathmatch", "0");
    else
        Cvar_Set("deathmatch", "1");
}


static void NET_Port_f(void) {
    int n;

    if (Cmd_Argc() != 2) {
        Con_Printf("\"port\" is \"%u\"\n", net_hostport);
        return;
    }

    n = Q_atoi(Cmd_Argv(1));
    if (n < 1 || n > 65534) {
        Con_Printf("Bad value, must be between 1 and 65534\n");
        return;
    }

    DEFAULTnet_hostport = n;
    net_hostport = n;

    if (listening) {
        // force a change to the new port
        Cbuf_AddText("listen 0\n");
        Cbuf_AddText("listen 1\n");
    }
}


/*
===================
NET_Connect
===================
*/

int hostCacheCount = 0;
hostcache_t hostcache[HOSTCACHESIZE];

qsocket_t* NET_Connect(char* host) {
    qsocket_t* ret;
    int n;
    int numdrivers = net_numdrivers;

    SetNetTime();

    if (host && *host == 0)
        host = NULL;

    if (host) {
        if (Q_strcasecmp(host, "local") == 0) {
            numdrivers = 1;
            goto JustDoIt;
        }

        if (hostCacheCount) {
            for (n = 0; n < hostCacheCount; n++)
                if (Q_strcasecmp(host, hostcache[n].name) == 0) {
                    host = hostcache[n].cname;
                    break;
                }
            if (n < hostCacheCount)
                goto JustDoIt;
        }
    }

    slistSilent = host ? true : false;
    NET_Slist_f();

    while (slistInProgress)
        NET_Poll();

    if (host == NULL) {
        if (hostCacheCount != 1)
            return NULL;
        host = hostcache[0].cname;
        Con_Printf("Connecting to...\n%s @ %s\n\n", hostcache[0].name, host);
    }

    if (hostCacheCount)
        for (n = 0; n < hostCacheCount; n++)
            if (Q_strcasecmp(host, hostcache[n].name) == 0) {
                host = hostcache[n].cname;
                break;
            }

JustDoIt:
    for (net_driverlevel = 0; net_driverlevel < numdrivers; net_driverlevel++) {
        if (net_drivers[net_driverlevel].initialized == false)
            continue;
        ret = dfunc.Connect(host);
        if (ret)
            return ret;
    }

    if (host) {
        NET_PrintSlist();
    }

    return NULL;
}


/*
===================
NET_CheckNewConnections
===================
*/

struct {
    double time;
    int op;
    intptr_t session;
} vcrConnect;

qsocket_t* NET_CheckNewConnections(void) {
    qsocket_t* ret;

    SetNetTime();

    for (net_driverlevel = 0; net_driverlevel < net_numdrivers;
         net_driverlevel++) {
        if (net_drivers[net_driverlevel].initialized == false)
            continue;
        if (net_driverlevel && listening == false)
            continue;
        ret = dfunc.CheckNewConnections();
        if (ret) {
            if (recording) {
                vcrConnect.time = host_time;
                vcrConnect.op = VCR_OP_CONNECT;
                vcrConnect.session = (intptr_t) ret;
                Sys_FileWrite(vcrFile, &vcrConnect, sizeof(vcrConnect));
                Sys_FileWrite(vcrFile, ret->address, NET_NAMELEN);
            }
            return ret;
        }
    }

    if (recording) {
        vcrConnect.time = host_time;
        vcrConnect.op = VCR_OP_CONNECT;
        vcrConnect.session = 0;
        Sys_FileWrite(vcrFile, &vcrConnect, sizeof(vcrConnect));
    }

    return NULL;
}


/*
=================
NET_GetMessage

If there is a complete message, return it in net_message

returns 0 if no data is waiting
returns 1 if a message was received
returns -1 if connection is invalid
=================
*/

struct {
    double time;
    int op;
    intptr_t session;
    int ret;
    int len;
} vcrGetMessage;

extern void PrintStats(qsocket_t* s);

int NET_GetMessage(qsocket_t* sock) {
    if (!sock) {
        return -1;
    }
    const int ret = NET_GetSocketMessage(sock);
    if (NET_IsSocketDisconnected(sock)) {
        return -1;
    }
    if (recording) {
        vcrGetMessage.time = host_time;
        vcrGetMessage.op = VCR_OP_GETMESSAGE;
        vcrGetMessage.session = (intptr_t) sock;
        vcrGetMessage.ret = ret;
        if (ret > 0) {
            vcrGetMessage.len = net_message.cursize;
            Sys_FileWrite(vcrFile, &vcrGetMessage, 24);
            Sys_FileWrite(vcrFile, net_message.data, net_message.cursize);
        } else {
            Sys_FileWrite(vcrFile, &vcrGetMessage, 20);
        }
    }
    return ret;
}


/*
==================
NET_SendMessage

Try to send a complete length+message unit over the reliable stream.
returns 0 if the message cannot be delivered reliably, but the connection
		is still considered valid
returns 1 if the message was sent properly
returns -1 if the connection died
==================
*/
struct {
    double time;
    int op;
    intptr_t session;
    int r;
} vcrSendMessage;

static void NET_SendVCRMessage(const qsocket_t* sock, const int op, const int ret) {
    if (NET_IsSocketDisconnected(sock)) {
        return;
    }
    vcrSendMessage.time = host_time;
    vcrSendMessage.op = op;
    vcrSendMessage.session = (intptr_t) sock;
    vcrSendMessage.r = ret;
    Sys_FileWrite(vcrFile, &vcrSendMessage, 20);
}


int NET_SendMessage(qsocket_t* sock, sizebuf_t* data) {
    if (!sock) {
        return -1;
    }
    const int ret = NET_SendSocketMessage(sock, data);
    if (recording) {
        NET_SendVCRMessage(sock, VCR_OP_SENDMESSAGE, ret);
    }
    return ret;
}


int NET_SendUnreliableMessage(qsocket_t* sock, sizebuf_t* data) {
    if (!sock) {
        return -1;
    }
    const int ret = NET_SendSocketUnreliableMessage(sock, data);
    if (recording) {
        NET_SendVCRMessage(sock, VCR_OP_SENDMESSAGE, ret);
    }
    return ret;
}


/*
==================
NET_CanSendMessage

Returns true or false if the given qsocket can currently accept a
message to be transmitted.
==================
*/
qboolean NET_CanSendMessage(qsocket_t* sock) {
    if (!sock) {
        return false;
    }
    const int ret = NET_CanSocketSendMessage(sock);
    if (recording) {
        NET_SendVCRMessage(sock, VCR_OP_CANSENDMESSAGE, ret);
    }
    return ret;
}


int NET_SendToAll(sizebuf_t* data, int blocktime) {
    double start;
    int i;
    int count = 0;
    qboolean state1[MAX_SCOREBOARD];
    qboolean state2[MAX_SCOREBOARD];

    for (i = 0, host_client = svs.clients; i < svs.maxclients;
         i++, host_client++) {
        if (!host_client->netconnection)
            continue;
        if (host_client->active) {
            if (host_client->netconnection->driver == 0) {
                NET_SendMessage(host_client->netconnection, data);
                state1[i] = true;
                state2[i] = true;
                continue;
            }
            count++;
            state1[i] = false;
            state2[i] = false;
        } else {
            state1[i] = true;
            state2[i] = true;
        }
    }

    start = Sys_FloatTime();
    while (count) {
        count = 0;
        for (i = 0, host_client = svs.clients; i < svs.maxclients;
             i++, host_client++) {
            if (!state1[i]) {
                if (NET_CanSendMessage(host_client->netconnection)) {
                    state1[i] = true;
                    NET_SendMessage(host_client->netconnection, data);
                } else {
                    NET_GetMessage(host_client->netconnection);
                }
                count++;
                continue;
            }

            if (!state2[i]) {
                if (NET_CanSendMessage(host_client->netconnection)) {
                    state2[i] = true;
                } else {
                    NET_GetMessage(host_client->netconnection);
                }
                count++;
                continue;
            }
        }
        if ((Sys_FloatTime() - start) > blocktime)
            break;
    }
    return count;
}


//=============================================================================

/*
====================
NET_Init
====================
*/

void NET_Init(void) {
    int i;
    int controlSocket;

    if (COM_CheckParm("-playback")) {
        net_numdrivers = 1;
        net_drivers[0].Init = VCR_Init;
    }

    if (COM_CheckParm("-record"))
        recording = true;

    i = COM_CheckParm("-port");
    if (!i)
        i = COM_CheckParm("-udpport");
    if (!i)
        i = COM_CheckParm("-ipxport");

    if (i) {
        if (i < com_argc - 1)
            DEFAULTnet_hostport = Q_atoi(com_argv[i + 1]);
        else
            Sys_Error("NET_Init: you must specify a number after -port");
    }
    net_hostport = DEFAULTnet_hostport;

    if (COM_CheckParm("-listen") || cls.state == ca_dedicated)
        listening = true;

    SetNetTime();

    NET_InitSockets();

    // allocate space for network message buffer
    SZ_Alloc(&net_message, NET_MAXMESSAGE);

    Cvar_RegisterVariable(&hostname);
    Cvar_RegisterVariable(&config_com_port);
    Cvar_RegisterVariable(&config_com_irq);
    Cvar_RegisterVariable(&config_com_baud);
    Cvar_RegisterVariable(&config_com_modem);
    Cvar_RegisterVariable(&config_modem_dialtype);
    Cvar_RegisterVariable(&config_modem_clear);
    Cvar_RegisterVariable(&config_modem_init);
    Cvar_RegisterVariable(&config_modem_hangup);

    Cmd_AddCommand("slist", NET_Slist_f);
    Cmd_AddCommand("listen", NET_Listen_f);
    Cmd_AddCommand("maxplayers", MaxPlayers_f);
    Cmd_AddCommand("port", NET_Port_f);

    // initialize all the drivers
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers;
         net_driverlevel++) {
        controlSocket = net_drivers[net_driverlevel].Init();
        if (controlSocket == -1)
            continue;
        net_drivers[net_driverlevel].initialized = true;
        net_drivers[net_driverlevel].controlSock = controlSocket;
        if (listening)
            net_drivers[net_driverlevel].Listen(true);
    }

    if (*my_ipx_address)
        Con_DPrintf("IPX address %s\n", my_ipx_address);
    if (*my_tcpip_address)
        Con_DPrintf("TCP/IP address %s\n", my_tcpip_address);
}

/*
====================
NET_Shutdown
====================
*/

void NET_Shutdown(void) {
    SetNetTime();

    NET_CloseSockets();

    //
    // shutdown the drivers
    //
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers;
         net_driverlevel++) {
        if (net_drivers[net_driverlevel].initialized == true) {
            net_drivers[net_driverlevel].Shutdown();
            net_drivers[net_driverlevel].initialized = false;
        }
    }

    if (vcrFile != -1) {
        Con_Printf("Closing vcrfile.\n");
        Sys_FileClose(vcrFile);
    }
}
