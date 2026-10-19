/*
 * @fidelity: verified
 */

#include <string.h>

#include "../qcommon/qcommon.h"
#include "cl_refapi.h"
#include "cl_local.h"



/* cl (clientActive_t, 0x01432960..0x0155F238).  cl_main_mp.c and cl_refapi.c reach its
 * first int (cl.timeoutcount) as `extern int cl_active`, so the whole-block definition
 * lives in a unit that does not also use it as an int.  tools/vc7/exe_layout.py replaces
 * it with the typed `cl` block (client_active_storage_owner, cl_main_mp.obj). */
unsigned char   cl_active[1231064];         /* 0x12C8D8 bytes, 0x01432960 */

/* two renderer blobs (renderer/tr_local.h): backEnd and the light-vis cache.  Their owners
 * (tr_backend.c, tr_light.c) link after l_precomp_mp.obj; defining them there renumbers every
 * symbol in between and flips the PC_EvaluateTokens / Sys_GetVideoRam scheduling ties
 * (tools/vc7/tie_probe.py), so they are defined here. */
unsigned char   backEndStorage[6732];          /* 0x1A4C bytes, 0x016D89C0 */
unsigned char   tr_lightVisCache[2097152];        /* 0x200000 bytes, 0x00CA28D0 */

/* g_color_table (0x00541950) is universal/q_shared.c's, in the .rdata block it
 * shares with the colorBlack run. */

