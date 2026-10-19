#ifndef CL_RECORDS_H
#define CL_RECORDS_H

#define ES_NUMBER               0x00
#define ES_EFLAGS               0x08
#define ES_ORIGIN               0x18
#define ES_STANCE               0xE0
#define ES_SIZE                 0xF0

/* clientState_t -- 92 bytes. Stride of cl.parseClients at 0x01531134, and the 22 netFields at 0x00541FC0 describe it. ------------------------------------------------------------------------ */
#define CS_NUMBER               0x00
#define CS_SIZE                 0x5C

#define SNAP_VALID              0x0000
#define SNAP_SNAPFLAGS          0x0004
#define SNAP_SERVERTIME         0x0008
#define SNAP_MESSAGENUM         0x000C
#define SNAP_DELTANUM           0x0010
#define SNAP_PING               0x0014
#define SNAP_PS                 0x001C
#define SNAP_NUMENTITIES        0x20EC
#define SNAP_NUMCLIENTS         0x20F0
#define SNAP_PARSEENTITIESNUM   0x20F4
#define SNAP_PARSECLIENTSNUM    0x20F8
#define SNAP_SERVERCOMMANDNUM   0x20FC
#define SNAP_SIZE               0x2100

#define SNAP_OUT_SNAPFLAGS      0x00000
#define SNAP_OUT_PING           0x00004
#define SNAP_OUT_SERVERTIME     0x00008
#define SNAP_OUT_PS             0x0000C
#define SNAP_OUT_NUMENTITIES    0x020DC
#define SNAP_OUT_NUMCLIENTS     0x020E0
#define SNAP_OUT_ENTITIES       0x020E4
#define SNAP_OUT_CLIENTS        0x110E4
#define SNAP_OUT_CMDSEQUENCE    0x127E8

#define MAX_ENTITIES_IN_SNAPSHOT  0x100
#define MAX_CLIENTS_IN_SNAPSHOT   0x40

#define UCMD_SIZE               24
#define CMD_BACKUP              64
#define CMD_MASK                0x3F

#define GS_STRINGOFFSETS        0x0000
#define GS_STRINGDATA           0x2000
#define GS_DATACOUNT            0x5E80
#define GS_SIZE                 0x5E84
#define CS_SYSTEMINFO           1
#define MAX_CONFIGSTRINGS       0x800
#define MAX_GAMESTATE_CHARS     0x3E80

#define cl_gameState_stringData \
	( (char *)cl_gameState_stringOffsets + GS_STRINGDATA )
#define cl_gameState_dataCount \
	( *(int *)( (char *)cl_gameState_stringOffsets + GS_DATACOUNT ) )


#define cin_file_p1             ( cin_file + 1 )
#define cin_file_p6             ( cin_file + 6 )
#define cin_file_p15             ( cin_file + 15 )
#define cin_mcomp_p16            ( cin_mcomp + 16 )

extern field_t                  g_consoleField;
extern field_t                  chatField;
extern field_t                  historyEditLines[32];

#define text_in                 ( g_consoleField.buffer )



#define cls_servernameTerminator            ( cls_servername[255] )
#define clc_downloadList_p1             ( (unsigned char *)clc_downloadList + 1 )

#define cls_downloadNameTerminator            ( cls_downloadName[255] )
#define clc_serverMessageTerminator            ( &clc_serverMessage[255] )

#endif
