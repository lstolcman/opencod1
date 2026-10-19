/*
 * cm_public.h -- the collision records shared by the collision module and
 * the server for Call of Duty 1.1 (Windows, CoDMP.exe).
 *
 * RTCW's qcommon.h includes its cm_public.h the same way.  Only the sector
 * walker work records and their entry points live here so far; the rest of
 * the collision API is still declared in server_mp/server.h and
 * qcommon/cm_local.h.
 *
 * @fidelity: verified
 */

#ifndef __CM_PUBLIC_H__
#define __CM_PUBLIC_H__

/*
=============================================================================

			SERVER-SIDE WORK RECORDS PASSED THROUGH THE SECTOR TREE

	These are built on a caller's stack in server_mp/sv_world_mp.c and handed
	to the CM_*ToEntities walkers in qcommon/cm_world.c, so both layers see
	them.  (RTCW keeps moveclip_t private to sv_world.c because its walker
	lives there too; CoD moved the walkers into the collision module.)

=============================================================================
*/

/*
 * 124 bytes (0x7C).  Built by SV_Trace and consumed by
 * CM_ClipMoveToEntities_r / SV_ClipMoveToEntity.  Matches opencoduo's
 * cmClipMoveWork_t exactly, whose 0x7c also matches the proven CoD1 size.
 *
 * expandedHalfSize is (maxs - mins) * 0.5 + 1.0 and is what the sector walker
 * reads at +0x18 + axis*4 to decide whether to descend a branch.
 */
typedef struct moveclip_t
{
	vec3_t mins;                    /* +0x00 */
	vec3_t maxs;                    /* +0x0C */
	vec3_t expandedHalfSize;        /* +0x18 */
	vec3_t start;                   /* +0x24 */
	vec3_t end;                     /* +0x30 */
	trace_t trace;                  /* +0x3C */
	int passEntityNum;              /* +0x6C */
	int passOwnerNum;               /* +0x70 */
	int contentmask;                /* +0x74 */
	qboolean capsule;               /* +0x78 */
} moveclip_t;
COD1_ASSERT_SIZE( moveclip_t, 124 );

/*
 * 100 bytes.  DIVERGENCE FROM opencoduo, and the binary wins.
 *
 * opencoduo's cmPointTraceWork_t is 0x5C (92): start, end, trace,
 * passEntityNum, passOwnerNum, contentmask, useDObj, dobjTracePartState.
 * CoD1's proven size is 100 (0x64), eight bytes longer.  The prefix through
 * +0x54 is common to both -- CM_PointTraceToEntities_r reads start +0x00,
 * end +0x0C, trace +0x18, pass entity +0x48, pass owner +0x4C and contents
 * +0x50 -- so the extra two dwords are at the tail, where UO has the DObj
 * pair.  Neither build touches them and nothing names them, so they are
 * reserved_ slots rather than guesses.
 */
typedef struct pointtrace_t
{
	vec3_t start;                   /* +0x00 */
	vec3_t end;                     /* +0x0C */
	trace_t trace;                  /* +0x18 */
	int passEntityNum;              /* +0x48 */
	int passOwnerNum;               /* +0x4C */
	int contentmask;                /* +0x50 */
	qboolean useDObj;               /* +0x54 */
	const byte      *dobjTracePartState; /* +0x58 */
	int reserved_0x5C;              /* +0x5C  not in UO's 0x5C-byte form; never written or read */
	int reserved_0x60;              /* +0x60  never written or read */
} pointtrace_t;
COD1_ASSERT_SIZE( pointtrace_t, 100 );

/*
 * 84 bytes (0x54).
 *
 * SV_SightTrace (0x0045FCA0) builds sightclip_t and sightpointtrace_t in ONE
 * stack buffer across its two mutually exclusive branches, so neither is
 * bounded by its own construction alone.  What settles it is that the shared
 * buffer starts at esp+0x1C in a frame whose return address sits at 0x70:
 * the box branch's last field is written at buffer+0x50, so the record runs
 * to buffer+0x54 and 0x1C + 0x54 = 0x70 exactly fills the frame.
 *
 * 0x54 is also precisely the size of opencoduo's cmClipSightTraceWork_t, and
 * the field-by-field writes at 0x0045FE01..0x0045FF85 reproduce its layout:
 *   +0x00 -halfsize   +0x0C +halfsize   +0x18 halfsize + 1
 *   +0x24 start + centre   +0x30 end + centre
 *   +0x3C/+0x40 the two skip entities, +0x44/+0x48 their owners
 *   +0x4C contentmask, +0x50 the trailing flag
 * SV_SightTrace's last parameter, `locational`, lands in the slot opencoduo
 * calls capsule; kept as capsule, the consumer being
 * CM_ClipSightTraceToEntities_r either way.
 */
typedef struct sightclip_t
{
	vec3_t mins;                    /* +0x00 */
	vec3_t maxs;                    /* +0x0C */
	vec3_t expandedHalfSize;        /* +0x18 */
	vec3_t start;                   /* +0x24 */
	vec3_t end;                     /* +0x30 */
	int passEntityNum;              /* +0x3C */
	int passOwnerNum;               /* +0x40 */
	int passEntityOwnerNum;         /* +0x44 */
	int passOwnerOwnerNum;          /* +0x48 */
	int contentmask;                /* +0x4C */
	qboolean capsule;               /* +0x50 */
} sightclip_t;
COD1_ASSERT_SIZE( sightclip_t, 84 );

/*
 * 44 bytes (0x2C).
 *
 * SV_SightTrace's point branch (0x0045FD2E..0x0045FDDD) writes start +0x00,
 * end +0x0C, the two skip entities +0x18/+0x1C, their owners +0x20/+0x24 and
 * the contents mask +0x28, then calls CM_PointSightTraceToEntities_r.  The
 * highest byte touched is +0x2B.  Identical to opencoduo's
 * cmPointSightTraceWork_t.
 */
typedef struct sightpointtrace_t
{
	vec3_t start;                   /* +0x00 */
	vec3_t end;                     /* +0x0C */
	int passEntityNum;              /* +0x18 */
	int passOwnerNum;               /* +0x1C */
	int passEntityOwnerNum;         /* +0x20 */
	int passOwnerOwnerNum;          /* +0x24 */
	int contentmask;                /* +0x28 */
} sightpointtrace_t;
COD1_ASSERT_SIZE( sightpointtrace_t, 44 );

/* cm_world.c: the sector walkers the server's traces call. */
void CM_ClipMoveToEntities( moveclip_t *clip );
void CM_PointTraceToEntities( pointtrace_t *clip );
qboolean CM_ClipSightTraceToEntities( sightclip_t *clip );
qboolean CM_PointSightTraceToEntities( sightpointtrace_t *clip );

#endif  /* __CM_PUBLIC_H__ */
