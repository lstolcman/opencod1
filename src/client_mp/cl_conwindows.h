#ifndef CL_CONWINDOWS_H
#define CL_CONWINDOWS_H

typedef struct {
	int  *times;         /* +0x00 */
	int  *endTimes;      /* +0x04 */
	int  *lines;         /* +0x08 */
	int   index;         /* +0x0C */
	int   size;          /* +0x10 */
	int   activeLines;   /* +0x14 */
	int   scrollTime;    /* +0x18 */
	int   fadeIn;        /* +0x1C */
	int   fadeOut;       /* +0x20 */
} conMessageWindow_t;

extern conMessageWindow_t con_notifyWindowRec;
extern conMessageWindow_t con_boldWindowRec;
extern conMessageWindow_t con_subtitleWindowRec;
extern conMessageWindow_t con_miniconsoleWindowRec;

/* notify (game message), retail 0x0142EFCC ---- */
#define con_notifyWindow        (*(int *)&con_notifyWindowRec.times)        /* +0x00 */
#define con_notifyWindowEndTimes           (*(int *)&con_notifyWindowRec.endTimes)     /* +0x04 */
#define con_notifyWindowIndex           con_notifyWindowRec.index                   /* +0x0C */
#define con_gamemessagelines    con_notifyWindowRec.size                    /* +0x10 */

/* bold game message, retail 0x0142F050 ---- */
#define con_boldWindow          (*(int *)&con_boldWindowRec.times)          /* +0x00 */
#define con_boldWindowEndTimes           (*(int *)&con_boldWindowRec.endTimes)       /* +0x04 */
#define con_boldWindowIndex           con_boldWindowRec.index                     /* +0x0C */
#define con_boldWindowSize           con_boldWindowRec.size                      /* +0x10 */

/* subtitle, retail 0x0142F0D4 ---- */
#define con_subtitleWindow      (*(int *)&con_subtitleWindowRec.times)      /* +0x00 */
#define con_subtitleWindowEndTimes           (*(int *)&con_subtitleWindowRec.endTimes)   /* +0x04 */
#define con_subtitleWindowIndex           con_subtitleWindowRec.index                 /* +0x0C */
#define con_subtitleWindowSize           con_subtitleWindowRec.size                  /* +0x10 */

/* mini-console, retail 0x0142F5A8 ---- */
#define con_miniconsoleWindow   (*(int *)&con_miniconsoleWindowRec.times)   /* +0x00 */

#endif
