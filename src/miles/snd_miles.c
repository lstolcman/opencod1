/*
 * miles/snd_miles.c
 *
 * Retail range 0x0044B930-0x00451960, 102 functions.
 *
 * @fidelity: likely
 */

#include "../qcommon/qcommon.h"
#include "mss32.h"

/* ---- miles/MSS sound globals ---- */
int mss_q3fs;
int mss_roomtype;
int mss_volume;
int mss_wetlevel;
int mss_3d_provider;                /* 0x01651574 */
int (*mss_bits)();                  /* 0x01651568 */
int mss_khz;                        /* 0x0165156C */
int mss_stereo;                     /* 0x01651560 */
cvar_t *mss_errorOnMissing;         /* 0x01651564, read by com_sndalias.c */


extern void *Com_PickSoundAlias( const char *name, int source );
extern int Com_UnloadSoundAliasSounds();
/* universal/com_sndalias.c 0x004340B0.  Declared locally rather than by
   including universal/com_sndalias.h. */
void Com_UnloadSoundAliases( int source );
extern int FS_FOpenFileRead_Internal();
extern int Hunk_AllocAlignInternal();
extern int j__atol();
extern int __cdecl FastRound( float f );   /* client_mp/cl_console_mp.c 0x00408370 */
vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 );   /* com_math.c (/Op), LTCG-inlined */
#ifdef COD1_VC7
double Distance( const vec3_t p1, const vec3_t p2 );         /* com_math.c (vec_t, /Op), LTCG-inlined; double here: MSS_StartAlias3DSample re-stores it (0x0044CC5D) */
#else
vec_t Distance( const vec3_t p1, const vec3_t p2 );
#endif

__declspec( dllimport ) unsigned long __stdcall timeGetTime( void );

#define MSS_CVAR( g )   ( (cvar_t *)(void *)( g ) )

#define MSS_MAX_3D              32
#define MSS_MAX_STREAM          13
#define MSS_MAX_2D              32
#define MSS_FIRST_3D            0
#define MSS_FIRST_STREAM        32
#define MSS_FIRST_2D            45
#define MSS_NUM_CHANNELS        77      /* 0x8E0890 .. 0x8E1968 */
#define MSS_NUM_BACKGROUND      5
#define MSS_FIRST_FREE_STREAM   5
#define MSS_NUM_ALIAS_CHANNELS  10

/* the <windef.h> min / max macros */
#define MSS_MIN( a, b )     ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#define MSS_MAX( a, b )     ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

#define MSS_NO_ENTITY           0x400

/* Every retail float-to-int conversion modelled by MSS_FTOL is FastRound
 * 0x00408370 (14 inlined copies in this range, e.g. 0x0044CA4F-0x0044CA5B,
 * plus direct calls at 0x00450DD6 / 0x0045106D / 0x004510B4): the value is
 * stored to a float, reloaded, 2^-30 is added and fistp rounds to nearest.
 * No site uses the truncating __ftol2 of an (int) cast. */
#define MSS_FTOL( f )           FastRound( (float)( f ) )

/* An int converted for float arithmetic: retail's (float) conversion gives
 * fild x; fmul/fdivr ..., unrounded on the x87 under VC7 (a (double) cast
 * emits fld y; fimul x instead); the modern build keeps double. */
#ifdef COD1_VC7
#define MSS_I2F( x )            ( (float)( x ) )
#else
#define MSS_I2F( x )            ( (double)( x ) )
#endif

typedef struct {
	int      entnum;        /* +0x00  0..1023, or MSS_NO_ENTITY             */
	int      channel;       /* +0x04  alias channel enum, doubles as the
	                                  replacement priority                  */
	int      touchTime;     /* +0x08  frame stamp a looping owner refreshes */
	int      endTime;       /* +0x0C  when this sound runs out              */
	float    volume;        /* +0x10                                        */
	int      baseRate;      /* +0x14  the sound file's own sample rate      */
	float    pitch;         /* +0x18                                        */
	void    *alias;         /* +0x1C                                        */
	void    *alias2;        /* +0x20  the far end of a blend                */
	float    blend;         /* +0x24  0 = alias, 1 = alias2                 */
	vec3_t   localOrigin;   /* +0x28  offset in the entity's own frame      */
	int      paused;        /* +0x34                                        */
} mssChannel_t;
COD1_ASSERT_SIZE( mssChannel_t, 0x38 );

typedef struct {
	int      spatialized;   /* +0x00 */
	vec3_t   origin;        /* +0x04 */
} mssStreamInfo_t;

typedef struct {
	float    target;
	float    rate;
} mssBackgroundFade_t;

/* The Miles state block 0x008E0760..0x008E1BD0, one global as in retail
   (MSS_Shutdown clears it with rep stosd).  The stream handles of channels
   32..44 are addressed from the 3D table's base (mss_3DHandles + chan). */
static struct {
	int                 digitalDriver;                         /* 0x008E0760 */
	int                 provider3D;                            /* 0x008E0764 */
	int                 driverRate;                            /* 0x008E0768 */
	int                 driverBits;                            /* 0x008E076C */
	int                 driverChannels;                        /* 0x008E0770 */
	float               timeScale;                             /* 0x008E0774 */
	int                 paused;                                /* 0x008E0778 */
	int                 pauseTime;                             /* 0x008E077C */
	int                 cpuPercent;                            /* 0x008E0780 */
	void               *pendingRestore;                        /* 0x008E0784 */
	int                 pendingRestoreSize;                    /* 0x008E0788 */
	float               masterVolume;                          /* 0x008E078C */
	float               globalFade[3];                         /* 0x008E0790 cur, target, rate */
	float               channelFade[ MSS_NUM_ALIAS_CHANNELS ][3];/* 0x008E079C */
	mssBackgroundFade_t bgFade[ MSS_NUM_BACKGROUND ];          /* 0x008E0814 */
	int                 ambientSlot;                           /* 0x008E083C */
	int                 roomType;                              /* 0x008E0840 */
	float               wetLevel;                              /* 0x008E0844 */
	float               wetTarget;                             /* 0x008E0848 */
	float               wetRate;                               /* 0x008E084C */
	vec3_t              listenerOrigin;                        /* 0x008E0850 */
	vec3_t              listenerAxis[3];                       /* 0x008E085C */
	int                 listenerEntnum;                        /* 0x008E0880 */
	int                 time;                                  /* 0x008E0884 */
	int                 loopFrameTime;                         /* 0x008E0888 */
	int                 anyMasters;                            /* 0x008E088C */
	mssChannel_t        channels[ MSS_NUM_CHANNELS ];          /* 0x008E0890 */
	int                 handles2D[ MSS_MAX_2D ];               /* 0x008E1968 */
	int                 handles3D[ MSS_MAX_3D ];               /* 0x008E19E8 */
	int                 handlesStream[ MSS_MAX_STREAM ];       /* 0x008E1A68 */
	mssStreamInfo_t     streamInfo[ MSS_MAX_STREAM ];          /* 0x008E1A9C */
	int                 num2DChannels;                         /* 0x008E1B6C */
	int                 max3DChannels;                         /* 0x008E1B70 */
	int                 numStreamChannels;                     /* 0x008E1B74 */
	int                 rawSampleHandle;                       /* 0x008E1B78 */
	int                 rawSampleRate;                         /* 0x008E1B7C */
	int                 rawSampleWidthBytes;                   /* 0x008E1B80 */
	int                 rawChannelCount;                       /* 0x008E1B84 */
	void               *rawRingBuffer;                         /* 0x008E1B88 */
	char                rawSegmentReady[32];                   /* 0x008E1B8C */
	int                 rawSegmentSize;                        /* 0x008E1BAC */
	int                 rawWriteSegmentOffset;                 /* 0x008E1BB0 */
	int                 rawReadSegmentIndex;                   /* 0x008E1BB4 */
	int                 rawWriteSegmentIndex;                  /* 0x008E1BB8 */
	double              rawSampleTimeBaseMsec;                 /* 0x008E1BC0 */
	double              rawSampleTimePerByteMsec;              /* 0x008E1BC8 */
} mss_state;

#define mss_digitalDriver            mss_state.digitalDriver
#define mss_3DProvider               mss_state.provider3D
#define mss_driverRate               mss_state.driverRate
#define mss_driverBits               mss_state.driverBits
#define mss_driverChannels           mss_state.driverChannels
#define mss_timeScale                mss_state.timeScale
#define mss_paused                   mss_state.paused
#define mss_pauseTime                mss_state.pauseTime
#define mss_cpuPercent               mss_state.cpuPercent
#define mss_pendingRestore           mss_state.pendingRestore
#define mss_pendingRestoreSize       mss_state.pendingRestoreSize
#define mss_masterVolume             mss_state.masterVolume
#define mss_globalFade               mss_state.globalFade
#define mss_globalFadeCur            mss_state.globalFade[0]
#define mss_globalFadeTarget         mss_state.globalFade[1]
#define mss_globalFadeRate           mss_state.globalFade[2]
#define mss_channelFade              mss_state.channelFade
#define mss_bgFade                   mss_state.bgFade
#define mss_ambientSlot              mss_state.ambientSlot
#define mss_roomType                 mss_state.roomType
#define mss_wetLevel                 mss_state.wetLevel
#define mss_wetTarget                mss_state.wetTarget
#define mss_wetRate                  mss_state.wetRate
#define mss_listenerOrigin           mss_state.listenerOrigin
#define mss_listenerAxis             mss_state.listenerAxis
#define mss_listenerEntnum           mss_state.listenerEntnum
#define mss_time                     mss_state.time
#define mss_loopFrameTime            mss_state.loopFrameTime
#define mss_anyMasters               mss_state.anyMasters
#define mss_channels                 mss_state.channels
#define mss_2DHandles                mss_state.handles2D
#define mss_3DHandles                mss_state.handles3D
#define mss_streamHandles            mss_state.handlesStream
#define mss_streamInfo               mss_state.streamInfo
#define mss_num2DChannels            mss_state.num2DChannels
#define mss_max3DChannels            mss_state.max3DChannels
#define mss_numStreamChannels        mss_state.numStreamChannels
#define mss_rawSampleHandle          mss_state.rawSampleHandle
#define mss_rawSampleRate            mss_state.rawSampleRate
#define mss_rawSampleWidthBytes      mss_state.rawSampleWidthBytes
#define mss_rawChannelCount          mss_state.rawChannelCount
#define mss_rawRingBuffer            mss_state.rawRingBuffer
#define mss_rawSegmentReady          mss_state.rawSegmentReady
#define mss_rawSegmentSize           mss_state.rawSegmentSize
#define mss_rawWriteSegmentOffset    mss_state.rawWriteSegmentOffset
#define mss_rawReadSegmentIndex      mss_state.rawReadSegmentIndex
#define mss_rawWriteSegmentIndex     mss_state.rawWriteSegmentIndex
#define mss_rawSampleTimeBaseMsec    mss_state.rawSampleTimeBaseMsec
#define mss_rawSampleTimePerByteMsec mss_state.rawSampleTimePerByteMsec

#define MSS_2D_HANDLE( c )      mss_2DHandles[ (c) - MSS_FIRST_2D ]
#define MSS_3D_HANDLE( c )      mss_3DHandles[ (c) ]
#define MSS_STREAM_HANDLE( c )  mss_streamHandles[ (c) - MSS_FIRST_STREAM ]
#define MSS_STREAM_INFO( c )    mss_streamInfo[ (c) - MSS_FIRST_STREAM ]

#define AL_NAME( a )        ( *(const char **)( (char *)(a) +  0 ) )
#define AL_FILENAME( a )    ( *(const char **)( (char *)(a) +  4 ) )
#define AL_SOUND( a )       ( *(int **)       ( (char *)(a) + 12 ) )
#define AL_VOLMIN( a )      ( *(float *)      ( (char *)(a) + 20 ) )
#define AL_VOLMAX( a )      ( *(float *)      ( (char *)(a) + 24 ) )
#define AL_PITCHMIN( a )    ( *(float *)      ( (char *)(a) + 28 ) )
#define AL_PITCHMAX( a )    ( *(float *)      ( (char *)(a) + 32 ) )
#define AL_MINDIST( a )     ( *(float *)      ( (char *)(a) + 36 ) )
#define AL_MAXDIST( a )     ( *(float *)      ( (char *)(a) + 40 ) )
#define AL_CHANNEL( a )     ( *(int *)        ( (char *)(a) + 44 ) )
#define AL_TYPE( a )        ( *(int *)        ( (char *)(a) + 48 ) )
#define AL_LOOPING( a )     ( *(unsigned char *)( (char *)(a) + 52 ) )
#define AL_MASTER( a )      ( *(unsigned char *)( (char *)(a) + 53 ) )
#define AL_DUCKS( a )       ( *(unsigned char *)( (char *)(a) + 54 ) )
#define AL_STREAMFOUND( a ) ( *(unsigned char *)( (char *)(a) + 55 ) )
#define AL_DUCKVOL( a )     ( *(float *)      ( (char *)(a) + 56 ) )

/* Sound alias layout (com_sndalias.c's snd_alias_t).  VC7 emits retail's
   fld [a] / fcomp [b] only for fields read through an uncast struct pointer;
   the cast-and-offset AL_ macros give fld / fld / fcompp. */
typedef struct mssAlias_s {
	const char *name;           /* +0x00 */
	const char *file;           /* +0x04 */
	const char *subtitle;       /* +0x08 */
	int        *soundData;      /* +0x0C */
	int         pickSequence;   /* +0x10 */
	float       volMin;         /* +0x14 */
	float       volMax;         /* +0x18 */
	float       pitchMin;       /* +0x1C */
	float       pitchMax;       /* +0x20 */
	float       distMin;        /* +0x24 */
	float       distMax;        /* +0x28 */
	int         channel;        /* +0x2C */
	int         type;           /* +0x30 */
	unsigned char loop;         /* +0x34 */
	unsigned char isMaster;     /* +0x35 */
	unsigned char isSlave;      /* +0x36 */
	unsigned char streamExists; /* +0x37 */
	float       slavePercent;   /* +0x38 */
} mssAlias_t;

#define AL_TYPE_SAMPLE      1
#define AL_TYPE_STREAM      2

#define SND_FORMAT( s )     ( ((int *)(s))[0] )
#define SND_DATA( s )       ( *(const void **)&((int *)(s))[1] )
#define SND_DATALEN( s )    ( ((unsigned int *)(s))[2] )
#define SND_RATE( s )       ( ((unsigned int *)(s))[3] )
#define SND_BITS( s )       ( ((int *)(s))[4] )
#define SND_CHANNELS( s )   ( ((int *)(s))[5] )
#define SND_SAMPLES( s )    ( ((unsigned int *)(s))[6] )
#define SND_BLOCKSIZE( s )  ( ((unsigned int *)(s))[7] )


static int   MSS_UpdateBackgroundVolume( int slot, int msec );
static void  MSS_SpatializeStream( float *volume, int slot, float *pan );
static int   MSS_FindReplacableChannel( int entnum, int priority, int first, int count );
static void *MSS_StartBackground( void *alias, int slot, int msec );

typedef struct {
	const char *name;
	float       requested;
	float       actual;
	int         baseRate;
	float       rateScale;
} mssOverlay_t;

void MSS_PauseSounds( void );
void MSS_UnpauseSounds( void );
void MSS_UpdatePause( void );
void  MSS_StopSounds( int flags );
void MSS_Update( void );
void MSS_Restore( int buffer, int size );

extern char *FS_ShortOSFilePath( const char *fullpath );

/* ---- MSS_Alloc  0x0044B930 ---- CONFIRMED */
void __cdecl MSS_Alloc( void *size )
{
	/* Miles memory-alloc callback: round the requested size up to a multiple of
	   4 and hand it to the hunk allocator.  The result is returned in EAX by the
	   tail call (the recovered prototype is void, so it is left implicit). */
	Hunk_AllocAlignInternal( ( (unsigned int)size + 3 ) & 0xFFFFFFFC, 32 );
}

/* ---- MSS_Free  0x0044B940 ---- CONFIRMED */
void MSS_Free()
{
  ;
}

/* ---- MSS_FileOpenCallback  0x0044B950 ---- CONFIRMED */
int __stdcall MSS_FileOpenCallback( char *filename, fileHandle_t *file )
{
	int len = FS_FOpenFileRead_Internal( filename, file, qtrue, 1 );

	return ( len == -1 ) ? 0 : len;
}

/* ---- MSS_FileCloseCallback  0x0044B980 ---- CONFIRMED */
void __stdcall MSS_FileCloseCallback(fileHandle_t f)
{
  FS_FCloseFile(f);
}

/* ---- MSS_FileSeekCallback  0x0044B990 ---- */
int __stdcall MSS_FileSeekCallback( fileHandle_t f, int offset, int type )
{
	int origin;

	switch ( type ) {
	case AIL_FILE_SEEK_BEGIN:
		origin = FS_SEEK_SET;
		break;
	case AIL_FILE_SEEK_CURRENT:
		origin = FS_SEEK_CUR;
		break;
	case AIL_FILE_SEEK_END:
		origin = FS_SEEK_END;
		break;
	default:
		return 0;
	}

	FS_Seek( f, offset, origin );
	return FS_FTell( f );
}

/* ---- MSS_FileReadCallback  0x0044BA10 ---- CONFIRMED */
int __stdcall MSS_FileReadCallback(fileHandle_t f, void *Buffer, size_t ElementCount)
{
  return FS_Read(Buffer, ElementCount, f);
}

/* ---- MSS_Init2D  0x0044BA30 ---- */
int MSS_Init2D( void )
{
	int             rate;           /* ebp -- Hz                       */
	int             bytes;          /* edi -- bytes per sample, 1 or 2 */
	int             channels;       /* ebx -- 1 mono, 2 stereo         */
	const char     *channelName;

	switch ( MSS_CVAR( mss_khz )->integer ) {
	case 11:
		rate = 11025;               /* 0x2B11 */
		break;
	case 44:
		rate = 44100;               /* 0xAC44 */
		break;
	default:
		Com_Printf( "invalid value %i for mss_khz, using 22 khz instead\n",
					MSS_CVAR( mss_khz )->integer );
		/* fall through: retail duplicates the 22 kHz store into both paths */
	case 22:
		rate = 22050;               /* 0x5622 */
		break;
	}

	if ( MSS_CVAR( mss_bits )->integer != 8 ) {
		if ( MSS_CVAR( mss_bits )->integer != 16 ) {
			Com_Printf( "invalid value %i for mss_bits (should be 8 or 16), "
				"using 16 instead\n", MSS_CVAR( mss_bits )->integer );
		}
		bytes = 2;
	} else { bytes = 1; }

	channels = MSS_CVAR( mss_stereo )->integer ? 2 : 1;
	channelName = ( channels == 2 ) ? "stereo" : "mono";

	Com_Printf( "Attempting %i kHz %i bit %s sound\n",
				rate / 1000, 8 * bytes, channelName );

	AIL_set_preference( 1, 77 );
	mss_digitalDriver = AIL_open_digital_driver( rate, bytes, channels, 0 );

	if ( !mss_digitalDriver ) {
		Com_Printf( "couldn't initialize 2D provider: %s\n", AIL_last_error() );
		return 0;
	}

	mss_num2DChannels     = 32;     /* 0x008E1B6C */
	mss_numStreamChannels = 13;     /* 0x008E1B74 */
	mss_driverRate        = rate;   /* 0x008E0768 */
	mss_driverBits        = 8 * bytes;
	mss_driverChannels    = channels;
	mss_timeScale         = 1.0f;

	/* Retail 0x0044BB1A pushes only the format (push 55FC48h, then
	 * add esp,4 at 0x0044BB54), with no varargs after it.  The three %i
	 * therefore read the next stack dwords: the edi, esi and ebp this
	 * function's prologue saved, which still hold MSS_Init's caller's
	 * values (0x0040FE98 / 0x0041161C) because MSS_Init has not changed
	 * them by its call at 0x0044FFA8.  The printed numbers are
	 * nondeterministic garbage, so no fixed value can stand in for them. */
	Com_Printf( "2D provider initialized at %i %i %i\n" );
	return 1;
}

/* ---- MSS_Init3DProvider  0x0044BB70 ---- CONFIRMED */
int __cdecl MSS_Init3DProvider( char *requestedName, int verbose )
{
	char *name;
	int   enumHandle;
	int   providerId;
	int   requestedId;
	int   fallbackId;      /* "Miles Fast 2D Positional Audio" */

	if ( verbose ) {
		Com_Printf( "available 3D providers:\n" );
	}

	requestedId = 0;
	fallbackId = 0;
	enumHandle = 0;
	while ( AIL_enumerate_3D_providers( &enumHandle, &providerId, &name ) ) {
		if ( verbose ) {
			Com_Printf( "  %s\n", name );
		}
		if ( !_stricmp( name, requestedName ) ) {
			requestedId = providerId;
		}
		if ( !_stricmp( name, "Miles Fast 2D Positional Audio" ) ) {
			fallbackId = providerId;
		}
	}

	mss_3DProvider = 0;

	if ( requestedId ) {
		if ( !AIL_open_3D_provider( requestedId ) ) {
			Com_Printf( "using 3D provider '%s'\n", requestedName );
			mss_3DProvider = requestedId;
		} else {
			Com_Printf( "couldn't open 3D provider '%s': %s\n",
						requestedName, AIL_last_error() );
		}
	}

	if ( !mss_3DProvider ) {
		/* requested provider unavailable or failed to open -- fall back to the
		   "Miles Fast 2D Positional Audio" software provider */
		if ( !fallbackId || fallbackId == requestedId ) {
			return 0;
		}
		if ( *requestedName
			&& _stricmp( requestedName, "Miles Fast 2D Positional Audio" ) ) {
			Com_Printf( "trying to use 'Miles Fast 2D Positional Audio' instead of '%s'\n",
						requestedName );
		}
		if ( !AIL_open_3D_provider( fallbackId ) ) {
			Com_Printf( "using 3D provider '%s'\n", "Miles Fast 2D Positional Audio" );
			mss_3DProvider = fallbackId;
			Cvar_Set2( "mss_3d_provider", "Miles Fast 2D Positional Audio", qtrue );
		} else {
			Com_Printf( "couldn't open 3D provider '%s': %s\n",
						"Miles Fast 2D Positional Audio", AIL_last_error() );
		}
		if ( !mss_3DProvider ) {
			return 0;
		}
	}

	AIL_3D_provider_attribute( mss_3DProvider, "Maximum supported samples",
							   &mss_max3DChannels );
	if ( mss_max3DChannels > 32 ) {
		mss_max3DChannels = 32;
	}
	Com_Printf( "%i max 3D channels\n", mss_max3DChannels );
	AIL_set_3D_distance_factor( mss_3DProvider, 0.0254f );
	return 1;
}

/* ---- MSS_SetListener  0x0044BD60 ---- */
void __cdecl MSS_SetListener( int *axis, int *origin, int entnum )
{
	int i;

	if ( !mss_digitalDriver ) {
		return;
	}
	for ( i = 0 ; i < 9 ; i++ ) {
		((float *)mss_listenerAxis)[i] = *(float *)&axis[i];
	}
	for ( i = 0 ; i < 3 ; i++ ) {
		mss_listenerOrigin[i] = *(float *)&origin[i];
	}
	mss_listenerEntnum = entnum;
}

/* ---- MSS_InitChannels  0x0044BDE0 ---- */
int MSS_InitChannels( void )
{
	int i;

	/* Retail 0x0044BDE0 does not clamp the counts here: MSS_Init2D
	 * sets 32 / 13 and MSS_Init3DProvider clamps the 3D count to 32. */
	for ( i = 0 ; i < mss_num2DChannels ; i++ ) {
		mss_2DHandles[i] = AIL_allocate_sample_handle( mss_digitalDriver );
		if ( !mss_2DHandles[i] ) {
			Com_Error( ERR_DROP,
				"\x15MILES 2D sound sample allocation failed on channel %i\n", i + 1 );
		}
	}

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		mss_3DHandles[i] = AIL_allocate_3D_sample_handle( mss_3DProvider );
		if ( !mss_3DHandles[i] ) {
			Com_Error( ERR_DROP,
				"\x15MILES 3D sound sample allocation failed on channel %i\n", i + 1 );
		}
	}

	mss_ambientSlot = 2;
	return mss_max3DChannels;
}

/* ---- MSS_Attenuate  0x0044BE80 ---- VERIFIED */
float __cdecl MSS_Attenuate( float distance, float minDist, float maxDist )
{
	rfloat delta = distance - minDist;
	rfloat frac;

	if ( delta <= 0.0f ) {
		return 1.0f;
	}
	frac = delta / ( maxDist - minDist );
	if ( frac >= 1.0f ) {
		return 0.0f;
	}
	return 1.0f - frac;
}

/* ---- MSS_GetCurrent3DPosition  0x0044BED0 ---- */
void __cdecl MSS_GetCurrent3DPosition( int entnum, const float *localOffset, float *out )
{
	vec3_t origin;
	vec3_t axis[3];

	VM_Call( cgvm, 13 , entnum, origin, axis );

	VectorMA( origin, localOffset[0], axis[0], origin );
	VectorMA( origin, localOffset[1], axis[1], origin );
	VectorMA( origin, localOffset[2], axis[2], origin );
	VectorCopy( origin, out );
}

/* ---- MSS_Set3DPosition  0x0044BF70 ---- */
void __cdecl MSS_Set3DPosition( int handle, const float *origin )
{
	vec3_t d;
	vec3_t local;   /* forward, left, up */

	VectorSubtract( origin, mss_listenerOrigin, d );

	local[0] = DotProduct( mss_listenerAxis[0], d );
	local[1] = DotProduct( mss_listenerAxis[1], d );
	local[2] = DotProduct( mss_listenerAxis[2], d );

	AIL_set_3D_position( handle, -local[1], local[2], local[0] );
}

/* ---- MSS_IsAliasChannel3D  0x0044C020 ---- HIGH */
int __cdecl MSS_IsAliasChannel3D( int aliasChan )
{
	return aliasChan != 1 && ( aliasChan <= 5 || aliasChan > 9 );
}

/* ---- MSS_SetChannelInfo  0x0044C040 ---- */
static void MSS_SetChannelInfo( int entnum, const float *origin, void *alias, int chan,
                                void *alias2, float blend, float volume, float pitch,
                                int baseRate, int lengthMs, int offsetMs )
{
	if ( entnum >= 0 && entnum < MSS_NO_ENTITY
		&& MSS_IsAliasChannel3D( AL_CHANNEL( alias ) ) ) {
		vec3_t entOrigin;
		float  axis[9];
#ifdef COD1_VC7
		vec3_t delta;
		vec3_t org;

		VM_Call( cgvm, 13, entnum, entOrigin, axis );

		/* Retail 0x0044C095-0x0044C115: VC7 keeps delta[1] and delta[2] on
		 * the x87 stack (delta[0] and org[2] are stored) and sums each row
		 * z, y, then x. */
		VectorCopy( origin, org );
		VectorSubtract( org, entOrigin, delta );
		mss_channels[chan].localOrigin[0] = DotProduct( delta, &axis[0] );
		mss_channels[chan].localOrigin[1] = DotProduct( delta, &axis[3] );
		mss_channels[chan].localOrigin[2] = DotProduct( delta, &axis[6] );
#else
		float  d0;
		double d1, d2;

		VM_Call( cgvm, 13, entnum, entOrigin, axis );

		/* Retail's precision: d0 is rounded to a float, d1 and d2 are not,
		 * and each row sums z, y, then x. */
		d0 = origin[0] - entOrigin[0];
		d1 = origin[1] - entOrigin[1];
		d2 = origin[2] - entOrigin[2];

		mss_channels[chan].localOrigin[0] = axis[2] * d2 + axis[1] * d1 + axis[0] * d0;
		mss_channels[chan].localOrigin[1] = axis[5] * d2 + axis[4] * d1 + axis[3] * d0;
		mss_channels[chan].localOrigin[2] = axis[8] * d2 + axis[7] * d1 + axis[6] * d0;
#endif
	} else {
		VectorClear( mss_channels[chan].localOrigin );
	}

	mss_channels[chan].entnum    = entnum;
	mss_channels[chan].channel   = AL_CHANNEL( alias );
	mss_channels[chan].volume    = volume;
	mss_channels[chan].pitch     = pitch;
	mss_channels[chan].baseRate  = baseRate;
	mss_channels[chan].alias     = alias;
	mss_channels[chan].alias2    = alias2;
	mss_channels[chan].blend     = blend;
	mss_channels[chan].touchTime = mss_loopFrameTime;
	mss_channels[chan].endTime   = mss_time - offsetMs + lengthMs;
	mss_channels[chan].paused    = mss_paused && AL_CHANNEL( alias ) != 1;
}

/* ---- MSS_CompareReplacableChannels  0x0044C1D0 ---- */
int MSS_CompareReplacableChannels( int a, int b, int entnum )
{
	int diff;

	if ( mss_channels[a].entnum != mss_channels[b].entnum ) {
		if ( mss_channels[a].entnum == entnum ) {
			return -1;
		}
		if ( mss_channels[b].entnum == entnum ) {
			return 1;
		}
	}
	diff = mss_channels[a].channel - mss_channels[b].channel;
	if ( diff ) {
		return diff;
	}
	diff = mss_channels[a].endTime - mss_channels[b].endTime;
	if ( diff ) {
		return diff;
	}
	return 0;
}

/* ---- MSS_IsChannelReplacable  0x0044C220 ---- */
qboolean MSS_IsChannelReplacable( int chan, int entnum, int priority )
{
	return mss_channels[chan].channel <= priority;
}

/* ---- MSS_SpatializeStream  0x0044C240 ---- */
static void MSS_SpatializeStream( float *volume, int slot, float *pan )
{
	vec3_t      dir;
	mssAlias_t *alias, *alias2;
	float       left, blend, minDist;
	rfloat      dist, diff, f, atten;

	VectorSubtract( mss_streamInfo[slot].origin, mss_listenerOrigin, dir );
	dist = VectorNormalize( dir );

#ifdef COD1_VC7
	left   = DotProduct( dir, mss_listenerAxis[1] );
#else
	/* Retail 0x0044C282-0x0044C2BF sums z, y, then x. */
	left   = mss_listenerAxis[1][2] * dir[2] + mss_listenerAxis[1][1] * dir[1]
	       + mss_listenerAxis[1][0] * dir[0];
#endif
	alias  = mss_channels[ MSS_FIRST_STREAM + slot ].alias;
	alias2 = mss_channels[ MSS_FIRST_STREAM + slot ].alias2;
	blend  = mss_channels[ MSS_FIRST_STREAM + slot ].blend;

	/* Retail 0x0044C2C4-0x0044C330: 1 - blend scales minDist unrounded and
	 * is kept (fst) for maxDist.  Only minDist is stored; the difference,
	 * the ratio and the attenuation stay on the x87 stack.  The first test
	 * is "<= 0" and the second "< 1" with C0 alone, so a NaN takes the
	 * attenuating path and yields 1 - f. */
	minDist = ( 1.0f - blend ) * alias->distMin
	        +          blend   * alias2->distMin;
	diff    = dist - minDist;

	if ( diff <= 0.0f ) {
		atten = 1.0f;
	} else {
		f = diff / ( ( 1.0f - blend ) * alias->distMax
		           +          blend   * alias2->distMax - minDist );
		atten = ( f >= 1.0f ) ? 0.0f : 1.0f - f;
	}

	*volume = atten * *volume;
	*pan    = ( 1.0f - left ) * 0.5f;
}

/* ---- MSS_FindReplacableChannel  0x0044C350 ---- */
static int MSS_FindReplacableChannel( int entnum, int priority, int first, int count )
{
	int best;
	int chan;

	best = -1;
	for ( chan = first ; chan < first + count ; chan++ ) {
		if ( MSS_IsChannelReplacable( chan, entnum, priority )
			&& ( best < 0 || MSS_CompareReplacableChannels( chan, best, entnum ) < 0 ) ) {
			best = chan;
		}
	}
	return best;
}

/* ---- MSS_Stop2DChannel  0x0044C3D0 ---- */
void MSS_Stop2DChannel( int chan )
{
	AIL_end_sample( MSS_2D_HANDLE( chan ) );
	mss_channels[chan].paused = 0;
}

/* ---- MSS_Pause2DChannel  0x0044C3F0 ---- */
void MSS_Pause2DChannel( int chan )
{
	AIL_stop_sample( MSS_2D_HANDLE( chan ) );
	mss_channels[chan].paused = 1;
}

/* ---- MSS_Unpause2DChannel  0x0044C410 ---- */
void MSS_Unpause2DChannel( int chan, int elapsed )
{
	AIL_resume_sample( MSS_2D_HANDLE( chan ) );
	mss_channels[chan].endTime += elapsed;
	mss_channels[chan].paused = 0;
}

/* ---- MSS_Is2DChannelFree  0x0044C440 ---- */
qboolean MSS_Is2DChannelFree( int chan )
{
	if ( mss_channels[chan].paused ) {
		return qfalse;
	}
	return AIL_sample_status( MSS_2D_HANDLE( chan ) ) == SMP_DONE;
}

/* ---- MSS_FindFree2DChannel  0x0044C470 ---- */
int MSS_FindFree2DChannel( int entnum, int priority )
{
	int i, chan;

	for ( i = 0 ; i < mss_num2DChannels ; i++ ) {
		if ( MSS_Is2DChannelFree( MSS_FIRST_2D + i ) ) {
			return MSS_FIRST_2D + i;
		}
	}

	chan = MSS_FindReplacableChannel( entnum, priority, MSS_FIRST_2D, mss_num2DChannels );
	if ( chan > 0 ) {
		AIL_end_sample( MSS_2D_HANDLE( chan ) );
		mss_channels[chan].paused = 0;
	}
	return chan;
}

/* ---- MSS_Stop3DChannel  0x0044C510 ---- */
void MSS_Stop3DChannel( int chan )
{
	AIL_end_3D_sample( MSS_3D_HANDLE( chan ) );
	mss_channels[chan].paused = 0;
}

/* ---- MSS_Pause3DChannel  0x0044C530 ---- */
void MSS_Pause3DChannel( int chan )
{
	AIL_stop_3D_sample( MSS_3D_HANDLE( chan ) );
	mss_channels[chan].paused = 1;
}

/* ---- MSS_Unpause3DChannel  0x0044C550 ---- */
void MSS_Unpause3DChannel( int chan, int elapsed )
{
	AIL_resume_3D_sample( MSS_3D_HANDLE( chan ) );
	mss_channels[chan].endTime += elapsed;
	mss_channels[chan].paused = 0;
}

/* ---- MSS_Is3DChannelFree  0x0044C580 ---- */
qboolean MSS_Is3DChannelFree( int chan )
{
	if ( mss_channels[chan].paused ) {
		return qfalse;
	}
	return AIL_3D_sample_status( MSS_3D_HANDLE( chan ) ) == SMP_DONE;
}

/* ---- MSS_FindFree3DChannel  0x0044C5B0 ---- */
int MSS_FindFree3DChannel( int entnum, int priority )
{
	int i, chan;

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( MSS_Is3DChannelFree( i ) ) {
			return i;
		}
	}

	chan = MSS_FindReplacableChannel( entnum, priority, MSS_FIRST_3D, mss_max3DChannels );
	if ( chan > 0 ) {
		AIL_end_3D_sample( MSS_3D_HANDLE( chan ) );
		mss_channels[chan].paused = 0;
	}
	return chan;
}

/* ---- MSS_StopStreamChannel  0x0044C640 ---- */
int MSS_StopStreamChannel( int chan )
{
	AIL_close_stream( MSS_STREAM_HANDLE( chan ) );
	MSS_STREAM_HANDLE( chan ) = 0;
	mss_channels[chan].paused = 0;
	return 0;
}

/* ---- MSS_PauseStreamChannel  0x0044C670 ---- */
void MSS_PauseStreamChannel( int chan )
{
	AIL_pause_stream( MSS_STREAM_HANDLE( chan ), 1 );
	mss_channels[chan].paused = 1;
}

/* ---- MSS_UnpauseStreamChannel  0x0044C6A0 ---- */
void MSS_UnpauseStreamChannel( int chan, int elapsed )
{
	AIL_pause_stream( MSS_STREAM_HANDLE( chan ), 0 );
	mss_channels[chan].endTime += elapsed;
	mss_channels[chan].paused = 0;
}

/* ---- MSS_IsStreamChannelFree  0x0044C6D0 ---- */
int MSS_IsStreamChannelFree( int chan )
{
	if ( !MSS_STREAM_HANDLE( chan ) ) {
		return 1;
	}
	if ( !mss_channels[chan].paused
		&& AIL_stream_status( MSS_STREAM_HANDLE( chan ) ) == SMP_DONE ) {
		AIL_close_stream( MSS_STREAM_HANDLE( chan ) );
		MSS_STREAM_HANDLE( chan ) = 0;
		return 1;
	}
	return 0;
}

/* ---- MSS_FindFreeStreamChannel  0x0044C720 ---- */
int MSS_FindFreeStreamChannel( int entnum, int priority )
{
	int i, chan;

	for ( i = MSS_FIRST_FREE_STREAM ; i < mss_numStreamChannels ; i++ ) {
		if ( MSS_IsStreamChannelFree( MSS_FIRST_STREAM + i ) ) {
			return MSS_FIRST_STREAM + i;
		}
	}

	chan = MSS_FindReplacableChannel( entnum, priority, MSS_FIRST_STREAM + MSS_FIRST_FREE_STREAM,
									  mss_numStreamChannels - MSS_FIRST_FREE_STREAM );
	if ( chan > 0 ) {
		MSS_StopStreamChannel( chan );
	}
	return chan;
}

/* ---- MSS_StopEntityChannel  0x0044C7E0 ---- */
void MSS_StopEntityChannel( int entnum, int aliasChan )
{
	int i, chan;

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( mss_channels[i].entnum == entnum && mss_channels[i].channel == aliasChan
			&& !MSS_Is3DChannelFree( i ) ) {
			MSS_Stop3DChannel( i );
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( mss_channels[chan].entnum == entnum && mss_channels[chan].channel == aliasChan
			&& !MSS_IsStreamChannelFree( chan ) ) {
			MSS_StopStreamChannel( chan );
		}
	}

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( mss_channels[chan].entnum == entnum && mss_channels[chan].channel == aliasChan
			&& !MSS_Is2DChannelFree( chan ) ) {
			MSS_Stop2DChannel( chan );
		}
	}
}

/* ---- MSS_SampleType  0x0044C950 ---- CONFIRMED */
/* Maps a sound's (format, bits, channels) -- the Mac 1.3 parameter order -- to
   a Miles DIG_F_* sample-type enum.
   Format 17 is the ADPCM/compressed marker; otherwise the low bit follows
   8-bit vs 16-bit and the +2 selects the stereo variants. */
int __cdecl MSS_SampleType( int format, int bits, int channels )
{
	if ( channels == 1 ) {
		if ( format == 17 ) {
			return 5;
		}
		return bits > 8;
	}
	if ( format == 17 ) {
		return 7;
	}
	return ( bits > 8 ) + 2;
}

/* ---- MSS_StartAlias2DSample  0x0044C980 ---- */
int MSS_StartAlias2DSample( void *alias, void *alias2, float blend, int entnum,
                            float volume, float pitch, int startMs,
                            float startFraction, int *outChan )
{
	int    chan, h, rate, totalMs, offsetMs;
	int   *snd;

	chan = MSS_FindFree2DChannel( entnum, AL_CHANNEL( alias ) );
	if ( outChan ) {
		*outChan = chan;
	}
	if ( chan < 0 ) {
		return 0;
	}

	h   = MSS_2D_HANDLE( chan );
	snd = AL_SOUND( alias );

	AIL_init_sample( h );
	AIL_set_sample_type( h, MSS_SampleType( SND_FORMAT( snd ), SND_BITS( snd ),
											SND_CHANNELS( snd ) ), 0 );
	AIL_set_sample_address( h, SND_DATA( snd ), SND_DATALEN( snd ) );
	AIL_set_sample_adpcm_block_size( h, SND_BLOCKSIZE( snd ) );

	rate = MSS_FTOL( MSS_I2F( SND_RATE( snd ) ) * mss_timeScale * pitch );
	AIL_set_sample_playback_rate( h, rate );

	AIL_set_sample_volume_pan( h,
		mss_masterVolume * mss_channelFade[ AL_CHANNEL( alias ) ][0] * volume, 0.5f );
	AIL_set_sample_loop_count( h, AL_LOOPING( alias ) == 0 );
	AIL_set_sample_reverb_levels( h, 1.0f, mss_wetLevel );

	AIL_sample_ms_position( h, &totalMs, 0 );   /* retail does not zero totalMs first */

	if ( startFraction != 0.0f ) {
		offsetMs = MSS_FTOL( (double)totalMs * startFraction );
	} else {
		offsetMs = startMs;
	}

	if ( offsetMs ) {
		AIL_set_sample_ms_position( h, offsetMs );
		if ( !mss_paused || AL_CHANNEL( alias ) == 1 ) {
			AIL_resume_sample( h );
		}
	} else if ( !mss_paused || AL_CHANNEL( alias ) == 1 ) {
		AIL_start_sample( h );
	}

	if ( AL_LOOPING( alias ) ) {
		totalMs = 0;
	}

	MSS_SetChannelInfo( entnum, 0, alias, chan, alias2, blend, volume, pitch,
						SND_RATE( snd ), totalMs, offsetMs );
	return totalMs;
}

/* ---- MSS_StartAlias3DSample  0x0044CB90 ---- */
int MSS_StartAlias3DSample( void *alias, void *alias2, float blend, int entnum,
                            const float *origin, float volume, float pitch,
                            int startMs, float startFraction, int *outChan )
{
	int    chan, h, rate, totalMs, offsetMs;
	int   *snd;
	float  oneMinusBlend, minDist, maxDist, dist, atten, scaled;

	chan = MSS_FindFree3DChannel( entnum, AL_CHANNEL( alias ) );
	if ( outChan ) {
		*outChan = chan;
	}
	if ( chan < 0 ) {
		return 0;
	}

	h   = MSS_3D_HANDLE( chan );
	snd = AL_SOUND( alias );

	/* Retail rounds each step below to a float exactly where the stores
	 * are (Distance is inlined from /Op com_math.c, so every float
	 * assignment is stored): (1 - blend) at 0x0044CBDB, dist - minDist at
	 * 0x0044CC69, the normalised distance at 0x0044CC94 and
	 * atten * fade * volume at 0x0044CCD7. */
	oneMinusBlend = 1.0f - blend;
	minDist = oneMinusBlend * ((mssAlias_t *)alias)->distMin + blend * ((mssAlias_t *)alias2)->distMin;
	maxDist = oneMinusBlend * ((mssAlias_t *)alias)->distMax + blend * ((mssAlias_t *)alias2)->distMax;

	AIL_set_3D_sample_info( h, snd );

	dist  = Distance( origin, mss_listenerOrigin );
	atten = dist - minDist;
	/* test ah,41h / jp at 0x0044CC79 and test ah,1 / jne at 0x0044CCA4:
	 * a NaN atten takes the divide branch and then 1 - atten. */
	if ( atten <= 0.0f ) {
		atten = 1.0f;
	} else {
		atten = atten / ( maxDist - minDist );
		atten = ( atten >= 1.0f ) ? 0.0f : 1.0f - atten;
	}
	scaled = atten * mss_channelFade[ AL_CHANNEL( alias ) ][0] * volume;

	AIL_set_3D_sample_volume( h, mss_masterVolume * scaled );
	AIL_set_3D_sample_distances( h, maxDist, maxDist );

	rate = MSS_FTOL( (float)AIL_3D_sample_playback_rate( h ) * mss_timeScale * pitch );
	AIL_set_3D_sample_playback_rate( h, rate );

	MSS_Set3DPosition( h, origin );
	AIL_set_3D_sample_loop_count( h, AL_LOOPING( alias ) == 0 );
	AIL_set_3D_sample_effects_level( h, mss_wetLevel );

	totalMs = MSS_FTOL( (float)( 1000 * SND_SAMPLES( snd ) ) * mss_timeScale / (float)rate );

	if ( startFraction != 0.0f ) {
		offsetMs = MSS_FTOL( (float)totalMs * startFraction );
	} else {
		offsetMs = startMs;
	}

	if ( offsetMs ) {
		AIL_set_3D_sample_offset( h,
			MSS_FTOL( (float)SND_DATALEN( snd ) * startFraction ) );
		if ( !mss_paused ) {
			AIL_resume_3D_sample( h );
		}
	} else if ( !mss_paused ) {
		AIL_start_3D_sample( h );
	}

	if ( AL_LOOPING( alias ) ) {
		totalMs = 0;
	}

	MSS_SetChannelInfo( entnum, origin, alias, chan, alias2, blend, volume, pitch,
						SND_RATE( snd ), totalMs, offsetMs );
	return totalMs;
}

/* ---- MSS_StartAliasSample  0x0044CED0 ---- */
int MSS_StartAliasSample( void *alias, void *alias2, float blend, int entnum,
                          const float *origin, float volume, float pitch,
                          int startMs, int *outChan )
{
	if ( !AL_SOUND( alias ) ) {
		Com_DPrintf(
			"Tried to play sound '%s' from alias '%s', but it was not successfully loaded.\n",
			AL_FILENAME( alias ), AL_NAME( alias ) );
		if ( outChan ) {
			*outChan = -1;
		}
		return 0;
	}

	if ( MSS_IsAliasChannel3D( AL_CHANNEL( alias ) ) ) {
		return MSS_StartAlias3DSample( alias, alias2, blend, entnum, origin,
									   volume, pitch, startMs, 0.0f, outChan );
	}
	return MSS_StartAlias2DSample( alias, alias2, blend, entnum,
								   volume, pitch, startMs, 0.0f, outChan );
}

/* ---- MSS_StartAliasStreamOnChannel  0x0044CF50 ---- */
int MSS_StartAliasStreamOnChannel( void *alias, void *alias2, float blend, int entnum,
                                   const float *origin, float volume, float pitch,
                                   int startMs, float startFraction, int chan )
{
	const char *qpath;
	const char *osPath;
	int         h, rate, totalMs, offsetMs;
	float       pan;

	if ( !AL_STREAMFOUND( alias ) ) {
		Com_DPrintf(
			"Tried to play streamed sound '%s' from alias '%s', but it was not found at load time.\n",
			AL_FILENAME( alias ), AL_NAME( alias ) );
		return 0;
	}

	if ( MSS_STREAM_HANDLE( chan ) ) {
		AIL_close_stream( MSS_STREAM_HANDLE( chan ) );
		MSS_STREAM_HANDLE( chan ) = 0;
	}

	qpath = va( "sound/%s", AL_FILENAME( alias ) );
	if ( MSS_CVAR( mss_q3fs )->integer ) {
		h = AIL_open_stream( mss_digitalDriver, qpath, 0 );
	} else {
		osPath = FS_ShortOSFilePath( qpath );
		h = AIL_open_stream( mss_digitalDriver, osPath, 0 );
	}
	if ( !h ) {
		Com_Printf( "Couldn't play stream '%s' from alias '%s' - %s\n",
					qpath, AL_NAME( alias ), AIL_last_error() );
		return 0;
	}
	MSS_STREAM_HANDLE( chan ) = h;

	rate = AIL_stream_playback_rate( h );
	AIL_set_stream_playback_rate( h,
		MSS_FTOL( MSS_I2F( rate ) * mss_timeScale * pitch ) );
	AIL_set_stream_volume_pan( h,
		mss_masterVolume * mss_channelFade[ AL_CHANNEL( alias ) ][0] * volume, 0.5f );
	AIL_set_stream_loop_count( h, AL_LOOPING( alias ) == 0 );
	AIL_set_stream_reverb_levels( h, 1.0f, mss_wetLevel );

	AIL_stream_ms_position( h, &totalMs, 0 );   /* retail does not zero totalMs first */

	if ( startFraction != 0.0f ) {
		offsetMs = MSS_FTOL( (double)totalMs * startFraction );
	} else {
		offsetMs = startMs;
	}

	if ( offsetMs ) {
		AIL_set_stream_ms_position( h, offsetMs );
		if ( !mss_paused || AL_CHANNEL( alias ) == 1 ) {
			AIL_pause_stream( h, 0 );
		}
	} else if ( !mss_paused || AL_CHANNEL( alias ) == 1 ) {
		AIL_start_stream( h );
	}

	if ( AL_LOOPING( alias ) ) {
		totalMs = 0;
	}

	MSS_STREAM_INFO( chan ).spatialized = MSS_IsAliasChannel3D( AL_CHANNEL( alias ) );
	VectorCopy( origin, MSS_STREAM_INFO( chan ).origin );

	MSS_SetChannelInfo( entnum, origin, alias, chan, alias2, blend, volume, pitch,
						rate, totalMs, offsetMs );

	if ( MSS_STREAM_INFO( chan ).spatialized ) {
		MSS_SpatializeStream( &volume, chan - MSS_FIRST_STREAM, &pan );
		AIL_set_stream_volume_pan( h, mss_masterVolume * volume, pan );
	}
	return totalMs;
}

/* ---- MSS_StartAliasStream  0x0044D200 ---- */
int MSS_StartAliasStream( void *alias, void *alias2, float blend, int entnum,
                          const float *origin, float volume, float pitch,
                          int startMs, float startFraction, int *outChan )
{
	int chan;

	chan = MSS_FindFreeStreamChannel( entnum, AL_CHANNEL( alias ) );
	if ( outChan ) {
		*outChan = chan;
	}
	if ( chan < 0 ) {
		return 0;
	}
	return MSS_StartAliasStreamOnChannel( alias, alias2, blend, entnum, origin,
										  volume, pitch, startMs, startFraction, chan );
}

/* ---- MSS_ContinueLoopingSound  0x0044D250 ---- */
int MSS_ContinueLoopingSound( void *alias, void *alias2, float blend, int entnum,
                              const float *origin, int *outChan )
{
	int i, chan;

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( mss_channels[i].entnum == entnum
			&& !MSS_Is3DChannelFree( i )
			&& AL_LOOPING( mss_channels[i].alias )
			&& AL_NAME( mss_channels[i].alias )  == AL_NAME( alias )
			&& AL_NAME( mss_channels[i].alias2 ) == AL_NAME( alias2 ) ) {
			mss_channels[i].volume = ( ( 1.0f - blend ) * ( (mssAlias_t *) alias )->volMin
			                         + blend * ( (mssAlias_t *) alias2 )->volMin ) * 0.80000001f;
			AIL_set_3D_sample_playback_rate( mss_3DHandles[i],
				MSS_FTOL( mss_channels[i].baseRate * mss_channels[i].pitch * mss_timeScale ) );
			MSS_Set3DPosition( mss_3DHandles[i], origin );
			mss_channels[i].touchTime = mss_loopFrameTime;
			mss_channels[i].blend     = blend;
			if ( outChan ) {
				*outChan = i;
			}
			return 1;
		}
	}

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( mss_channels[chan].entnum == entnum
			&& !MSS_Is2DChannelFree( chan )
			&& AL_LOOPING( mss_channels[chan].alias )
			&& AL_NAME( mss_channels[chan].alias )  == AL_NAME( alias )
			&& AL_NAME( mss_channels[chan].alias2 ) == AL_NAME( alias2 ) ) {
			mss_channels[chan].volume = ( ( 1.0f - blend ) * ( (mssAlias_t *) alias )->volMin
			                            + blend * ( (mssAlias_t *) alias2 )->volMin ) * 0.80000001f;
			AIL_set_sample_playback_rate( MSS_2D_HANDLE( chan ),
				MSS_FTOL( mss_channels[chan].baseRate * mss_channels[chan].pitch * mss_timeScale ) );
			mss_channels[chan].touchTime = mss_loopFrameTime;
			mss_channels[chan].blend     = blend;
			if ( outChan ) {
				*outChan = chan;
			}
			return 1;
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( mss_channels[chan].entnum == entnum
			&& !MSS_IsStreamChannelFree( chan )
			&& AL_LOOPING( mss_channels[chan].alias )
			&& AL_NAME( mss_channels[chan].alias )  == AL_NAME( alias )
			&& AL_NAME( mss_channels[chan].alias2 ) == AL_NAME( alias2 ) ) {
			mss_channels[chan].volume = ( ( 1.0f - blend ) * ( (mssAlias_t *) alias )->volMin
			                            + blend * ( (mssAlias_t *) alias2 )->volMin ) * 0.80000001f;
			AIL_set_stream_playback_rate( MSS_STREAM_HANDLE( chan ),
				MSS_FTOL( mss_channels[chan].baseRate * mss_channels[chan].pitch * mss_timeScale ) );
			VectorCopy( origin, MSS_STREAM_INFO( chan ).origin );
			mss_channels[chan].touchTime = mss_loopFrameTime;
			mss_channels[chan].blend     = blend;
			if ( outChan ) {
				*outChan = chan;
			}
			return 1;
		}
	}
	return 0;
}

/* ---- MSS_ChoosePitchAndVolume  0x0044D5C0 ---- HIGH */
/* Blends the two aliases' volume/pitch ranges (blend 0 = alias, 1 = alias2),
   then picks a random value inside each range.  0.000030517578f = 2^-15 =
   1/32768 exactly and 0.80000001f = 0.8f: retail multiplies by the floats at
   0x00568E98 (00 00 00 38) and 0x00568F18 (CD CC 4C 3F), fmul dword at
   0x0044D624 / 0x0044D638 / 0x0044D651. */
int __cdecl MSS_ChoosePitchAndVolume( mssAlias_t *alias, mssAlias_t *alias2, float blend,
                                      float *outVolume, float *outPitch )
{
	rfloat invBlend = (rfloat)1.0f - blend;
	float  volMin   = invBlend * alias->volMin   + blend * alias2->volMin;
	float  volMax   = invBlend * alias->volMax   + blend * alias2->volMax;
	float  pitchMin = invBlend * alias->pitchMin + blend * alias2->pitchMin;
	float  pitchMax = invBlend * alias->pitchMax + blend * alias2->pitchMax;
	int    result;

	*outVolume = ( FCAST( MSS_I2F( rand() ) * 0.000030517578f ) * ( volMax - volMin ) + volMin ) * 0.80000001f;
	result     = rand();
	*outPitch  = FCAST( MSS_I2F( result ) * 0.000030517578f ) * ( pitchMax - pitchMin ) + pitchMin;
	return result;
}

/* ---- MSS_PlaySoundAlias_Internal  0x0044D670 ---- */
int __cdecl MSS_PlaySoundAlias_Internal( void *alias, float *alias2, float blend,
                                         int entnum, float *origin, int *outChan,
                                         int startMs )
{
	float  volume, pitch;

	if ( !mss_digitalDriver ) {
		return 0;
	}
	if ( outChan ) {
		*outChan = -1;
	}

	if ( MSS_IsAliasChannel3D( AL_CHANNEL( alias ) ) ) {
		float maxDist;
#ifndef COD1_VC7
		vec3_t d;
#endif

		maxDist = ( 1.0f - blend ) * ( (mssAlias_t *) alias )->distMax
				+          blend   * ( (mssAlias_t *) alias2 )->distMax;
		/* Retail 0x0044D6C6-0x0044D717: DistanceSquared inlined (/Op stores of
		 * the delta), then fcompp / test ah,5 / jnp culls only on an ordered
		 * sum > maxDist^2, so a NaN compare keeps playing. */
#ifdef COD1_VC7
		if ( DistanceSquared( origin, mss_listenerOrigin ) > maxDist * maxDist ) {
			return 0;
		}
#else
		/* Modern MSVC rounds DistanceSquared's return to float; retail
		 * compares the unrounded sum. */
		VectorSubtract( mss_listenerOrigin, origin, d );
		if ( maxDist * maxDist < d[2] * d[2] + d[1] * d[1] + d[0] * d[0] ) {
			return 0;
		}
#endif
	}

	if ( MSS_ContinueLoopingSound( alias, alias2, blend, entnum, origin, outChan ) ) {
		return 0;
	}

	if ( AL_CHANNEL( alias ) ) {
		MSS_StopEntityChannel( entnum, AL_CHANNEL( alias ) );
	}

	MSS_ChoosePitchAndVolume( (mssAlias_t *)alias, (mssAlias_t *)alias2, blend, &volume, &pitch );

	switch ( AL_TYPE( alias ) ) {
	case AL_TYPE_SAMPLE:
		return MSS_StartAliasSample( alias, alias2, blend, entnum, origin,
									 volume, pitch, startMs, outChan );
	case AL_TYPE_STREAM:
		return MSS_StartAliasStream( alias, alias2, blend, entnum, origin,
									 volume, pitch, startMs, 0.0f, outChan );
	}
	return 0;
}

/* ---- MSS_PlaySoundAlias  0x0044D7D0 ---- */
int __cdecl MSS_PlaySoundAlias( float *alias, int entnum, float *origin, int startMs )
{
	if ( !alias ) {
		return 0;
	}
	return MSS_PlaySoundAlias_Internal( alias, alias, 0.0f, entnum, origin, 0, startMs );
}

/* ---- MSS_ValidateSoundAliasBlend  0x0044D800 ---- HIGH */
/* Two aliases may only be blended if their key fields match.  The pitch/volume
   tests are plain !=: a NaN min/max counts as "not a single fixed value",
   exactly as the shipped mixer does. */
int __cdecl MSS_ValidateSoundAliasBlend( mssAlias_t *alias1, mssAlias_t *alias2, int reportErrors )
{
	if ( alias1 == alias2 ) {
		return 1;
	}
	if ( alias1->file != alias2->file ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but they don't use the same sound file\n", alias1->name, alias2->name );   /* 0x0055F9A0 */
		}
		return 0;
	}
	if ( alias1->loop != alias2->loop ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but they do not have the same looping status\n", alias1->name, alias2->name );   /* 0x0055F938 */
		}
		return 0;
	}
	if ( alias1->type != alias2->type ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but they are not both loaded or both streamed\n", alias1->name, alias2->name );   /* 0x0055F8D0 */
		}
		return 0;
	}
	if ( alias1->channel != alias2->channel ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but they don't use the same channel\n", alias1->name, alias2->name );   /* 0x0055F870 */
		}
		return 0;
	}
	if ( alias1->isMaster != alias2->isMaster ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but only one of them is a 'master' alias\n", alias1->name, alias2->name );   /* 0x0055F810 */
		}
		return 0;
	}
	if ( alias1->isSlave != alias2->isSlave ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but only one of them is a 'slave' alias\n", alias1->name, alias2->name );   /* 0x0055F7B0 */
		}
		return 0;
	}
	if ( alias1->loop
	  && ( alias1->pitchMin != alias1->pitchMax
	    || alias2->pitchMin != alias2->pitchMax ) ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but they are looping and at least one of them has a random pitch\n", alias1->name, alias2->name );   /* 0x0055F738 */
		}
		return 0;
	}
	if ( alias1->loop
	  && ( alias1->volMin != alias1->volMax
	    || alias2->volMin != alias2->volMax ) ) {
		if ( reportErrors ) {
			Com_Error( ERR_DROP, "\x15tried to blend between sound aliases '%s' and '%s', but they are looping and at least one of them has a random volume\n", alias1->name, alias2->name );   /* 0x0055F6C0 */
		}
		return 0;
	}
	return 1;
}

/* ---- MSS_PlayBlendedSoundAliases  0x0044D980 ---- CONFIRMED */
int __cdecl MSS_PlayBlendedSoundAliases( void *alias, float *alias2, int blendBits,
                                         int entnum, float *origin, int startMs )
{
	if ( !alias || !alias2 ) {
		return 0;
	}
	MSS_ValidateSoundAliasBlend( (mssAlias_t *) alias, (mssAlias_t *) alias2, 1 );
	return MSS_PlaySoundAlias_Internal( alias, alias2, *(float *)&blendBits, entnum, origin, 0, startMs );
}

/* ---- MSS_PlayLocalSoundAlias  0x0044D9C0 ---- VERIFIED */
int __cdecl MSS_PlayLocalSoundAlias( int source, int aliasName )
{
	float *alias;

	if ( aliasName ) {
		alias = (float *)Com_PickSoundAlias( (const char *) aliasName, source );
		if ( alias ) {
			return MSS_PlaySoundAlias_Internal( alias, alias, 0.0f, mss_listenerEntnum,
												mss_listenerOrigin, 0, 0 );
		}
	}
	return 0;
}

/* ---- MSS_StartBackground  0x0044DA00 ---- */
static void *MSS_StartBackground( void *alias, int slot, int msec )
{
	int   chan;
	float volume, pitch;

	MSS_UpdatePause();

	if ( MSS_IsAliasChannel3D( AL_CHANNEL( alias ) ) ) {
		Com_Error( ERR_DROP,
			"\x15" "alias %s sound %s played as an ambient / music track uses a 3D channel type; should probably be channel 'local'\n",
			AL_NAME( alias ), AL_FILENAME( alias ) );
	}
	if ( AL_TYPE( alias ) != AL_TYPE_STREAM ) {
		Com_Error( ERR_DROP,
			"\x15" "alias %s sound %s played as an ambient / music track is not streamed; type must be 'streamed'\n",
			AL_NAME( alias ), AL_FILENAME( alias ) );
	}

	/* float 2^-15 at 0x0044DA85 / 0x0044DAB9, as in MSS_ChoosePitchAndVolume */
	volume = ( FCAST( MSS_I2F( rand() ) * 0.000030517578f )
			 * ( AL_VOLMAX( alias ) - AL_VOLMIN( alias ) ) + AL_VOLMIN( alias ) )
			 * 0.80000001f;
	pitch  = FCAST( MSS_I2F( rand() ) * 0.000030517578f )
			 * ( AL_PITCHMAX( alias ) - AL_PITCHMIN( alias ) ) + AL_PITCHMIN( alias );

	chan = MSS_FIRST_STREAM + slot;
	if ( !MSS_IsStreamChannelFree( chan ) ) {
		MSS_StopStreamChannel( chan );
	}

	MSS_StartAliasStreamOnChannel( alias, alias, 0.0f, MSS_NO_ENTITY,
								   mss_listenerOrigin, volume, pitch, 0, 0.0f, chan );

	mss_bgFade[slot].target = mss_channels[chan].volume;
	if ( msec > 0 ) {
		mss_bgFade[slot].rate = mss_channels[chan].volume / msec;
		mss_channels[chan].volume = 0.0f;
	} else {
		mss_bgFade[slot].rate = 0.0f;
	}
	return &mss_channels[chan].volume;
}

/* ---- MSS_StopBackground  0x0044DB90 ---- */
void __cdecl MSS_StopBackground( int slot, int msec )
{
	if ( MSS_IsStreamChannelFree( MSS_FIRST_STREAM + slot ) ) {
		return;
	}
	if ( !msec ) {
		MSS_StopStreamChannel( MSS_FIRST_STREAM + slot );
		return;
	}
	if ( mss_bgFade[slot].target > 0.0f ) {
		mss_bgFade[slot].rate   = -mss_bgFade[slot].target / msec;
		mss_bgFade[slot].target = 0.0f;
	}
}

/* ---- MSS_PlayMusicAlias  0x0044DC40 ---- CONFIRMED */
float *__cdecl MSS_PlayMusicAlias( float *alias )
{
	float *result = (float *)mss_digitalDriver;

	if ( mss_digitalDriver && alias ) {
		result = (float *)MSS_IsStreamChannelFree( MSS_FIRST_STREAM );
		if ( result ) {
			return MSS_StartBackground( alias, 0, 0 );
		}
	}
	return result;
}

/* ---- MSS_StopMusic  0x0044DC70 ---- CONFIRMED */
void __cdecl MSS_StopMusic( int msec )
{
	MSS_StopBackground( 0, msec );
}

/* ---- MSS_PlayAmbientAlias  0x0044DC80 ---- */
void __cdecl MSS_PlayAmbientAlias( mssAlias_t *alias, int msec )
{
	int                  chan;
	mssAlias_t          *cur;
	mssBackgroundFade_t *fade;
	float                volume;

	if ( !mss_digitalDriver || !alias ) {
		return;
	}

	chan = MSS_FIRST_STREAM + mss_ambientSlot;
	fade = &mss_bgFade[mss_ambientSlot];

	if ( !MSS_IsStreamChannelFree( chan ) ) {
		cur = (mssAlias_t *) mss_channels[chan].alias;
		if ( cur->name == alias->name ) {
			return;
		}
		if ( cur->file == alias->file
			&& cur->loop == alias->loop
			&& cur->pitchMin == alias->pitchMin
			&& cur->pitchMax == alias->pitchMax
			&& cur->channel == alias->channel ) {
			mss_channels[chan].alias  = alias;
			mss_channels[chan].alias2 = alias;
			/* float 2^-15 at 0x0044DD1D, as in MSS_ChoosePitchAndVolume */
			volume = ( FCAST( MSS_I2F( rand() ) * 0.000030517578f )
					 * ( alias->volMax - alias->volMin ) + alias->volMin )
					 * 0.80000001f;
			if ( msec ) {
				fade->rate = ( volume - fade->target ) / msec;
			} else {
				fade->rate = 0.0f;
			}
			fade->target = volume;
			return;
		}
	}

	MSS_StopBackground( mss_ambientSlot, msec );
	mss_ambientSlot = 3 - mss_ambientSlot;
	MSS_StartBackground( alias, mss_ambientSlot, msec );
}

/* ---- MSS_BeginRawSamples  0x0044DD90 ---- CONFIRMED */
/* Allocates the streaming sample used by the raw-PCM feed (cinematics, VoIP).
   mss_rawSampleHandle = sample handle, mss_rawSegmentSize = ring-buffer chunk size,
   mss_rawRingBuffer = 32-chunk ring, mss_rawSampleTimePerByteMsec = milliseconds per byte. */
int __cdecl MSS_BeginRawSamples( int rate, int width, int channels )
{
	int sampleType;
	int bufSize;

	mss_rawSampleHandle = AIL_allocate_sample_handle( mss_digitalDriver );
	if ( !mss_rawSampleHandle ) {
		Com_Error( ERR_DROP, "\x15MILES 2D sound sample allocation failed on raw channel\n" );   /* 0x0055F5A8 */
	}
	mss_rawSampleRate = rate;
	mss_rawSampleWidthBytes = width;
	mss_rawChannelCount = channels;

	AIL_init_sample( mss_rawSampleHandle );
	if ( channels == 1 ) {
		sampleType = ( width != 1 );
	} else {
		sampleType = ( width != 1 ) + 2;
	}
	AIL_set_sample_type( mss_rawSampleHandle, sampleType, 0 );
	AIL_set_sample_playback_rate( mss_rawSampleHandle, rate );
	AIL_set_sample_volume_pan( mss_rawSampleHandle, mss_masterVolume, 0.5f );

	bufSize = AIL_minimum_sample_buffer_size( mss_digitalDriver, rate, sampleType );
	mss_rawSegmentSize = 0x2000;
	if ( bufSize > mss_rawSegmentSize ) {
		mss_rawSegmentSize = bufSize;
	}
	mss_rawRingBuffer = Z_MallocInternal( 32 * mss_rawSegmentSize );

	memset( mss_rawSegmentReady, 0, sizeof( mss_rawSegmentReady ) );
	mss_rawReadSegmentIndex = 0;
	mss_rawWriteSegmentIndex = 0;
	mss_rawWriteSegmentOffset = 0;
	mss_rawSampleTimeBaseMsec = 0.0;
	mss_rawSampleTimePerByteMsec = (rfloat)1000.0f / ( mss_rawChannelCount * mss_rawSampleWidthBytes * mss_rawSampleRate );
	return 0;
}

/* ---- MSS_EndRawSamples  0x0044DEC0 ---- CONFIRMED */
void MSS_EndRawSamples( void )
{
	if ( mss_rawSampleHandle ) {
		AIL_end_sample( mss_rawSampleHandle );
		AIL_release_sample_handle( mss_rawSampleHandle );
		mss_rawSampleHandle = 0;
		free( mss_rawRingBuffer );
	}
}

/* ---- MSS_RawSamplesTime  0x0044DF00 ---- CONFIRMED */
int MSS_RawSamplesTime( void )
{
	/* Retail returns LODWORD(v0) with v0's low dword set to mss_rawSampleHandle in the
	   no-sample path; since mss_rawSampleHandle is 0 there and the return is 32-bit,
	   that path is just "return 0".  The (unsigned __int64) cast is the retail
	   ftol and is kept so the float->int truncation matches exactly. */
	if ( !mss_rawSampleHandle ) {
		return 0;
	}
	return (unsigned __int64)( (double)(unsigned int)AIL_sample_position( mss_rawSampleHandle )
	                           * mss_rawSampleTimePerByteMsec + mss_rawSampleTimeBaseMsec );
}

/* ---- MSS_UpdateRawSamples  0x0044DF40 ---- */
void MSS_UpdateRawSamples( void )
{
	int buf;

	if ( !mss_rawSampleHandle ) {
		return;
	}
	AIL_set_sample_volume_pan( mss_rawSampleHandle, mss_masterVolume, 0.5f );

	if ( !mss_rawSegmentReady[ mss_rawReadSegmentIndex ] ) {
		return;
	}
	buf = AIL_sample_buffer_ready( mss_rawSampleHandle );
	if ( buf == -1 ) {
		return;
	}
	mss_rawSampleTimeBaseMsec = (double)mss_rawSegmentSize * mss_rawSampleTimePerByteMsec + mss_rawSampleTimeBaseMsec;
	AIL_load_sample_buffer( mss_rawSampleHandle, buf,
		(char *)mss_rawRingBuffer + mss_rawReadSegmentIndex * mss_rawSegmentSize, mss_rawSegmentSize );
	/* retail 0x0044DFC4 clears the flag with no bound check; written as a pointer store because
	   the indexed form makes modern /GS add a range-check fast-fail for index >= 32 */
	*( mss_rawSegmentReady + mss_rawReadSegmentIndex ) = 0;
	mss_rawReadSegmentIndex = ( mss_rawReadSegmentIndex + 1 ) % 32;
}

/* ---- MSS_RawSamples  0x0044DFF0 ---- */
void __cdecl MSS_RawSamples( int samples, int rate, int width, int channels, void *data )
{
	int remaining, n;

	if ( !mss_digitalDriver ) {
		return;
	}
	if ( !mss_rawSampleHandle ) {
		MSS_BeginRawSamples( rate, width, channels );
		if ( !mss_rawSampleHandle ) {
			return;
		}
	}

	remaining = channels * width * samples;
	while ( remaining ) {
		while ( mss_rawSegmentReady[ mss_rawWriteSegmentIndex ] ) {
			MSS_Update();
		}
		n = mss_rawSegmentSize - mss_rawWriteSegmentOffset;
		if ( n > remaining ) {
			n = remaining;
		}
		Com_Memcpy( (char *)mss_rawRingBuffer + mss_rawSegmentSize * mss_rawWriteSegmentIndex + mss_rawWriteSegmentOffset,
					data, n );
		data = (char *)data + n;
		remaining -= n;
		mss_rawWriteSegmentOffset += n;
		if ( mss_rawWriteSegmentOffset == mss_rawSegmentSize ) {
			mss_rawWriteSegmentOffset = 0;
			mss_rawSegmentReady[ mss_rawWriteSegmentIndex ] = 1;
			mss_rawWriteSegmentIndex = ( mss_rawWriteSegmentIndex + 1 ) % 32;
		}
	}
}

/* ---- MSS_FadeAllSounds  0x0044E0F0 ---- CONFIRMED */
void __cdecl MSS_FadeAllSounds( float target, int msec )
{
	mss_globalFadeTarget = target;
	mss_globalFadeRate = target - mss_globalFadeCur;
	if ( msec ) {
		mss_globalFadeRate /= MSS_I2F( msec );
	} else if ( target == 0.0f ) {
		MSS_StopSounds( 0 );
	}
}

/* ---- MSS_FadeSelectSounds  0x0044E140 ---- */
int __cdecl MSS_FadeSelectSounds( float *targets, int msec )
{
	{ double rate;
		mss_channelFade[0][1] = targets[0];
		rate = (double)targets[0] - mss_channelFade[0][0];
		mss_channelFade[0][2] = (float)rate;
		if (msec) mss_channelFade[0][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[1][1] = targets[1];
		rate = (double)targets[1] - mss_channelFade[1][0];
		mss_channelFade[1][2] = (float)rate;
		if (msec) mss_channelFade[1][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[2][1] = targets[2];
		rate = (double)targets[2] - mss_channelFade[2][0];
		mss_channelFade[2][2] = (float)rate;
		if (msec) mss_channelFade[2][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[3][1] = targets[3];
		rate = (double)targets[3] - mss_channelFade[3][0];
		mss_channelFade[3][2] = (float)rate;
		if (msec) mss_channelFade[3][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[4][1] = targets[4];
		rate = (double)targets[4] - mss_channelFade[4][0];
		mss_channelFade[4][2] = (float)rate;
		if (msec) mss_channelFade[4][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[5][1] = targets[5];
		rate = (double)targets[5] - mss_channelFade[5][0];
		mss_channelFade[5][2] = (float)rate;
		if (msec) mss_channelFade[5][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[6][1] = targets[6];
		rate = (double)targets[6] - mss_channelFade[6][0];
		mss_channelFade[6][2] = (float)rate;
		if (msec) mss_channelFade[6][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[7][1] = targets[7];
		rate = (double)targets[7] - mss_channelFade[7][0];
		mss_channelFade[7][2] = (float)rate;
		if (msec) mss_channelFade[7][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[8][1] = targets[8];
		rate = (double)targets[8] - mss_channelFade[8][0];
		mss_channelFade[8][2] = (float)rate;
		if (msec) mss_channelFade[8][2] = (float)(rate / (double)msec);
	}
	{ double rate;
		mss_channelFade[9][1] = targets[9];
		rate = (double)targets[9] - mss_channelFade[9][0];
		mss_channelFade[9][2] = (float)rate;
		if (msec) mss_channelFade[9][2] = (float)(rate / (double)msec);
	}
	return (int)targets;
}

/* ---- MSS_PauseSounds  0x0044E2F0 ---- */
void MSS_PauseSounds( void )
{
	int chan, i;

	if ( !mss_digitalDriver || mss_paused ) {
		return;
	}

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( !MSS_Is2DChannelFree( chan )
			&& AL_CHANNEL( mss_channels[chan].alias ) != 1 ) {
			MSS_Pause2DChannel( chan );
		}
	}

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( !MSS_Is3DChannelFree( i ) ) {
			MSS_Pause3DChannel( i );
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( !MSS_IsStreamChannelFree( chan )
			&& AL_CHANNEL( mss_channels[chan].alias ) != 1 ) {
			MSS_PauseStreamChannel( chan );
		}
	}

	mss_paused    = 1;
	mss_pauseTime = mss_time;
}

/* ---- MSS_UnpauseSounds  0x0044E470 ---- */
void MSS_UnpauseSounds( void )
{
	int chan, i, elapsed;

	if ( !mss_digitalDriver || !mss_paused ) {
		return;
	}
	elapsed = mss_time - mss_pauseTime;

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( !MSS_Is2DChannelFree( chan ) && mss_channels[chan].paused ) {
			MSS_Unpause2DChannel( chan, elapsed );
		}
	}

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( !MSS_Is3DChannelFree( i ) && mss_channels[i].paused ) {
			MSS_Unpause3DChannel( i, elapsed );
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( !MSS_IsStreamChannelFree( chan ) && mss_channels[chan].paused ) {
			MSS_UnpauseStreamChannel( chan, elapsed );
		}
	}

	mss_paused    = 0;
	mss_pauseTime = 0;
}

/* ---- MSS_UpdateLoopingSounds  0x0044E600 ---- */
void __cdecl MSS_UpdateLoopingSounds( void )
{
	int chan, i;

	if ( !mss_digitalDriver || mss_paused ) {
		return;
	}

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( !MSS_Is3DChannelFree( i )
			&& AL_LOOPING( mss_channels[i].alias )
			&& mss_channels[i].touchTime != mss_loopFrameTime ) {
			MSS_Stop3DChannel( i );
		}
	}

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( !MSS_Is2DChannelFree( chan )
			&& AL_LOOPING( mss_channels[chan].alias )
			&& mss_channels[chan].touchTime != mss_loopFrameTime ) {
			MSS_Stop2DChannel( chan );
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( !MSS_IsStreamChannelFree( chan )
			&& AL_LOOPING( mss_channels[chan].alias )
			&& mss_channels[chan].touchTime != mss_loopFrameTime ) {
			MSS_StopStreamChannel( chan );
		}
	}

	mss_loopFrameTime = mss_time;
}

/* ---- MSS_UpdateBackgroundVolume  0x0044E7A0 ---- */
static int MSS_UpdateBackgroundVolume( int slot, int msec )
{
	int   chan = MSS_FIRST_STREAM + slot;
	float v;

	v = msec * (rfloat)mss_bgFade[slot].rate + mss_channels[chan].volume;

	if ( mss_bgFade[slot].rate > 0.0f ) {
		if ( v > mss_bgFade[slot].target ) {
			v = mss_bgFade[slot].target;
		}
	} else if ( v < mss_bgFade[slot].target ) {
		v = mss_bgFade[slot].target;
		if ( v == 0.0f ) {
			AIL_close_stream( MSS_STREAM_HANDLE( chan ) );
			MSS_STREAM_HANDLE( chan ) = 0;
			mss_channels[chan].paused = 0;
			return 0;
		}
	}

	mss_channels[chan].touchTime = mss_loopFrameTime;
	mss_channels[chan].volume    = v;
	return 1;
}

/* ---- MSS_UpdateVolume  0x0044E840 ---- */
void MSS_UpdateVolume( float *fade, int msec )
{
	fade[0] = msec * fade[2] + fade[0];

	if ( fade[2] < 0.0f ) {
		if ( fade[0] < fade[1] ) {
			fade[0] = fade[1];
			fade[2] = 0.0f;
		}
	} else if ( fade[0] > fade[1] ) {
		fade[0] = fade[1];
		fade[2] = 0.0f;
	}
}

/* ---- MSS_UpdateMasterVolumes  0x0044E890 ---- */
void __cdecl MSS_UpdateMasterVolumes( int msec )
{
	int i;

	for ( i = 0 ; i < MSS_NUM_ALIAS_CHANNELS ; i++ ) {
		MSS_UpdateVolume( mss_channelFade[i], msec );
	}

	if ( mss_globalFadeRate == 0.0f ) {
		if ( !MSS_CVAR( mss_volume )->modified ) {
			return;
		}
	} else {
		MSS_UpdateVolume( mss_globalFade, msec );
		if ( mss_globalFadeCur == 0.0f && mss_globalFadeRate == 0.0f ) {
			MSS_StopSounds( 0 );
		}
	}

	MSS_CVAR( mss_volume )->modified = qfalse;
	mss_masterVolume = MSS_MAX( 0.0f, MSS_MIN( 1.0f, MSS_CVAR( mss_volume )->value ) ) * mss_globalFadeCur;
}

/* ---- MSS_AnyMasters  0x0044ED60 ---- */
int MSS_AnyMasters( void )
{
	int chan, i, h;

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( ( !MSS_Is3DChannelFree( i ) )
			&& AL_MASTER( mss_channels[i].alias ) ) {
			return 1;
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( !MSS_STREAM_HANDLE( chan ) ) {
			continue;
		}
		if ( mss_channels[chan].paused || AIL_stream_status( MSS_STREAM_HANDLE( chan ) ) != SMP_DONE ) {
			if ( AL_MASTER( mss_channels[chan].alias ) ) {
				return 1;
			}
		} else {
			AIL_close_stream( MSS_STREAM_HANDLE( chan ) );
			MSS_STREAM_HANDLE( chan ) = 0;
		}
	}

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( ( !MSS_Is2DChannelFree( chan ) )
			&& AL_MASTER( mss_channels[chan].alias ) ) {
			return 1;
		}
	}
	return 0;
}

/* ---- MSS_Update3DChannel  0x0044EEA0 ---- */
static void MSS_Update3DChannel( int msec, int chan )
{
	mssAlias_t *alias, *alias2;
	int         h;
	vec3_t      origin;
#ifndef COD1_VC7
	vec3_t      d;
#endif
	float       blend, volume, oneMinusBlend, minDist, maxDist, dist, ducked;

	(void)msec;

	alias  = mss_channels[chan].alias;
	alias2 = mss_channels[chan].alias2;
	blend  = mss_channels[chan].blend;
	volume = mss_channels[chan].volume;
	h      = mss_3DHandles[chan];

	MSS_GetCurrent3DPosition( mss_channels[chan].entnum, mss_channels[chan].localOrigin, origin );
	MSS_Set3DPosition( h, origin );

	oneMinusBlend = 1.0f - blend;
	minDist = oneMinusBlend * alias->distMin + blend * alias2->distMin;
	maxDist = oneMinusBlend * alias->distMax + blend * alias2->distMax;

	/* Retail 0x0044EF2E-0x0044EF77: Distance inlined from com_math.c (/Op),
	 * so every float local goes through its slot. */
#ifdef COD1_VC7
	dist = Distance( origin, mss_listenerOrigin );
#else
	/* The double products are exact and the 53-bit z, y, x sum is what
	 * retail passes to sqrt in st0. */
	VectorSubtract( mss_listenerOrigin, origin, d );
	dist = (float)sqrt( (double)d[2] * d[2] + (double)d[1] * d[1] + (double)d[0] * d[0] );
#endif
	dist = dist - minDist;
	if ( dist <= 0.0f ) {
		dist = 1.0f;
	} else {
		dist = dist / ( maxDist - minDist );
		if ( dist >= 1.0f ) {
			dist = 0.0f;
		} else {
			dist = 1.0f - dist;
		}
	}

	volume = dist * volume;
	if ( mss_anyMasters && alias->isSlave ) {
		ducked = oneMinusBlend * alias->slavePercent + blend * alias2->slavePercent;
		if ( volume > ducked ) {
			volume = ducked;
		}
	}

	volume = volume * mss_channelFade[ alias->channel ][0];
	AIL_set_3D_sample_volume( h, mss_masterVolume * volume );
}

/* ---- MSS_Update2DChannel  0x0044F070 ---- */
static void MSS_Update2DChannel( int msec, int chan )
{
	mssAlias_t *alias;
	int         h;
	float       volume, blend, ducked;

	(void)msec;

	alias  = mss_channels[chan].alias;
	volume = mss_channels[chan].volume;
	blend  = mss_channels[chan].blend;
	h      = MSS_2D_HANDLE( chan );

	if ( mss_anyMasters && alias->isSlave ) {
		ducked = ( 1.0f - blend ) * alias->slavePercent
		       +          blend   * ( (mssAlias_t *)mss_channels[chan].alias2 )->slavePercent;
		if ( volume > ducked ) {
			volume = ducked;
		}
	}

	volume = volume * mss_channelFade[ alias->channel ][0];
	AIL_set_sample_volume_pan( h, volume * mss_masterVolume, 0.5f );
}

/* ---- MSS_UpdateStreamChannel  0x0044F100 ---- */
static void MSS_UpdateStreamChannel( int msec, int chan )
{
	mssAlias_t *alias, *alias2;
	int         h;
	float       volume, pan, blend, ducked;

	if ( chan < MSS_FIRST_STREAM + MSS_NUM_BACKGROUND
		&& !MSS_UpdateBackgroundVolume( chan - MSS_FIRST_STREAM, msec ) ) {
		return;
	}

	alias  = mss_channels[chan].alias;
	alias2 = mss_channels[chan].alias2;
	blend  = mss_channels[chan].blend;
	volume = mss_channels[chan].volume;
	h      = MSS_STREAM_HANDLE( chan );
	pan    = 0.5f;

	if ( MSS_STREAM_INFO( chan ).spatialized ) {
		MSS_GetCurrent3DPosition( mss_channels[chan].entnum, mss_channels[chan].localOrigin,
								  MSS_STREAM_INFO( chan ).origin );
		MSS_SpatializeStream( &volume, chan - MSS_FIRST_STREAM, &pan );
	}

	if ( mss_anyMasters && alias->isSlave ) {
		ducked = ( 1.0f - blend ) * alias->slavePercent
		       +          blend   * alias2->slavePercent;
		if ( volume > ducked ) {
			volume = ducked;
		}
	}

	volume *= mss_channelFade[ alias->channel ][0];
	AIL_set_stream_volume_pan( h, mss_masterVolume * volume, pan );
}

/* ---- MSS_UpdateAllChannels  0x0044F210 ---- */
static void MSS_UpdateAllChannels( int msec )
{
	int chan;

	mss_anyMasters = MSS_AnyMasters();

	for ( chan = 0 ; chan < mss_max3DChannels ; chan++ ) {
		if ( !MSS_Is3DChannelFree( chan ) ) {
			MSS_Update3DChannel( msec, chan );
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( !MSS_IsStreamChannelFree( chan ) ) {
			MSS_UpdateStreamChannel( msec, chan );
		}
	}

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( !MSS_Is2DChannelFree( chan ) ) {
			MSS_Update2DChannel( msec, chan );
		}
	}
}

static const char * const mss_roomTypeNames[26] = {
	"generic",      "paddedcell",   "room",        "bathroom",
	"livingroom",   "stoneroom",    "auditorium",  "concerthall",
	"cave",         "arena",        "hangar",      "carpetedhallway",
	"hallway",      "stonecorridor","alley",       "forest",
	"city",         "mountains",    "quarry",      "plain",
	"parkinglot",   "sewerpipe",    "underwater",  "drugged",
	"dizzy",        "psychotic"
};

/* ---- MSS_RoomTypeFromString  0x0044F340 ---- */
unsigned int __cdecl MSS_RoomTypeFromString( const char *name )
{
	unsigned int i;
	int          index;

	if ( *name < '0' || *name > '9' ) {
		for ( i = 0 ; i < 26 ; i++ ) {
			if ( !_stricmp( name, mss_roomTypeNames[i] ) ) {
				return i;
			}
		}
	} else {
		index = j__atol( name );
		if ( index >= 0 && (unsigned int)index < 26 ) {
			return index;
		}
	}

	Com_Printf( "roomtype must be an integer from 0 to %i, or one of the following strings:\n", 25 );
	for ( i = 0 ; i < 26 ; i++ ) {
		Com_Printf( "  %s\n", mss_roomTypeNames[i] );
	}
	return 0;
}

/* ---- MSS_SetEnvironmentEffects  0x0044F3C0 ---- CONFIRMED */
void __cdecl MSS_SetEnvironmentEffects( const char *roomName, float target, int msec )
{
	unsigned int roomType;

	if ( !mss_digitalDriver ) {
		return;
	}

	roomType = MSS_RoomTypeFromString( roomName );
	AIL_set_digital_master_room_type( mss_digitalDriver, roomType );
	AIL_set_3D_room_type( mss_3DProvider, roomType );
	mss_roomType = roomType;

	mss_wetTarget = target;

	if ( msec < 1 ) {
		msec = 1;
	}
	mss_wetRate = ( target - mss_wetLevel ) / (double)msec;
}

/* ---- MSS_UpdateRoomEffects  0x0044F430 ---- */
void __cdecl MSS_UpdateRoomEffects( int msec )
{
	int     chan, i;
	float   level;

	if ( MSS_CVAR( mss_roomtype )->modified || MSS_CVAR( mss_wetlevel )->modified ) {
		MSS_CVAR( mss_roomtype )->modified = qfalse;
		MSS_CVAR( mss_wetlevel )->modified = qfalse;
		MSS_SetEnvironmentEffects( MSS_CVAR( mss_roomtype )->string,
			MSS_MAX( 0.0f, MSS_MIN( 1.0f, MSS_CVAR( mss_wetlevel )->value ) ), 0 );
	}

	if ( mss_wetRate != 0.0f ) {
		/* level stays on the x87 stack: retail stores it with fst and tests the
		   unrounded value against the target (0x0044F51D-0x0044F531). */
		level = msec * mss_wetRate + mss_wetLevel;
		mss_wetLevel = level;
		if ( mss_wetRate < 0.0f ) {
			if ( level <= mss_wetTarget ) {
				mss_wetLevel = mss_wetTarget;
				mss_wetRate  = 0.0f;
			}
		} else if ( level >= mss_wetTarget ) {
			mss_wetLevel = mss_wetTarget;
			mss_wetRate  = 0.0f;
		}

		for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
			if ( !MSS_Is3DChannelFree( i ) ) {
				AIL_set_3D_sample_effects_level( mss_3DHandles[i], mss_wetLevel );
			}
		}

		for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
			if ( !MSS_Is2DChannelFree( chan ) ) {
				AIL_set_sample_reverb_levels( MSS_2D_HANDLE( chan ), 1.0f, mss_wetLevel );
			}
		}

		for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
			if ( !MSS_IsStreamChannelFree( chan ) ) {
				AIL_set_stream_reverb_levels( MSS_STREAM_HANDLE( chan ), 1.0f, mss_wetLevel );
			}
		}
	}
}

/* ---- MSS_UpdateTimeScale  0x0044F6C0 ---- */
void MSS_UpdateTimeScale( void )
{
	float scale = com_timescale->value;
	float ratio;
	int   chan, i, h, rate;

	if ( scale <= 0.0f ) {
		scale = 1.0f;
	}
	if ( scale == mss_timeScale ) {
		return;
	}

	ratio = scale / mss_timeScale;
	mss_timeScale = scale;

	for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
		if ( !MSS_Is3DChannelFree( i ) ) {
			h = mss_3DHandles[i];
			rate = AIL_3D_sample_playback_rate( h );
			AIL_set_3D_sample_playback_rate( h, MSS_FTOL( ratio * rate ) );
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( !MSS_IsStreamChannelFree( chan ) ) {
			h = MSS_STREAM_HANDLE( chan );
			rate = AIL_stream_playback_rate( h );
			AIL_set_stream_playback_rate( h, MSS_FTOL( ratio * rate ) );
		}
	}

	for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
		if ( !MSS_Is2DChannelFree( chan ) ) {
			h = MSS_2D_HANDLE( chan );
			rate = AIL_sample_playback_rate( h );
			AIL_set_sample_playback_rate( h, MSS_FTOL( ratio * rate ) );
		}
	}
}

/* ---- MSS_UpdatePause  0x0044F8F0 ---- */
void MSS_UpdatePause( void )
{
	int paused = ( cl_paused->integer != 0 );

	if ( paused == mss_paused ) {
		return;
	}
	if ( paused ) {
		MSS_PauseSounds();
	} else {
		MSS_UnpauseSounds();
	}
}

/* ---- MSS_Update  0x0044F920 ---- */
void MSS_Update( void )
{
	int now, msec, paused;

	if ( !mss_digitalDriver ) {
		return;
	}
	mss_cpuPercent = AIL_digital_CPU_percent( mss_digitalDriver );
	if ( com_statmon->integer && mss_cpuPercent > 2 ) {
		StatMon_Warning( 2, 3000, "gfx/2d/warning@soundcpu.jpg" );
	}

	now       = Sys_Milliseconds();
	msec      = now - mss_time;
	mss_time += msec;

	paused = ( cl_paused->integer != 0 );
	if ( paused != mss_paused ) {
		if ( paused ) {
			MSS_PauseSounds();
		} else {
			MSS_UnpauseSounds();
		}
	}

	MSS_UpdateMasterVolumes( msec );

	if ( !mss_paused ) {
		if ( mss_pendingRestore ) {
			MSS_Restore( (int)mss_pendingRestore, mss_pendingRestoreSize );
			free( mss_pendingRestore );
			mss_pendingRestore     = 0;
			mss_pendingRestoreSize = 0;
		}
		MSS_UpdateTimeScale();
		MSS_UpdateRoomEffects( msec );
		MSS_UpdateAllChannels( msec );
	}

	MSS_UpdateRawSamples();
}

/* ---- MSS_StopSounds  0x0044FA40 ---- */
void __cdecl MSS_StopSounds( int flags )
{
	int chan, i;

	if ( !mss_digitalDriver ) {
		return;
	}

	if ( ( flags & 8 ) == 0 ) {
		for ( chan = MSS_FIRST_2D ; chan < MSS_FIRST_2D + mss_num2DChannels ; chan++ ) {
			if ( !MSS_Is2DChannelFree( chan ) ) {
				MSS_Stop2DChannel( chan );
			}
		}
		for ( i = 0 ; i < mss_max3DChannels ; i++ ) {
			if ( !MSS_Is3DChannelFree( i ) ) {
				MSS_Stop3DChannel( i );
			}
		}
	}

	for ( chan = MSS_FIRST_STREAM ; chan < MSS_FIRST_STREAM + mss_numStreamChannels ; chan++ ) {
		if ( !MSS_IsStreamChannelFree( chan )
			&& ( !( flags & 2 ) || chan != 32 )
			&& ( !( flags & 4 ) || ( chan != 33 && chan != 34 ) ) ) {
			MSS_StopStreamChannel( chan );
		}
	}

	if ( ( flags & 1 ) == 0 ) {
		MSS_SetEnvironmentEffects( "generic", 0.0f, 0 );
	}
}

COD1_ASSERT_SIZE( AILSOUNDINFO, 36 );
COD1_ASSERT_SIZE( AILMIXINFO, 120 );

/* ---- MSS_LoadSoundFile  0x0044FC10 ---- CONFIRMED */
/* Loads sound/<name>, parses its WAV header, and returns a hunk block holding
   an AILSOUNDINFO header immediately followed by the (optionally resampled)
   audio.  data_ptr and initial_ptr both point just past the 36-byte header. */
AILSOUNDINFO *__cdecl MSS_LoadSoundFile( const char *name )
{
	char         *path;
	void         *fileData;
	AILMIXINFO    info;
	AILSOUNDINFO *sound;

	if ( !mss_digitalDriver ) {
		return 0;
	}

	path = va( "sound/%s", name );
	if ( FS_ReadFile( path, &fileData ) < 0 ) {
		Com_Printf( "^1Sound file '%s' not found\n", path );
		return 0;
	}

	if ( !AIL_WAV_info( fileData, &info.Info ) ) {
		Com_Printf( "Sound file '%s' is in an invalid or corrupted format\n", path );
		FS_FreeFile( fileData );
		return 0;
	}

	if ( info.Info.rate <= (unsigned int)mss_driverRate
		&& ( info.Info.bits <= mss_driverBits || info.Info.format == 17 )
		&& (int)info.Info.channels <= mss_driverChannels ) {
		/* already playable by the driver -- copy the header and samples verbatim */
		sound = (AILSOUNDINFO *)Hunk_AllocAlignInternal( ( info.Info.data_len + 39 ) & 0xFFFFFFFC, 32 );
		memcpy( sound, &info.Info, sizeof( info.Info ) );
		sound->data_ptr    = (char *)sound + sizeof( AILSOUNDINFO );
		sound->initial_ptr = (char *)sound + sizeof( AILSOUNDINFO );
		Com_Memcpy( (int)( (char *)sound + sizeof( AILSOUNDINFO ) ),
					(int *)info.Info.data_ptr, info.Info.data_len );
	} else {
		/* resample down to the driver's rate / bit depth / channel count */
		unsigned int destRate    = info.Info.rate;
		unsigned int destSamples = info.Info.samples;
		int          destBits, destChannels;
		int          destFormat, destSize;
		void        *destData;

		if ( info.Info.rate > (unsigned int)mss_driverRate ) {
			do {
				destRate    >>= 1;
				destSamples >>= 1;
			} while ( destRate > (unsigned int)mss_driverRate );
		}
		destBits = mss_driverBits < info.Info.bits ? mss_driverBits : info.Info.bits;
		destChannels = mss_driverChannels < (int)info.Info.channels ? mss_driverChannels : info.Info.channels;

		destFormat = MSS_SampleType( info.Info.format, destBits, destChannels );
		destSize   = AIL_size_processed_digital_audio( destRate, destFormat, 1, &info );

		sound = (AILSOUNDINFO *)Hunk_AllocAlignInternal( ( destSize + 39 ) & 0xFFFFFFFC, 32 );

		sound->format      = info.Info.format;
		destData           = (char *)sound + sizeof( AILSOUNDINFO );
		sound->data_ptr    = destData;
		sound->data_len    = destSize;
		sound->rate        = destRate;
		sound->bits        = destBits;
		sound->channels    = destChannels;
		sound->samples     = destSamples;
		sound->block_size  = info.Info.block_size;
		sound->initial_ptr = destData;

		AIL_process_digital_audio( destData, destSize, destRate, destFormat, 1, &info );
	}

	FS_FreeFile( fileData );
	return sound;
}

/* ---- MSS_UnloadSoundFile  0x0044FE10 ---- CONFIRMED */
void MSS_UnloadSoundFile()
{
  ;
}

/* ---- MSS_GetSoundFileSize  0x0044FE20 ---- CONFIRMED */
int __cdecl MSS_GetSoundFileSize( int sound )
{
	/* audio-data length plus the 36-byte AILSOUNDINFO header */
	return SND_DATALEN( sound ) + 36;
}

/* MSS_ShutdownAtExit  0x00524F30 (IDA "AIL_shutdown()") */
static void MSS_ShutdownAtExit( void )
{
	AIL_shutdown();
}

/* ---- MSS_auto_cleanup  0x00524F40 ----
 * Retail's comes from RAD's mss32.lib (a separately compiled member,
 * linked among the Miles import thunks), so it is never inlined into
 * MSS_Init; mss32.h's AIL_startup() macro calls it. */
#pragma auto_inline( off )
int __cdecl MSS_auto_cleanup( void )
{
	atexit( MSS_ShutdownAtExit );
	return 0;
}
#pragma auto_inline( on )

/* MSS_InitFailed */
static void MSS_InitFailed( void )
{
	if ( Cvar_Get( "r_vc_compile", "0", 0 )->integer != 2 ) {
		Com_Printf( "Miles sound system initialization failed\n" );
	}
}

/* MSS_ClearState
 * Retail zeroes the whole 0x1470-byte Miles state block 0x008E0760..0x008E1BD0
 * with rep stosd (MSS_Shutdown 0x0045022B, MSS_Init failure paths 0x0044FFBF
 * and 0x00450010).  mss_rawRingBuffer
 * and mss_pendingRestore are dropped without being freed, as in retail.  The
 * cvar pointers mss_q3fs, mss_volume, mss_roomtype and mss_wetlevel live
 * outside the block and are kept. */
static void MSS_ClearState( void )
{
	memset( &mss_state, 0, sizeof( mss_state ) );
}

/* ---- MSS_Init  0x0044FE60 ---- */
void MSS_Init( void )
{
	cvar_t      *volume;
	unsigned int now;
	int          i;

	Com_Printf( "\n------- Miles sound system initialization -------\n" );

	mss_q3fs = (int)Cvar_Get( "mss_q3fs", "1", 32  );
	if ( MSS_CVAR( mss_q3fs )->integer ) {
		AIL_set_file_callbacks( (AIL_FILE_OPEN_CB) MSS_FileOpenCallback,
								(AIL_FILE_CLOSE_CB)MSS_FileCloseCallback,
								(AIL_FILE_SEEK_CB) MSS_FileSeekCallback,
								(AIL_FILE_READ_CB) MSS_FileReadCallback );
	}
	AIL_set_redist_directory( "miles" );

	if ( !AIL_startup() ) {
		MSS_InitFailed();
		return;
	}

	mss_errorOnMissing = (int)Cvar_Get( "mss_errorOnMissing", "1", 1 );
	mss_khz            = (int)Cvar_Get( "mss_khz",    "44", 0x21 );
	mss_bits           = (int (*)())Cvar_Get( "mss_bits",   "16", 0x21 );
	mss_stereo         = (int)Cvar_Get( "mss_stereo", "1",  0x21 );
	mss_3d_provider    = (int)Cvar_Get( "mss_3d_provider",
										"Miles Fast 2D Positional Audio", 0x21 );
	mss_volume         = (int)Cvar_Get( "mss_volume",   "0.8", 1 );
	mss_roomtype       = (int)Cvar_Get( "mss_roomtype", "0", 0x200 );
	mss_wetlevel       = (int)Cvar_Get( "mss_wetlevel", "0", 0x200 );

	if ( !MSS_Init2D() ) {
		AIL_shutdown();
		MSS_ClearState();
		MSS_InitFailed();
		return;
	}

	if ( !MSS_Init3DProvider( MSS_CVAR( mss_3d_provider )->string, 1 ) ) {
		AIL_shutdown();
		MSS_ClearState();
		MSS_InitFailed();
		return;
	}

	MSS_InitChannels();

	mss_globalFadeCur    = 1.0f;
	mss_globalFadeTarget = 1.0f;
	mss_globalFadeRate   = 0.0f;

	/* sizeof bound (unsigned compare): retail indexes by byte offset and tests with jb */
	for ( i = 0 ; i < sizeof( mss_channelFade ) / sizeof( mss_channelFade[0] ) ; i++ ) {
		mss_channelFade[i][0] = 1.0f;
		mss_channelFade[i][1] = 1.0f;
		mss_channelFade[i][2] = 0.0f;
	}

	if ( !sys_timeBaseInit ) {
		sys_timeBase     = timeGetTime();
		sys_timeBaseInit = 1;
	}
	now               = timeGetTime() - sys_timeBase;
	mss_time          = now;
	mss_loopFrameTime = now;
	mss_anyMasters    = 0;

	/* max( 0, min( 1, mss_volume ) ) with min/max macros, as on the Mac build */
	volume = MSS_CVAR( mss_volume );
	mss_masterVolume = MSS_MAX( 0.0f, MSS_MIN( 1.0f, volume->value ) );

	Com_Printf( "------- Miles successfully initialized -------\n" );
}

/* ---- MSS_Shutdown  0x00450130 ---- */
void MSS_Shutdown( void )
{
	/* Retail frees without clearing the pointer (0x0045013D); only the
	   block wipe at the end resets it, so with no driver it stays dangling. */
	if ( mss_pendingRestore ) {
		free( mss_pendingRestore );
	}
	if ( !mss_digitalDriver ) {
		return;
	}

	MSS_StopSounds( 0 );

	/* Retail inlines Com_UnloadSoundAliases for both alias sets here, the
	   in-game set first (0x00450158) and then the menu set (0x004501BD). */
	Com_UnloadSoundAliases( 1 );   /* SND_LOCALE_INGAME */
	Com_UnloadSoundAliases( 0 );   /* SND_LOCALE_MENU   */

	AIL_close_3D_provider( mss_3DProvider );
	mss_3DProvider = 0;
	AIL_shutdown();
	MSS_ClearState();
}

/* ---- MSS_ErrorCleanup  0x00450240 ---- CONFIRMED */
void MSS_ErrorCleanup()
{
  if ( mss_pendingRestore )
  {
    free(mss_pendingRestore);
    mss_pendingRestore = 0;
  }
}

/* ---- MSS_SetDirectSoundHWND_m  0x00450260 ---- VERIFIED */
void __cdecl MSS_SetDirectSoundHWND_m( void *hwnd )
{
	if ( mss_digitalDriver ) {
		AIL_set_DirectSound_HWND( mss_digitalDriver, hwnd );
	}
}

#define MSS_ERR_SAVE_OVERFLOW \
	"\x15" "buffer overflow while saving sound state: more than %i bytes used\n"
#define MSS_ERR_RESTORE_OVERFLOW \
	"\x15" "buffer overflow while restoring sound state: probably due to an old or corrupt savegame\n"
#define MSS_ERR_STATE_TOO_BIG \
	"\x15" "sound state buffer size too big; probably due to an old or corrupt savegame\n"

/* ---- MSS_Write  0x00450280 ---- CONFIRMED */
/* Parameter order from the Mac 1.3 calls (buffer, offset, size, src, count):
   it decides where the MSS_Save*Channel callers evaluate the running offset. */
int __cdecl MSS_Write( int base, int offset, signed int size, const void *src, unsigned int count )
{
	if ( (int)( offset + count ) > size ) {
		Com_Error( ERR_DROP, MSS_ERR_SAVE_OVERFLOW, size );
	}
	memcpy( (void *)( base + offset ), src, count );
	return offset + count;
}

/* ---- MSS_Read  0x004502C0 ---- CONFIRMED */
/* base precedes offset in retail's parameter list: VC7 orders the commutative
   base + offset by declaration (lea esi,[base+offset]); the stack order of the
   non-register arguments is unchanged. */
int __cdecl MSS_Read( signed int size, int base, int offset, unsigned int count, void *dst )
{
	if ( (int)( offset + count ) > size ) {
		Com_Error( ERR_DROP, MSS_ERR_RESTORE_OVERFLOW );
	}
	memcpy( dst, (const void *)( base + offset ), count );
	return offset + count;
}

/* ---- MSS_SaveChanInfo  0x00450300 ---- HIGH */
/* Serializes the persistent part of a channel into the save buffer (27 bytes,
   tightly packed / unaligned): entnum (word), channel (byte), endTime, volume,
   blend, then localOrigin.  Bounds are re-checked before each field. */
/* (buffer, offset, size, channel) as in the Mac 1.3 build, like MSS_Write. */
static int MSS_SaveChanInfo( int base, int offset, int size, int chanPtr )
{
	const mssChannel_t *ch = (const mssChannel_t *)chanPtr;

	offset = MSS_Write( base, offset, size, &ch->entnum, 2 );
	offset = MSS_Write( base, offset, size, &ch->channel, 1 );
	offset = MSS_Write( base, offset, size, &ch->endTime, 4 );
	offset = MSS_Write( base, offset, size, &ch->volume, 4 );
	offset = MSS_Write( base, offset, size, &ch->blend, 4 );
	return MSS_Write( base, offset, size, ch->localOrigin, 12 );
}

/* ---- MSS_RestoreChanInfo  0x00450410 ---- HIGH */
/* Inverse of MSS_SaveChanInfo: zero-fills the channel, then reads back the
   27-byte packed record.  Fields not saved (touchTime, rates, aliases, ...)
   stay zero, which is why the whole struct is cleared first. */
static int MSS_RestoreChanInfo( mssChannel_t *ch, int size, int base, int offset )
{
	memset( ch, 0, sizeof( *ch ) );

	offset = MSS_Read( size, base, offset, 2, &ch->entnum );
	offset = MSS_Read( size, base, offset, 1, &ch->channel );
	offset = MSS_Read( size, base, offset, 4, &ch->endTime );
	offset = MSS_Read( size, base, offset, 4, &ch->volume );
	offset = MSS_Read( size, base, offset, 4, &ch->blend );
	offset = MSS_Read( size, base, offset, 12, &ch->localOrigin );
	return offset;
}

extern void *Com_GetSoundAlias( int source, int index );
extern int   Com_SoundAliasIndex( const void *alias, int source );
extern int   Com_SoundAliasChecksum( int source );

#define MSS_ALIAS_SOURCE    1

#define MSS_TYPE_LOADED     1
#define MSS_TYPE_STREAMED   2

typedef struct {
	float   startFraction;
	float   pitch;
	float   volume;
	vec3_t  position;
} mssSave3D_t;

typedef struct {
	float   startFraction;
	float   pitch;
	float   volume;
	float   pan;
} mssSave2D_t;

typedef struct {
	float   startFraction;
	int     rate;
	float   channelVolume;
	float   volume;
	float   pan;
	vec3_t  origin;
} mssSaveStream_t;

/* ---- MSS_Save3DChannel  0x00450530 ---- VERIFIED */
static int MSS_Save3DChannel( int chan, int base, int offset, int size )
{
	int           h;
	int           aliasIndex, alias2Index;
	unsigned int  sampleOffset, sampleLength;
	int           rate;
	float         vol;
	mssSave3D_t   saved;

	if ( MSS_Is3DChannelFree( chan ) ) {
		return offset;
	}

	h = MSS_3D_HANDLE( chan );
	aliasIndex = Com_SoundAliasIndex( mss_channels[chan].alias, MSS_ALIAS_SOURCE );
	if ( !aliasIndex ) {
		return offset;
	}
	alias2Index = Com_SoundAliasIndex( mss_channels[chan].alias2, MSS_ALIAS_SOURCE );
	if ( !alias2Index ) {
		return offset;
	}

	sampleOffset = AIL_3D_sample_offset( h );
	sampleLength = AIL_3D_sample_length( h );
	rate         = AIL_3D_sample_playback_rate( h );
	if ( !sampleLength || !rate ) {
		return offset;
	}

	/* Both unsigned ints are fild'ed straight into the divide, with no float
	   store of either operand (0x0045061B-0x00450648). */
	saved.startFraction = (float)( MSS_I2F( sampleOffset ) / MSS_I2F( sampleLength ) );
	saved.pitch         = mss_channels[chan].pitch;

	vol = AIL_3D_sample_volume( h );
	if ( mss_masterVolume != 0.0f ) {
		saved.volume = vol / mss_masterVolume;
	} else {
		saved.volume = mss_channels[chan].volume;
	}

	AIL_3D_position( h, &saved.position[0], &saved.position[1], &saved.position[2] );

	offset = MSS_Write( base, offset, size, &aliasIndex, 2 );
	offset = MSS_Write( base, offset, size, &alias2Index, 2 );
	offset = MSS_SaveChanInfo( base, offset, size, (int) &mss_channels[chan] );
	return MSS_Write( base, offset, size, &saved, sizeof( mssSave3D_t ) );
}

/* ---- MSS_Restore3DChannel  0x00450700 ---- VERIFIED */
static int MSS_Restore3DChannel( int base, int offset, int size, int aliasIndex )
{
	mssChannel_t ch;
	mssSave3D_t  saved;
	int          alias2Index;
	void        *alias, *alias2;
	int          chan;

	alias2Index = 0;
	offset = MSS_Read( size, base, offset, 2, &alias2Index );
	offset = MSS_RestoreChanInfo( &ch, size, base, offset );
	offset = MSS_Read( size, base, offset, sizeof(saved), &saved );

	alias  = Com_GetSoundAlias( MSS_ALIAS_SOURCE, aliasIndex );
	alias2 = Com_GetSoundAlias( MSS_ALIAS_SOURCE, alias2Index );

	if ( alias && alias2
		&& AL_TYPE( alias ) == MSS_TYPE_LOADED
		&& AL_TYPE( alias2 ) == MSS_TYPE_LOADED
		&& AL_SOUND( alias ) && AL_SOUND( alias2 ) == AL_SOUND( alias )
		&& MSS_ValidateSoundAliasBlend( (mssAlias_t *) alias, (mssAlias_t *) alias2, 0 ) )
	{
		MSS_StartAlias3DSample( alias, alias2, ch.blend, ch.entnum, saved.position,
		                        saved.volume, saved.pitch, 0,
		                        saved.startFraction, &chan );
		if ( chan >= 0 ) {
			VectorCopy( ch.localOrigin, mss_channels[chan].localOrigin );
		}
	}
	return offset;
}

/* ---- MSS_Save2DChannel  0x004508D0 ---- VERIFIED */
static int MSS_Save2DChannel( int chan, int base, int offset, int size )
{
	int           h;
	int           aliasIndex, alias2Index;
	int           totalMs, currentMs, rate;
	mssSave2D_t   saved;

	if ( MSS_Is2DChannelFree( chan ) ) {
		return offset;
	}

	h = MSS_2D_HANDLE(chan);
	aliasIndex = Com_SoundAliasIndex( mss_channels[chan].alias, MSS_ALIAS_SOURCE );
	if ( !aliasIndex ) {
		return offset;
	}
	alias2Index = Com_SoundAliasIndex( mss_channels[chan].alias2, MSS_ALIAS_SOURCE );
	if ( !alias2Index ) {
		return offset;
	}

	AIL_sample_ms_position( h, &totalMs, &currentMs );
	rate = AIL_sample_playback_rate( h );
	if ( !totalMs || !rate ) {
		return offset;
	}

	/* fild / fild / fdivp with no float store of either operand
	   (0x004509BC-0x004509F3). */
	saved.startFraction = (float)( MSS_I2F( (unsigned int) currentMs )
	                             / MSS_I2F( (unsigned int) totalMs ) );
	saved.pitch         = mss_channels[chan].pitch;

	AIL_sample_volume_pan( h, &saved.volume, &saved.pan );
	if ( mss_masterVolume != 0.0f ) {
		saved.volume = saved.volume / mss_masterVolume;
	} else {
		saved.volume = mss_channels[chan].volume;
	}

	offset = MSS_Write( base, offset, size, &aliasIndex, 2 );
	offset = MSS_Write( base, offset, size, &alias2Index, 2 );
	offset = MSS_SaveChanInfo( base, offset, size, (int) &mss_channels[chan] );
	return MSS_Write( base, offset, size, &saved, sizeof( mssSave2D_t ) );
}

/* ---- MSS_Restore2DChannel  0x00450AA0 ---- VERIFIED */
static int MSS_Restore2DChannel( int base, int offset, int size, int aliasIndex )
{
	mssChannel_t ch;
	mssSave2D_t  saved;
	void        *alias, *alias2;
	int          chan;

	if ( offset + 2 > size ) {
		Com_Error( ERR_DROP, MSS_ERR_RESTORE_OVERFLOW );
	}

	offset = MSS_RestoreChanInfo( &ch, size, base, offset + 2 );

	if ( offset + (int) sizeof( saved ) > size ) {
		Com_Error( ERR_DROP, MSS_ERR_RESTORE_OVERFLOW );
	}
	memcpy( &saved, (const void *)( base + offset ), sizeof( saved ) );
	offset += sizeof( saved );

	alias  = Com_GetSoundAlias( MSS_ALIAS_SOURCE, aliasIndex );
	alias2 = Com_GetSoundAlias( MSS_ALIAS_SOURCE, aliasIndex );

	if ( alias && alias2
		&& AL_TYPE( alias ) == MSS_TYPE_LOADED
		&& AL_TYPE( alias2 ) == MSS_TYPE_LOADED
		&& AL_SOUND( alias ) && AL_SOUND( alias2 ) == AL_SOUND( alias )
		&& MSS_ValidateSoundAliasBlend( (mssAlias_t *) alias, (mssAlias_t *) alias2, 0 ) )
	{
		MSS_StartAlias2DSample( alias, alias2, ch.blend, ch.entnum,
		                        saved.volume, saved.pitch, 0,
		                        saved.startFraction, &chan );
		if ( chan >= 0 ) {
			AIL_set_sample_volume_pan( MSS_2D_HANDLE( chan ),
			                           mss_masterVolume * saved.volume,
			                           saved.pan );
			VectorCopy( ch.localOrigin, mss_channels[chan].localOrigin );
		}
	}
	return offset;
}

/* ---- MSS_SaveStreamChannel  0x00450C80 ---- VERIFIED */
static int MSS_SaveStreamChannel( int chan, int base, int offset, int size )
{
	int             h;
	mssStreamInfo_t *info;
	int             aliasIndex, alias2Index;
	int             totalMs, currentMs, rate;
	mssSaveStream_t saved;

	if ( chan >= MSS_FIRST_STREAM + MSS_NUM_BACKGROUND ) {
		if ( MSS_IsStreamChannelFree( chan ) ) {
			return offset;
		}
	} else {
		if ( chan == MSS_FIRST_STREAM + 1 || chan == MSS_FIRST_STREAM + 2 ) {
			return offset;
		}
		if ( MSS_IsStreamChannelFree( chan ) ) {
			aliasIndex = 0;
			return MSS_Write( base, offset, size, &aliasIndex, 2 );
		}
	}

	h = MSS_STREAM_HANDLE( chan );

	aliasIndex = Com_SoundAliasIndex( mss_channels[chan].alias, MSS_ALIAS_SOURCE );
	if ( !aliasIndex ) {
		return offset;
	}
	alias2Index = Com_SoundAliasIndex( mss_channels[chan].alias2, MSS_ALIAS_SOURCE );
	if ( !alias2Index ) {
		return offset;
	}

	AIL_stream_ms_position( h, &totalMs, &currentMs );
	if ( !totalMs ) {
		return offset;
	}
	info = &MSS_STREAM_INFO( chan );

	/* fild / fild / fdivp with no float store of either operand
	   (0x00450D8E-0x00450DBA). */
	saved.startFraction = (float)( MSS_I2F( (unsigned int)currentMs )
	                             / MSS_I2F( (unsigned int)totalMs ) );

	rate       = AIL_stream_playback_rate( h );
	saved.rate = MSS_FTOL( (double) rate / mss_timeScale );   /* fild / fdiv, no float store: 0x00450DC8 */

	saved.channelVolume = mss_channels[chan].volume;

	AIL_stream_volume_pan( h, &saved.volume, &saved.pan );
	if ( mss_masterVolume != 0.0f ) {
		saved.volume = saved.volume / mss_masterVolume;
	} else {
		saved.volume = mss_channels[chan].volume;
	}

	VectorCopy( info->origin, saved.origin );

	offset = MSS_Write( base, offset, size, &aliasIndex, 2 );
	offset = MSS_Write( base, offset, size, &alias2Index, 2 );
	offset = MSS_SaveChanInfo( base, offset, size, (int) &mss_channels[chan] );
	return MSS_Write( base, offset, size, &saved, sizeof( mssSaveStream_t ) );
}

/* ---- MSS_RestoreStreamChannel  0x00450EA0 ---- VERIFIED */
static int MSS_RestoreStreamChannel( int base, int offset, int size,
                                     int aliasIndex, int chan )
{
	mssChannel_t    ch;
	mssSaveStream_t saved;
	void           *alias, *alias2;
	int             duration;
	int             h;
	int             offsetMs;

	if ( offset + 2 > size ) {
		Com_Error( ERR_DROP, MSS_ERR_RESTORE_OVERFLOW );
	}

	offset = MSS_RestoreChanInfo( &ch, size, base, offset + 2 );

	if ( offset + (int) sizeof( saved ) > size ) {
		Com_Error( ERR_DROP, MSS_ERR_RESTORE_OVERFLOW );
	}
	memcpy( &saved, (const void *)( base + offset ), sizeof( saved ) );
	offset += sizeof( saved );

	alias  = Com_GetSoundAlias( MSS_ALIAS_SOURCE, aliasIndex );
	alias2 = Com_GetSoundAlias( MSS_ALIAS_SOURCE, aliasIndex );

	if ( alias && alias2
		&& AL_TYPE( alias ) == MSS_TYPE_STREAMED
		&& AL_TYPE( alias2 ) == MSS_TYPE_STREAMED
		&& MSS_ValidateSoundAliasBlend( (mssAlias_t *) alias, (mssAlias_t *) alias2, 0 ) )
	{
		if ( chan < 0 ) {
			duration = MSS_StartAliasStream( alias, alias2, ch.blend, ch.entnum,
			                                 saved.origin,
			                                 saved.channelVolume, 1.0f, 0,
			                                 saved.startFraction, &chan );
		} else {
			duration = MSS_StartAliasStreamOnChannel( alias, alias2, ch.blend,
			                                          ch.entnum, saved.origin,
			                                          saved.channelVolume, 1.0f,
			                                          0, saved.startFraction,
			                                          chan );
		}

		if ( chan >= 0 ) {
			h = MSS_STREAM_HANDLE( chan );

			/* Both ints are fild'ed straight into the multiply, with no float
			   store before it (0x0045105A, 0x004510A0). */
			offsetMs = MSS_FTOL( MSS_I2F( duration ) * saved.startFraction );
			mss_channels[chan].endTime -= offsetMs;

			AIL_set_stream_volume_pan( h, mss_masterVolume * saved.volume,
			                           saved.pan );
			AIL_set_stream_playback_rate( h,
				MSS_FTOL( MSS_I2F( saved.rate ) * mss_channels[chan].pitch
				          * mss_timeScale ) );

			VectorCopy( ch.localOrigin, mss_channels[chan].localOrigin );
		}
	}
	return offset;
}

/* ---- MSS_Save  0x004510F0 ---- VERIFIED */
int MSS_Save( int size, int base )
{
	int   offset;
	int   i;
	int   checksum;
	short terminator = 0;


	checksum = Com_SoundAliasChecksum( MSS_ALIAS_SOURCE );

	offset = MSS_Write( base, 0, size, &checksum, sizeof( checksum ) );
	offset = MSS_Write( base, offset, size, mss_channelFade, sizeof( mss_channelFade ) );

	offset = MSS_Write( base, offset, size, &mss_roomType, 16 );

	offset = MSS_Write( base, offset, size, &mss_bgFade[0], sizeof( mss_bgFade[0] ) );
	offset = MSS_Write( base, offset, size, &mss_bgFade[3], sizeof( mss_bgFade[3] ) );
	offset = MSS_Write( base, offset, size, &mss_bgFade[4], sizeof( mss_bgFade[4] ) );

	if ( mss_digitalDriver ) {
		for ( i = 0; i < mss_max3DChannels; i++ ) {
			offset = MSS_Save3DChannel( i, base, offset, size );
		}
	}
	offset = MSS_Write( base, offset, size, &terminator, 2 );

	if ( mss_digitalDriver ) {
		for ( i = MSS_FIRST_2D; i < mss_num2DChannels + MSS_FIRST_2D; i++ ) {
			offset = MSS_Save2DChannel( i, base, offset, size );
		}
	}
	offset = MSS_Write( base, offset, size, &terminator, 2 );

	if ( mss_digitalDriver ) {
		for ( i = MSS_FIRST_STREAM; i < mss_numStreamChannels + MSS_FIRST_STREAM; i++ ) {
			offset = MSS_SaveStreamChannel( i, base, offset, size );
		}
	}
	return MSS_Write( base, offset, size, &terminator, 2 );
}

/* ---- MSS_Restore  0x00451330 ---- VERIFIED */
void MSS_Restore( int buffer, int size )
{
	int offset;
	int i;
	int aliasIndex;
	int checksum;

	if ( !mss_digitalDriver ) {
		return;
	}

	offset = MSS_Read( size, buffer, 0, sizeof( checksum ), &checksum );
	if ( checksum != Com_SoundAliasChecksum( MSS_ALIAS_SOURCE ) ) {
		Com_Printf( "^3Couldn't restore sound state: sound aliases have changed since saving.\n" );
		return;
	}

	offset = MSS_Read( size, buffer, offset, sizeof(mss_channelFade), mss_channelFade );
	offset = MSS_Read( size, buffer, offset, 16, &mss_roomType );

	AIL_set_digital_master_room_type( mss_digitalDriver, mss_roomType );
	AIL_set_3D_room_type( mss_3DProvider, mss_roomType );

	offset = MSS_Read( size, buffer, offset, sizeof(mss_bgFade[0]), &mss_bgFade[0] );
	offset = MSS_Read( size, buffer, offset, sizeof(mss_bgFade[3]), &mss_bgFade[3] );
	offset = MSS_Read( size, buffer, offset, sizeof(mss_bgFade[4]), &mss_bgFade[4] );

	for ( ;; ) {
		aliasIndex = 0;
		offset = MSS_Read( size, buffer, offset, 2, &aliasIndex );
		if ( !aliasIndex ) {
			break;
		}
		offset = MSS_Restore3DChannel( buffer, offset, size, aliasIndex );
	}

	for ( ;; ) {
		aliasIndex = 0;
		offset = MSS_Read( size, buffer, offset, 2, &aliasIndex );
		if ( !aliasIndex ) {
			break;
		}
		offset = MSS_Restore2DChannel( buffer, offset, size, aliasIndex );
	}

	for ( i = 0; i < MSS_NUM_BACKGROUND; i++ ) {
		if ( i == 1 || i == 2 ) {
			continue;
		}
		aliasIndex = 0;
		offset = MSS_Read( size, buffer, offset, 2, &aliasIndex );
		if ( !aliasIndex ) {
			continue;
		}
		offset = MSS_RestoreStreamChannel( buffer, offset, size, aliasIndex,
		                                   MSS_FIRST_STREAM + i );
	}

	for ( ;; ) {
		aliasIndex = 0;
		offset = MSS_Read( size, buffer, offset, 2, &aliasIndex );
		if ( !aliasIndex ) {
			break;
		}
		offset = MSS_RestoreStreamChannel( buffer, offset, size, aliasIndex, -1 );
	}

	if ( offset != size ) {
		Com_Error( ERR_DROP, MSS_ERR_STATE_TOO_BIG );
	}
}

/* ---- MSS_QueueRestore_m  0x004515F0 ---- VERIFIED */
void __cdecl MSS_QueueRestore_m( int size, int *data )
{
	MSS_StopSounds( 6 );
	if ( mss_pendingRestore ) {
		free( mss_pendingRestore );
	}
	mss_pendingRestoreSize = size;
	mss_pendingRestore = Z_MallocInternal( size );
	Com_Memcpy( (int)mss_pendingRestore, data, size );
}

/* ---- MSS_GetSoundOverlay2D  0x00451630 ---- */
static int MSS_GetSoundOverlay2D( mssOverlay_t *buffer, int maxCount )
{
	int i, h, rate, baseRate, chan;

	if ( maxCount > mss_num2DChannels ) {
		maxCount = mss_num2DChannels;
	}

	for ( i = 0 ; i < maxCount ; i++ ) {
		chan = MSS_FIRST_2D + i;
		if ( MSS_Is2DChannelFree( chan ) ) {
			buffer[i].name = 0;
			continue;
		}
		buffer[i].name = AL_FILENAME( mss_channels[chan].alias );
		h = MSS_2D_HANDLE( chan );

		rate = AIL_sample_playback_rate( h );
		if ( !rate ) {
			rate = mss_channels[chan].baseRate;
		}
		buffer[i].requested = mss_channels[chan].volume * 1.25f;
		AIL_sample_volume_pan( h, &buffer[i].actual, 0 );
		buffer[i].actual = buffer[i].actual * 1.25f;
		if ( mss_masterVolume != 0.0f ) {
			buffer[i].actual = buffer[i].actual / mss_masterVolume;
		}
		baseRate = mss_channels[chan].baseRate;
		buffer[i].baseRate  = baseRate;
		/* fild / fidiv, no float store of either int (0x004516F4-0x00451707). */
		buffer[i].rateScale = (float)( (double)rate / (double)baseRate );
	}
	return maxCount;
}

/* ---- MSS_GetSoundOverlay3D  0x00451730 ---- */
static int MSS_GetSoundOverlay3D( mssOverlay_t *buffer, int maxCount )
{
	int    i, h, rate, baseRate;
	vec3_t pos;

	if ( maxCount > mss_max3DChannels ) {
		maxCount = mss_max3DChannels;
	}

	for ( i = 0 ; i < maxCount ; i++ ) {
		if ( MSS_Is3DChannelFree( i ) ) {
			buffer[i].name = 0;
			continue;
		}
		buffer[i].name = AL_FILENAME( mss_channels[i].alias );
		h = mss_3DHandles[i];

		AIL_3D_sample_length( h );
		rate = AIL_3D_sample_playback_rate( h );
		if ( !rate ) {
			rate = mss_channels[i].baseRate;
		}
		AIL_3D_position( h, &pos[0], &pos[1], &pos[2] );
		/* Unused, but the LTCG-inlined /Op helper is why retail stores every
		 * float in this loop (0x004517EA, 0x00451822-0x0045182D). */
		Distance( pos, mss_listenerOrigin );

		buffer[i].requested = mss_channels[i].volume * 1.25f;
		buffer[i].actual    = AIL_3D_sample_volume( h ) * 1.25f;
		if ( mss_masterVolume != 0.0f ) {
			buffer[i].actual = buffer[i].actual / mss_masterVolume;
		}
		baseRate = mss_channels[i].baseRate;
		buffer[i].baseRate  = baseRate;
		buffer[i].rateScale = (float)rate / (float)baseRate;
	}
	return maxCount;
}

/* ---- MSS_GetSoundOverlayStream  0x00451860 ---- */
static int MSS_GetSoundOverlayStream( mssOverlay_t *buffer, int maxCount )
{
	int i, h, rate, baseRate, chan;

	if ( maxCount > mss_numStreamChannels ) {
		maxCount = mss_numStreamChannels;
	}

	for ( i = 0 ; i < maxCount ; i++ ) {
		chan = MSS_FIRST_STREAM + i;
		if ( MSS_IsStreamChannelFree( chan ) ) {
			buffer[i].name = 0;
			continue;
		}
		buffer[i].name = AL_FILENAME( mss_channels[chan].alias );
		h = MSS_STREAM_HANDLE( chan );

		rate = AIL_stream_playback_rate( h );
		buffer[i].requested = mss_channels[chan].volume * 1.25f;
		AIL_stream_volume_pan( h, &buffer[i].actual, 0 );
		buffer[i].actual = buffer[i].actual * 1.25f;
		if ( mss_masterVolume != 0.0f ) {
			buffer[i].actual = buffer[i].actual / mss_masterVolume;
		}
		baseRate = mss_channels[chan].baseRate;
		buffer[i].baseRate  = baseRate;
		/* fild / fidiv, no float store of either int (0x00451922-0x00451938). */
		buffer[i].rateScale = (float)( (double)rate / (double)baseRate );
	}
	return maxCount;
}

/* ---- MSS_GetSoundOverlay  0x00451960 ---- VERIFIED */
int __cdecl MSS_GetSoundOverlay( int which, void *buffer, int maxCount, int *cpuOut )
{
	if ( !mss_digitalDriver ) {
		return 0;
	}
	if ( cpuOut ) {
		*cpuOut = mss_cpuPercent;
	}
	switch ( which ) {
	case 1:
		return MSS_GetSoundOverlay3D( buffer, maxCount );
	case 2:
		return MSS_GetSoundOverlayStream( buffer, maxCount );
	case 3:
		return MSS_GetSoundOverlay2D( buffer, maxCount );
	default:
		return 0;
	}
}
