// cg_public.h -- definitions shared by the cgame module and the client
// each module carries its own copy of the shared types (entityState_t is
// also declared in g_public.h and server.h)

#ifndef __CG_PUBLIC_H__
#define __CG_PUBLIC_H__

#include "../universal/q_shared.h"

#ifndef QDECL
#define QDECL   __cdecl
#endif

// compile-time field offset check
#define CG_ASSERT_OFFSET( type, member, off ) 	COD1_STATIC_ASSERT( offsetof( type, member ) == ( off ), #type "." #member )

typedef int qhandle_t;
typedef int sfxHandle_t;        // sound alias handle

// an animated model instance; owned by the engine and opaque to cgame
typedef struct DObj_s DObj;

#define MAX_GAMESTATE_CHARS     0x3E80

#define MAX_ENTITIES_IN_SNAPSHOT    256
#define MAX_CLIENTS_IN_SNAPSHOT     64

/*
=============================================================================

	THE SHARED ENTITY VIEW

=============================================================================
*/

typedef struct trajectory_t {
	int trType;
	int trTime;
	int trDuration;             // if non 0, trTime + trDuration = stop time
	vec3_t trBase;
	vec3_t trDelta;             // velocity, etc
} trajectory_t;
COD1_ASSERT_SIZE( trajectory_t, 36 );

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)

typedef struct entityState_s {
	int number;             // entity index
	int eType;              // entityType_t
	int eFlags;
	trajectory_t pos;       // for calculating position
	trajectory_t apos;      // for calculating angles
	int time;
	int time2;
	vec3_t origin2;
	vec3_t angles2;
	int otherEntityNum;     // shotgun sources, etc
	int attackerEntityNum;
	int groundEntityNum;        // -1 = in air
	int constantLight;      // r + (g<<8) + (b<<16) + (intensity<<24)
	int loopSound;          // constantly loop this sound
	int surfType;
	int index;
	int clientNum;          // 0 to (MAX_CLIENTS - 1), for players and corpses
	int iHeadIcon;
	int iHeadIconTeam;
	int solid;              // for client side prediction, trap_linkentity sets this properly
	int eventParm;
	int eventSequence;      // pmove generated events
	int events[4];
	unsigned int eventParms[4];
	int weapon;             // determines weapon and flash model, etc
	int legsAnim;           // mask off ANIM_TOGGLEBIT
	int torsoAnim;          // mask off ANIM_TOGGLEBIT
	float leanf;
	int scale;
	int dmgFlags;           // to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities
	int animMovetype;       // clients can't derive movetype of other clients for anim scripting system
	float fTorsoHeight;
	float fTorsoPitch;
	float fWaistPitch;
} entityState_t;
COD1_ASSERT_SIZE( entityState_t, 240 );

// the per-client information the server sends along with each snapshot
typedef struct clientState_s {
	int clientNum;
	int team;
	int modelIndex;                 // CS_MODELS index
	int attachModelIndex[6];        // MAX_ATTACHED_MODELS, CS_MODELS indexes
	int attachTagIndex[6];          // CS_TAGS indexes
	char name[32];
} clientState_t;
COD1_ASSERT_SIZE( clientState_t, 92 );
CG_ASSERT_OFFSET( clientState_t, name, 0x3C );

typedef struct gameState_t {
	int stringOffsets[MAX_CONFIGSTRINGS];
	char stringData[MAX_GAMESTATE_CHARS];
	int dataCount;
} gameState_t;
COD1_ASSERT_SIZE( gameState_t, 0x5E84 );

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct snapshot_t {
	int snapFlags;                      // SNAPFLAG_RATE_DELAYED, etc
	int ping;
	int serverTime;                 // server time the message is valid for (in msec)
	playerState_t ps;                       // complete information about the current player at this time
	int numEntities;                        // all of the entities that need to be presented
	int numClients;
	entityState_t entities[MAX_ENTITIES_IN_SNAPSHOT];   // at the time of this snapshot
	clientState_t clients[MAX_CLIENTS_IN_SNAPSHOT];
	int numServerCommands;                  // text based server commands to execute when this
	int serverCommandSequence;              // snapshot becomes current
} snapshot_t;
COD1_ASSERT_SIZE( snapshot_t, 0x127EC );

/*
=============================================================================

	THE SHARED RENDER VIEW

=============================================================================
*/

/*
** glconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
typedef struct glconfig_t {
	char   *renderer_string;
	char   *vendor_string;
	char   *version_string;
	char   *extensions_string;
	char   *wextensions_string;    // WGL extensions
	int     maxTextureSize;                             // queried from GL
	int     maxActiveTextures;                          // multitexture ability
	int     maxLights;
	int     colorBits;
	int     depthBits;
	int     stencilBits;
	int     deviceSupportsGamma;
	int     maxAnisotropy;
	int     maxTextureFilterAnisotropy;
	int     textureEnvAddAvailable;
	int     textureCubeMap;
	int     textureEnvCombine;
	int     textureEnvDot3;
	int     ARBVertexBufferObject;
	int     ARBVertexProgram;
	int     rescaleNormal;
	int     NVFogAvailable;
	int     NVFogMode;
	int     NVVertexArrayRange;
	int     NVFenceAvailable;
	int     NVRegisterCombiners;
	int     NVTextureShader;
	int     maxPNTrianglesTessellationLevel;
	int     pnTrianglesNormalMode;
	int     pnTrianglesPointMode;
	int     ATIVertexArrayObject;
	int     ATIElementArray;
	int     ATIFragmentShader;
	int     vidWidth;
	int     vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float   windowAspect;
	int     displayFrequency;
	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	int     isFullscreen;
	int     stereoEnabled;
	qboolean smpActive;                     // dual processor
} glconfig_t;
COD1_ASSERT_SIZE( glconfig_t, 160 );
CG_ASSERT_OFFSET( glconfig_t, NVFogMode, 0x58 );

// CoD drops RTCW's skinNum, customSkin, torso and backlerp members
typedef struct refEntity_s {
	int         reType;
	int         renderfx;
	qhandle_t   hModel;               // opaque type outside refresh

	// most recent data
	vec3_t      lightingOrigin;          // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float       shadowPlane;              // projection shadows go here, stencils go slightly lower

	vec3_t      axis[3];                 // rotation vectors
	float       nonNormalizedAxes;     // axis are not normalized, i.e. they have scale
	vec3_t      origin;                // also used as MODEL_BEAM's "from"
	int         frame;                      // also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	vec3_t      oldorigin;             // also used as MODEL_BEAM's "to"
	int         oldframe;
	float       spriteSize;     // name inferred

	// texturing
	qhandle_t   customShader;         // use one image for the entire thing

	// misc
	byte        shaderRGBA[4];             // colors used by rgbgen entity shaders
	float       shaderTexCoord[2];        // texture coordinates used by tcMod entity modifiers
	float       shaderTime;               // subtracted from refdef time to control effect start times

	// extra sprite information
	float       radius;
	float       rotation;

	vec3_t      fireRiseDir;
	DObj       *dobj;
	void       *owner;          // the centity_t
	void       *staticModel;
} refEntity_t;
COD1_ASSERT_SIZE( refEntity_t, 156 );
CG_ASSERT_OFFSET( refEntity_t, oldframe, 0x60 );
CG_ASSERT_OFFSET( refEntity_t, shaderRGBA, 0x6C );
CG_ASSERT_OFFSET( refEntity_t, shaderTexCoord, 0x70 );
CG_ASSERT_OFFSET( refEntity_t, fireRiseDir, 0x84 );
CG_ASSERT_OFFSET( refEntity_t, dobj, 0x90 );

typedef struct refdef_s {
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];             // transformation matrix
	int time;           // time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;                    // RDF_NOWORLDMODEL, etc
} refdef_t;
COD1_ASSERT_SIZE( refdef_t, 80 );

// CoD carries a second texture coordinate pair
typedef struct polyVert_t {
	vec3_t xyz;
	float st[2];
	float st2[2];                   // name inferred
	byte modulate[4];
} polyVert_t;
COD1_ASSERT_SIZE( polyVert_t, 32 );

// markfragments are returned by CM_MarkFragments()
typedef struct markFragment_t {
	qhandle_t   markShader;         // shader of the surface the fragment was clipped from
	int         firstPoint;
	int         numPoints;
} markFragment_t;
COD1_ASSERT_SIZE( markFragment_t, 12 );

#define MAX_TOKENLENGTH     1024

typedef struct pc_token_s {
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;
COD1_ASSERT_SIZE( pc_token_t, 1040 );

/*
==================================================================

functions imported from the main executable

==================================================================
*/

typedef enum
{
	CG_PRINT                               =   0,
	CG_ERROR                               =   1,
	CG_GAMEMESSAGE                         =   2,
	CG_BOLDGAMEMESSAGE                     =   3,
	CG_DEATHMESSAGE                        =   4,
	CG_SUBTITLE                            =   5,
	CG_MILLISECONDS                        =   6,
	CG_CVAR_REGISTER                       =   7,
	CG_CVAR_UPDATE                         =   8,
	CG_CVAR_SET                            =   9,
	CG_CVAR_SETVAR                         =  10,
	CG_CVAR_VARIABLESTRINGBUFFER           =  11,
	CG_ARGC                                =  12,
	CG_ARGV                                =  13,
	CG_ARGS                                =  14,
	CG_FS_FOPENFILE                        =  15,
	CG_FS_READ                             =  16,
	CG_FS_WRITE                            =  17,
	CG_FS_FCLOSEFILE                       =  18,
	CG_FS_GETFILELIST                      =  19,
	CG_COM_SAVECVARSTOBUFFER               =  20,
	CG_COM_LOADCVARSFROMBUFFER             =  21,
	CG_SENDCONSOLECOMMAND                  =  22,
	CG_ADDCOMMAND                          =  23,
	CG_SENDCLIENTCOMMAND                   =  24,
	CG_UPDATESCREEN                        =  25,
	CG_DRAWNOTIFYLINES                     =  26,
	CG_DRAWBOLDMESSAGES                    =  27,
	CG_DRAWMINICONSOLE                     =  28,
	CG_DRAWSUBTITLES                       =  29,
	CG_DRAWSAY                             =  30,
	CG_CM_LOADMAP                          =  31,
	CG_CM_NUMINLINEMODELS                  =  32,
	CG_CM_INLINEMODEL                      =  33,
	// 34 is unused
	CG_CM_TEMPBOXMODEL                     =  35,
	CG_CM_POINTCONTENTS                    =  36,
	CG_CM_TRANSFORMEDPOINTCONTENTS         =  37,
	CG_CM_BOXTRACE                         =  38,
	CG_CM_TRANSFORMEDBOXTRACE              =  39,
	CG_CM_CAPSULETRACE                     =  40,
	CG_CM_TRANSFORMEDCAPSULETRACE          =  41,
	CG_CM_TEMPCAPSULEMODEL                 =  42,
	CG_R_MARKFRAGMENTS                     =  43,
	CG_R_LOADWORLDMAP                      =  44,
	CG_R_REGISTERMODEL                     =  45,
	CG_R_FINISHLOADINGMODELS               =  46,
	CG_R_SETIGNOREPRECACHEERRORS           =  47,
	CG_R_REGISTERSHADER                    =  48,
	CG_R_GETSHADERFROMMODEL                =  49,
	CG_R_GETXMODELBYHANDLE                 =  50,
	CG_R_REGISTERFONT                      =  51,
	CG_R_TEXT_WIDTH                        =  52,
	CG_R_TEXT_HEIGHT                       =  53,
	CG_R_TEXT_PAINT                        =  54,
	CG_R_TEXT_PAINTWITHCURSOR              =  55,
	CG_SE_TRANSLATEREFERENCE               =  56,
	CG_SE_LOCALIZEMESSAGE                  =  57,
	CG_SE_PRINTSTRLEN                      =  58,
	CG_SE_READCHARFROMSTRING               =  59,
	CG_R_CLEARSCENE                        =  60,
	CG_R_ADDREFENTITYTOSCENE               =  61,
	CG_R_GETENTITYTOKEN                    =  62,
	CG_HUNKUSED                            =  63,
	CG_R_ADDPOLYTOSCENE                    =  64,
	CG_R_ADDPOLYSTOSCENE                   =  65,
	CG_R_ADDLIGHTTOSCENE                   =  66,
	CG_R_ADDCORONATOSCENE                  =  67,
	CG_R_SETFOG                            =  68,
	CG_R_RENDERSCENE                       =  69,
	CG_R_SAVESCREEN                        =  70,
	CG_R_BLENDSAVEDSCREEN                  =  71,
	CG_R_SETCOLOR                          =  72,
	CG_R_DRAWSTRETCHPIC                    =  73,
	CG_R_DRAWSTRETCHPICGRADIENT            =  74,
	CG_R_DRAWSTRETCHPICROTATE              =  75,
	CG_R_DRAWQUADPIC                       =  76,
	CG_R_MODELBOUNDS                       =  77,
	CG_GETGLCONFIG                         =  78,
	CG_GETGAMESTATE                        =  79,
	CG_GETCURRENTSNAPSHOTNUMBER            =  80,
	CG_GETSNAPSHOT                         =  81,
	CG_GETSERVERCOMMAND                    =  82,
	CG_GETCURRENTCMDNUMBER                 =  83,
	CG_GETUSERCMD                          =  84,
	CG_SETUSERCMDVALUE                     =  85,
	CG_SETUSERCMDAIMVALUES                 =  86,
	CG_SETUSERCMDINSHELLSHOCK              =  87,
	CG_R_REGISTERSHADERNOMIP               =  88,
	CG_MEMORYREMAINING                     =  89,
	CG_KEY_ISDOWN                          =  90,
	CG_KEY_GETCATCHER                      =  91,
	CG_KEY_SETCATCHER                      =  92,
	CG_KEY_GETKEY                          =  93,
	CG_PC_ADDGLOBALDEFINE                  =  94,
	CG_PC_LOADSOURCE                       =  95,
	CG_PC_FREESOURCE                       =  96,
	CG_PC_READTOKEN                        =  97,
	CG_PC_SOURCEFILEANDLINE                =  98,
	CG_REALTIME                            =  99,
	CG_SNAPVECTOR                          = 100,
	// 101 is Cmd_RemoveCommand, which cgame never issues
	CG_CIN_PLAYCINEMATIC                   = 102,
	CG_CIN_STOPCINEMATIC                   = 103,
	CG_CIN_RUNCINEMATIC                    = 104,
	CG_CIN_DRAWCINEMATIC                   = 105,
	CG_CIN_SETEXTENTS                      = 106,
	CG_R_TRACKSTATISTICS                   = 107,
	CG_R_PICKSHADER                        = 108,
	// 109-117 and 120 are the qvm memset / memcpy / strncpy / math traps
	CG_PRINT_INT                           = 118,   // Com_Printf( "%s%i\n", s, i )
	CG_PRINT_FLOAT                         = 119,   // Com_Printf( "%s%f\n", s, f )
	CG_UI_LOADMENU                         = 121,
	CG_UI_POPUP                            = 122,
	CG_UI_CLOSEPOPUP                       = 123,
	CG_UI_CLOSEALLMENUS                    = 124,
	CG_UI_GETMAPDISPLAYNAME                = 125,
	CG_UI_GETGAMETYPEDISPLAYNAME           = 126,
	CG_CL_GETSERVERIPADDRESS               = 127,
	CG_XANIMPRECACHE                       = 128,
	CG_XANIMCREATEANIMS                    = 129,
	CG_XANIMCREATE                         = 130,
	CG_XANIMCREATETREE                     = 131,
	CG_XANIMBLEND                          = 132,
	CG_DOBJFREE                            = 133,
	CG_XANIMCLEARGOALWEIGHT                = 134,
	CG_XANIMCLEARTREEGOALWEIGHTS           = 135,
	CG_XANIMCLEARTREEGOALWEIGHTSSTRICT     = 136,
	CG_XANIMSETANIMKNOB                    = 137,
	CG_XANIMSETCOMPLETEGOALWEIGHTKNOBALL   = 138,
	CG_XANIMCLEARANIMCHILDREN              = 139,
	CG_XANIMSETANIMLIMITED                 = 140,
	CG_XANIMSETCOMPLETEGOALWEIGHT          = 141,
	CG_XANIMSETANIMRATE                    = 142,
	CG_XANIMISLOOPED                       = 143,
	CG_XANIMISLOOPEDINDEX                  = 144,
	CG_XANIMSETTIME                        = 145,
	CG_XANIMGETTIME                        = 146,
	CG_XANIMGETWEIGHT                      = 147,
	CG_DOBJINVALIDATESKELS                 = 148,
	CG_DOBJUPDATECLIENTINFO                = 149,
	CG_XANIMGETDEFERREDNOTIFIES            = 150,
	CG_DOBJCALCANIM                        = 151,
	CG_DOBJDISPLAYANIM                     = 152,
	CG_XANIMCALCRELDELTA                   = 153,
	CG_XANIMCALCABSDELTA                   = 154,
	CG_XANIMGETRELDELTA                    = 155,
	CG_XANIMGETABSDELTA                    = 156,
	CG_DOBJGETMATRIXARRAY                  = 157,
	CG_DOBJGETROTTRANSARRAY                = 158,
	CG_DOBJSETROTTRANSINDEX                = 159,
	CG_DOBJSETCONTROLROTTRANSINDEX         = 160,
	CG_XANIMGETANIMNAME                    = 161,
	CG_GETCLIENTDOBJ                       = 162,
	CG_DOBJCREATEDIRECT                    = 163,
	CG_DOBJCREATE                          = 164,
	CG_SAFEDOBJFREE                        = 165,
	CG_DEREF_DWORD                         = 166,
	CG_DOBJGETSKELSIZE                     = 167,
	CG_DOBJCREATESKELFORBONE               = 168,
	CG_DOBJCREATESKELFORBONES              = 169,
	CG_DOBJGETHIERARCHYBITS                = 170,
	CG_DOBJCALCSKEL                        = 171,
	CG_XMODELEXISTS                        = 172,
	CG_DOBJNUMPARTS                        = 173,
	CG_DOBJGETBONEINDEX                    = 174,
	CG_DOBJGETBONENAME                     = 175,
	CG_DOBJBUILDPARTCOLLISIONTABLE         = 176,
	CG_DOBJGETTREE                         = 177,
	CG_XANIMISPRIMITIVE                    = 178,
	CG_XANIMGETLENGTH                      = 179,
	CG_XANIMHASTIME                        = 180,
	CG_XANIMGETNUMCHILDREN                 = 181,
	CG_XANIMGETCHILDAT                     = 182,
	CG_XANIMGETANIMTREESIZE                = 183,
	CG_XANIMCOPYTREE                       = 184,
	CG_DOBJDUMPINFO                        = 185,   // developer only
	CG_STATMON_WARNING                     = 186,
	CG_STATMON_GETSTATSARRAY               = 187,
	CG_Z_MALLOCINTERNAL                    = 188,
	CG_Z_FREEINTERNAL                      = 189,
	CG_COM_LOADSOUNDALIASES                = 190,
	CG_COM_SOUNDALIASSTRING                = 191,
	CG_COM_PICKSOUNDALIAS                  = 192,
	CG_COM_GETSOUNDALIAS                   = 193,
	CG_MSS_PLAYSOUNDALIAS                  = 194,
	CG_MSS_PLAYBLENDEDSOUNDALIASES         = 195,
	CG_SURFACETYPEFROMNAME                 = 196,
	CG_SURFACETYPETONAME                   = 197,
	CG_ADD_DEBUG_LINE                      = 198,
	CG_GETWEAPONINFOMEMORY                 = 199,
	CG_FREEWEAPONINFOMEMORY                = 200,
	CG_HUNK_ALLOCINTERNAL                  = 201,   // 32 byte aligned
	CG_HUNKALLOCLOWINTERNAL                = 202,   // 32 byte aligned
	CG_HUNK_ALLOCALIGNINTERNAL             = 203,
	CG_HUNKALLOCLOWALIGNINTERNAL           = 204,
	CG_SCR_GETNUMVARS                      = 205,
	CG_SCR_GETNUMTHREADS                   = 206,
	CG_SCR_GETSTRINGUSAGE                  = 207,
	CG_SETCLIENTLERPORIGIN                 = 208,
	CG_MSS_SETLISTENER                     = 209,
	CG_MSS_UPDATELOOPINGSOUNDS             = 210,
	CG_MSS_STOPSOUNDS                      = 211,
	CG_MSS_PLAYMUSICALIAS                  = 212,
	CG_MSS_STOPBACKGROUND                  = 213,
	CG_MSS_PLAYAMBIENTALIAS                = 214,
	CG_MSS_FADEALLSOUNDS                   = 215,
	CG_MSS_FADESELECTSOUNDS                = 216,
	CG_MSS_SETENVIRONMENTEFFECTS           = 217,
	CG_MSS_GETSOUNDOVERLAY                 = 218,
	CG_KEY_GETBINDINGBUF                   = 219,
	CG_KEY_SETBINDING                      = 220,
	CG_KEY_KEYNUMTOSTRINGBUF               = 221,
	CG_FX_REGISTEREFFECT                   = 222,
	CG_FX_GETBONEINDEX                     = 223,
	CG_FX_PLAYSIMPLEEFFECT                 = 224,
	CG_FX_PLAYEFFECT                       = 225,
	CG_FX_PLAYBOLTEDEFFECT                 = 226,
	CG_FX_PLAYSIMPLEEFFECTID               = 227,
	CG_FX_PLAYEFFECTID                     = 228,
	CG_FX_PLAYORIENTEDEFFECTID             = 229,
	CG_FX_ADDSCHEDULEDEFFECTS              = 230,
	CG_FX_INIT                             = 231,
	CG_FX_FREEALL                          = 232,
	CG_FX_FREEEFFECTS                      = 233,
	CG_FX_ADJUSTTIME                       = 234,
	CG_FX_ADJUSTCAMERA                     = 235,
	CG_UNUSED_236                          = 236,   // no engine case
	CG_UNUSED_237                          = 237,   // no engine case
	CG_UNUSED_238                          = 238,   // no engine case
	CG_UNUSED_239                          = 239,   // no engine case
	CG_SETMOUSEMAXSPEEDS                   = 240,
	CG_CL_FIRSTSNAPSHOT                    = 241
} cgameImport_t;

/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef enum
{
	CG_INIT                 = 0,
//	void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN             = 1,
//	void (*CG_Shutdown)( void );
	// oportunity to flush and close any open files

	CG_CONSOLE_COMMAND      = 2,
//	qboolean (*CG_ConsoleCommand)( void );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	// command is not known to the game

	CG_DRAW_ACTIVE_FRAME    = 3,
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER     = 4,
//	int (*CG_CrosshairPlayer)( void );

	CG_KEY_EVENT            = 5,
	CG_CHECK_EXEC_KEY       = 6,    // CG_KeyInterceptEvent
	CG_MOUSE_EVENT          = 7,
	CG_EVENT_HANDLING       = 8,
	// 9-14, 17 and 18 are game model, DObj and impact mark callbacks
	CG_PROFILE_DRAW_TRAP    = 15,   // CG_DrawStringExt
	CG_SCRIPT_HOOK          = 16
} cgameExport_t;

#endif  // __CG_PUBLIC_H__
