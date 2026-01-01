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
// net_poll.c


#include "net_poll.h"
#include "client.h"
#include "console.h"
#include "server.h"
#include "sys.h"

// macro to make the code more readable
#define dfunc net_drivers[net_driverlevel]


extern qboolean configRestored;
extern cvar_t config_com_port;
extern cvar_t config_com_irq;
extern cvar_t config_com_baud;
extern cvar_t config_com_modem;
extern cvar_t config_modem_dialtype;
extern cvar_t config_modem_clear;
extern cvar_t config_modem_init;
extern cvar_t config_modem_hangup;


static void Slist_Send(void);
static void Slist_Poll(void);

static double slistStartTime;
static int slistLastShown;

static poll_procedure_t* pollProcedureList = NULL;
static poll_procedure_t slistSendProcedure = {NULL, 0.0, Slist_Send};
static poll_procedure_t slistPollProcedure = {NULL, 0.0, Slist_Poll};

qboolean slistInProgress = false;
qboolean slistSilent = false;
qboolean slistLocal = true;


/*
================================================================================

PROCEDURE SCHEDULING

================================================================================
*/

void NET_SchedulePollProcedure(poll_procedure_t* proc, double timeOffset) {
    poll_procedure_t* pp;
    poll_procedure_t* prev = NULL;

    proc->nextTime = Sys_FloatTime() + timeOffset;
    for (pp = pollProcedureList; pp; pp = pp->next) {
        if (pp->nextTime >= proc->nextTime) {
            break;
        }
        prev = pp;
    }
    if (prev == NULL) {
        proc->next = pollProcedureList;
        pollProcedureList = proc;
        return;
    }
    proc->next = pp;
    prev->next = proc;
}

//==============================================================================


/*
================================================================================

SERVER LIST

================================================================================
*/

static void NET_PrintServers(void) {
    int n;

    for (n = slistLastShown; n < hostCacheCount; n++) {
        const hostcache_t* host = &hostcache[n];
        const char* name = host->name;
        const char* map = host->map;
        const int users = host->users;
        const int max_users = host->maxusers;
        if (max_users == 0) {
            Con_Printf("%-15.15s %-15.15s\n", name, map);
            continue;
        }
        Con_Printf("%-15.15s %-15.15s %2u/%2u\n", name, map, users, max_users);
    }
    slistLastShown = n;
}

static void NET_PrintSlistTrailer(void) {
    if (hostCacheCount) {
        Con_Printf("== end list ==\n\n");
    } else {
        Con_Printf("No Quake servers found.\n\n");
    }
}

static void NET_PrintSlistHeader(void) {
    Con_Printf("Server          Map             Users\n");
    Con_Printf("--------------- --------------- -----\n");
    slistLastShown = 0;
}

void NET_PrintSlist(void) {
    Con_Printf("\n");
    NET_PrintSlistHeader();
    NET_PrintServers();
    NET_PrintSlistTrailer();
}

void NET_Slist_f(void) {
    if (slistInProgress) {
        return;
    }
    if (!slistSilent) {
        Con_Printf("Looking for Quake servers...\n");
        NET_PrintSlistHeader();
    }

    slistInProgress = true;
    slistStartTime = Sys_FloatTime();

    NET_SchedulePollProcedure(&slistSendProcedure, 0.0);
    NET_SchedulePollProcedure(&slistPollProcedure, 0.1);

    hostCacheCount = 0;
}

//==============================================================================


/*
================================================================================

PROCEDURES

================================================================================
*/

static void Slist_Send(void) {
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        if (!slistLocal && net_driverlevel == 0) {
            continue;
        }
        if (dfunc.initialized) {
            dfunc.SearchForHosts(true);
        }
    }

    if ((Sys_FloatTime() - slistStartTime) < 0.5) {
        NET_SchedulePollProcedure(&slistSendProcedure, 0.75);
    }
}

static void Slist_Poll(void) {
    for (net_driverlevel = 0; net_driverlevel < net_numdrivers; net_driverlevel++) {
        if (!slistLocal && net_driverlevel == 0) {
            continue;
        }
        if (dfunc.initialized) {
            dfunc.SearchForHosts(false);
        }
    }

    if (!slistSilent)
        NET_PrintServers();

    if ((Sys_FloatTime() - slistStartTime) < 1.5) {
        NET_SchedulePollProcedure(&slistPollProcedure, 0.1);
        return;
    }

    if (!slistSilent)
        NET_PrintSlistTrailer();
    slistInProgress = false;
    slistSilent = false;
    slistLocal = true;
}

//==============================================================================


void NET_Poll(void) {
    if (!configRestored) {
        if (serialAvailable) {
            qboolean useModem = (config_com_modem.value == 1.0);
            SetComPortConfig(0, (int) config_com_port.value,
                             (int) config_com_irq.value,
                             (int) config_com_baud.value, useModem);
            SetModemConfig(0, config_modem_dialtype.string,
                           config_modem_clear.string, config_modem_init.string,
                           config_modem_hangup.string);
        }
        configRestored = true;
    }

    SetNetTime();

    for (poll_procedure_t* pp = pollProcedureList; pp; pp = pp->next) {
        if (pp->nextTime > net_time) {
            break;
        }
        pollProcedureList = pp->next;
        pp->procedure();
    }
}
