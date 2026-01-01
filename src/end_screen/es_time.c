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


#include "es_time.h"
#include "es_buffer.h"
#include <SDL_events.h>
#include <SDL_timer.h>


void ES_Sleep(void) {
    if (!ES_HasBlinkChar()) {
        SDL_WaitEvent(NULL);
        return;
    }

    // There are blinking characters on the screen,
    // so we must time out after a while.
    const Uint32 time_to_next_blink = BLINK_PERIOD - (SDL_GetTicks() % BLINK_PERIOD);
    // Add one so it is always positive.
    const Uint32 timeout = time_to_next_blink + 1;

    // Sit in a busy loop until the timeout expires,
    // or we have to redraw the blinking screen.
    const Uint32 start_time = SDL_GetTicks();
    const Uint32 end_time = start_time + timeout;
    while (SDL_GetTicks() < end_time) {
        if (SDL_PollEvent(NULL)) {
            // Received an event, so stop waiting.
            break;
        }
        // Don't hog the CPU.
        SDL_Delay(1);
    }
}
