/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"

extern int R_AddDrawSurf();
extern int R_AddWorldSurfaceNoCull();
extern int R_DlightBmodel();
extern void R_RotateForModelEntity(trRefEntity_t *ent, viewParms_t *viewParms, orientationr_t *entOr);
extern int R_CullLocalBox();
extern int RE_AddRefEntityToScene();

/* ---- R_CullSurface  0x0050AF30 ---- */
static int R_CullSurface(srfLit_t *data)
{
  if ( !r_nocull->integer && data->surfaceType >= SF_INDEXED_FIRST )
    return R_CullLocalBox((float *)&data->boundsMin) == CULL_OUT;
  return 0;
}

/* ---- R_DlightTris  0x0050AF60 ----  VERIFIED
 * Return the subset of dlightBits whose lights actually overlap the surface's
 * world-space bounding box (VC7 unrolls the loop four lights at a time).
 * Bounds-first comparisons give retail's fadd operand order; the <=/>= reject
 * tests keep retail's NaN behaviour (unordered counts as overlapping). */
int __cdecl R_DlightTris(srfLit_t *surf, int dlightBits)
{
  dlight_t *dl;
  int       survivingBits;
  int       i;

  dl = (dlight_t *)tr_refdef_dlights;
  survivingBits = 0;

  for ( i = 0; i < tr_refdef_num_dlights; i++, dl++ )
  {
    if ( !(dlightBits & (1 << i)) )
      continue;
    if ( surf->boundsMin[0] >= dl->transformedPosition[0] + dl->radius
      || surf->boundsMax[0] <= dl->transformedPosition[0] - dl->radius
      || surf->boundsMin[1] >= dl->transformedPosition[1] + dl->radius
      || surf->boundsMax[1] <= dl->transformedPosition[1] - dl->radius
      || surf->boundsMin[2] >= dl->transformedPosition[2] + dl->radius
      || surf->boundsMax[2] <= dl->transformedPosition[2] - dl->radius )
      continue;
    survivingBits |= 1 << i;
  }

  return survivingBits;
}

/* ---- R_DlightSurface  0x0050B240 ----  VERIFIED
 * Q3 else-zero shape: inlined into R_AddWorldSurface it puts retail's
 * dlightBits = 0 block out of line. */
int __cdecl R_DlightSurface(msurface_t *surf, int dlightBits)
{
  srfLit_t *data;

  data = (srfLit_t *)surf->data;
  if ( data->surfaceType >= SF_INDEXED_FIRST )
    data->dlightBits = R_DlightTris(data, dlightBits);
  else
    dlightBits = 0;

  if ( dlightBits )
    ++*(int *)&tr_pc_dlightSurfaceAndCullCount;                 /* tr.pc.dlightSurfaces */

  return dlightBits;
}

/* ---- R_AddWorldSurface  0x0050B270 ----  VERIFIED
 * Q3 shape: R_CullSurface / R_DlightSurface inlined, then surf->data re-read
 * for R_AddDrawSurf (retail reloads it after the dlight stores). */
int __cdecl R_AddWorldSurface(int dlightBits, msurface_t *surf)
{
  srfLit_t *data;
  int       result;

  result = tr_viewCount;
  if ( surf->viewCount == tr_viewCount )
    return result;
  surf->viewCount = tr_viewCount;

  if ( (result = R_CullSurface((srfLit_t *)surf->data)) != 0 )
    return result;

  if ( dlightBits )
  {
    dlightBits = R_DlightSurface(surf, dlightBits);
    dlightBits = dlightBits != 0;
  }

  data = (srfLit_t *)surf->data;
  if ( data->surfaceType >= SF_INDEXED_FIRST )
    return R_AddDrawSurf(data->storageMode, surf->shader, data, dlightBits, 0, 0);
  else
    return R_AddDrawSurf(storageClass, surf->shader, data, dlightBits, 0, 0);
}

/* ---- R_AddBrushModelSurfaces  0x0050B310 ----  VERIFIED
 * void as in retail (whose EAX on exit is just the last value loaded); the
 * tr_main.c caller declares it int but ignores the result. */
void __cdecl R_AddBrushModelSurfaces(int entArg)
{
  trRefEntity_t *ent = (trRefEntity_t *)entArg;
  bmodel_t      *bmodel;
  int            i;

  if ( ent->cullState == CULL_OUT )
    return;
  if ( !r_drawBModels->integer )
    return;

  R_RotateForModelEntity(ent, (viewParms_t *)&tr_viewParms_originX, (orientationr_t *)&tr_or);
  bmodel = ((model_t *)tr_models[ent->e.hModel])->bmodel;
  R_DlightBmodel(bmodel);

  for ( i = 0; i < bmodel->numSurfaces; i++ )
    R_AddWorldSurfaceNoCull(((trRefEntity_t *)tr_currentEntity)->dlightBits,
                            &bmodel->firstSurface[i]);
}

/* ---- R_PointInLeaf  0x0050B390 ----  VERIFIED */
mnode_t *__cdecl R_PointInLeaf(const float *p)
{
  mnode_t *node;

  if ( !tr_world )
    ri.Error(1, "\x15" "R_PointInLeaf: bad model");

  node = *(mnode_t **)(tr_world + 152);        /* tr.world->nodes */
  while ( node->contents == NODE_INTERNAL )
  {
    const cplane_t *plane = node->u.node.plane;
    /* retail fcomp 0.0 / test ah,41h / jne children[1] (0x50B3DA-0x50B3EC):
     * only an ordered d > 0 goes front, so a NaN distance takes children[1] */
    if ( plane->normal[2] * p[2] + plane->normal[1] * p[1] + plane->normal[0] * p[0] - plane->dist > 0.0f )
      node = node->u.node.children[0];
    else
      node = node->u.node.children[1];
  }
  return node;
}

/* ---- R_AddWorldSurfaces  0x0050B400 ----
 * Q3's world entry point, kept (extern) although nothing calls it any more --
 * R_AddWorldSurfacesDPVS replaced it.  No dlights: every world surface is
 * added as R_AddWorldSurface adds one, then every cell's linked static
 * models once per view (as R_AddStaticModelDPVS does, without culling). */
void R_AddWorldSurfaces( void )
{
  msurface_t *surf;
  srfLit_t   *data;
  int         cell;      /* dpvsCell_t (0x40) */
  int        *link;      /* dpvsCellModelLink_t: +0 model, +4 next */
  int         i;

  if ( !r_drawworld->integer )
    return;
  if ( tr_refdef_rdflags & 1 )                  /* RDF_NOWORLDMODEL */
    return;

  tr_currentEntityNumber = 1022;                /* ENTITYNUM_WORLD */
  tr_shiftedEntityNumber = 1022 << 8;
  tr_refdef_num_dlights = 0;

  for ( i = 0; i < *(int *)(tr_world + 156); i++ )               /* tr.world->numsurfaces */
  {
    surf = &(*(msurface_t **)(tr_world + 160))[i];
    if ( surf->viewCount == tr_viewCount )
      continue;
    surf->viewCount = tr_viewCount;
    data = (srfLit_t *)surf->data;
    if ( R_CullSurface( data ) )
      continue;
    if ( data->surfaceType >= SF_INDEXED_FIRST )
      R_AddDrawSurf( data->storageMode, surf->shader, data, 0, 0, 0 );
    else
      R_AddDrawSurf( storageClass, surf->shader, data, 0, 0, 0 );
  }

  cell = *(int *)(tr_world + 292);                                /* tr.world->cells */
  for ( i = 0; i < *(int *)(tr_world + 288); i++, cell += 64 )   /* tr.world->cellCount */
  {
    for ( link = *(int **)(cell + 56); link; link = (int *)link[1] )  /* dpvsCell_t modelLinks */
    {
      if ( ((int *)link[0])[45] != tr_viewCount )              /* dpvsStaticModel_t viewCount */
      {
        ((int *)link[0])[45] = tr_viewCount;
        RE_AddRefEntityToScene( link[0], link[0] );
      }
    }
  }
}
