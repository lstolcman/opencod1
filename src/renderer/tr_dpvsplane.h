#ifndef TR_DPVSPLANE_H
#define TR_DPVSPLANE_H

typedef struct {
	float         normal[3];      /* +0x00 */
	float         distance;       /* +0x0c */
	unsigned char sideOffsets[3]; /* +0x10 */
	unsigned char cameraSide;     /* +0x13  never written by CoD1 */
} dpvsPlane_t;

/* Retail keeps all DPVS frame state in one object (0x011CC100..0x011CC22B).
 * Its address escapes (the frustum planes and near plane are passed by
 * pointer), so VC7 keeps every store to it in source order relative to
 * stores and loads through pointers (R_AddSkySurfacesDPVS, R_AddCellSurfaces). */
typedef struct dpvsGlob_s {
  int         viewOrigin[3];                  /* 0x011CC100  floats, read as *(float *)& */
  int         frustumPlanes[20];              /* 0x011CC10C  4 dpvsPlane_t */
  dpvsPlane_t nearPlane;                      /* 0x011CC15C */
  dpvsPlane_t farPlane;                       /* 0x011CC170 */
  int         activeNearPlane;                /* 0x011CC184  dpvsPlane_t * */
  int         activeFarPlane;                 /* 0x011CC188  dpvsPlane_t * */
  float       cullDist;                       /* 0x011CC18C */
  int         cellEntityLinks;                /* 0x011CC190 */
  int         cellEntityLinkCount;            /* 0x011CC194 */
  int         transformMatrixTransposed[16];  /* 0x011CC198 */
  float       transformMatrixInverse[16];     /* 0x011CC1D8 */
  int         occluderCount;                  /* 0x011CC218 */
  int         occluders;                      /* 0x011CC21C */
  int         activePlaneCount;               /* 0x011CC220 */
  int         activePlanes;                   /* 0x011CC224 */
  int         cullPlaneLimit;                 /* 0x011CC228 */
} dpvsGlob_t;

extern dpvsGlob_t dpvsGlob;

#define dpvs_viewOrigin          dpvsGlob.viewOrigin[0]
#define dpvs_viewOriginY         dpvsGlob.viewOrigin[1]
#define dpvs_viewOriginZ         dpvsGlob.viewOrigin[2]
#define dpvs_frustumPlanes       dpvsGlob.frustumPlanes
#define dpvs_nearPlaneRec        dpvsGlob.nearPlane
#define dpvs_farPlaneRec         dpvsGlob.farPlane
#define dpvs_activeNearPlane     dpvsGlob.activeNearPlane
#define dpvs_activeFarPlane      dpvsGlob.activeFarPlane
#define dpvs_occluderCount       dpvsGlob.occluderCount
#define dpvs_occluders           dpvsGlob.occluders
#define dpvs_activePlaneCount    dpvsGlob.activePlaneCount
#define dpvs_activePlanes        dpvsGlob.activePlanes
#define dpvs_cullPlaneLimit      dpvsGlob.cullPlaneLimit

#undef  dpvs_nearPlane
#define dpvs_nearPlane   (*(int *)&dpvs_nearPlaneRec.normal[0])      /* +0x00 */
#undef  dpvs_nearPlaneNormalY
#define dpvs_nearPlaneNormalY    (*(int *)&dpvs_nearPlaneRec.normal[1])      /* +0x04 */
#undef  dpvs_nearPlaneNormalZ
#define dpvs_nearPlaneNormalZ    (*(int *)&dpvs_nearPlaneRec.normal[2])      /* +0x08 */
#undef  dpvs_nearPlaneDistance
#define dpvs_nearPlaneDistance      dpvs_nearPlaneRec.distance                  /* +0x0c */
#undef  dpvs_nearPlaneSideOffsetX
#define dpvs_nearPlaneSideOffsetX     dpvs_nearPlaneRec.sideOffsets[0]            /* +0x10 */
#undef  dpvs_nearPlaneSideOffsetY
#define dpvs_nearPlaneSideOffsetY     dpvs_nearPlaneRec.sideOffsets[1]            /* +0x11 */
#undef  dpvs_nearPlaneSideOffsetZ
#define dpvs_nearPlaneSideOffsetZ     dpvs_nearPlaneRec.sideOffsets[2]            /* +0x12 */

#undef  dpvs_farPlane
#define dpvs_farPlane    dpvs_farPlaneRec.normal[0]                  /* +0x00 */
#undef  dpvs_farPlaneNormalY
#define dpvs_farPlaneNormalY      dpvs_farPlaneRec.normal[1]                  /* +0x04 */
#undef  dpvs_farPlaneNormalZ
#define dpvs_farPlaneNormalZ      dpvs_farPlaneRec.normal[2]                  /* +0x08 */
#undef  dpvs_farPlaneDistance
#define dpvs_farPlaneDistance      dpvs_farPlaneRec.distance                   /* +0x0c */
#undef  dpvs_farPlaneSideOffsetX
#define dpvs_farPlaneSideOffsetX     dpvs_farPlaneRec.sideOffsets[0]             /* +0x10 */
#undef  dpvs_farPlaneSideOffsetY
#define dpvs_farPlaneSideOffsetY     dpvs_farPlaneRec.sideOffsets[1]             /* +0x11 */
#undef  dpvs_farPlaneSideOffsetZ
#define dpvs_farPlaneSideOffsetZ     dpvs_farPlaneRec.sideOffsets[2]             /* +0x12 */

#endif
