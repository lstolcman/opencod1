/*
 * xanim/xanim_local.h
 *
 * State shared by the xanim units (dobj.cpp, xanim.cpp, xmodel.cpp); opencod2 keeps the
 * same subsystem-private header (src/PC/xanim/xanim_local.h).  Retail addresses quoted.
 */

#ifndef __XANIM_LOCAL_H__
#define __XANIM_LOCAL_H__

/* dobj.cpp: counters of the XAnimInfo pool (xanim_pool, 0x00A7A620) */
extern int xanim_poolUsedCount;   /* 0x00A7A610 */
extern int xanim_poolHighWaterCount;   /* 0x00A9CC68 */

/* xanim.cpp */
extern int xanim_endNotifyStringHandle;   /* 0x01407338, the "end" notify string handle */
extern int xanim_evalPoolWeightSelector;   /* 0x00A9C620 */
extern unsigned char xanim_evalLeafOutputMode;   /* 0x00A9C63C, 1 byte flag */
extern int xanim_currentTree;   /* 0x00A7A618 */
extern int xanim_numDeferredNotifies;   /* 0x00A9C638 */
extern int xanim_currentTime;   /* 0x00A9CC5C */

/* script/scr_memorytree.cpp (scr_local.h): the script string arena base */
extern int GetRefString_var;   /* 0x008E6208 */

#endif /* __XANIM_LOCAL_H__ */
