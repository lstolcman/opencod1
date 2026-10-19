/*
 * qcommon/msg_mp.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/qcommon/msg_mp.c
 *
 * Retail range 0x004446E0-0x00448390, 60 functions.
 *
 * @fidelity: verified
 */

#include "qcommon.h"

huffman_t msgHuff;
qboolean msgInit;               /* 0x014073F0 */

static char msg_stringBuf[MAX_STRING_CHARS];
static char msg_bigStringBuf[BIG_INFO_STRING];
static char msg_stringLineBuf[MAX_STRING_CHARS];

extern cvar_t *cl_shownet;

unsigned int kbitmask[33] = {
	0x00000000u, 0x00000001u, 0x00000003u, 0x00000007u,
	0x0000000fu, 0x0000001fu, 0x0000003fu, 0x0000007fu,
	0x000000ffu, 0x000001ffu, 0x000003ffu, 0x000007ffu,
	0x00000fffu, 0x00001fffu, 0x00003fffu, 0x00007fffu,
	0x0000ffffu, 0x0001ffffu, 0x0003ffffu, 0x0007ffffu,
	0x000fffffu, 0x001fffffu, 0x003fffffu, 0x007fffffu,
	0x00ffffffu, 0x01ffffffu, 0x03ffffffu, 0x07ffffffu,
	0x0fffffffu, 0x1fffffffu, 0x3fffffffu, 0x7fffffffu,
	0xffffffffu
};

int msg_hData[256] = {
	250315,  41193,   6292,   7106,   3730,   3750,   6110,  23283,
	 33317,   6950,   7838,   9714,   9257,  17259,   3949,   1778,
	  8288,   1604,   1590,   1663,   1100,   1213,   1238,   1134,
	  1749,   1059,   1246,   1149,   1273,   4486,   2805,   3472,
	 21819,   1159,   1670,   1066,   1043,   1012,   1053,   1070,
	  1726,    888,   1180,    850,    960,    780,   1752,   3296,
	 10630,   4514,   5881,   2685,   4650,   3837,   2093,   1867,
	  2584,   1949,   1972,    940,   1134,   1788,   1670,   1206,
	  5719,   6128,   7222,   6654,   3710,   3795,   1492,   1524,
	  2215,   1140,   1355,    971,   2180,   1248,   1328,   1195,
	  1770,   1078,   1264,   1266,   1168,    965,   1155,   1186,
	  1347,   1228,   1529,   1600,   2617,   2048,   2546,   3275,
	  2410,   3585,   2504,   2800,   2675,   6146,   3663,   2840,
	 14253,   3164,   2221,   1687,   3208,   2739,   3512,   4796,
	  4091,   3515,   5288,   4016,   7937,   6031,   5360,   3924,
	  4892,   3743,   4566,   4807,   5852,   6400,   6225,   8291,
	 23243,   7838,   7073,   8935,   5437,   4483,   3641,   5256,
	  5312,   5328,   5370,   3492,   2458,   1694,   1821,   2121,
	  1916,   1149,   1516,   1367,   1236,   1029,   1258,   1104,
	  1245,   1006,   1149,   1025,   1241,    952,   1287,    997,
	  1713,   1009,   1187,    879,   1099,    929,   1078,    951,
	  1656,    930,   1153,   1030,   1262,   1062,   1214,   1060,
	  1621,    930,   1106,    912,   1034,    892,   1158,    990,
	  1175,    850,   1121,    903,   1087,    920,   1144,   1056,
	  3462,   2240,   4397,  12136,   7758,   1345,   1307,   3278,
	  1950,    886,   1023,   1112,   1077,   1042,   1061,   1071,
	  1484,   1001,   1096,    915,   1052,    995,   1070,    876,
	  1111,    851,   1059,    805,   1112,    923,   1103,    817,
	  1899,   1872,    976,    841,   1127,    956,   1159,    950,
	  7791,    954,   1289,    933,   1127,   3207,   1020,    927,
	  1355,    768,   1040,    745,    952,    805,   1073,    740,
	  1013,    805,   1008,    796,    996,   1057,  11457,  13504
};

#define MSG_ENTITY_FIELDS       59
#define MSG_ENTITY_BASE_FIELDS  67
#define MSG_CLIENT_FIELDS       22
#define MSG_PLAYERSTATE_FIELDS  103
#define MSG_OBJECTIVE_FIELDS     6
#define MSG_HUDELEM_FIELDS      28

#define MSG_ENTITY_NUM_BITS     10
#define MSG_CLIENT_NUM_BITS      6

netField_t stateFields[MSG_ENTITY_FIELDS] = {
	{ "pos.trTime",         16, 32 },
	{ "pos.trBase[0]",      24,  0 },
	{ "pos.trBase[1]",      28,  0 },
	{ "pos.trDelta[1]",     40,  0 },
	{ "pos.trDelta[0]",     36,  0 },
	{ "angles2[1]",        108,  0 },
	{ "apos.trBase[1]",     64,  0 },
	{ "apos.trBase[0]",     60,  0 },
	{ "pos.trBase[2]",      32,  0 },
	{ "pos.trDelta[2]",     44,  0 },
	{ "eventSequence",     164,  8 },
	{ "eType",               4,  8 },
	{ "eFlags",              8, 24 },
	{ "groundEntityNum",   124, 10 },
	{ "legsAnim",          204, 10 },
	{ "clientNum",         144,  8 },
	{ "apos.trBase[2]",     68,  0 },
	{ "events[0]",         168,  8 },
	{ "index",             140,  9 },
	{ "events[1]",         172,  8 },
	{ "events[2]",         176,  8 },
	{ "events[3]",         180,  8 },
	{ "eventParm",         160,  8 },
	{ "torsoAnim",         208, 10 },
	{ "surfType",          136,  8 },
	{ "scale",             216,  8 },
	{ "otherEntityNum",    116, 10 },
	{ "fWaistPitch",       236,  0 },
	{ "pos.trType",         12,  8 },
	{ "angles2[0]",        104,  0 },
	{ "fTorsoHeight",      228,  0 },
	{ "fTorsoPitch",       232,  0 },
	{ "apos.trType",        48,  8 },
	{ "solid",             156, 24 },
	{ "weapon",            200,  6 },
	{ "apos.trTime",        52, 32 },
	{ "apos.trDelta[0]",    72,  0 },
	{ "eventParms[0]",     184,  8 },
	{ "pos.trDuration",     20, 32 },
	{ "animMovetype",      224,  4 },
	{ "eventParms[1]",     188,  8 },
	{ "apos.trDelta[2]",    80,  0 },
	{ "eventParms[2]",     192,  8 },
	{ "eventParms[3]",     196,  8 },
	{ "leanf",             212,  0 },
	{ "apos.trDelta[1]",    76,  0 },
	{ "loopSound",         132,  8 },
	{ "attackerEntityNum", 120, 10 },
	{ "iHeadIcon",         148,  4 },
	{ "iHeadIconTeam",     152,  2 },
	{ "apos.trDuration",    56, 32 },
	{ "time",               84, 32 },
	{ "time2",              88, 32 },
	{ "origin2[0]",         92,  0 },
	{ "origin2[1]",         96,  0 },
	{ "origin2[2]",        100,  0 },
	{ "angles2[2]",        112,  0 },
	{ "constantLight",     128, 32 },
	{ "dmgFlags",          220, 32 }
};

netField_t archivedEntityFields[MSG_ENTITY_BASE_FIELDS] = {
	{ "absmin[1]",         252,  0 },
	{ "absmax[1]",         264,  0 },
	{ "absmin[0]",         248,  0 },
	{ "absmax[0]",         260,  0 },
	{ "absmin[2]",         256,  0 },
	{ "absmax[2]",         268,  0 },
	{ "pos.trBase[1]",      28,  0 },
	{ "pos.trBase[0]",      24,  0 },
	{ "eType",               4,  8 },
	{ "eFlags",              8, 24 },
	{ "pos.trBase[2]",      32,  0 },
	{ "svFlags",           240, 32 },
	{ "groundEntityNum",   124, 10 },
	{ "apos.trBase[1]",     64,  0 },
	{ "clientNum",         144,  8 },
	{ "apos.trBase[0]",     60,  0 },
	{ "index",             140,  9 },
	{ "apos.trBase[2]",     68,  0 },
	{ "eventSequence",     164,  8 },
	{ "events[0]",         168,  8 },
	{ "legsAnim",          204, 10 },
	{ "events[1]",         172,  8 },
	{ "events[2]",         176,  8 },
	{ "events[3]",         180,  8 },
	{ "weapon",            200,  6 },
	{ "pos.trType",         12,  8 },
	{ "pos.trTime",         16, 32 },
	{ "apos.trType",        48,  8 },
	{ "solid",             156, 24 },
	{ "pos.trDuration",     20, 32 },
	{ "eventParms[0]",     184,  8 },
	{ "torsoAnim",         208, 10 },
	{ "pos.trDelta[0]",     36,  0 },
	{ "pos.trDelta[1]",     40,  0 },
	{ "angles2[1]",        108,  0 },
	{ "angles2[0]",        104,  0 },
	{ "animMovetype",      224,  4 },
	{ "pos.trDelta[2]",     44,  0 },
	{ "otherEntityNum",    116, 10 },
	{ "eventParms[1]",     188,  8 },
	{ "surfType",          136,  8 },
	{ "eventParm",         160,  8 },
	{ "eventParms[2]",     192,  8 },
	{ "scale",             216,  8 },
	{ "eventParms[3]",     196,  8 },
	{ "fTorsoHeight",      228,  0 },
	{ "fWaistPitch",       236,  0 },
	{ "fTorsoPitch",       232,  0 },
	{ "apos.trTime",        52, 32 },
	{ "apos.trDelta[0]",    72,  0 },
	{ "apos.trDelta[2]",    80,  0 },
	{ "singleClient",      244,  6 },
	{ "leanf",             212,  0 },
	{ "apos.trDelta[1]",    76,  0 },
	{ "loopSound",         132,  8 },
	{ "attackerEntityNum", 120, 10 },
	{ "iHeadIcon",         148,  4 },
	{ "iHeadIconTeam",     152,  2 },
	{ "apos.trDuration",    56, 32 },
	{ "time",               84, 32 },
	{ "time2",              88, 32 },
	{ "origin2[0]",         92,  0 },
	{ "origin2[1]",         96,  0 },
	{ "origin2[2]",        100,  0 },
	{ "angles2[2]",        112,  0 },
	{ "constantLight",     128, 32 },
	{ "dmgFlags",          220, 32 }
};

netField_t clientStateFields[MSG_CLIENT_FIELDS] = {
	{ "team",                 4,  2 },
	{ "name[0]",             60, 32 },
	{ "name[4]",             64, 32 },
	{ "attachModelIndex[0]", 12,  8 },
	{ "attachModelIndex[2]", 20,  8 },
	{ "attachModelIndex[3]", 24,  8 },
	{ "modelindex",           8,  8 },
	{ "attachModelIndex[1]", 16,  8 },
	{ "name[8]",             68, 32 },
	{ "name[12]",            72, 32 },
	{ "attachModelIndex[4]", 28,  8 },
	{ "name[16]",            76, 32 },
	{ "attachTagIndex[0]",   36,  5 },
	{ "attachTagIndex[1]",   40,  5 },
	{ "attachTagIndex[2]",   44,  5 },
	{ "attachTagIndex[3]",   48,  5 },
	{ "attachTagIndex[4]",   52,  5 },
	{ "attachModelIndex[5]", 32,  8 },
	{ "attachTagIndex[5]",   56,  5 },
	{ "name[20]",            80, 32 },
	{ "name[24]",            84, 32 },
	{ "name[28]",            88, 32 }
};

static netField_t msg_playerStateFields[MSG_PLAYERSTATE_FIELDS] = {
	{ "commandTime",             0, 32 },
	{ "origin[1]",              24,  0 },
	{ "origin[0]",              20,  0 },
	{ "origin[2]",              28,  0 },
	{ "viewangles[1]",         196,  0 },
	{ "viewangles[0]",         192,  0 },
	{ "iCompassFriendInfo",    960, 32 },
	{ "eFlags",                128, 24 },
	{ "bobCycle",                8,  8 },
	{ "viewHeightCurrent",     208,  0 },
	{ "eventSequence",         132,  8 },
	{ "legsAnim",              112, 10 },
	{ "pm_flags",               12, 19 },
	{ "delta_angles[1]",        76, 16 },
	{ "velocity[1]",            36,  0 },
	{ "velocity[0]",            32,  0 },
	{ "speed",                  68, 16 },
	{ "mins[0]",               804,  0 },
	{ "mins[1]",               808,  0 },
	{ "maxs[0]",               816,  0 },
	{ "maxs[1]",               820,  0 },
	{ "maxs[2]",               824,  0 },
	{ "proneViewHeight",       828, -8 },
	{ "crouchViewHeight",      832, -8 },
	{ "standViewHeight",       836, -8 },
	{ "deadViewHeight",        840, -8 },
	{ "walkSpeedScale",        844,  0 },
	{ "runSpeedScale",         848,  0 },
	{ "proneSpeedScale",       852,  0 },
	{ "crouchSpeedScale",      856,  0 },
	{ "strafeSpeedScale",      860,  0 },
	{ "backSpeedScale",        864,  0 },
	{ "leanSpeedScale",        868,  0 },
	{ "friction",              892,  0 },
	{ "groundEntityNum",        84, 10 },
	{ "viewHeightTarget",      204, -8 },
	{ "events[0]",             136,  8 },
	{ "weapons[0]",            780, 32 },
	{ "weaponslots[0]",        788, 32 },
	{ "gravity",                60, 16 },
	{ "serverCursorHintString", 908, -8 },
	{ "viewmodelIndex",        188,  8 },
	{ "events[1]",             140,  8 },
	{ "events[2]",             144,  8 },
	{ "events[3]",             148,  8 },
	{ "delta_angles[0]",        72, 16 },
	{ "weapon",                176,  6 },
	{ "movementDir",           124,  8 },
	{ "viewHeightLerpTarget",  216, -8 },
	{ "weaponslots[4]",        792, 32 },
	{ "proneDirection",        872,  0 },
	{ "aimSpreadScale",        984,  0 },
	{ "weapAnim",              980, 10 },
	{ "jumpTime",              100, 32 },
	{ "velocity[2]",            40,  0 },
	{ "weaponTime",             44, -16 },
	{ "proneTorsoPitch",       880,  0 },
	{ "proneDirectionPitch",   876,  0 },
	{ "fTorsoPitch",           968,  0 },
	{ "fWaistPitch",           972,  0 },
	{ "fTorsoHeight",          964,  0 },
	{ "weaponstate",           180,  4 },
	{ "torsoTimer",            116, 16 },
	{ "torsoAnim",             120, 10 },
	{ "eventParms[0]",         152,  8 },
	{ "vLadderVec[0]",          88,  0 },
	{ "eventParms[3]",         164,  8 },
	{ "viewHeightLerpDown",    220,  1 },
	{ "weaponDelay",            48, -16 },
	{ "eventParms[1]",         156,  8 },
	{ "viewHeightLerpTime",    212, 32 },
	{ "eventParms[2]",         160,  8 },
	{ "vLadderVec[1]",          92,  0 },
	{ "fWeaponPosFrac",        184,  0 },
	{ "pm_type",                 4,  8 },
	{ "legsTimer",             108, 16 },
	{ "fJumpPeak",             104,  0 },
	{ "leanf",                  64,  0 },
	{ "damageEvent",           228,  8 },
	{ "damageYaw",             232,  8 },
	{ "damagePitch",           236,  8 },
	{ "damageCount",           240,  7 },
	{ "weaponrechamber[0]",    796, 32 },
	{ "grenadeTimeLeft",        52, -16 },
	{ "pm_time",                16, -16 },
	{ "iFoliageSoundTime",      56, 32 },
	{ "deltaTime",            8396, 32 },
	{ "serverCursorHint",      900,  8 },
	{ "serverCursorHintVal",   904,  8 },
	{ "shellshockIndex",       988,  4 },
	{ "shellshockTime",        992, 32 },
	{ "shellshockDuration",    996, 16 },
	{ "delta_angles[2]",        80, 16 },
	{ "vLadderVec[2]",          96,  0 },
	{ "clientNum",             172,  8 },
	{ "weapons[1]",            784, 32 },
	{ "weaponrechamber[1]",    800, 32 },
	{ "viewangles[2]",         200,  0 },
	{ "viewHeightLerpPosAdj",  224,  0 },
	{ "mins[2]",               812,  0 },
	{ "viewlocked",            884,  8 },
	{ "viewlocked_entNum",     888, 16 },
	{ "gunfx",                 896,  8 }
};

static netField_t msg_objectiveFields[MSG_OBJECTIVE_FIELDS] = {
	{ "origin[0]",  4,  0 },
	{ "origin[1]",  8,  0 },
	{ "origin[2]", 12,  0 },
	{ "icon",      24, 12 },
	{ "entNum",    16, 10 },
	{ "teamNum",   20,  4 }
};

static netField_t msg_hudElemFields[MSG_HUDELEM_FIELDS] = {
	{ "color.rgba",      28, 32 },
	{ "type",             0,  4 },
	{ "fontScale",       12,  0 },
	{ "y",                8, 10 },
	{ "x",                4, 10 },
	{ "alignY",          24,  2 },
	{ "alignX",          20,  2 },
	{ "time",            92, 32 },
	{ "font",            16,  4 },
	{ "text",           104,  8 },
	{ "shaderIndex",     56,  8 },
	{ "width",           48, 10 },
	{ "height",          52, 10 },
	{ "sort",           108,  0 },
	{ "fromColor.rgba",  32, 32 },
	{ "fadeStartTime",   36, 32 },
	{ "fadeTime",        40, 16 },
	{ "scaleStartTime",  68, 32 },
	{ "scaleTime",       72, 16 },
	{ "fromHeight",      64, 10 },
	{ "value",          100,  0 },
	{ "label",           44,  8 },
	{ "fromWidth",       60, 10 },
	{ "moveStartTime",   84, 32 },
	{ "moveTime",        88, 16 },
	{ "fromX",           76, 10 },
	{ "fromY",           80, 10 },
	{ "duration",        96, 32 }
};

/* MSG_initHuffman  0x00448390  VERIFIED */
void MSG_initHuffman( void ) {
	int i, j;

	msgInit = qtrue;
	Huff_Init( &msgHuff );
	for ( i = 0; i < 256; i++ ) {
		for ( j = 0; j < msg_hData[i]; j++ ) {
			Huff_addRef( &msgHuff.compressor, (byte)i );
			Huff_addRef( &msgHuff.decompressor, (byte)i );
		}
	}
}

/* MSG_Init  0x004446E0  VERIFIED */
void MSG_Init( msg_t *buf, byte *data, int length ) {
	if ( msgInit == qfalse ) {
		MSG_initHuffman();
	}
	memset( buf, 0, sizeof( msg_t ) );
	buf->data = data;
	buf->maxsize = length;
}

/* MSG_BeginReading  0x00444710  VERIFIED */
void MSG_BeginReading( msg_t *msg ) {
	msg->readcount = 0;
	msg->bit = 0;
}

/* MSG_WriteBits  0x00444720  VERIFIED */
void MSG_WriteBits( msg_t *msg, int value, int bits ) {
	int bitInByte;

	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = qtrue;
		return;
	}

	while ( bits ) {
		bitInByte = msg->bit & 7;
		if ( !bitInByte ) {
			msg->bit = 8 * msg->cursize;
			msg->data[msg->cursize] = 0;
			msg->cursize++;
		}
		if ( value & 1 ) {
			msg->data[msg->bit >> 3] |= 1 << bitInByte;
		}
		msg->bit++;
		value >>= 1;
		bits--;
	}
}

/* MSG_WriteBit0  0x00444790  VERIFIED */
void MSG_WriteBit0( msg_t *msg ) {
	if ( msg->cursize >= msg->maxsize ) {
		msg->overflowed = qtrue;
		return;
	}
	if ( ( msg->bit & 7 ) == 0 ) {
		msg->bit = 8 * msg->cursize;
		msg->data[msg->cursize] = 0;
		msg->cursize++;
	}
	msg->bit++;
}

/* MSG_WriteBit1  0x004447C0  VERIFIED */
void MSG_WriteBit1( msg_t *msg ) {
	int bitInByte;

	if ( msg->cursize >= msg->maxsize ) {
		msg->overflowed = qtrue;
		return;
	}
	bitInByte = msg->bit & 7;
	if ( !bitInByte ) {
		msg->bit = 8 * msg->cursize;
		msg->data[msg->cursize] = 0;
		msg->cursize++;
	}
	msg->data[msg->bit >> 3] |= 1 << bitInByte;
	msg->bit++;
}

/* MSG_ReadBits  0x00444810  VERIFIED */
int MSG_ReadBits( msg_t *msg, int bits ) {
	unsigned int value;
	int i, bitInByte;

	value = 0;
	for ( i = 0; i < bits; i++ ) {
		bitInByte = msg->bit & 7;
		if ( !bitInByte ) {
			msg->bit = 8 * msg->readcount;
			msg->readcount++;
		}
		value |= (unsigned int)( ( msg->data[msg->bit >> 3] >> bitInByte ) & 1 ) << i;
		msg->bit++;
	}
	return (int)value;
}

/* MSG_ReadBit  0x00444870  VERIFIED */
int MSG_ReadBit( msg_t *msg ) {
	int bitInByte, value;

	bitInByte = msg->bit & 7;
	if ( !bitInByte ) {
		msg->bit = 8 * msg->readcount;
		msg->readcount++;
	}
	value = ( msg->data[msg->bit >> 3] >> bitInByte ) & 1;
	msg->bit++;
	return value;
}

/* MSG_WriteBitsCompress  0x004448B0  VERIFIED */
int MSG_WriteBitsCompress( const byte *datasrc, int bytecount, byte *buffdest ) {
	int offset;

	offset = 0;
	while ( bytecount-- ) {
		Huff_offsetTransmit( &msgHuff.compressor, *datasrc++, buffdest, &offset );
	}
	return ( offset + 7 ) >> 3;
}

/* MSG_ReadBitsCompress  0x00444900  VERIFIED */
int MSG_ReadBitsCompress( const byte *input, byte *outputBuf, int readsize ) {
	byte *outptr;
	int get, offset;

	readsize = readsize * 8;
	outptr = outputBuf;
	offset = 0;
	while ( offset < readsize ) {
		Huff_offsetReceive( msgHuff.decompressor.tree, &get, (byte *)input, &offset );
		*outptr++ = (byte)get;
	}
	return outptr - outputBuf;
}

/* MSG_WriteByte  0x00444960  VERIFIED */
void MSG_WriteByte( msg_t *msg, int c ) {
	if ( msg->cursize < msg->maxsize ) {
		msg->data[msg->cursize] = (byte)c;
		msg->cursize++;
	} else {
		msg->overflowed = qtrue;
	}
}

/* MSG_WriteData  0x00444980  VERIFIED */
void MSG_WriteData( msg_t *msg, int length, const void *data ) {
	int newsize;

	newsize = msg->cursize + length;
	if ( newsize <= msg->maxsize ) {
		memcpy( &msg->data[msg->cursize], data, (size_t)(unsigned int)length );
		msg->cursize = newsize;
	} else {
		msg->overflowed = qtrue;
	}
}

/* MSG_WriteShort  0x004449C0  VERIFIED */
void MSG_WriteShort( msg_t *msg, int c ) {
	int newsize;

	newsize = msg->cursize + 2;
	if ( newsize <= msg->maxsize ) {
		*(short *)&msg->data[msg->cursize] = (short)c;
		msg->cursize = newsize;
	} else {
		msg->overflowed = qtrue;
	}
}

/* MSG_WriteLong  0x004449F0  VERIFIED */
void MSG_WriteLong( msg_t *msg, int c ) {
	int newsize;

	newsize = msg->cursize + 4;
	if ( newsize <= msg->maxsize ) {
		*(int *)&msg->data[msg->cursize] = c;
		msg->cursize = newsize;
	} else {
		msg->overflowed = qtrue;
	}
}

static char MSG_CleanChar( char c ) {
	if ( c == (char)0x92 ) {
		return '\'';
	}
	if ( (byte)c > 0x7F ) {
		return '.';
	}
	return c;
}

/* MSG_WriteString  0x00444A20  VERIFIED */
void MSG_WriteString( msg_t *sb, const char *s ) {
	char string[MAX_STRING_CHARS];
	int i, len;

	if ( !s ) {
		MSG_WriteData( sb, 1, "" );
		return;
	}

	len = strlen( s );
	if ( len >= MAX_STRING_CHARS ) {
		Com_Printf( "MSG_WriteString: MAX_STRING_CHARS" );
		MSG_WriteData( sb, 1, "" );
		return;
	}

	for ( i = 0; i < len; i++ ) {
		string[i] = MSG_CleanChar( s[i] );
	}
	/* pointer store: string[i] = 0 makes modern /GS add a range-check fast-fail VC7 never emitted */
	*( string + i ) = 0;
	MSG_WriteData( sb, len + 1, string );
}

/* MSG_WriteBigString  0x00444B40  VERIFIED */
void MSG_WriteBigString( msg_t *sb, const char *s ) {
	char string[BIG_INFO_STRING];
	int i, len;

	if ( !s ) {
		MSG_WriteData( sb, 1, "" );
		return;
	}

	len = strlen( s );
	if ( len >= BIG_INFO_STRING ) {
		Com_Printf( "MSG_WriteString: BIG_INFO_STRING" );
		MSG_WriteData( sb, 1, "" );
		return;
	}

	strncpy( string, s, BIG_INFO_STRING - 1 );
	string[BIG_INFO_STRING - 1] = 0;
	for ( i = 0; i < len; i++ ) {
		string[i] = MSG_CleanChar( string[i] );
	}
	MSG_WriteData( sb, len + 1, string );
}

/* MSG_WriteAngle  0x00444CD0  VERIFIED */
void MSG_WriteAngle( msg_t *sb, float f ) {
	MSG_WriteByte( sb, (int)( f * 0.71111113f ) & 255 );
}

/* MSG_WriteAngle16  0x00444D00  VERIFIED */
void MSG_WriteAngle16( msg_t *sb, float f ) {
	MSG_WriteShort( sb, (int)( f * 182.04445f ) & 65535 );
}

/* MSG_ReadByte  0x00444D40  VERIFIED */
int MSG_ReadByte( msg_t *msg ) {
	int c;

	if ( msg->readcount < msg->cursize ) {
		c = msg->data[msg->readcount];
		msg->readcount++;
		return c;
	}
	return -1;
}

/* MSG_ReadShort  0x00444D60  VERIFIED */
int MSG_ReadShort( msg_t *msg ) {
	int newcount;
	short c;

	newcount = msg->readcount + 2;
	if ( newcount <= msg->cursize ) {
		c = *(short *)&msg->data[msg->readcount];
		msg->readcount = newcount;
		return c;
	}
	return -1;
}

/* MSG_ReadLong  0x00444D90  VERIFIED */
int MSG_ReadLong( msg_t *msg ) {
	int newcount;
	int c;

	newcount = msg->readcount + 4;
	if ( newcount <= msg->cursize ) {
		c = *(int *)&msg->data[msg->readcount];
		msg->readcount = newcount;
		return c;
	}
	return -1;
}

/* MSG_ReadString  0x00444DB0  VERIFIED */
char *MSG_ReadString( msg_t *msg ) {
	unsigned int i;
	int c;

	i = 0;
	do {
		c = MSG_ReadByte( msg );
		if ( c == -1 || c == 0 ) {
			break;
		}
		msg_stringBuf[i] = MSG_CleanChar( (char)c );
		i++;
	} while ( i < MAX_STRING_CHARS - 1 );
	/* pointer store: msg_stringBuf[i] = 0 makes modern /GS add a range-check fast-fail VC7 never emitted */
	*( msg_stringBuf + i ) = 0;
	return msg_stringBuf;
}

/* MSG_ReadBigString  0x00444E00  VERIFIED */
char *MSG_ReadBigString( msg_t *msg ) {
	unsigned int i;
	int c;

	for ( i = 0; i < BIG_INFO_STRING - 1; i++ ) {
		c = MSG_ReadByte( msg );
		if ( c == -1 || c == 0 ) {
			break;
		}
		if ( c == '%' ) {
			c = '.';
		}
		msg_bigStringBuf[i] = MSG_CleanChar( (char)c );
	}
	/* pointer store: msg_bigStringBuf[i] = 0 makes modern /GS add a range-check fast-fail VC7 never emitted */
	*( msg_bigStringBuf + i ) = 0;
	return msg_bigStringBuf;
}

/* MSG_ReadStringLine  0x00444E60  VERIFIED */
char *MSG_ReadStringLine( msg_t *msg ) {
	unsigned int i;
	int c;

	for ( i = 0; i < MAX_STRING_CHARS - 1; i++ ) {
		c = MSG_ReadByte( msg );
		if ( c == -1 || c == 0 || c == '\n' ) {
			break;
		}
		if ( c == '%' ) {
			c = '.';
		}
		msg_stringLineBuf[i] = MSG_CleanChar( (char)c );
	}
	/* pointer store: msg_stringLineBuf[i] = 0 makes modern /GS add a range-check fast-fail VC7 never emitted */
	*( msg_stringLineBuf + i ) = 0;
	return msg_stringLineBuf;
}

/* MSG_ReadAngle16  0x00444EC0  VERIFIED */
float MSG_ReadAngle16( msg_t *msg ) {
	return (float)MSG_ReadShort( msg ) * 0.0054931640625f;
}

/* MSG_ReadData  0x00444F10  VERIFIED */
void MSG_ReadData( msg_t *msg, void *data, int len ) {
	int newcount;

	newcount = msg->readcount + len;
	if ( newcount <= msg->cursize ) {
		memcpy( data, &msg->data[msg->readcount], (size_t)(unsigned int)len );
		msg->readcount = newcount;
	} else {
		memset( data, 0xFF, (size_t)(unsigned int)len );
	}
}

/* MSG_WriteDeltaKeyFloat  0x00444F60  VERIFIED */
void MSG_WriteDeltaKeyFloat( msg_t *msg, int oldV, int newV, int bits ) {
	if ( oldV == newV ) {
		MSG_WriteBit0( msg );
		return;
	}
	MSG_WriteBit1( msg );
	MSG_WriteBits( msg, newV, bits );
}

/* MSG_ReadDeltaKeyFloat  0x00444FB0  VERIFIED */
int MSG_ReadDeltaKeyFloat( msg_t *msg, int oldV, int bits ) {
	if ( MSG_ReadBit( msg ) ) {
		return MSG_ReadBits( msg, bits );
	}
	return oldV;
}

/* MSG_WriteDeltaKey  0x00445010  VERIFIED */
void MSG_WriteDeltaKey( msg_t *msg, int key, int oldV, int newV, int bits ) {
	if ( oldV == newV ) {
		MSG_WriteBit0( msg );
		return;
	}
	MSG_WriteBit1( msg );
	MSG_WriteBits( msg, key ^ newV, bits );
}

/* MSG_ReadDeltaKey  0x00445060  VERIFIED */
int MSG_ReadDeltaKey( msg_t *msg, int key, int oldV, int bits ) {
	if ( MSG_ReadBit( msg ) ) {
		return MSG_ReadBits( msg, bits ) ^ ( key & kbitmask[bits] );
	}
	return oldV;
}

/* MSG_WriteKey  0x004450D0  VERIFIED */
void MSG_WriteKey( msg_t *msg, int key, int newV, int bits ) {
	MSG_WriteBits( msg, key ^ newV, bits );
}

/* MSG_ReadKey  0x004450E0  VERIFIED */
int MSG_ReadKey( msg_t *msg, int key, int bits ) {
	return MSG_ReadBits( msg, bits ) ^ ( key & kbitmask[bits] );
}

/* MSG_WriteDeltaKeyByte  0x00445100  VERIFIED */
void MSG_WriteDeltaKeyByte( msg_t *msg, int key, int oldV, int newV ) {
	if ( (byte)oldV == (byte)newV ) {
		MSG_WriteBit0( msg );
		return;
	}
	MSG_WriteBit1( msg );
	MSG_WriteByte( msg, key ^ newV );
}

/* MSG_ReadDeltaKeyByte  0x00445160  VERIFIED */
int MSG_ReadDeltaKeyByte( msg_t *msg, int key, int oldV ) {
	if ( MSG_ReadBit( msg ) ) {
		return (byte)( MSG_ReadByte( msg ) ^ key );
	}
	return oldV;
}

/* MSG_WriteDeltaKeyShort  0x004451E0  VERIFIED */
void MSG_WriteDeltaKeyShort( msg_t *msg, int key, int oldV, int newV ) {
	if ( (short)oldV == (short)newV ) {
		MSG_WriteBit0( msg );
		return;
	}
	MSG_WriteBit1( msg );
	MSG_WriteShort( msg, key ^ newV );
}

/* MSG_ReadDeltaKeyShort  0x00445250  VERIFIED */
int MSG_ReadDeltaKeyShort( msg_t *msg, int key, int oldV ) {
	if ( MSG_ReadBit( msg ) ) {
		return (short)( MSG_ReadShort( msg ) ^ key );
	}
	return oldV;
}

/* MSG_WriteReliableCommandToBuffer  0x004452D0  VERIFIED */
void MSG_WriteReliableCommandToBuffer( const char *pszCommand, int iBufferSize, char *pszBuffer ) {
	int len, i;

	len = strlen( pszCommand );
	if ( len >= iBufferSize ) {
		Com_Printf( "WARNING: Reliable command is too long (%i/%i) and will be truncated: '%s'\n",
					len, iBufferSize, pszCommand );
	}
	if ( !len ) {
		Com_Printf( "WARNING: Empty reliable command\n" );
	}

	for ( i = 0; i < iBufferSize && pszCommand[i] != '\0'; i++ ) {
		pszBuffer[i] = MSG_CleanChar( pszCommand[i] );
		if ( pszBuffer[i] == '%' ) {
			pszBuffer[i] = '.';
		}
	}

	if ( i < iBufferSize ) {
		pszBuffer[i] = 0;
	} else {
		pszBuffer[iBufferSize - 1] = 0;
	}
}

#define PMF_FOLLOW_ACTIVE       0x40000
#define EF_PRONE                0x40
#define EF_CROUCH               0x20

#define UCMD_WBUTTON_PRONE      0x40
#define UCMD_WBUTTON_CROUCH     0x80
#define UCMD_WBUTTON_LEAN_LEFT  0x10
#define UCMD_WBUTTON_LEAN_RIGHT 0x20
#define UCMD_BUTTON_ADS         0x10
#define UCMD_STANCE_UPMOVE      -127

/* MSG_SetDefaultUserCmd  0x00445360  VERIFIED */
void MSG_SetDefaultUserCmd( const byte *ps, usercmd_t *ucmd ) {
	const int *psi = (const int *)ps;
	const float *psf = (const float *)ps;
	int i;

	memset( ucmd, 0, sizeof( usercmd_t ) );
	ucmd->weapon = (byte)psi[44];

	for ( i = 0; i < 2; i++ ) {
		ucmd->angles[i] = ( (int)( psf[48 + i] * 182.04445f )
							- psi[18 + i] ) & 0xFFFF;
	}

	if ( ( psi[3] & PMF_FOLLOW_ACTIVE ) == 0 ) {
		return;
	}

	if ( psi[32] & EF_PRONE ) {
		ucmd->wbuttons |= UCMD_WBUTTON_PRONE;
		ucmd->upmove = UCMD_STANCE_UPMOVE;
	} else if ( psi[32] & EF_CROUCH ) {
		ucmd->wbuttons |= UCMD_WBUTTON_CROUCH;
		ucmd->upmove = UCMD_STANCE_UPMOVE;
	}

	if ( psf[16] > 0.0f ) {
		ucmd->wbuttons |= UCMD_WBUTTON_LEAN_RIGHT;
	} else if ( psf[16] < 0.0f ) {
		ucmd->wbuttons |= UCMD_WBUTTON_LEAN_LEFT;
	}

	if ( psf[46] != 0.0f ) {
		ucmd->buttons |= UCMD_BUTTON_ADS;
	}
}

#define UCMD_MOVE_THRESHOLD  10
#define UCMD_MOVE_POSITIVE   127
#define UCMD_MOVE_NEGATIVE   ( -127 )

/* MSG_HorMoveTo  0x00445430  VERIFIED */
int MSG_HorMoveTo( int forwardmove, int rightmove ) {
	int flags = 0;

	if ( forwardmove > UCMD_MOVE_THRESHOLD ) {
		flags = 1;
	} else if ( forwardmove < -UCMD_MOVE_THRESHOLD ) {
		flags = 2;
	}
	if ( rightmove > UCMD_MOVE_THRESHOLD ) {
		flags |= 4;
	} else if ( rightmove < -UCMD_MOVE_THRESHOLD ) {
		flags |= 8;
	}
	return flags;
}

/* MSG_VertMoveTo  0x00445460  VERIFIED */
int MSG_VertMoveTo( int upmove ) {
	int flags = 0;

	if ( upmove > UCMD_MOVE_THRESHOLD ) {
		flags = 1;
	} else if ( upmove < -UCMD_MOVE_THRESHOLD ) {
		flags = 2;
	}
	return flags;
}

/* MSG_HorMoveFrom  0x00445480  VERIFIED */
void MSG_HorMoveFrom( int flags, signed char *forwardmove, signed char *rightmove ) {
	if ( flags & 1 ) {
		*forwardmove = UCMD_MOVE_POSITIVE;
	} else if ( flags & 2 ) {
		*forwardmove = UCMD_MOVE_NEGATIVE;
	} else {
		*forwardmove = 0;
	}

	if ( flags & 4 ) {
		*rightmove = UCMD_MOVE_POSITIVE;
	} else if ( flags & 8 ) {
		*rightmove = UCMD_MOVE_NEGATIVE;
	} else {
		*rightmove = 0;
	}
}

/* MSG_VertMoveFrom  0x004454B0  VERIFIED */
void MSG_VertMoveFrom( int flags, signed char *upmove ) {
	if ( flags & 1 ) {
		*upmove = UCMD_MOVE_POSITIVE;
	} else if ( flags & 2 ) {
		*upmove = UCMD_MOVE_NEGATIVE;
	} else {
		*upmove = 0;
	}
}

/* MSG_WriteDeltaUsercmdKey  0x004454D0  VERIFIED */
void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, const usercmd_t *from, const usercmd_t *to ) {
	int delta;
	int newHor, oldHor, newVert, oldVert;

	delta = to->serverTime - from->serverTime;
	if ( (unsigned int)delta < 256 ) {
		MSG_WriteBit1( msg );
		MSG_WriteByte( msg, delta );
	} else {
		MSG_WriteBit0( msg );
		MSG_WriteLong( msg, to->serverTime );
	}

	newHor = MSG_HorMoveTo( to->forwardmove, to->rightmove );
	oldHor = MSG_HorMoveTo( from->forwardmove, from->rightmove );
	newVert = MSG_VertMoveTo( to->upmove );
	oldVert = MSG_VertMoveTo( from->upmove );

	if ( ( to->buttons & 0xFE ) == ( from->buttons & 0xFE )
		 && from->wbuttons == to->wbuttons
		 && from->weapon == to->weapon
		 && from->angles[2] == to->angles[2]
		 && oldVert == newVert ) {

		if ( from->angles[0] == to->angles[0]
			 && from->angles[1] == to->angles[1]
			 && ( to->buttons & 1 ) == ( from->buttons & 1 )
			 && oldHor == newHor ) {
			MSG_WriteKey( msg, key, 0, 1 );
			return;
		}

		MSG_WriteKey( msg, key, 1, 1 );
		MSG_WriteKey( msg, key, 0, 1 );
		key ^= to->serverTime;
		MSG_WriteKey( msg, key, to->buttons, 1 );
		MSG_WriteDeltaKeyShort( msg, key, from->angles[0], to->angles[0] );
		MSG_WriteDeltaKeyShort( msg, key, from->angles[1], to->angles[1] );
		MSG_WriteDeltaKey( msg, key, oldHor, newHor, 4 );
		return;
	}

	MSG_WriteKey( msg, key, 1, 1 );
	MSG_WriteKey( msg, key, 1, 1 );
	MSG_WriteKey( msg, key, to->buttons, 1 );
	MSG_WriteDeltaKeyShort( msg, key, from->angles[0], to->angles[0] );
	MSG_WriteDeltaKeyShort( msg, key, from->angles[1], to->angles[1] );
	MSG_WriteDeltaKey( msg, key, oldHor, newHor, 4 );

	key ^= to->serverTime;
	MSG_WriteDeltaKeyShort( msg, key, from->angles[2], to->angles[2] );
	MSG_WriteDeltaKey( msg, key, from->buttons >> 1, to->buttons >> 1, 6 );
	MSG_WriteDeltaKeyByte( msg, key, from->wbuttons, to->wbuttons );
	MSG_WriteDeltaKey( msg, key, oldVert, newVert, 2 );
	MSG_WriteDeltaKey( msg, key, from->weapon, to->weapon, 6 );
}

/* MSG_ReadDeltaUsercmdKey  0x004458E0  VERIFIED */
void MSG_ReadDeltaUsercmdKey( msg_t *msg, int key, const usercmd_t *from, usercmd_t *to ) {
	memcpy( to, from, sizeof( usercmd_t ) );

	if ( MSG_ReadBit( msg ) ) {
		to->serverTime = from->serverTime + MSG_ReadByte( msg );
	} else {
		to->serverTime = MSG_ReadLong( msg );
	}

	if ( !MSG_ReadKey( msg, key, 1 ) ) {
		return;
	}

	to->buttons &= ~1u;
	if ( !MSG_ReadKey( msg, key, 1 ) ) {
		key ^= to->serverTime;
		to->buttons |= (byte)MSG_ReadKey( msg, key, 1 );
		to->angles[0] = (unsigned short)MSG_ReadDeltaKeyShort( msg, key, from->angles[0] );
		to->angles[1] = (unsigned short)MSG_ReadDeltaKeyShort( msg, key, from->angles[1] );
		MSG_HorMoveFrom( MSG_ReadDeltaKey( msg, key,
										   MSG_HorMoveTo( from->forwardmove, from->rightmove ), 4 ),
						 &to->forwardmove, &to->rightmove );
		return;
	}

	to->buttons |= (byte)MSG_ReadKey( msg, key, 1 );
	to->angles[0] = (unsigned short)MSG_ReadDeltaKeyShort( msg, key, from->angles[0] );
	to->angles[1] = (unsigned short)MSG_ReadDeltaKeyShort( msg, key, from->angles[1] );
	MSG_HorMoveFrom( MSG_ReadDeltaKey( msg, key,
									   MSG_HorMoveTo( from->forwardmove, from->rightmove ), 4 ),
					 &to->forwardmove, &to->rightmove );

	key ^= to->serverTime;
	to->angles[2] = (unsigned short)MSG_ReadDeltaKeyShort( msg, key, from->angles[2] );
	to->buttons &= 1u;
	to->buttons |= (byte)( MSG_ReadDeltaKey( msg, key, from->buttons >> 1, 6 ) << 1 );
	to->wbuttons = (byte)MSG_ReadDeltaKeyByte( msg, key, from->wbuttons );
	MSG_VertMoveFrom( MSG_ReadDeltaKey( msg, key,
										MSG_VertMoveTo( from->upmove ), 2 ),
					  &to->upmove );
	to->weapon = (byte)MSG_ReadDeltaKey( msg, key, from->weapon, 6 );
}

#define MSG_SMALL_FLOAT_BIAS   4096
#define MSG_SMALL_FLOAT_RANGE  8192
#define MSG_SMALL_FLOAT_BITS   5

/* MSG_WriteDeltaField  0x00445C00  VERIFIED */
/* Mac 1.3 argument order (msg, from, to, field): MSG_WriteDeltaHudElems' register set-up follows it */
void MSG_WriteDeltaField( msg_t *msg, const byte *from, const byte *to, const netField_t *field ) {
	int offset;
	int v;	/* signed: retail shifts with sar (0x00445D2C, 0x00445D4B) */
	int bits, lowBits;
	float fullFloat;
	int trunc;

	offset = field->offset;
	if ( *(const int *)( from + offset ) == *(const int *)( to + offset ) ) {
		MSG_WriteBit0( msg );
		return;
	}
	MSG_WriteBit1( msg );

	if ( !field->bits ) {
		fullFloat = *(const float *)( to + offset );
		trunc = (int)fullFloat;
		if ( fullFloat == 0.0f ) {
			MSG_WriteBit0( msg );
			return;
		}
		MSG_WriteBit1( msg );
		if ( (float)trunc == fullFloat && trunc + MSG_SMALL_FLOAT_BIAS >= 0
			 && trunc + MSG_SMALL_FLOAT_BIAS < MSG_SMALL_FLOAT_RANGE ) {
			MSG_WriteBit0( msg );
			MSG_WriteBits( msg, trunc + MSG_SMALL_FLOAT_BIAS, MSG_SMALL_FLOAT_BITS );
			MSG_WriteByte( msg, ( trunc + MSG_SMALL_FLOAT_BIAS ) >> MSG_SMALL_FLOAT_BITS );
		} else {
			MSG_WriteBit1( msg );
			MSG_WriteLong( msg, *(const int *)( to + offset ) );
		}
		return;
	}

	if ( !*(const int *)( to + offset ) ) {
		MSG_WriteBit0( msg );
		return;
	}
	MSG_WriteBit1( msg );

	v = *(const int *)( to + offset );
	bits = field->bits > 0 ? field->bits : -field->bits;
	lowBits = bits & 7;
	if ( lowBits ) {
		MSG_WriteBits( msg, v, lowBits );
		bits -= lowBits;
		v >>= lowBits;
	}
	while ( bits ) {
		MSG_WriteByte( msg, v & 0xFF );
		v >>= 8;
		bits -= 8;
	}
}

/* MSG_WriteDeltaFields_0  0x00445D60  VERIFIED */
void MSG_WriteDeltaFields_0( int force, msg_t *msg, const byte *from, const byte *to,
							 int numFields, const netField_t *stateFields ) {
	int i;

	if ( !force ) {
		for ( i = 0; i < numFields; i++ ) {
			if ( *(const int *)( from + stateFields[i].offset )
				 != *(const int *)( to + stateFields[i].offset ) ) {
				goto changed;
			}
		}
		MSG_WriteBit0( msg );
		return;
	}

changed:
	MSG_WriteBit1( msg );
	for ( i = 0; i < numFields; i++ ) {
		MSG_WriteDeltaField( msg, from, to, &stateFields[i] );
	}
}

/* MSG_WriteDeltaStruct  0x00445E40  VERIFIED */
void MSG_WriteDeltaStruct( msg_t *msg, const byte *from, const byte *to, qboolean force,
						   int numFields, int indexBits, const netField_t *stateFields,
						   qboolean bChangeBit ) {
	int i, lastChanged;
	const netField_t *field;

	if ( !to ) {
		if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) ) {
			Com_Printf( "W|%3i: #%-3i remove\n", msg->cursize, *(const int *)from );
		}
		if ( bChangeBit ) {
			MSG_WriteBit1( msg );
		}
		MSG_WriteBits( msg, *(const int *)from, indexBits );
		MSG_WriteBit1( msg );
		return;
	}

	lastChanged = 0;
	for ( i = 0; i < numFields; i++ ) {
		if ( *(const int *)( from + stateFields[i].offset )
			 != *(const int *)( to + stateFields[i].offset ) ) {
			lastChanged = i + 1;
		}
	}

	if ( !lastChanged ) {
		if ( force ) {
			if ( bChangeBit ) {
				MSG_WriteBit1( msg );
			}
			MSG_WriteBits( msg, *(const int *)to, indexBits );
			MSG_WriteBit0( msg );
			MSG_WriteBit0( msg );
		}
		return;
	}

	if ( bChangeBit ) {
		MSG_WriteBit1( msg );
	}
	MSG_WriteBits( msg, *(const int *)to, indexBits );
	MSG_WriteBit0( msg );
	MSG_WriteBit1( msg );
	MSG_WriteByte( msg, lastChanged );
	for ( i = 0, field = stateFields; i < lastChanged; i++, field++ ) {
		MSG_WriteDeltaField( msg, from, to, field );
	}
}

/* MSG_WriteDeltaEntity  0x00446050  VERIFIED */
void MSG_WriteDeltaEntity( msg_t *msg, const byte *from, const byte *to, qboolean force ) {
	MSG_WriteDeltaStruct( msg, from, to, force, MSG_ENTITY_FIELDS,
						  MSG_ENTITY_NUM_BITS, stateFields, qfalse );
}

/* MSG_WriteDeltaArchivedEntity  0x00446070  VERIFIED */
void MSG_WriteDeltaArchivedEntity( msg_t *msg, const byte *from, const byte *to, qboolean force ) {
	MSG_WriteDeltaStruct( msg, from, to, force, MSG_ENTITY_BASE_FIELDS,
						  MSG_ENTITY_NUM_BITS, archivedEntityFields, qfalse );
}

/* MSG_WriteDeltaClient  0x00446090  VERIFIED */
void MSG_WriteDeltaClient( msg_t *msg, const byte *from, const byte *to, qboolean force ) {
	byte nullClient[92];

	if ( !from ) {
		from = nullClient;
		memset( nullClient, 0, sizeof( nullClient ) );
	}
	MSG_WriteDeltaStruct( msg, from, to, force, MSG_CLIENT_FIELDS,
						  MSG_CLIENT_NUM_BITS, clientStateFields, qtrue );
}

/* MSG_ReadDeltaField  0x004460F0  VERIFIED */
void MSG_ReadDeltaField( const byte *from, msg_t *msg, byte *to,
						 const netField_t *field, qboolean print ) {
	const int *fromF;
	int *toF;
	unsigned int v;
	int bits, lowBits, bit;
	int small;

	/* Q3-style field pointers taken up front; with the offset kept instead, VC7
	 * addresses the inlined MSG_ReadByte loads as [data + readcount], not retail's
	 * [readcount + data] */
	fromF = (const int *)( from + field->offset );
	toF = (int *)( to + field->offset );

	if ( !MSG_ReadBit( msg ) ) {
		*toF = *fromF;
		return;
	}

	if ( !field->bits ) {
		if ( !MSG_ReadBit( msg ) ) {
			*toF = 0;
			return;
		}
		if ( !MSG_ReadBit( msg ) ) {
			small = MSG_ReadBits( msg, MSG_SMALL_FLOAT_BITS );
			/* retail folds the bias into the high byte: ( b - 128 ) << 5 */
			small += ( MSG_ReadByte( msg ) - ( MSG_SMALL_FLOAT_BIAS >> MSG_SMALL_FLOAT_BITS ) )
					 << MSG_SMALL_FLOAT_BITS;
			*(float *)toF = (float)small;
			if ( print ) {
				Com_Printf( "%s:%i ", field->name, small );
			}
		} else {
			*toF = MSG_ReadLong( msg );
			if ( print ) {
				Com_Printf( "%s:%f ", field->name, *(float *)toF );
			}
		}
		return;
	}

	if ( !MSG_ReadBit( msg ) ) {
		*toF = 0;
		return;
	}
	bits = field->bits >= 0 ? field->bits : -field->bits;
	lowBits = bits & 7;
	v = lowBits ? (unsigned int)MSG_ReadBits( msg, lowBits ) : 0;
	for ( bit = lowBits; bit < bits; bit += 8 ) {
		v |= (unsigned int)MSG_ReadByte( msg ) << bit;
	}
	*toF = (int)v;
	if ( print ) {
		Com_Printf( "%s:%i ", field->name, (int)v );
	}
}

/* MSG_ReadDeltaFields  0x004462A0  VERIFIED */
void MSG_ReadDeltaFields( msg_t *msg, int numFields, const byte *from, byte *to,
						  const netField_t *stateFields ) {
	int i;

	if ( MSG_ReadBit( msg ) ) {
		for ( i = 0; i < numFields; i++ ) {
			MSG_ReadDeltaField( from, msg, to, &stateFields[i], qfalse );
		}
	} else {
		for ( i = 0; i < numFields; i++ ) {
			*(int *)( to + stateFields[i].offset ) =
				*(const int *)( from + stateFields[i].offset );
		}
	}
}

/* MSG_ReadDeltaStruct  0x004464E0  VERIFIED */
qboolean MSG_ReadDeltaStruct( msg_t *msg, const byte *from, byte *to, unsigned int number,
							  int numFields, int indexBits, const netField_t *stateFields ) {
	int lastChanged, i;
	qboolean print;
	const netField_t *field;

	(void)indexBits;

	if ( MSG_ReadBit( msg ) ) {
		if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) ) {
			Com_Printf( "%3i: #%-3i remove\n", msg->readcount, number );
		}
		return qtrue;
	}

	if ( !MSG_ReadBit( msg ) ) {
		memcpy( to, from, (size_t)( 4 * numFields + 4 ) );
		return qfalse;
	}

	lastChanged = MSG_ReadByte( msg );
	if ( lastChanged > numFields ) {
		lastChanged = numFields;
	}

	if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) ) {
		print = qtrue;
		Com_Printf( "%3i: #%-3i ", msg->readcount, *(const int *)to );
	} else {
		print = qfalse;
	}

	*(int *)to = (int)number;

	for ( i = 0, field = stateFields; i < lastChanged; i++, field++ ) {
		MSG_ReadDeltaField( from, msg, to, field, print );
	}
	for ( i = lastChanged, field = &stateFields[lastChanged]; i < numFields; i++, field++ ) {
		*(int *)( to + field->offset ) = *(const int *)( from + field->offset );
	}
	return qfalse;
}

/* MSG_ReadDeltaEntity  0x004464E0  VERIFIED */
qboolean MSG_ReadDeltaEntity( msg_t *msg, const byte *from, byte *to, int number ) {
	return MSG_ReadDeltaStruct( msg, from, to, (unsigned int)number,
								MSG_ENTITY_FIELDS, MSG_ENTITY_NUM_BITS, stateFields );
}

/* MSG_ReadDeltaArchivedEntity  0x00446500  VERIFIED */
qboolean MSG_ReadDeltaArchivedEntity( msg_t *msg, const byte *from, byte *to, int number ) {
	return MSG_ReadDeltaStruct( msg, from, to, (unsigned int)number,
								MSG_ENTITY_BASE_FIELDS, MSG_ENTITY_NUM_BITS, archivedEntityFields );
}

/* MSG_ReadDeltaClient  0x00446520  VERIFIED */
qboolean MSG_ReadDeltaClient( msg_t *msg, const byte *from, byte *to, int number ) {
	byte nullClient[92];

	if ( !from ) {
		from = nullClient;
		memset( nullClient, 0, sizeof( nullClient ) );
	}
	return MSG_ReadDeltaStruct( msg, from, to, (unsigned int)number,
								MSG_CLIENT_FIELDS, MSG_CLIENT_NUM_BITS, clientStateFields );
}

#define MSG_HUD_COUNT_BITS      5
#define MSG_HUD_LASTFIELD_BITS  5

/* MSG_WriteDeltaHudElems  0x00446570  VERIFIED */
void MSG_WriteDeltaHudElems( msg_t *msg, const hudelem_t *from, const hudelem_t *to, int count ) {
	int used, elem, i, lastChanged;
	const netField_t *field;

	for ( used = 0; used < count && *(const int *)&to[used] != 0; used++ ) {
	}

	MSG_WriteBits( msg, used, MSG_HUD_COUNT_BITS );

	for ( elem = 0; elem < used; elem++ ) {
		lastChanged = 0;
		/* unsigned bound: retail runs the unrolled scan on a separate trip counter */
		for ( i = 0, field = msg_hudElemFields; (unsigned int)i < MSG_HUDELEM_FIELDS;
			  i++, field++ ) {
			if ( *(const int *)( (const byte *)&from[elem] + field->offset )
				 != *(const int *)( (const byte *)&to[elem] + field->offset ) ) {
				lastChanged = i;
			}
		}

		MSG_WriteBits( msg, lastChanged, MSG_HUD_LASTFIELD_BITS );
		for ( i = 0; i <= lastChanged; i++ ) {
			MSG_WriteDeltaField( msg, (const byte *)&from[elem], (const byte *)&to[elem],
								 &msg_hudElemFields[i] );
		}
	}
}

/* MSG_ReadDeltaHudElems  0x00446710  VERIFIED */
void MSG_ReadDeltaHudElems( msg_t *msg, const hudelem_t *from, hudelem_t *to, int count ) {
	int used, elem, i, lastChanged;

	used = MSG_ReadBits( msg, MSG_HUD_COUNT_BITS );

	for ( elem = 0; elem < used; elem++ ) {
		lastChanged = MSG_ReadBits( msg, MSG_HUD_LASTFIELD_BITS );


		for ( i = 0; i <= lastChanged; i++ ) {
			MSG_ReadDeltaField( (const byte *)&from[elem], msg, (byte *)&to[elem],
								&msg_hudElemFields[i], qfalse );
		}
		for ( ; (unsigned int)i < MSG_HUDELEM_FIELDS; i++ ) {
			*(int *)( (byte *)&to[elem] + msg_hudElemFields[i].offset ) =
				*(const int *)( (const byte *)&from[elem] + msg_hudElemFields[i].offset );
		}
	}

	memset( &to[used], 0, (size_t)( count - used ) * sizeof( hudelem_t ) );
}

#define PS_STATS_OFFSET        244
#define PS_STATS_COUNT           6
#define PS_STATS_BITS            6
#define PS_STATS_WEAPON_INDEX    3
#define PS_STATS_WEAPON_BITS     6

#define PS_AMMO_OFFSET         268
#define PS_AMMOCLIP_OFFSET     524
#define PS_AMMO_GROUPS           4
#define PS_AMMO_GROUP_SIZE      16

#define PS_OBJECTIVE_OFFSET   1000
#define PS_OBJECTIVE_COUNT      16
#define PS_OBJECTIVE_SIZE       28
#define PS_OBJECTIVE_STATE_BITS  3

#define PS_HUD_CURRENT_OFFSET 1448
#define PS_HUD_ARCHIVE_OFFSET 4920
#define PS_HUD_ELEMS            31
#define PS_HUD_TOTAL_BYTES    6944

/* MSG_WriteDeltaPlayerstate  0x00446830  VERIFIED */
void MSG_WriteDeltaPlayerstate( msg_t *msg, const byte *from, const byte *to ) {
	playerState_t nullState;
	int i, j, lc;
	const netField_t *field;
	const int *fromF, *toF;
	float fullFloat;
	int trunc;
	int v, bits, lowBits;
	int statsbits;
	int ammobits[PS_AMMO_GROUPS];
	int clipbits;


	if ( !from ) {
		from = (const byte *)&nullState;
		memset( &nullState, 0, sizeof( nullState ) );
	}

	lc = 0;
	for ( i = 0, field = msg_playerStateFields; i < MSG_PLAYERSTATE_FIELDS; i++, field++ ) {
		fromF = (const int *)( from + field->offset );
		toF = (const int *)( to + field->offset );
		if ( *fromF != *toF ) {
			lc = i + 1;
		}
	}

	MSG_WriteByte( msg, lc );

	for ( i = 0, field = msg_playerStateFields; i < lc; i++, field++ ) {
		fromF = (const int *)( from + field->offset );
		toF = (const int *)( to + field->offset );

		if ( *fromF == *toF ) {
			MSG_WriteBit0( msg );
			continue;
		}

		MSG_WriteBit1( msg );

		if ( !field->bits ) {
			fullFloat = *(const float *)toF;
			trunc = (int)fullFloat;
			if ( (float)trunc == fullFloat && trunc + MSG_SMALL_FLOAT_BIAS >= 0
				 && trunc + MSG_SMALL_FLOAT_BIAS < MSG_SMALL_FLOAT_RANGE ) {
				MSG_WriteBit0( msg );
				MSG_WriteBits( msg, trunc + MSG_SMALL_FLOAT_BIAS, MSG_SMALL_FLOAT_BITS );
				MSG_WriteByte( msg, ( trunc + MSG_SMALL_FLOAT_BIAS ) >> MSG_SMALL_FLOAT_BITS );
			} else {
				MSG_WriteBit1( msg );
				MSG_WriteLong( msg, *toF );
			}
		} else {
			v = *toF;
			bits = field->bits > 0 ? field->bits : -field->bits;
			lowBits = bits & 7;
			if ( lowBits ) {
				MSG_WriteBits( msg, v, lowBits );
				bits -= lowBits;
				v >>= lowBits;
			}
			while ( bits ) {
				MSG_WriteByte( msg, v & 0xFF );
				v >>= 8;
				bits -= 8;
			}
		}
	}

	statsbits = 0;
	for ( i = 0; i < PS_STATS_COUNT; i++ ) {
		if ( *(const int *)( to + PS_STATS_OFFSET + i * 4 )
			 != *(const int *)( from + PS_STATS_OFFSET + i * 4 ) ) {
			statsbits |= 1 << i;
		}
	}
	if ( statsbits ) {
		MSG_WriteBit1( msg );
		MSG_WriteBits( msg, statsbits, PS_STATS_BITS );
		if ( statsbits & 1 ) {
			MSG_WriteShort( msg, *(const int *)( to + PS_STATS_OFFSET + 0 * 4 ) );
		}
		if ( statsbits & 2 ) {
			MSG_WriteShort( msg, *(const int *)( to + PS_STATS_OFFSET + 1 * 4 ) );
		}
		if ( statsbits & 4 ) {
			MSG_WriteShort( msg, *(const int *)( to + PS_STATS_OFFSET + 2 * 4 ) );
		}
		if ( statsbits & 8 ) {
			MSG_WriteBits( msg, *(const int *)( to + PS_STATS_OFFSET + PS_STATS_WEAPON_INDEX * 4 ),
						   PS_STATS_WEAPON_BITS );
		}
		if ( statsbits & 16 ) {
			MSG_WriteShort( msg, *(const int *)( to + PS_STATS_OFFSET + 4 * 4 ) );
		}
		if ( statsbits & 32 ) {
			MSG_WriteByte( msg, *(const int *)( to + PS_STATS_OFFSET + 5 * 4 ) );
		}
	} else {
		MSG_WriteBit0( msg );
	}

	for ( j = 0; j < PS_AMMO_GROUPS; j++ ) {
		ammobits[j] = 0;
		for ( i = 0; i < PS_AMMO_GROUP_SIZE; i++ ) {
			if ( *(const int *)( to + PS_AMMO_OFFSET + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 )
				 != *(const int *)( from + PS_AMMO_OFFSET + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 ) ) {
				ammobits[j] |= 1 << i;
			}
		}
	}
	if ( !ammobits[0] && !ammobits[1] && !ammobits[2] && !ammobits[3] ) {
		MSG_WriteBit0( msg );
	} else {
		MSG_WriteBit1( msg );
		for ( j = 0; j < PS_AMMO_GROUPS; j++ ) {
			if ( ammobits[j] ) {
				MSG_WriteBit1( msg );
				MSG_WriteShort( msg, ammobits[j] );
				for ( i = 0; i < PS_AMMO_GROUP_SIZE; i++ ) {
					if ( ammobits[j] & ( 1 << i ) ) {
						MSG_WriteShort( msg, *(const int *)( to + PS_AMMO_OFFSET
															 + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 ) );
					}
				}
			} else {
				MSG_WriteBit0( msg );
			}
		}
	}

	for ( j = 0; j < PS_AMMO_GROUPS; j++ ) {
		clipbits = 0;
		for ( i = 0; i < PS_AMMO_GROUP_SIZE; i++ ) {
			if ( *(const int *)( to + PS_AMMOCLIP_OFFSET + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 )
				 != *(const int *)( from + PS_AMMOCLIP_OFFSET + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 ) ) {
				clipbits |= 1 << i;
			}
		}
		if ( clipbits ) {
			MSG_WriteBit1( msg );
			MSG_WriteShort( msg, clipbits );
			for ( i = 0; i < PS_AMMO_GROUP_SIZE; i++ ) {
				if ( clipbits & ( 1 << i ) ) {
					MSG_WriteShort( msg, *(const int *)( to + PS_AMMOCLIP_OFFSET
														 + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 ) );
				}
			}
		} else {
			MSG_WriteBit0( msg );
		}
	}

	if ( memcmp( from + PS_OBJECTIVE_OFFSET, to + PS_OBJECTIVE_OFFSET,
				 PS_OBJECTIVE_COUNT * PS_OBJECTIVE_SIZE ) == 0 ) {
		MSG_WriteBit0( msg );
	} else {
		MSG_WriteBit1( msg );
		for ( i = 0; i < PS_OBJECTIVE_COUNT; i++ ) {
			MSG_WriteBits( msg, *(const int *)( to + PS_OBJECTIVE_OFFSET + i * PS_OBJECTIVE_SIZE ),
						   PS_OBJECTIVE_STATE_BITS );
			MSG_WriteDeltaFields_0( qfalse, msg, from + PS_OBJECTIVE_OFFSET + i * PS_OBJECTIVE_SIZE,
									to + PS_OBJECTIVE_OFFSET + i * PS_OBJECTIVE_SIZE,
									MSG_OBJECTIVE_FIELDS, msg_objectiveFields );
		}
	}

	if ( memcmp( from + PS_HUD_CURRENT_OFFSET, to + PS_HUD_CURRENT_OFFSET,
				 PS_HUD_TOTAL_BYTES ) == 0 ) {
		MSG_WriteBit0( msg );
	} else {
		MSG_WriteBit1( msg );
		MSG_WriteDeltaHudElems( msg, (const hudelem_t *)( from + PS_HUD_ARCHIVE_OFFSET ),
								(const hudelem_t *)( to + PS_HUD_ARCHIVE_OFFSET ), PS_HUD_ELEMS );
		MSG_WriteDeltaHudElems( msg, (const hudelem_t *)( from + PS_HUD_CURRENT_OFFSET ),
								(const hudelem_t *)( to + PS_HUD_CURRENT_OFFSET ), PS_HUD_ELEMS );
	}
}

/* MSG_ReadDeltaPlayerstate  0x00447860  VERIFIED */
void MSG_ReadDeltaPlayerstate( msg_t *msg, const byte *from, byte *to ) {
	playerState_t nullState;
	int i, j, lc;
	int bits, lowBits, bit;
	const netField_t *field;
	const int *fromF;
	int *toF;
	qboolean print;
	int trunc, v;


	if ( !from ) {
		from = (const byte *)&nullState;
		memset( &nullState, 0, sizeof( nullState ) );
	}

	memcpy( to, from, sizeof( playerState_t ) );

	if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -2 ) ) {
		print = qtrue;
		Com_Printf( "%3i: playerstate ", msg->readcount );
	} else {
		print = qfalse;
	}

	lc = MSG_ReadByte( msg );
	/* Tree-only clamp: retail 0x004478F7 loops lastChanged (up to 255) times
	 * over the 103-entry field table, reading past it on a malformed packet,
	 * and an overread lc of -1 starts its remainder copy at table[-1].
	 * Those out-of-bounds reads depend on data layout and are not reproduced. */

	for ( i = 0, field = msg_playerStateFields; i < lc; i++, field++ ) {
		fromF = (const int *)( from + field->offset );
		toF = (int *)( to + field->offset );

		if ( !MSG_ReadBit( msg ) ) {
			*toF = *fromF;
		} else {
			if ( field->bits == 0 ) {
				if ( MSG_ReadBit( msg ) == 0 ) {
					trunc = MSG_ReadBits( msg, MSG_SMALL_FLOAT_BITS );
					trunc += ( MSG_ReadByte( msg ) - ( MSG_SMALL_FLOAT_BIAS >> MSG_SMALL_FLOAT_BITS ) )
							 << MSG_SMALL_FLOAT_BITS;
					*(float *)toF = (float)trunc;
					if ( print ) {
						Com_Printf( "%s:%i ", field->name, trunc );
					}
				} else {
					*toF = MSG_ReadLong( msg );
					if ( print ) {
						Com_Printf( "%s:%f ", field->name, *(float *)toF );
					}
				}
			} else {
				bits = field->bits >= 0 ? field->bits : -field->bits;
				lowBits = bits & 7;
				v = lowBits ? MSG_ReadBits( msg, lowBits ) : 0;
				for ( bit = lowBits; bit < bits; bit += 8 ) {
					v |= MSG_ReadByte( msg ) << bit;
				}
				*toF = v;
				if ( print ) {
					Com_Printf( "%s:%i ", field->name, *toF );
				}
			}
		}
	}
	for ( i = lc, field = &msg_playerStateFields[lc]; i < MSG_PLAYERSTATE_FIELDS; i++, field++ ) {
		fromF = (const int *)( from + field->offset );
		toF = (int *)( to + field->offset );
		*toF = *fromF;
	}

	if ( MSG_ReadBit( msg ) ) {
		if ( cl_shownet && cl_shownet->integer == 4 ) {   /* retail 0x00447B4D */
			Com_Printf( "%s ", "PS_STATS" );
		}
		bits = MSG_ReadBits( msg, PS_STATS_BITS );
		if ( bits & 1 ) {
			*(int *)( to + PS_STATS_OFFSET + 0 * 4 ) = MSG_ReadShort( msg );
		}
		if ( bits & 2 ) {
			*(int *)( to + PS_STATS_OFFSET + 1 * 4 ) = MSG_ReadShort( msg );
		}
		if ( bits & 4 ) {
			*(int *)( to + PS_STATS_OFFSET + 2 * 4 ) = MSG_ReadShort( msg );
		}
		if ( bits & 8 ) {
			*(int *)( to + PS_STATS_OFFSET + PS_STATS_WEAPON_INDEX * 4 ) =
				MSG_ReadBits( msg, PS_STATS_WEAPON_BITS );
		}
		if ( bits & 16 ) {
			*(int *)( to + PS_STATS_OFFSET + 4 * 4 ) = MSG_ReadShort( msg );
		}
		if ( bits & 32 ) {
			*(int *)( to + PS_STATS_OFFSET + 5 * 4 ) = MSG_ReadByte( msg );
		}
	}

	if ( MSG_ReadBit( msg ) ) {
		for ( j = 0; j < PS_AMMO_GROUPS; j++ ) {
			if ( MSG_ReadBit( msg ) ) {
				if ( cl_shownet && cl_shownet->integer == 4 ) {   /* retail 0x00447CFE */
					Com_Printf( "%s ", "PS_AMMO" );
				}
				bits = MSG_ReadShort( msg );
				for ( i = 0; i < PS_AMMO_GROUP_SIZE; i++ ) {
					if ( bits & ( 1 << i ) ) {
						*(int *)( to + PS_AMMO_OFFSET + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 ) =
							MSG_ReadShort( msg );
					}
				}
			}
		}
	}

	for ( j = 0; j < PS_AMMO_GROUPS; j++ ) {
		if ( MSG_ReadBit( msg ) ) {
			if ( cl_shownet && cl_shownet->integer == 4 ) {   /* retail 0x00447F92 */
				Com_Printf( "%s ", "PS_AMMOCLIP" );
			}
			bits = MSG_ReadShort( msg );
			for ( i = 0; i < PS_AMMO_GROUP_SIZE; i++ ) {
				if ( bits & ( 1 << i ) ) {
					*(int *)( to + PS_AMMOCLIP_OFFSET + ( j * PS_AMMO_GROUP_SIZE + i ) * 4 ) =
						MSG_ReadShort( msg );
				}
			}
		}
	}

	if ( MSG_ReadBit( msg ) ) {
		for ( i = 0; i < PS_OBJECTIVE_COUNT; i++ ) {
			*(int *)( to + PS_OBJECTIVE_OFFSET + i * PS_OBJECTIVE_SIZE ) =
				MSG_ReadBits( msg, PS_OBJECTIVE_STATE_BITS );
			MSG_ReadDeltaFields( msg, MSG_OBJECTIVE_FIELDS,
								 from + PS_OBJECTIVE_OFFSET + i * PS_OBJECTIVE_SIZE,
								 to + PS_OBJECTIVE_OFFSET + i * PS_OBJECTIVE_SIZE,
								 msg_objectiveFields );
		}
	}

	if ( MSG_ReadBit( msg ) ) {
		MSG_ReadDeltaHudElems( msg, (const hudelem_t *)( from + PS_HUD_ARCHIVE_OFFSET ),
							   (hudelem_t *)( to + PS_HUD_ARCHIVE_OFFSET ), PS_HUD_ELEMS );
		MSG_ReadDeltaHudElems( msg, (const hudelem_t *)( from + PS_HUD_CURRENT_OFFSET ),
							   (hudelem_t *)( to + PS_HUD_CURRENT_OFFSET ), PS_HUD_ELEMS );
	}
}
