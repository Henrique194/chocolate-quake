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
// sv_edict.c -- entity dictionary


#include "progs.h"
#include "cmd.h"
#include "console.h"
#include "crc.h"
#include "cvar.h"
#include "server.h"
#include "sys.h"
#include "world.h"
#include <string.h>


dprograms_t* progs;
dfunction_t* pr_functions;
char* pr_strings;
static i32 pr_stringssize;
static const char** pr_knownstrings;
static i32 pr_maxknownstrings;
static i32 pr_numknownstrings;
ddef_t* pr_fielddefs;
ddef_t* pr_globaldefs;
dstatement_t* pr_statements;
globalvars_t* pr_global_struct;
float* pr_globals; // same as pr_global_struct
i32 pr_edict_size; // in bytes

u16 pr_crc;

i32 type_size[8] = {
    1,
    sizeof(string_t) / 4,
    1,
    3,
    1,
    1,
    sizeof(func_t) / 4,
    sizeof(void*) / 4,
};

ddef_t* ED_FieldAtOfs(i32 ofs);
qboolean ED_ParseEpair(void* base, ddef_t* key, char* s);

cvar_t nomonsters = {"nomonsters", "0"};
cvar_t gamecfg = {"gamecfg", "0"};
cvar_t scratch1 = {"scratch1", "0"};
cvar_t scratch2 = {"scratch2", "0"};
cvar_t scratch3 = {"scratch3", "0"};
cvar_t scratch4 = {"scratch4", "0"};
cvar_t savedgamecfg = {"savedgamecfg", "0", true};
cvar_t saved1 = {"saved1", "0", true};
cvar_t saved2 = {"saved2", "0", true};
cvar_t saved3 = {"saved3", "0", true};
cvar_t saved4 = {"saved4", "0", true};

#define MAX_FIELD_LEN  64
#define GEFV_CACHESIZE 2

typedef struct {
    ddef_t* pcache;
    char field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache gefvCache[GEFV_CACHESIZE] = {{NULL, ""}, {NULL, ""}};

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict(edict_t* e) {
    Q_memset(&e->v, 0, progs->entityfields * 4);
    e->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t* ED_Alloc(void) {
    i32 i;
    edict_t* e;

    for (i = svs.maxclients + 1; i < sv.num_edicts; i++) {
        e = EDICT_NUM(i);
        // the first couple seconds of server time can involve a lot of
        // freeing and allocating, so relax the replacement policy
        if (e->free && (e->freetime < 2 || sv.time - e->freetime > 0.5)) {
            ED_ClearEdict(e);
            return e;
        }
    }

    if (i == MAX_EDICTS)
        Sys_Error("ED_Alloc: no free edicts");

    sv.num_edicts++;
    e = EDICT_NUM(i);
    ED_ClearEdict(e);

    return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free(edict_t* ed) {
    SV_UnlinkEdict(ed); // unlink from world bsp

    ed->free = true;
    ed->v.model = 0;
    ed->v.takedamage = 0;
    ed->v.modelindex = 0;
    ed->v.colormap = 0;
    ed->v.skin = 0;
    ed->v.frame = 0;
    VectorCopy(vec3_origin, ed->v.origin);
    VectorCopy(vec3_origin, ed->v.angles);
    ed->v.nextthink = -1;
    ed->v.solid = 0;

    ed->freetime = sv.time;
}

//===========================================================================

/*
============
ED_GlobalAtOfs
============
*/
ddef_t* ED_GlobalAtOfs(i32 ofs) {
    ddef_t* def;
    i32 i;

    for (i = 0; i < progs->numglobaldefs; i++) {
        def = &pr_globaldefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
ddef_t* ED_FieldAtOfs(i32 ofs) {
    ddef_t* def;
    i32 i;

    for (i = 0; i < progs->numfielddefs; i++) {
        def = &pr_fielddefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}

/*
============
ED_FindField
============
*/
ddef_t* ED_FindField(char* name) {
    ddef_t* def;
    i32 i;

    for (i = 0; i < progs->numfielddefs; i++) {
        def = &pr_fielddefs[i];
        if (!Q_strcmp(PR_GetString(def->s_name), name))
            return def;
    }
    return NULL;
}


/*
============
ED_FindGlobal
============
*/
ddef_t* ED_FindGlobal(char* name) {
    ddef_t* def;
    i32 i;

    for (i = 0; i < progs->numglobaldefs; i++) {
        def = &pr_globaldefs[i];
        if (!Q_strcmp(PR_GetString(def->s_name), name))
            return def;
    }
    return NULL;
}


/*
============
ED_FindFunction
============
*/
dfunction_t* ED_FindFunction(char* name) {
    dfunction_t* func;
    i32 i;

    for (i = 0; i < progs->numfunctions; i++) {
        func = &pr_functions[i];
        if (!Q_strcmp(PR_GetString(func->s_name), name))
            return func;
    }
    return NULL;
}


eval_t* GetEdictFieldValue(edict_t* ed, char* field) {
    ddef_t* def = NULL;
    i32 i;
    static i32 rep = 0;

    for (i = 0; i < GEFV_CACHESIZE; i++) {
        if (!Q_strcmp(field, gefvCache[i].field)) {
            def = gefvCache[i].pcache;
            goto Done;
        }
    }

    def = ED_FindField(field);

    if (Q_strlen(field) < MAX_FIELD_LEN) {
        gefvCache[rep].pcache = def;
        Q_strcpy(gefvCache[rep].field, field);
        rep ^= 1;
    }

Done:
    if (!def)
        return NULL;

    return (eval_t*) ((char*) &ed->v + def->ofs * 4);
}


/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char* PR_ValueString(etype_t type, eval_t* val) {
    static char line[256];
    ddef_t* def;
    dfunction_t* f;

    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
        case ev_string:
            sprintf(line, "%s", PR_GetString(val->string));
            break;
        case ev_entity:
            sprintf(line, "entity %i",
                    NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
            break;
        case ev_function:
            f = pr_functions + val->function;
            sprintf(line, "%s()", PR_GetString(f->s_name));
            break;
        case ev_field:
            def = ED_FieldAtOfs(val->_int);
            sprintf(line, ".%s", PR_GetString(def->s_name));
            break;
        case ev_void:
            sprintf(line, "void");
            break;
        case ev_float:
            sprintf(line, "%5.1f", val->_float);
            break;
        case ev_vector:
            sprintf(line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1],
                    val->vector[2]);
            break;
        case ev_pointer:
            sprintf(line, "pointer");
            break;
        default:
            sprintf(line, "bad type %i", type);
            break;
    }

    return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char* PR_UglyValueString(etype_t type, eval_t* val) {
    static char line[256];
    ddef_t* def;
    dfunction_t* f;

    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
        case ev_string:
            sprintf(line, "%s", PR_GetString(val->string));
            break;
        case ev_entity:
            sprintf(line, "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
            break;
        case ev_function:
            f = pr_functions + val->function;
            sprintf(line, "%s", PR_GetString(f->s_name));
            break;
        case ev_field:
            def = ED_FieldAtOfs(val->_int);
            sprintf(line, "%s", PR_GetString(def->s_name));
            break;
        case ev_void:
            sprintf(line, "void");
            break;
        case ev_float:
            sprintf(line, "%f", val->_float);
            break;
        case ev_vector:
            sprintf(line, "%f %f %f", val->vector[0], val->vector[1],
                    val->vector[2]);
            break;
        default:
            sprintf(line, "bad type %i", type);
            break;
    }

    return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char* PR_GlobalString(i32 ofs) {
    char* s;
    i32 i;
    ddef_t* def;
    void* val;
    static char line[128];

    val = (void*) &pr_globals[ofs];
    def = ED_GlobalAtOfs(ofs);
    if (!def)
        sprintf(line, "%i(\?\?\?)", ofs);
    else {
        s = PR_ValueString(def->type, val);
        sprintf(line, "%i(%s)%s", ofs, PR_GetString(def->s_name), s);
    }

    i = Q_strlen(line);
    for (; i < 20; i++)
        Q_strcat(line, " ");
    Q_strcat(line, " ");

    return line;
}

char* PR_GlobalStringNoContents(i32 ofs) {
    i32 i;
    ddef_t* def;
    static char line[128];

    def = ED_GlobalAtOfs(ofs);
    if (!def)
        sprintf(line, "%i(\?\?\?)", ofs);
    else
        sprintf(line, "%i(%s)", ofs, PR_GetString(def->s_name));

    i = Q_strlen(line);
    for (; i < 20; i++)
        Q_strcat(line, " ");
    Q_strcat(line, " ");

    return line;
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print(edict_t* ed) {
    i32 l;
    ddef_t* d;
    i32* v;
    i32 i, j;
    char* name;
    i32 type;

    if (ed->free) {
        Con_Printf("FREE\n");
        return;
    }

    Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
    for (i = 1; i < progs->numfielddefs; i++) {
        d = &pr_fielddefs[i];
        name = PR_GetString(d->s_name);
        if (name[Q_strlen(name) - 2] == '_')
            continue; // skip _x, _y, _z vars

        v = (i32*) ((char*) &ed->v + d->ofs * 4);

        // if the value is still all 0, skip the field
        type = d->type & ~DEF_SAVEGLOBAL;

        for (j = 0; j < type_size[type]; j++)
            if (v[j])
                break;
        if (j == type_size[type])
            continue;

        Con_Printf("%s", name);
        l = Q_strlen(name);
        while (l++ < 15)
            Con_Printf(" ");

        Con_Printf("%s\n", PR_ValueString(d->type, (eval_t*) v));
    }
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write(FILE* f, edict_t* ed) {
    ddef_t* d;
    i32* v;
    i32 i, j;
    char* name;
    i32 type;

    fprintf(f, "{\n");

    if (ed->free) {
        fprintf(f, "}\n");
        return;
    }

    for (i = 1; i < progs->numfielddefs; i++) {
        d = &pr_fielddefs[i];
        name = PR_GetString(d->s_name);
        if (name[Q_strlen(name) - 2] == '_')
            continue; // skip _x, _y, _z vars

        v = (i32*) ((char*) &ed->v + d->ofs * 4);

        // if the value is still all 0, skip the field
        type = d->type & ~DEF_SAVEGLOBAL;
        for (j = 0; j < type_size[type]; j++)
            if (v[j])
                break;
        if (j == type_size[type])
            continue;

        fprintf(f, "\"%s\" ", name);
        fprintf(f, "\"%s\"\n", PR_UglyValueString(d->type, (eval_t*) v));
    }

    fprintf(f, "}\n");
}

void ED_PrintNum(i32 ent) {
    ED_Print(EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts(void) {
    i32 i;

    Con_Printf("%i entities\n", sv.num_edicts);
    for (i = 0; i < sv.num_edicts; i++)
        ED_PrintNum(i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f(void) {
    i32 i;

    i = Q_atoi(Cmd_Argv(1));
    if (i >= sv.num_edicts) {
        Con_Printf("Bad edict number\n");
        return;
    }
    ED_PrintNum(i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count(void) {
    i32 i;
    edict_t* ent;
    i32 active, models, solid, step;

    active = models = solid = step = 0;
    for (i = 0; i < sv.num_edicts; i++) {
        ent = EDICT_NUM(i);
        if (ent->free)
            continue;
        active++;
        if (ent->v.solid)
            solid++;
        if (ent->v.model)
            models++;
        if (ent->v.movetype == MOVETYPE_STEP)
            step++;
    }

    Con_Printf("num_edicts:%3i\n", sv.num_edicts);
    Con_Printf("active    :%3i\n", active);
    Con_Printf("view      :%3i\n", models);
    Con_Printf("touch     :%3i\n", solid);
    Con_Printf("step      :%3i\n", step);
}

/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals(FILE* f) {
    ddef_t* def;
    i32 i;
    char* name;
    i32 type;

    fprintf(f, "{\n");
    for (i = 0; i < progs->numglobaldefs; i++) {
        def = &pr_globaldefs[i];
        type = def->type;
        if (!(def->type & DEF_SAVEGLOBAL))
            continue;
        type &= ~DEF_SAVEGLOBAL;

        if (type != ev_string && type != ev_float && type != ev_entity)
            continue;

        name = PR_GetString(def->s_name);
        fprintf(f, "\"%s\" ", name);
        fprintf(f, "\"%s\"\n",
                PR_UglyValueString(type, (eval_t*) &pr_globals[def->ofs]));
    }
    fprintf(f, "}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals(char* data) {
    char keyname[64];
    ddef_t* key;

    while (1) {
        // parse key
        data = COM_Parse(data);
        if (com_token[0] == '}')
            break;
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        Q_strcpy(keyname, com_token);

        // parse value
        data = COM_Parse(data);
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        if (com_token[0] == '}')
            Sys_Error("ED_ParseEntity: closing brace without data");

        key = ED_FindGlobal(keyname);
        if (!key) {
            Con_Printf("'%s' is not a global\n", keyname);
            continue;
        }

        if (!ED_ParseEpair((void*) pr_globals, key, com_token))
            Host_Error("ED_ParseGlobals: parse error");
    }
}

//============================================================================


/*
=============
ED_NewString
=============
*/
string_t ED_NewString(char* string) {
    char* new_p;
    i32 i, l;

    l = Q_strlen(string) + 1;
    string_t num = PR_NewString(l, &new_p);

    for (i = 0; i < l; i++) {
        if (string[i] == '\\' && i < l - 1) {
            i++;
            if (string[i] == 'n')
                *new_p++ = '\n';
            else
                *new_p++ = '\\';
        } else
            *new_p++ = string[i];
    }

    return num;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
qboolean ED_ParseEpair(void* base, ddef_t* key, char* s) {
    i32 i;
    char string[128];
    ddef_t* def;
    char *v, *w;
    void* d;
    dfunction_t* func;

    d = (void*) ((i32*) base + key->ofs);

    switch (key->type & ~DEF_SAVEGLOBAL) {
        case ev_string:
            *(string_t*) d = ED_NewString(s);
            break;

        case ev_float:
            *(float*) d = atof(s);
            break;

        case ev_vector:
            Q_strcpy(string, s);
            v = string;
            w = string;
            for (i = 0; i < 3; i++) {
                while (*v && *v != ' ')
                    v++;
                *v = 0;
                ((float*) d)[i] = atof(w);
                w = v = v + 1;
            }
            break;

        case ev_entity:
            *(i32*) d = EDICT_TO_PROG(EDICT_NUM(atoi(s)));
            break;

        case ev_field:
            def = ED_FindField(s);
            if (!def) {
                Con_Printf("Can't find field %s\n", s);
                return false;
            }
            *(i32*) d = G_INT(def->ofs);
            break;

        case ev_function:
            func = ED_FindFunction(s);
            if (!func) {
                Con_Printf("Can't find function %s\n", s);
                return false;
            }
            *(func_t*) d = func - pr_functions;
            break;

        default:
            break;
    }
    return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
char* ED_ParseEdict(char* data, edict_t* ent) {
    ddef_t* key;
    qboolean anglehack;
    qboolean init;
    char keyname[256];
    i32 n;

    init = false;

    // clear it
    if (ent != sv.edicts) // hack
        Q_memset(&ent->v, 0, progs->entityfields * 4);

    // go through all the dictionary pairs
    while (1) {
        // parse key
        data = COM_Parse(data);
        if (com_token[0] == '}')
            break;
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        // anglehack is to allow QuakeEd to write single scalar angles
        // and allow them to be turned into vectors. (FIXME...)
        if (!Q_strcmp(com_token, "angle")) {
            Q_strcpy(com_token, "angles");
            anglehack = true;
        } else
            anglehack = false;

        // FIXME: change light to _light to get rid of this hack
        if (!Q_strcmp(com_token, "light"))
            Q_strcpy(com_token, "light_lev"); // hack for single light def

        Q_strcpy(keyname, com_token);

        // another hack to fix heynames with trailing spaces
        n = Q_strlen(keyname);
        while (n && keyname[n - 1] == ' ') {
            keyname[n - 1] = 0;
            n--;
        }

        // parse value
        data = COM_Parse(data);
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        if (com_token[0] == '}')
            Sys_Error("ED_ParseEntity: closing brace without data");

        init = true;

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (keyname[0] == '_')
            continue;

        key = ED_FindField(keyname);
        if (!key) {
            Con_Printf("'%s' is not a field\n", keyname);
            continue;
        }

        if (anglehack) {
            char temp[32];
            Q_strcpy(temp, com_token);
            sprintf(com_token, "0 %s 0", temp);
        }

        if (!ED_ParseEpair((void*) &ent->v, key, com_token))
            Host_Error("ED_ParseEdict: parse error");
    }

    if (!init)
        ent->free = true;

    return data;
}


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile(char* data) {
    edict_t* ent;
    i32 inhibit;
    dfunction_t* func;

    ent = NULL;
    inhibit = 0;
    pr_global_struct->time = sv.time;

    // parse ents
    while (1) {
        // parse the opening brace
        data = COM_Parse(data);
        if (!data)
            break;
        if (com_token[0] != '{')
            Sys_Error("ED_LoadFromFile: found %s when expecting {", com_token);

        if (!ent)
            ent = EDICT_NUM(0);
        else
            ent = ED_Alloc();
        data = ED_ParseEdict(data, ent);

        // remove things from different skill levels or deathmatch
        if (deathmatch.value) {
            if (((i32) ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH)) {
                ED_Free(ent);
                inhibit++;
                continue;
            }
        } else if ((current_skill == 0 &&
                    ((i32) ent->v.spawnflags & SPAWNFLAG_NOT_EASY)) ||
                   (current_skill == 1 &&
                    ((i32) ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
                   (current_skill >= 2 &&
                    ((i32) ent->v.spawnflags & SPAWNFLAG_NOT_HARD))) {
            ED_Free(ent);
            inhibit++;
            continue;
        }

        //
        // immediately call spawn function
        //
        if (!ent->v.classname) {
            Con_Printf("No classname for:\n");
            ED_Print(ent);
            ED_Free(ent);
            continue;
        }

        // look for the spawn function
        func = ED_FindFunction(PR_GetString(ent->v.classname));

        if (!func) {
            Con_Printf("No spawn function for:\n");
            ED_Print(ent);
            ED_Free(ent);
            continue;
        }

        pr_global_struct->self = EDICT_TO_PROG(ent);
        PR_ExecuteProgram(func - pr_functions);
    }

    Con_DPrintf("%i entities inhibited\n", inhibit);
}


/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs(void) {
    i32 i;

    // flush the non-C variable lookup cache
    for (i = 0; i < GEFV_CACHESIZE; i++)
        gefvCache[i].field[0] = 0;

    CRC_Init(&pr_crc);

    progs = (dprograms_t*) COM_LoadHunkFile("progs.dat");
    if (!progs)
        Sys_Error("PR_LoadProgs: couldn't load progs.dat");
    Con_DPrintf("Programs occupy %iK.\n", com_filesize / 1024);

    for (i = 0; i < com_filesize; i++)
        CRC_ProcessByte(&pr_crc, ((byte*) progs)[i]);

    // byte swap the header
    for (i = 0; i < sizeof(*progs) / 4; i++)
        ((i32*) progs)[i] = LittleLong(((i32*) progs)[i]);

    if (progs->version != PROG_VERSION)
        Sys_Error("progs.dat has wrong version number (%i should be %i)",
                  progs->version, PROG_VERSION);
    if (progs->crc != PROGHEADER_CRC)
        Sys_Error("progs.dat system vars have been modified, progdefs.h is out "
                  "of date");

    pr_functions = (dfunction_t*) ((byte*) progs + progs->ofs_functions);

    pr_strings = (char*) progs + progs->ofs_strings;
    pr_stringssize = progs->numstrings;
    pr_numknownstrings = 0;
    pr_maxknownstrings = 0;
    if (pr_knownstrings) {
        Z_Free((void*) pr_knownstrings);
    }
    pr_knownstrings = NULL;
    PR_SetString("");

    pr_globaldefs = (ddef_t*) ((byte*) progs + progs->ofs_globaldefs);
    pr_fielddefs = (ddef_t*) ((byte*) progs + progs->ofs_fielddefs);
    pr_statements = (dstatement_t*) ((byte*) progs + progs->ofs_statements);

    pr_global_struct = (globalvars_t*) ((byte*) progs + progs->ofs_globals);
    pr_globals = (float*) pr_global_struct;

    pr_edict_size =
        progs->entityfields * 4 + sizeof(edict_t) - sizeof(entvars_t);

    // byte swap the lumps
    for (i = 0; i < progs->numstatements; i++) {
        pr_statements[i].op = LittleShort(pr_statements[i].op);
        pr_statements[i].a = LittleShort(pr_statements[i].a);
        pr_statements[i].b = LittleShort(pr_statements[i].b);
        pr_statements[i].c = LittleShort(pr_statements[i].c);
    }

    for (i = 0; i < progs->numfunctions; i++) {
        pr_functions[i].first_statement =
            LittleLong(pr_functions[i].first_statement);
        pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
        pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
        pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
        pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
        pr_functions[i].locals = LittleLong(pr_functions[i].locals);
    }

    for (i = 0; i < progs->numglobaldefs; i++) {
        pr_globaldefs[i].type = LittleShort(pr_globaldefs[i].type);
        pr_globaldefs[i].ofs = LittleShort(pr_globaldefs[i].ofs);
        pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
    }

    for (i = 0; i < progs->numfielddefs; i++) {
        pr_fielddefs[i].type = LittleShort(pr_fielddefs[i].type);
        if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
            Sys_Error("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
        pr_fielddefs[i].ofs = LittleShort(pr_fielddefs[i].ofs);
        pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
    }

    for (i = 0; i < progs->numglobals; i++)
        ((i32*) pr_globals)[i] = LittleLong(((i32*) pr_globals)[i]);
}


/*
===============
PR_Init
===============
*/
void PR_Init(void) {
    Cmd_AddCommand("edict", ED_PrintEdict_f);
    Cmd_AddCommand("edicts", ED_PrintEdicts);
    Cmd_AddCommand("edictcount", ED_Count);
    Cmd_AddCommand("profile", PR_Profile_f);
    Cvar_RegisterVariable(&nomonsters);
    Cvar_RegisterVariable(&gamecfg);
    Cvar_RegisterVariable(&scratch1);
    Cvar_RegisterVariable(&scratch2);
    Cvar_RegisterVariable(&scratch3);
    Cvar_RegisterVariable(&scratch4);
    Cvar_RegisterVariable(&savedgamecfg);
    Cvar_RegisterVariable(&saved1);
    Cvar_RegisterVariable(&saved2);
    Cvar_RegisterVariable(&saved3);
    Cvar_RegisterVariable(&saved4);
}


edict_t* EDICT_NUM(i32 n) {
    if (n < 0 || n >= sv.max_edicts)
        Sys_Error("EDICT_NUM: bad number %i", n);
    return (edict_t*) ((byte*) sv.edicts + (n) *pr_edict_size);
}

i32 NUM_FOR_EDICT(edict_t* e) {
    i32 b;

    b = (byte*) e - (byte*) sv.edicts;
    b = b / pr_edict_size;

    if (b < 0 || b >= sv.num_edicts)
        Sys_Error("NUM_FOR_EDICT: bad pointer");
    return b;
}

static void PR_ExpandStringSlots() {
    pr_maxknownstrings += 256;
    void* prt = (void*) pr_knownstrings;
    i32 size = (i32) (pr_maxknownstrings * sizeof(char*));
    pr_knownstrings = (const char**) Z_Realloc(prt, size);
}

static string_t PR_FindString(const char* str) {
    string_t i = 0;
    for (; i < pr_numknownstrings; i++) {
        if (pr_knownstrings[i] == str) {
            break;
        }
    }
    return i;
}

static void PR_SetStringAt(i32 idx, const char* str) {
    if (idx >= pr_maxknownstrings) {
        PR_ExpandStringSlots();
    }
    pr_knownstrings[idx] = str;
    pr_numknownstrings++;
}

string_t PR_SetString(const char* str) {
    if (!str) {
        return 0;
    }
    if (str >= pr_strings && str <= &pr_strings[pr_stringssize - 2]) {
        return str - pr_strings;
    }
    string_t i = PR_FindString(str);
    if (i >= pr_numknownstrings) {
        PR_SetStringAt(i, str);
    }
    return -1 - i;
}

const char* PR_GetString(string_t num) {
    if (num >= 0 && num < pr_stringssize) {
        return &pr_strings[num];
    }
    if (num >= 0 || num < -pr_numknownstrings) {
        Host_Error("PR_GetString: invalid string offset %d\n", num);
        return "";
    }
    if (pr_knownstrings[-1 - num]) {
        return pr_knownstrings[-1 - num];
    }
    Host_Error("PR_GetString: attempt to get a non-existant string %d\n", num);
    return "";
}

string_t PR_NewString(i32 size, char** ptr) {
    if (!size) {
        return 0;
    }
    // Find empty slot.
    string_t i = PR_FindString(NULL);
    char* str = (char*) Hunk_AllocName(size, "string");
    PR_SetStringAt(i, str);
    if (ptr) {
        *ptr = str;
    }
    return -1 - i;
}
