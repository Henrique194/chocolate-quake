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
// r_misc.c


#include "quakedef.h"
#include "r_local.h"


/*
===============
R_CheckVariables
===============
*/
void R_CheckVariables(void) {
    static float oldbright;

    if (r_fullbright.value != oldbright) {
        oldbright = r_fullbright.value;
        D_FlushCaches(); // so all lighting changes
    }
}


/*
============
Show

Debugging use
============
*/
void Show(void) {
    vrect_t vr;

    vr.x = vr.y = 0;
    vr.width = vid.width;
    vr.height = vid.height;
    VID_Update(&vr);
}


/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f(void) {
    int i;
    float start, stop, time;
    int startangle;
    vrect_t vr;

    startangle = r_refdef.viewangles[1];

    start = Sys_FloatTime();
    for (i = 0; i < 128; i++) {
        r_refdef.viewangles[1] = i / 128.0 * 360.0;

        VID_LockBuffer();

        R_RenderView();

        VID_UnlockBuffer();

        vr.x = r_refdef.vrect.x;
        vr.y = r_refdef.vrect.y;
        vr.width = r_refdef.vrect.width;
        vr.height = r_refdef.vrect.height;
        VID_Update(&vr);
    }
    stop = Sys_FloatTime();
    time = stop - start;
    Con_Printf("%f seconds (%f fps)\n", time, 128 / time);

    r_refdef.viewangles[1] = startangle;
}


/*
================
R_LineGraph

Only called by R_DisplayTime
================
*/
void R_LineGraph(int x, int y, int h) {
    int i;
    byte* dest;
    int s;

    // FIXME: should be disabled on no-buffer adapters, or should be in the driver

    x += r_refdef.vrect.x;
    y += r_refdef.vrect.y;

    dest = vid.buffer + vid.width * y + x;

    s = r_graphheight.value;

    if (h > s)
        h = s;

    for (i = 0; i < h; i++, dest -= vid.width * 2) {
        dest[0] = 0xff;
        *(dest - vid.width) = 0x30;
    }
    for (; i < s; i++, dest -= vid.width * 2) {
        dest[0] = 0x30;
        *(dest - vid.width) = 0x30;
    }
}

/*
==============
R_TimeGraph

Performance monitoring tool
==============
*/
#define MAX_TIMINGS 100
extern float mouse_x, mouse_y;
void R_TimeGraph(void) {
    static int timex;
    int a;
    float r_time2;
    static byte r_timings[MAX_TIMINGS];
    int x;

    r_time2 = Sys_FloatTime();

    a = (r_time2 - r_time1) / 0.01;
    //a = fabs(mouse_y * 0.05);
    //a = (int)((r_refdef.vieworg[2] + 1024)/1)%(int)r_graphheight.value;
    //a = fabs(velocity[0])/20;
    //a = ((int)fabs(origin[0])/8)%20;
    //a = (cl.idealpitch + 30)/5;
    r_timings[timex] = a;
    a = timex;

    if (r_refdef.vrect.width <= MAX_TIMINGS)
        x = r_refdef.vrect.width - 1;
    else
        x = r_refdef.vrect.width - (r_refdef.vrect.width - MAX_TIMINGS) / 2;
    do {
        R_LineGraph(x, r_refdef.vrect.height - 2, r_timings[a]);
        if (x == 0)
            break; // screen too small to hold entire thing
        x--;
        a--;
        if (a == -1)
            a = MAX_TIMINGS - 1;
    } while (a != timex);

    timex = (timex + 1) % MAX_TIMINGS;
}


/*
=============
R_PrintTimes
=============
*/
void R_PrintTimes(void) {
    float r_time2;
    float ms;

    r_time2 = Sys_FloatTime();

    ms = 1000 * (r_time2 - r_time1);

    Con_Printf("%5.1f ms %3i/%3i/%3i poly %3i surf\n", ms, c_faceclip,
               r_polycount, r_drawnpolycount, c_surf);
    c_surf = 0;
}


/*
=============
R_PrintDSpeeds
=============
*/
void R_PrintDSpeeds(void) {
    float ms, dp_time, r_time2, rw_time, db_time, se_time, de_time, dv_time;

    r_time2 = Sys_FloatTime();

    dp_time = (dp_time2 - dp_time1) * 1000;
    rw_time = (rw_time2 - rw_time1) * 1000;
    db_time = (db_time2 - db_time1) * 1000;
    se_time = (se_time2 - se_time1) * 1000;
    de_time = (de_time2 - de_time1) * 1000;
    dv_time = (dv_time2 - dv_time1) * 1000;
    ms = (r_time2 - r_time1) * 1000;

    Con_Printf("%3i %4.1fp %3iw %4.1fb %3is %4.1fe %4.1fv\n", (int) ms, dp_time,
               (int) rw_time, db_time, (int) se_time, de_time, dv_time);
}


/*
=============
R_PrintAliasStats
=============
*/
void R_PrintAliasStats(void) {
    Con_Printf("%3i polygon model drawn\n", r_amodels_drawn);
}


void WarpPalette(void) {
    int i, j;
    byte newpalette[768];
    int basecolor[3];

    basecolor[0] = 130;
    basecolor[1] = 80;
    basecolor[2] = 50;

    // pull the colors halfway to bright brown
    for (i = 0; i < 256; i++) {
        for (j = 0; j < 3; j++) {
            newpalette[i * 3 + j] =
                (host_basepal[i * 3 + j] + basecolor[j]) / 2;
        }
    }

    VID_ShiftPalette(newpalette);
}


/*
===================
R_TransformFrustum
===================
*/
void R_TransformFrustum(void) {
    int i;
    vec3_t v, v2;

    for (i = 0; i < 4; i++) {
        v[0] = screenedge[i].normal[2];
        v[1] = -screenedge[i].normal[0];
        v[2] = screenedge[i].normal[1];

        v2[0] = v[1] * vright[0] + v[2] * vup[0] + v[0] * vpn[0];
        v2[1] = v[1] * vright[1] + v[2] * vup[1] + v[0] * vpn[1];
        v2[2] = v[1] * vright[2] + v[2] * vup[2] + v[0] * vpn[2];

        VectorCopy(v2, view_clipplanes[i].normal);

        view_clipplanes[i].dist = DotProduct(modelorg, v2);
    }
}

/*
================
TransformVector
================
*/
void TransformVector(vec3_t in, vec3_t out) {
    out[0] = DotProduct(in, vright);
    out[1] = DotProduct(in, vup);
    out[2] = DotProduct(in, vpn);
}

/*
================
R_TransformPlane
================
*/
void R_TransformPlane(mplane_t* p, float* normal, float* dist) {
    float d;

    d = DotProduct(r_origin, p->normal);
    *dist = p->dist - d;
    // TODO: when we have rotating entities, this will need to use the view matrix
    TransformVector(p->normal, normal);
}


/*
===============
R_SetUpFrustumIndexes
===============
*/
void R_SetUpFrustumIndexes(void) {
    int i, j, *pindex;

    pindex = r_frustum_indexes;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            if (view_clipplanes[i].normal[j] < 0) {
                pindex[j] = j;
                pindex[j + 3] = j + 3;
            } else {
                pindex[j] = j + 3;
                pindex[j + 3] = j;
            }
        }

        // FIXME: do just once at start
        pfrustum_indexes[i] = pindex;
        pindex += 6;
    }
}


/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame(void) {
    int edgecount;
    vrect_t vrect;
    float w, h;

    // don't allow cheats in multiplayer
    if (cl.maxclients > 1) {
        Cvar_Set("r_draworder", "0");
        Cvar_Set("r_fullbright", "0");
        Cvar_Set("r_ambient", "0");
        Cvar_Set("r_drawflat", "0");
    }

    if (r_numsurfs.value) {
        if ((surface_p - surfaces) > r_maxsurfsseen)
            r_maxsurfsseen = surface_p - surfaces;

        Con_Printf("Used %d of %d surfs; %d max\n", surface_p - surfaces,
                   surf_max - surfaces, r_maxsurfsseen);
    }

    if (r_numedges.value) {
        edgecount = edge_p - r_edges;

        if (edgecount > r_maxedgesseen)
            r_maxedgesseen = edgecount;

        Con_Printf("Used %d of %d edges; %d max\n", edgecount,
                   r_numallocatededges, r_maxedgesseen);
    }

    r_refdef.ambientlight = r_ambient.value;

    if (r_refdef.ambientlight < 0)
        r_refdef.ambientlight = 0;

    if (!sv.active)
        r_draworder.value = 0; // don't let cheaters look behind walls

    R_CheckVariables();

    R_AnimateLight();

    r_framecount++;

    numbtofpolys = 0;

// debugging
#if 0
r_refdef.vieworg[0]=  80;
r_refdef.vieworg[1]=      64;
r_refdef.vieworg[2]=      40;
r_refdef.viewangles[0]=    0;
r_refdef.viewangles[1]=    46.763641357;
r_refdef.viewangles[2]=    0;
#endif

    // build the transformation matrix for the given view angles
    VectorCopy(r_refdef.vieworg, modelorg);
    VectorCopy(r_refdef.vieworg, r_origin);

    AngleVectors(r_refdef.viewangles, vpn, vright, vup);

    // current viewleaf
    r_oldviewleaf = r_viewleaf;
    r_viewleaf = Mod_PointInLeaf(r_origin, cl.worldmodel);

    r_dowarpold = r_dowarp;
    r_dowarp = r_waterwarp.value && (r_viewleaf->contents <= CONTENTS_WATER);

    if ((r_dowarp != r_dowarpold) || r_viewchanged || lcd_x.value) {
        if (r_dowarp) {
            if ((vid.width <= WARP_WIDTH) &&
                (vid.height <= WARP_HEIGHT)) {
                vrect.x = 0;
                vrect.y = 0;
                vrect.width = vid.width;
                vrect.height = vid.height;

                R_ViewChanged(&vrect, sb_lines, vid.aspect);
            } else {
                w = vid.width;
                h = vid.height;

                if (w > WARP_WIDTH) {
                    h *= (float) WARP_WIDTH / w;
                    w = WARP_WIDTH;
                }

                if (h > WARP_HEIGHT) {
                    h = WARP_HEIGHT;
                    w *= (float) WARP_HEIGHT / h;
                }

                vrect.x = 0;
                vrect.y = 0;
                vrect.width = (int) w;
                vrect.height = (int) h;

                R_ViewChanged(
                    &vrect, (int) ((float) sb_lines * (h / (float) vid.height)),
                    vid.aspect * (h / w) *
                        ((float) vid.width / (float) vid.height));
            }
        } else {
            vrect.x = 0;
            vrect.y = 0;
            vrect.width = vid.width;
            vrect.height = vid.height;

            R_ViewChanged(&vrect, sb_lines, vid.aspect);
        }

        r_viewchanged = false;
    }

    // start off with just the four screen edge clip planes
    R_TransformFrustum();

    // save base values
    VectorCopy(vpn, base_vpn);
    VectorCopy(vright, base_vright);
    VectorCopy(vup, base_vup);
    VectorCopy(modelorg, base_modelorg);

    R_SetSkyFrame();

    R_SetUpFrustumIndexes();

    r_cache_thrash = false;

    // clear frame counts
    c_faceclip = 0;
    d_spanpixcount = 0;
    r_polycount = 0;
    r_drawnpolycount = 0;
    r_wholepolycount = 0;
    r_amodels_drawn = 0;
    r_outofsurfaces = 0;
    r_outofedges = 0;

    D_SetupFrame();
}
