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
// net_poll.h


#ifndef __NET_POLL__
#define __NET_POLL__

#include "quakedef.h"
#include "net.h"

typedef struct poll_procedure_s {
    struct poll_procedure_s* next;
    double nextTime;
    void (*procedure)();
} poll_procedure_t;

void NET_PrintSlist(void);
void NET_SchedulePollProcedure(poll_procedure_t* proc, double timeOffset);

#endif
