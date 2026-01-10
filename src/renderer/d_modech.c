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
// d_modech.c: called when mode has just changed


#include "d_local.h"


i32 d_vrectx, d_vrecty, d_vrectright_particle, d_vrectbottom_particle;

i32 d_y_aspect_shift, d_pix_min, d_pix_max, d_pix_shift;

i32 d_scantable[MAXHEIGHT];
i16* zspantable[MAXHEIGHT];


/*
================
D_ViewChanged
================
*/
void D_ViewChanged(void) {
    i32 rowbytes;

    if (r_dowarp)
        rowbytes = WARP_WIDTH;
    else
        rowbytes = vid.width;

    scale_for_mip = xscale;
    if (yscale > xscale)
        scale_for_mip = yscale;

    d_zrowbytes = vid.width * 2;
    d_zwidth = vid.width;

    d_pix_min = r_refdef.vrect.width / 320;
    if (d_pix_min < 1)
        d_pix_min = 1;

    d_pix_max = (i32) ((float) r_refdef.vrect.width / (320.0 / 4.0) + 0.5);
    d_pix_shift = 8 - (i32) ((float) r_refdef.vrect.width / 320.0 + 0.5);
    if (d_pix_max < 1)
        d_pix_max = 1;

    if (pixelAspect > 1.4)
        d_y_aspect_shift = 1;
    else
        d_y_aspect_shift = 0;

    d_vrectx = r_refdef.vrect.x;
    d_vrecty = r_refdef.vrect.y;
    d_vrectright_particle = r_refdef.vrectright - d_pix_max;
    d_vrectbottom_particle =
        r_refdef.vrectbottom - (d_pix_max << d_y_aspect_shift);

    {
        i32 i;

        for (i = 0; i < vid.height; i++) {
            d_scantable[i] = i * rowbytes;
            zspantable[i] = d_pzbuffer + i * d_zwidth;
        }
    }
}
