// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#ifdef MISSIONPACK
#include "cg_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;
#endif

static int forceModelModificationCount = -1;
static int enemyModelModificationCount  = -1;
static int enemyColorsModificationCount = -1;
static int teamModelModificationCount  = -1;
static int teamColorsModificationCount = -1;
static int breadcrumbModificationCount = -1;
static int lazyloadModificationCount = -1;
static int customheightModificationCount = -1;
static int customwidthModificationCount = -1;

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );

// extension interface
qboolean intShaderTime = qfalse;
qboolean linearLight = qfalse;

#ifdef Q3_VM
qboolean (*trap_GetValue)( char *value, int valueSize, const char *key );
void (*trap_R_AddRefEntityToScene2)( const refEntity_t *re );
void (*trap_R_AddLinearLightToScene)( const vec3_t start, const vec3_t end, float intensity, float r, float g, float b );
#else
int dll_com_trapGetValue;
int dll_trap_R_AddRefEntityToScene2;
int dll_trap_R_AddLinearLightToScene;
#endif

#ifdef BUILD_GAME_STATIC
void CG_ResetGlobals(void) {
  static int instance = 0;
  instance++;
  
  
  
}
#endif

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
#ifdef BUILD_GAME_STATIC
intptr_t CG_Call( int command, int arg0, int arg1, int arg2 )
#else
DLLEXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2 )
#endif
{
	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0, arg1, arg2 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
#ifdef BUILD_GAME_STATIC
    //Init_Display(&cgDC);
#endif
		CG_DrawActiveFrame( arg0, arg1, arg2 );
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
#ifdef MISSIONPACK
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
#endif
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
  case CG_EXPORT_LAST:
    return 0;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS*MAX_CLASSES];
itemInfo_t			cg_items[MAX_ITEMS];


vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_railTrailRadius;
vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_drawWeaponSelect;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_drawSpeed;
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_buildScript;
vmCvar_t 	cg_forceModel;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
#ifdef MISSIONPACK
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
#endif
vmCvar_t	cg_hudFiles;
vmCvar_t 	cg_scorePlum;
vmCvar_t 	cg_damagePlum;
vmCvar_t 	cg_itemTimer;
vmCvar_t 	cg_smoothClients;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_oldRail;
vmCvar_t	cg_oldRocket;
vmCvar_t	cg_oldPlasma;
vmCvar_t	cg_trueLightning;

#ifdef MISSIONPACK
vmCvar_t 	cg_redTeamName;
vmCvar_t 	cg_blueTeamName;
vmCvar_t	cg_currentSelectedPlayer;
vmCvar_t	cg_currentSelectedPlayerName;
vmCvar_t	cg_singlePlayer;
vmCvar_t	cg_enableDust;
vmCvar_t	cg_enableBreath;
vmCvar_t	cg_singlePlayerActive;
vmCvar_t	cg_recordSPDemo;
vmCvar_t	cg_recordSPDemoName;
vmCvar_t	cg_obeliskRespawnDelay;
#endif

vmCvar_t	cg_hitSounds;

vmCvar_t	cg_enemyModel;
vmCvar_t	cg_enemyColors;
vmCvar_t	cg_teamModel;
vmCvar_t	cg_teamColors;

vmCvar_t	cg_deadBodyDarken;
vmCvar_t	cg_fovAdjust;
vmCvar_t	cg_followKiller;

#ifdef USE_WEAPON_VARS
vmCvar_t  cgwp_gauntCycle;
vmCvar_t  cgwp_lightCycle;
vmCvar_t  cgwp_shotgunCycle;
vmCvar_t  cgwp_machineCycle;
vmCvar_t  cgwp_grenadeCycle;
vmCvar_t  cgwp_rocketCycle;
vmCvar_t  cgwp_plasmaCycle;
vmCvar_t  cgwp_railCycle;
vmCvar_t  cgwp_bfgCycle;
#ifdef USE_GRAPPLE
vmCvar_t  cgwp_grappleCycle;
#endif
#ifdef MISSIONPACK
vmCvar_t  cgwp_nailCycle;
vmCvar_t  cgwp_proxCycle;
vmCvar_t  cgwp_chainCycle;
#endif
#ifdef USE_FLAME_THROWER
vmCvar_t  cgwp_flameCycle;
#endif
#endif // USE_WEAPON_VARS

#ifdef USE_PHYSICS_VARS
#ifdef MISSIONPACK
vmCvar_t  cg_scoutFactor;
#endif
vmCvar_t  cg_hasteFactor;
vmCvar_t	cg_jumpVelocity;
vmCvar_t	cg_gravity;
vmCvar_t	cg_wallWalk;
#endif
#ifdef USE_PORTALS
vmCvar_t  cgwp_portalEnable;
vmCvar_t  cg_altPortal;
#endif
#ifdef USE_GRAPPLE
vmCvar_t  cgwp_grappleEnable;
vmCvar_t  cgwp_grapplePull;
vmCvar_t  cg_altGrapple;
#endif
#ifdef USE_WEAPON_CENTER
vmCvar_t  cg_gunCenter;
#endif
#ifdef USE_WEAPON_ORDER
vmCvar_t  cg_weaponOrder;
int cg_weaponsCount = -1; //WarZone
#endif

vmCvar_t  cg_developer;
vmCvar_t  cg_breadCrumb;
vmCvar_t  cg_lazyLoad;
vmCvar_t  cg_customHeight;
vmCvar_t  cg_customWidth;
vmCvar_t  cg_atmosphere;
vmCvar_t  cg_atmosphericEffects;
int atmosphereModificationCount = -1;

typedef struct {
	vmCvar_t	*vmCvar;
	const char	*cvarName;
	const char	*defaultString;
	const int	cvarFlags;
} cvarTable_t;

static const cvarTable_t cvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0 }, // used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
#ifdef USE_3D_WEAPONS
	{ &cg_draw3dIcons, "cg_draw3dIcons", "2", CVAR_ARCHIVE },
#else
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE },
#endif
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE },
	{ &cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE },
	{ &cg_drawWeaponSelect, "cg_drawWeaponSelect", "1", CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE  },
	{ &cg_railTrailRadius, "cg_railTrailRadius", "0", CVAR_ARCHIVE  },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup , "cg_bobup", "0.005", CVAR_ARCHIVE },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", 0 },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE  },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE  },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
	{ &cg_developer, "developer", "", 0 },
#ifdef MISSIONPACK
	{ &cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE },
#else
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
#endif
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE },
#endif
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0 },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{ &cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},
	{ &cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO},
	{ &cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO},
	{ &cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{ &cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO},
	{ &cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE},
#endif
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", 0},
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{ &cg_timescale, "timescale", "1", 0},
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
#ifdef USE_DAMAGE_PLUMS
  { &cg_damagePlum, "cg_damagePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},
#endif
#ifdef USE_ITEM_TIMERS
  { &cg_itemTimer, "cg_itemTimers", "1", CVAR_USERINFO | CVAR_ARCHIVE},
#endif
#ifdef USE_PHYSICS_VARS
#ifdef MISSIONPACK
  { &cg_scoutFactor, "cg_scoutFactor", "1.5", CVAR_SERVERINFO},
#endif
  { &cg_hasteFactor, "g_hasteFactor", "1.3", CVAR_SERVERINFO},
  { &cg_jumpVelocity, "g_jumpVelocity", "270", CVAR_SERVERINFO},
  { &cg_gravity, "g_gravity", "800", CVAR_SERVERINFO},
  { &cg_wallWalk, "g_wallWalk", "0.7", CVAR_SERVERINFO},
#endif
#ifdef USE_WEAPON_ORDER
  { &cg_weaponOrder, "cg_weaponOrder", "1/2/3/4/6/8/5/7/9", CVAR_ARCHIVE }, //WarZone
#endif
#ifdef USE_WEAPON_CENTER
  { &cg_gunCenter, "cg_gunCenter", "1", CVAR_ARCHIVE },
#endif
#ifdef USE_PORTALS
  { &cgwp_portalEnable,  "wp_portalEnable", "1", CVAR_SERVERINFO },
  { &cg_altPortal,       "g_altPortal", "0", CVAR_SERVERINFO },
#endif
#ifdef USE_GRAPPLE
  { &cgwp_grappleEnable, "wp_grappleEnable", "1", CVAR_SERVERINFO },
  { &cgwp_grapplePull,  "wp_grapplePull", "700", CVAR_SERVERINFO },
  { &cg_altGrapple,  "g_altGrapple", "1", CVAR_SERVERINFO },
#endif
#ifdef USE_WEAPON_VARS
  { &cgwp_gauntCycle, "wp_gauntCycle", "400", CVAR_SERVERINFO },
  { &cgwp_lightCycle, "wp_lightCycle", "50", CVAR_SERVERINFO },
  { &cgwp_shotgunCycle, "wp_shotgunCycle", "1000", CVAR_SERVERINFO },
  { &cgwp_machineCycle, "wp_machineCycle", "100", CVAR_SERVERINFO },
  { &cgwp_grenadeCycle, "wp_grenadeCycle", "800", CVAR_SERVERINFO },
  { &cgwp_rocketCycle, "wp_rocketCycle", "800", CVAR_SERVERINFO },
  { &cgwp_plasmaCycle, "wp_plasmaCycle", "100", CVAR_SERVERINFO },
  { &cgwp_railCycle, "wp_railCycle", "1500", CVAR_SERVERINFO },
  { &cgwp_bfgCycle, "wp_bfgCycle", "200", CVAR_SERVERINFO },
#ifdef USE_GRAPPLE
  { &cgwp_grappleCycle, "wp_grappleCycle", "400", CVAR_SERVERINFO },
#endif
#ifdef MISSIONPACK
  { &cgwp_nailCycle, "wp_nailCycle", "1000", CVAR_SERVERINFO },
  { &cgwp_proxCycle, "wp_proxCycle", "800", CVAR_SERVERINFO },
  { &cgwp_chainCycle, "wp_chainCycle", "30", CVAR_SERVERINFO },
#endif
#ifdef USE_FLAME_THROWER
  { &cgwp_flameCycle, "wp_flameCycle", "40", CVAR_SERVERINFO },
#endif
#endif
	{ &cg_atmosphere, "g_atmosphere", "", CVAR_SERVERINFO | CVAR_TEMP },
	{ &cg_breadCrumb, "ui_breadCrumb", "", 0 },
	{ &cg_lazyLoad, "ui_lazyLoad", "", 0 },
	{ &cg_atmosphericEffects, "cg_atmosphericEffects", "1", CVAR_ARCHIVE },
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE },
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT },
	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE },
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE },
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE },
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE },
	{ &cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE },
	{ &cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE },
	{ &cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE },
	{ &cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE },
	{ &cg_hitSounds, "cg_hitSounds", "0", CVAR_ARCHIVE },
	{ &cg_enemyModel, "cg_enemyModel", "", CVAR_ARCHIVE },
	{ &cg_enemyColors, "cg_enemyColors", "", CVAR_ARCHIVE },
	{ &cg_teamModel, "cg_teamModel", "", CVAR_ARCHIVE },
	{ &cg_teamColors, "cg_teamColors", "", CVAR_ARCHIVE },
	{ &cg_deadBodyDarken, "cg_deadBodyDarken", "1", CVAR_ARCHIVE },
	{ &cg_fovAdjust, "cg_fovAdjust", "0", CVAR_ARCHIVE },
	{ &cg_followKiller, "cg_followKiller", "0", CVAR_ARCHIVE },
	{ &cg_customHeight, "r_customHeight", "480", CVAR_ARCHIVE },
	{ &cg_customWidth, "r_customWidth", "640", CVAR_ARCHIVE },

};


/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	const cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < ARRAY_LEN( cvarTable ) ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;
	enemyModelModificationCount = cg_enemyModel.modificationCount;
	enemyColorsModificationCount = cg_enemyColors.modificationCount;
	teamModelModificationCount = cg_teamModel.modificationCount;
	teamColorsModificationCount = cg_teamColors.modificationCount;
	atmosphereModificationCount = cg_atmosphere.modificationCount;
	breadcrumbModificationCount = cg_breadCrumb.modificationCount;
	lazyloadModificationCount = cg_lazyLoad.modificationCount;
	customheightModificationCount = cg_customHeight.modificationCount;
	customwidthModificationCount = cg_customWidth.modificationCount;

}


/*																																			
===================
CG_ForceModelChange
===================
*/
void CG_ForceModelChange( void ) {
	const char *clientInfo;
	int	i;

	for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
		clientInfo = CG_ConfigString( CS_PLAYERS + i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
}


#ifdef USE_WEAPON_ORDER
extern int weaponOrder[WP_NUM_WEAPONS]; 
extern int weaponRawOrder[WP_NUM_WEAPONS]; 

int contains(int *list, int size, int number) 
{ 
  int i; 

  for (i = 0; i < size; i++) 
    if (list[i] == number) return 1; 

  return 0; 
} 


void UpdateWeaponOrder (void) 
{ 
  char *order = cg_weaponOrder.string; 
  char weapon[3]; 
  int i, start; 
  int tempOrder[WP_NUM_WEAPONS]; 
  int weapUsed[WP_NUM_WEAPONS]; 
  int temp; 

  weapon[1] = '\0'; 
  memset(tempOrder, 0, sizeof(tempOrder)); 
  memset(weapUsed, 0, sizeof(weapUsed)); 

  i = 0; 
  while (order != NULL && *order != '\0' && i < WP_NUM_WEAPONS) 
  { 
    weapon[0] = *order; 
    order++; 

    if (*order != '\\' && *order != '/') 
    { 
      weapon[1] = *order; 
      weapon[2] = '\0'; 
      order++; 
    } else { 
      weapon[1] = '\0'; 
    } 

    if (*order != '\0') 
      order++; 

    temp = atoi( weapon ); 
    if (temp < 1 || temp > WP_NUM_WEAPONS) 
    { 
      CG_Printf( "Error: %i is out of range. Ignoring..\n", temp ); 
    } 
    else if ( contains( tempOrder, sizeof(tempOrder)/sizeof(tempOrder[0]), temp ) )

    { 
      CG_Printf( "Error: %s (%i) already in list. Ignoring..\n",
        (BG_FindItemForWeapon( temp ))->pickup_name, temp );

    } else { 
      tempOrder[i] = temp; 
      weapUsed[temp - 1] = 1; 
      i++; 
    } 
  } 

  //error checking.. 
  start = 0; 
  for (i = 0; i < WP_NUM_WEAPONS - 1; i++) 
  { 
    if (weapUsed[i]) 
      continue; 
    CG_Printf( "Error: %s (%i) not in list. Adding it to front of the list..\n",
      (BG_FindItemForWeapon( i + 1 ))->pickup_name, i + 1 );

    weaponRawOrder[start++] = i + 1; 
  } 
  //build the raw order list 
  for (i = start; i < WP_NUM_WEAPONS; i++) 
    weaponRawOrder[i] = tempOrder[i - start]; 

  //built the remaping table 
  for (i = 0; i < WP_NUM_WEAPONS; i++) 
    weaponOrder[weaponRawOrder[i] - 1] = i + 1; 

} 
//</WarZone>
#endif



void CG_LoadClientInfo( clientInfo_t *ci );
static void CG_Register2D( void );
static void CG_IconCache( void );
static void CG_WeapMisc( const char * );
static void CG_WeaphitsCache( void );
static void CG_PlayerCache( const char * );
static void CG_PowerupCache( const char * );
static void CG_InitGLConfig( void );



/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	const cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < ARRAY_LEN( cvarTable ) ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;
#if 0
		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_Set( "teamoverlay", "1" );
		} else {
			trap_Cvar_Set( "teamoverlay", "0" );
		}
#endif
		// FIXME E3 HACK
		trap_Cvar_Set( "teamoverlay", "1" );
	}

#ifdef USE_WEAPON_ORDER
  //WarZone 
  if ( cg_weaponsCount != cg_weaponOrder.modificationCount ) 
  { 
    UpdateWeaponOrder(); 
    cg_weaponsCount = cg_weaponOrder.modificationCount; 
  } 
#endif

	if(atmosphereModificationCount != cg_atmosphere.modificationCount) {
		atmosphereModificationCount = cg_atmosphere.modificationCount;
		CG_EffectParse(cg_atmosphere.string);
	}

	if(lazyloadModificationCount != cg_lazyLoad.modificationCount) {
		const char *s;
		lazyloadModificationCount = cg_lazyLoad.modificationCount;
		if(Q_stristr(cg_lazyLoad.string, "gfx/2d") != NULL) {
#ifdef USE_NEW_FONT_RENDERER
			CG_LoadFonts();
#endif
			CG_Register2D();
		}

		if(Q_stristr(cg_lazyLoad.string, "icons") != NULL) {
			CG_IconCache();
		}

		if((s = Q_stristr(cg_lazyLoad.string, "models/weaphits")) != NULL) {
			CG_WeaphitsCache();
		}

		if((s = Q_stristr(cg_lazyLoad.string, "models/weapons2")) != NULL) {
			CG_WeapMisc(s);
		}

		if((s = Q_stristr(cg_lazyLoad.string, "models/powerups")) != NULL) {
			CG_PowerupCache(s);
		}

/*
		if((s = Q_stristr(cg_lazyLoad.string, "models/players")) != NULL) {
			//CG_PlayerCache(s);
		}
*/
	}

	if(customheightModificationCount != cg_customHeight.modificationCount
		|| customwidthModificationCount != cg_customWidth.modificationCount) {
		customwidthModificationCount = cg_customWidth.modificationCount;
		customheightModificationCount = cg_customHeight.modificationCount;
		CG_InitGLConfig();
	}

	if(breadcrumbModificationCount != cg_breadCrumb.modificationCount) {
		
	}

	// if model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount 
		|| enemyModelModificationCount != cg_enemyModel.modificationCount
		|| enemyColorsModificationCount != cg_enemyColors.modificationCount
		|| teamModelModificationCount != cg_teamModel.modificationCount
		|| teamColorsModificationCount != cg_teamColors.modificationCount ) {

		forceModelModificationCount = cg_forceModel.modificationCount;
		enemyModelModificationCount = cg_enemyModel.modificationCount;
		enemyColorsModificationCount = cg_enemyColors.modificationCount;
		teamModelModificationCount = cg_teamModel.modificationCount;
		teamColorsModificationCount = cg_teamColors.modificationCount;

		CG_ForceModelChange();
	}
}


int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}


void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	ED_vsprintf (text, msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	ED_vsprintf (text, msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

#ifndef BUILD_GAME_STATIC
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	ED_vsprintf (text, error, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	ED_vsprintf (text, msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL Com_DPrintf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	ED_vsprintf (text, msg, argptr);
	va_end (argptr);

	if(cg_developer.integer) {
		trap_Print( text );
	}
}


#endif

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) 
{
	static char	buffer[ 2 ][ MAX_STRING_CHARS ];
	static int index = 0;

	index ^= 1;
	trap_Argv( arg, buffer[ index ], sizeof( buffer[ 0 ] ) );

	return buffer[ index ];
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	const char		*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound, qfalse );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data, qfalse );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	// voice commands
#ifdef MISSIONPACK
	CG_LoadVoiceChats();
#endif

	cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/feedback/1_minute.wav", qtrue );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/feedback/5_minute.wav", qtrue );
	cgs.media.suddenDeathSound = trap_S_RegisterSound( "sound/feedback/sudden_death.wav", qtrue );
	cgs.media.oneFragSound = trap_S_RegisterSound( "sound/feedback/1_frag.wav", qtrue );
	cgs.media.twoFragSound = trap_S_RegisterSound( "sound/feedback/2_frags.wav", qtrue );
	cgs.media.threeFragSound = trap_S_RegisterSound( "sound/feedback/3_frags.wav", qtrue );
	cgs.media.count3Sound = trap_S_RegisterSound( "sound/feedback/three.wav", qtrue );
	cgs.media.count2Sound = trap_S_RegisterSound( "sound/feedback/two.wav", qtrue );
	cgs.media.count1Sound = trap_S_RegisterSound( "sound/feedback/one.wav", qtrue );
	cgs.media.countFightSound = trap_S_RegisterSound( "sound/feedback/fight.wav", qtrue );
	cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/feedback/prepare.wav", qtrue );
#ifdef MISSIONPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound( "sound/feedback/prepare_team.wav", qtrue );
#endif

	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {

		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/feedback/redleads.wav", qtrue );
		cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/feedback/blueleads.wav", qtrue );
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/feedback/teamstied.wav", qtrue );
		cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue );

		cgs.media.redScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_red_scores.wav", qtrue );
		cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_scores.wav", qtrue );

		cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue );

		cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
		cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );

		cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
		cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue );

		if ( cgs.gametype == GT_CTF || cg_buildScript.integer ) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_red_returned.wav", qtrue );
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_returned.wav", qtrue );
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_flag.wav", qtrue );
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_flag.wav", qtrue );
		}

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_1flag.wav", qtrue );
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_1flag.wav", qtrue );
		}

		if ( cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF || cg_buildScript.integer ) {
			cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
			cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
		}

		if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound( "sound/teamplay/voc_base_attack.wav", qtrue );
		}
		cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
#else
		cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
		cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
		cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_1flag.wav", qtrue );
		cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_1flag.wav", qtrue );
#endif
	}

	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse );
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav", qfalse );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav", qfalse );
	cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav", qfalse );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav", qfalse );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav", qfalse );

#ifdef MISSIONPACK
	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound( "sound/items/invul_activate.wav", qfalse );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "sound/items/invul_impact_01.wav", qfalse );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "sound/items/invul_impact_02.wav", qfalse );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "sound/items/invul_impact_03.wav", qfalse );
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound( "sound/items/invul_juiced.wav", qfalse );
	cgs.media.obeliskHitSound1 = trap_S_RegisterSound( "sound/items/obelisk_hit_01.wav", qfalse );
	cgs.media.obeliskHitSound2 = trap_S_RegisterSound( "sound/items/obelisk_hit_02.wav", qfalse );
	cgs.media.obeliskHitSound3 = trap_S_RegisterSound( "sound/items/obelisk_hit_03.wav", qfalse );
	cgs.media.obeliskRespawnSound = trap_S_RegisterSound( "sound/items/obelisk_respawn.wav", qfalse );

	cgs.media.ammoregenSound = trap_S_RegisterSound("sound/items/cl_ammoregen.wav", qfalse);
	cgs.media.doublerSound = trap_S_RegisterSound("sound/items/cl_doubler.wav", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound("sound/items/cl_guard.wav", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound("sound/items/cl_scout.wav", qfalse);
#endif

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/telein.wav", qfalse );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/teleout.wav", qfalse );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav", qfalse );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav", qfalse );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse);

	cgs.media.hitSounds[0] = trap_S_RegisterSound( "sound/feedback/hit25.wav", qfalse );
	cgs.media.hitSounds[1] = trap_S_RegisterSound( "sound/feedback/hit50.wav", qfalse );
	cgs.media.hitSounds[2] = trap_S_RegisterSound( "sound/feedback/hit75.wav", qfalse );
	cgs.media.hitSounds[3] = trap_S_RegisterSound( "sound/feedback/hit100.wav", qfalse );

	cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav", qfalse );

#ifdef MISSIONPACK
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound( "sound/feedback/hithi.wav", qfalse );
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound( "sound/feedback/hitlo.wav", qfalse );
#endif

	cgs.media.impressiveSound = trap_S_RegisterSound( "sound/feedback/impressive.wav", qtrue );
	cgs.media.excellentSound = trap_S_RegisterSound( "sound/feedback/excellent.wav", qtrue );
	cgs.media.deniedSound = trap_S_RegisterSound( "sound/feedback/denied.wav", qtrue );
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/feedback/humiliation.wav", qtrue );
	cgs.media.assistSound = trap_S_RegisterSound( "sound/feedback/assist.wav", qtrue );
	cgs.media.defendSound = trap_S_RegisterSound( "sound/feedback/defense.wav", qtrue );
#ifdef MISSIONPACK
	cgs.media.firstImpressiveSound = trap_S_RegisterSound( "sound/feedback/first_impressive.wav", qtrue );
	cgs.media.firstExcellentSound = trap_S_RegisterSound( "sound/feedback/first_excellent.wav", qtrue );
	cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/feedback/first_gauntlet.wav", qtrue );
#endif

	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/feedback/lostlead.wav", qtrue);

#ifdef MISSIONPACK
	cgs.media.voteNow = trap_S_RegisterSound( "sound/feedback/vote_now.wav", qtrue);
	cgs.media.votePassed = trap_S_RegisterSound( "sound/feedback/vote_passed.wav", qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound( "sound/feedback/vote_failed.wav", qtrue);
#endif

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/world/jumppad.wav", qfalse );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
//		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
	}

#if defined(USE_GAME_FREEZETAG) || defined(USE_REFEREE_CMDS)
  cgs.media.frozenSound = trap_S_RegisterSound("sound/teamplay/freezing.wav", qfalse);
#endif

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav", qfalse );
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound ("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound ("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound ("sound/weapons/machinegun/ric3.wav", qfalse);
	//cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound ("sound/weapons/plasma/plasmx1a.wav", qfalse);
#ifdef MISSIONPACK
	cgs.media.sfx_proxexp = trap_S_RegisterSound( "sound/weapons/proxmine/wstbexpl.wav" , qfalse);
	cgs.media.sfx_nghit = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpd.wav" , qfalse);
	cgs.media.sfx_nghitflesh = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpl.wav" , qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpm.wav", qfalse );
	cgs.media.sfx_chghit = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpd.wav", qfalse );
	cgs.media.sfx_chghitflesh = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpl.wav", qfalse );
	cgs.media.sfx_chghitmetal = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpm.wav", qfalse );
	cgs.media.weaponHoverSound = trap_S_RegisterSound( "sound/weapons/weapon_hover.wav", qfalse );
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound( "sound/items/kam_explode.wav", qfalse );
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound( "sound/items/kam_implode.wav", qfalse );
	cgs.media.kamikazeFarSound = trap_S_RegisterSound( "sound/items/kam_explode_far.wav", qfalse );
	cgs.media.winnerSound = trap_S_RegisterSound( "sound/feedback/voc_youwin.wav", qfalse );
	cgs.media.loserSound = trap_S_RegisterSound( "sound/feedback/voc_youlose.wav", qfalse );

	cgs.media.wstbimplSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);
#endif

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse );
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);

#ifdef MISSIONPACK
	trap_S_RegisterSound("sound/player/james/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/taunt.wav", qfalse );

	trap_S_RegisterSound("sound/player/janet/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/taunt.wav", qfalse );
#endif

}


//===================================================================================

static void CG_Register2D( void ) {
	int i;
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};
	cgs.media.charsetShader		= trap_R_RegisterShader( "gfx/2d/bigchars" );
	for ( i = 0 ; i < ARRAY_LEN( sb_nums ) ; i++ ) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}
  //cgs.media.timerSlices[i] = trap_R_RegisterShader( sb_nums[i] );
  cgs.media.timerSlices[0] = trap_R_RegisterShaderNoMip( "gfx/2d/timer/slice5" );
  cgs.media.timerSlices[1] = trap_R_RegisterShaderNoMip( "gfx/2d/timer/slice7" );
  cgs.media.timerSlices[2] = trap_R_RegisterShaderNoMip( "gfx/2d/timer/slice12" );
  cgs.media.timerSlices[3] = trap_R_RegisterShaderNoMip( "gfx/2d/timer/slice24" );
	cgs.media.backTileShader = trap_R_RegisterShaderNoMip( "gfx/2d/backtile" );
	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );
	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c", 'a'+i) );
	}
#ifdef MISSIONPACK
	cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
#endif

}


static void CG_MenuCache( void ) {

	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );
	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );
	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
#ifdef MISSIONPACK
	// new stuff
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");
#endif
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );

}


static void CG_IconCache( void ) {
	int			i;

	if(!cgs.media.noammoShader) {
		cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );
	}
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
#else
	if ( cgs.gametype == GT_CTF || cg_buildScript.integer ) {
#endif
		cgs.media.redCubeIcon = trap_R_RegisterShader( "icons/skull_red" );
		cgs.media.blueCubeIcon = trap_R_RegisterShader( "icons/skull_blue" );
	}
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
#else
	if ( cgs.gametype == GT_CTF || cg_buildScript.integer ) {
#endif
		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red3" );
		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu3" );
	}
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}
#endif
	if(!cgs.media.armorIcon) {
		cgs.media.armorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );
	}
	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( !cg_items[ i ].icon ) {
			cg_items[ i ].icon_df = cg_items[ i ].icon = trap_R_RegisterShader( bg_itemlist[ i ].icon );
			if ( bg_itemlist[ i ].giType == IT_WEAPON ) {
				cg_weapons[ bg_itemlist[ i ].giTag ].weaponIcon = cg_items[ i ].icon;
				cg_weapons[ bg_itemlist[ i ].giTag ].ammoIcon = cg_items[ i ].icon;
			}
		}
	}

}


static void CG_PowerupCache( const char *update ) {
	int i;

	// powerup shaders
	if(!cgs.media.quadShader) {
		cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	}
	if(!cgs.media.quadShader) {
		cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	}
	if(!cgs.media.quadShader) {
		cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit" );
	}
	if(!cgs.media.quadShader) {
		cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	}
	if(!cgs.media.quadShader) {
		cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
	}
	if(!cgs.media.quadShader) {
		cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
	}
	if(!cgs.media.quadShader) {
		cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );
	}

	if ( cgs.gametype == GT_CTF || cg_buildScript.integer 
#ifdef MISSIONPACK
		|| cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER
#endif
	) {
		cgs.media.redCubeModel = trap_R_RegisterModel( "models/powerups/orb/r_orb.md3" );
		cgs.media.blueCubeModel = trap_R_RegisterModel( "models/powerups/orb/b_orb.md3" );
	}
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
		cgs.media.overloadBaseModel = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel = trap_R_RegisterModel( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel = trap_R_RegisterModel( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel = trap_R_RegisterModel( "models/powerups/overload_energy.md3" );
	}
	if ( cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.harvesterModel = trap_R_RegisterModel( "models/powerups/harvester/harvester.md3" );
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin( "models/powerups/harvester/red.skin" );
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin( "models/powerups/harvester/blue.skin" );
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel( "models/powerups/obelisk/obelisk.md3" );
	}
#endif
	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
	}
	cgs.media.armorModel = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
#ifdef MISSIONPACK
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel( "models/powerups/kamikazi.md3" );
	cgs.media.kamikazeHeadTrail = trap_R_RegisterModel( "models/powerups/trailtest.md3" );
	cgs.media.guardPowerupModel = trap_R_RegisterModel( "models/powerups/guard_player.md3" );
	cgs.media.scoutPowerupModel = trap_R_RegisterModel( "models/powerups/scout_player.md3" );
	cgs.media.doublerPowerupModel = trap_R_RegisterModel( "models/powerups/doubler_player.md3" );
	cgs.media.ammoRegenPowerupModel = trap_R_RegisterModel( "models/powerups/ammo_player.md3" );
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel( "models/powerups/shield/impact.md3" );
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel( "models/powerups/shield/juicer.md3" );
	cgs.media.medkitUsageModel = trap_R_RegisterModel( "models/powerups/regen.md3" );
	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
#endif
	if(!cgs.media.teleportEffectModel) {
		cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/powerups/pop.md3" );
	}

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if (bg_itemlist[ i ].giType != IT_WEAPON) {
			if(Q_stricmpn(&bg_itemlist[ i ].world_model[0][16], &update[16], 6)
				&& Q_stricmpn(&bg_itemlist[ i ].world_model[1][16], &update[16], 6)) {
				continue;
			}
			if(!cg_items[ i ].models[0] || !cg_items[ i ].models[1]) {
				cg_items[ i ].registered = qfalse;
				CG_RegisterItemVisuals( i );
			}
		}
	}
}


static void CG_TeamCache( void ) {

#ifdef MISSIONPACK
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer )
#else
	if ( cgs.gametype == GT_CTF || cg_buildScript.integer )
#endif
	{
		cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
		cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );

		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/white.skin" );
#ifdef MISSIONPACK
		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
#endif
	}
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
		cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
	}
#endif
	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {
		cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
	}

}


static void CG_WeaphitsCache( void ) {
#ifdef MISSIONPACK
	if(!cgs.media.dustPuffShader) {
		cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff" );
	}
#endif
	if(!cgs.media.smokePuffShader) {
		cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	}
	if(!cgs.media.smokePuffRageProShader) {
		cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	}
	if(!cgs.media.shotgunSmokePuffShader) {
		cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
	}

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
#ifdef MISSIONPACK
	if(!cgs.media.nailPuffShader) {
		cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail" );
	}
	if(!cgs.media.blueProxMine) {
		cgs.media.blueProxMine = trap_R_RegisterModel( "models/weaphits/proxmineb.md3" );
	}
	cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel( "models/weaphits/kamboom2.md3" );
	cgs.media.kamikazeShockWave = trap_R_RegisterModel( "models/weaphits/kamwave.md3" );
	cgs.media.heartShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );
	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {
		cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
	}
#endif

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );

}


static void CG_GibsCache( void ) {
	
	cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );
	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );

	cgs.media.gibAbdomen = trap_R_RegisterModel( "models/gibs/abdomen.md3" );
	cgs.media.gibArm = trap_R_RegisterModel( "models/gibs/arm.md3" );
	cgs.media.gibChest = trap_R_RegisterModel( "models/gibs/chest.md3" );
	cgs.media.gibFist = trap_R_RegisterModel( "models/gibs/fist.md3" );
	cgs.media.gibFoot = trap_R_RegisterModel( "models/gibs/foot.md3" );
	cgs.media.gibForearm = trap_R_RegisterModel( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibs/intestine.md3" );
	cgs.media.gibLeg = trap_R_RegisterModel( "models/gibs/leg.md3" );
	cgs.media.gibSkull = trap_R_RegisterModel( "models/gibs/skull.md3" );
	cgs.media.gibBrain = trap_R_RegisterModel( "models/gibs/brain.md3" );

}


static void CG_WeapMisc( const char *update ) {
	int i;

#ifdef MISSIONPACK
#else
	if(!cgs.media.teleportEffectModel) {
		cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
	}
	if(!cgs.media.teleportEffectShader) {
		cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
	}
#endif
	if(!cgs.media.plasmaBallShader) {
		cgs.media.plasmaBallShader = trap_R_RegisterShader( "sprites/plasma1" );
	}
	if(!cgs.media.waterBubbleShader) {
		cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );
	}
	if(!cgs.media.tracerShader) {
		cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	}
#ifdef USE_LASER_SIGHT
	if(!cgs.media.laserShader) {
  	cgs.media.laserShader = trap_R_RegisterShader( "sprites/laser" );
	}
#endif
#ifdef USE_FLAME_THROWER
	if(!cgs.media.flameBallShader) {
  	cgs.media.flameBallShader = trap_R_RegisterShader( "sprites/flameball" );
	}
#endif
	if(!cgs.media.machinegunBrassModel) {
		cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/m_shell.md3" );
	}
	if(!cgs.media.shotgunBrassModel) {
		cgs.media.shotgunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );
	}
	if(!cgs.media.smoke2) {
		cgs.media.smoke2 = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );
	}
	if(!cgs.media.balloonShader) {
		cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );
	}
	if(!cgs.media.bloodExplosionShader) {
		cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );
	}
#if defined(USE_GAME_FREEZETAG) || defined(USE_REFEREE_CMDS)
  cgs.media.frozenShader = trap_R_RegisterShader("freezeShader" );
#endif

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( // cg_items[ i ].registered 
			bg_itemlist[ i ].giType == IT_WEAPON
			&& !Q_stricmpn(&bg_itemlist[ i ].world_model[0][16], &update[16], 6)
			//&& !Q_stricmp(&update[16], &bg_itemlist[ i ].world_model[0][16])
		) {
			weaponInfo_t *info = &cg_weapons[ bg_itemlist[ i ].giTag ];
			if(!info->weaponModel || !info->barrelModel 
				|| !info->handsModel || !info->flashModel 
				|| !info->ammoModel || !info->missileModel) {
				cg_items[ i ].registered = qfalse;
				cg_weapons[bg_itemlist[ i ].giTag].registered = qfalse;
				CG_RegisterItemVisuals( i );
			}
		}
	}

}


static void CG_ItemCache( void ) {
	char		items[MAX_ITEMS+1];
	int			i;

	// only register the items that the server says we need
	Q_strncpyz( items, CG_ConfigString(CS_ITEMS), sizeof( items ) );

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}


	cg.skipDFshaders = qfalse;

}


static void CG_ModelCache( void ) {
	int			i;
	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}
}


static void CG_PlayerCache( const char *update ) {
	int			i;

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		if(!cgs.gameModels[i]) {
			cgs.gameModels[i] = trap_R_RegisterModel( modelName );
		}
	}
	
#ifdef MISSIONPACK

	trap_R_RegisterModel( "models/players/james/lower.md3" );
	trap_R_RegisterModel( "models/players/james/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/james/james.md3" );

	trap_R_RegisterModel( "models/players/janet/lower.md3" );
	trap_R_RegisterModel( "models/players/janet/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/janet/janet.md3" );

#endif

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	//trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	//trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		//if ( !cgs.clientinfo[ i ].infoValid || cgs.clientinfo[ i ].deferred ) {
		//	continue;
		//}
		//if(Q_stricmpn(&update[15], cgs.clientinfo[ i ].modelName, 5)
		//	&& Q_stricmpn(&update[15], DEFAULT_MODEL, 5)) {
		//	continue;
		//}
		if(!(cgs.clientinfo[ i ].legsModel 
			&& cgs.clientinfo[ i ].torsoModel
			&& cgs.clientinfo[ i ].headModel
			&& cgs.clientinfo[ i ].legsSkin
			&& cgs.clientinfo[ i ].torsoSkin
			&& cgs.clientinfo[ i ].headSkin
			&& cgs.clientinfo[ i ].animations[FLAG_RUN].numFrames)) {
			CG_LoadClientInfo(&cgs.clientinfo[ i ]);
		}
	}

}


/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	CG_Register2D();

	CG_MenuCache();

	CG_IconCache();

	CG_PowerupCache(NULL);

	CG_TeamCache();

	CG_WeaphitsCache();

	CG_GibsCache();

	CG_WeapMisc(NULL);

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	CG_ItemCache();

	CG_ModelCache();

	CG_PlayerCache(NULL);
	
	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/
}



/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString( void ) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
		return "";
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}
#ifdef MISSIONPACK
//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
    
	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			trap_R_RegisterFont(tempStr, pointSize, &cgDC.Assets.textFont);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			trap_R_RegisterFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			trap_R_RegisterFont(tempStr, pointSize, &cgDC.Assets.bigFont);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse;
}

void CG_ParseMenu(const char *menuFile) {
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource(menuFile);
	if (!handle)
		handle = trap_PC_LoadSource("ui/testhud.menu");
	if (!handle)
		return;

	while ( 1 ) {
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			CG_Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}

qboolean CG_Load_Menu(char **p) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt(p, qtrue);
    
		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token); 
	}
	return qfalse;
}



void CG_LoadMenus(const char *menuFile) {
	char	*token;
	char *p;
	int	len, start;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );
#ifdef USE_CLASSIC_HUD
	if ( !f ) {
		trap_Print( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if (!f) {
			trap_Print( va( S_COLOR_RED "default menu file not found: ui/hud.txt, using vanilla hud!\n", menuFile ) );
			trap_Cvar_Set( "cg_hudFiles", "" );
			return;
		}
	}
#else
	if ( !f ) {
		trap_Error( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if (!f) {
			trap_Error( va( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n", menuFile ) );
		}
	}
#endif

	if ( len >= MAX_MENUDEFFILE ) {
		trap_FS_FCloseFile( f );
		trap_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE ) );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	
	COM_Compress(buf);

//#ifndef BUILD_GAME_STATIC
	CG_Menu_Reset();
//#endif

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) {
			if (CG_Load_Menu(&p)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

}



static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


void CG_SetScoreSelection(void *p) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if ( cgs.gametype >= GT_TEAM ) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		CG_Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		CG_Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;
	if ( cgs.gametype >= GT_TEAM ) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle) {
	gitem_t *item;
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
			case 0:
				if ( cg_entities[cg.scores[index].client].items[ITEM_PW_MIN + PW_NEUTRALFLAG] ) {
					item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( cg_entities[cg.scores[index].client].items[ITEM_PW_MIN + PW_REDFLAG] ) {
					item = BG_FindItemForPowerup( PW_REDFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( cg_entities[cg.scores[index].client].items[ITEM_PW_MIN + PW_BLUEFLAG] ) {
					item = BG_FindItemForPowerup( PW_BLUEFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else {
					if ( info->botSkill > 0 && info->botSkill <= 5 ) {
						*handle = cgs.media.botSkillShaders[ info->botSkill - 1 ];
					} else if ( info->handicap < 100 ) {
					return va("%i", info->handicap );
					}
				}
			break;
			case 1:
				if (team == -1) {
					return "";
				} else {
					*handle = CG_StatusHandle(info->teamTask);
				}
		  break;
			case 2:
				if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
					return "Ready";
				}
				if (team == -1) {
					if (cgs.gametype == GT_TOURNAMENT) {
						return va("%i/%i", info->wins, info->losses);
					} else if (info->infoValid && info->team == TEAM_SPECTATOR ) {
						return "Spectator";
					} else {
						return "";
					}
				} else {
					if (info->teamLeader) {
						return "Leader";
					}
				}
			break;
			case 3:
				return info->name;
			break;
			case 4:
				return va("%i", info->score);
			break;
			case 5:
				return va("%4i", sp->time);
			break;
			case 6:
				if ( sp->ping == -1 ) {
					return "connecting";
				} 
				return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static void CG_FeederSelection(float feederID, int index) {
	if ( cgs.gametype >= GT_TEAM ) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}
}
#endif

#ifdef MISSIONPACK
static float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}
#endif

#ifdef MISSIONPACK
void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	switch (ownerDraw) {
	  case CG_GAME_TYPE:
			return CG_Text_Width(CG_GameTypeString(), scale, 0);
	  case CG_GAME_STATUS:
			return CG_Text_Width(CG_GetGameStatusText(), scale, 0);
			break;
	  case CG_KILLER:
			return CG_Text_Width(CG_GetKillerText(), scale, 0);
			break;
	  case CG_RED_NAME:
			return CG_Text_Width(cg_redTeamName.string, scale, 0);
			break;
	  case CG_BLUE_NAME:
			return CG_Text_Width(cg_blueTeamName.string, scale, 0);
			break;


	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
  trap_CIN_SetExtents(handle, x, y, w, h);
  trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}


/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu( void ) {
	char buff[1024];
	const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;   
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding = &trap_Key_SetBinding;
	//cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error; 
	cgDC.Print = &Com_Printf; 
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;

	CG_Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

  CG_LoadMenus(hudSet);
}

void CG_AssetCache( void ) {
	//if (Assets.textFont == NULL) {
	//  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}
#endif

static void CG_InitGLConfig( void ) {

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );

	cgs.screenXBias = 0.0;
	cgs.screenYBias = 0.0;
	
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
		// wide screen, scale by height
		cgs.screenXScale = cgs.screenYScale = cgs.glconfig.vidHeight * (1.0/480.0);
		cgs.screenXBias = 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else {
		// no wide screen, scale by width
		cgs.screenXScale = cgs.screenYScale = cgs.glconfig.vidWidth * (1.0/640.0);
		cgs.screenYBias = 0.5 * ( cgs.glconfig.vidHeight - ( cgs.glconfig.vidWidth * (480.0/640.0) ) );
	}

	cgs.screenXmin = 0.0 - (cgs.screenXBias / cgs.screenXScale);
	cgs.screenXmax = 640.0 + (cgs.screenXBias / cgs.screenXScale);

	cgs.screenYmin = 0.0 - (cgs.screenYBias / cgs.screenYScale);
	cgs.screenYmax = 480.0 + (cgs.screenYBias / cgs.screenYScale);

	cgs.cursorScaleR = 1.0 / cgs.screenXScale;
	if ( cgs.cursorScaleR < 0.5 ) {
		cgs.cursorScaleR = 0.5;
	}

}


/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	char  value[MAX_CVAR_VALUE_STRING];
	const char	*s;
	vec3_t mins, maxs;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	trap_Cvar_VariableStringBuffer( "//trap_GetValue", value, sizeof( value ) );
	if ( value[0] ) {
#ifdef Q3_VM
		trap_GetValue = (void*)~atoi( value );
		if ( trap_GetValue( value, sizeof( value ), "trap_R_AddRefEntityToScene2" ) ) {
			trap_R_AddRefEntityToScene2 = (void*)~atoi( value );
			intShaderTime = qtrue;
		}
		if ( trap_GetValue( value, sizeof( value ), "trap_R_AddLinearLightToScene_Q3E" ) ) {
			trap_R_AddLinearLightToScene = (void*)~atoi( value );
			linearLight = qtrue;
		}
#else
		dll_com_trapGetValue = atoi( value );
		if ( trap_GetValue( value, sizeof( value ), "trap_R_AddRefEntityToScene2" ) ) {
			dll_trap_R_AddRefEntityToScene2 = atoi( value );
			intShaderTime = qtrue;
		}
		if ( trap_GetValue( value, sizeof( value ), "trap_R_AddLinearLightToScene_Q3E" ) ) {
			dll_trap_R_AddLinearLightToScene = atoi( value );
			linearLight = qtrue;
		}
#endif
	}

	// load a few needed things before we do any screen updates
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	CG_InitGLConfig();

#ifdef USE_NEW_FONT_RENDERER
	CG_LoadFonts();
#endif

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strstr( s, "defrag-" ) ) {
		cgs.defrag = qtrue;
	} else if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	cgs.ospEnc = atoi( CG_ConfigString( 872 ) ) & 1;

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();
	CG_ParseSysteminfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

//#ifndef BUILD_GAME_STATIC
#ifdef MISSIONPACK
	CG_String_Init();
#endif
//#endif

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	trap_R_ModelBounds( 0, mins, maxs );
	cg.mapcoordsMins[0] = (int)mins[0];
	cg.mapcoordsMaxs[0] = (int)maxs[0];
	cg.mapcoordsMins[1] = (int)mins[1];
	cg.mapcoordsMaxs[1] = (int)maxs[1];
	if(cg.mapcoordsMaxs[1] - cg.mapcoordsMins[1] > cg.mapcoordsMaxs[0] - cg.mapcoordsMins[0]) {
		int dif = (int)((cg.mapcoordsMaxs[1] - cg.mapcoordsMins[1]) - (cg.mapcoordsMaxs[0] - cg.mapcoordsMins[0]));
		cg.mapcoordsMins[0] -= dif / 2.0f;
		cg.mapcoordsMaxs[0] += dif / 2.0f;
	} else {
		int dif = (int)((cg.mapcoordsMaxs[0] - cg.mapcoordsMins[0]) - (cg.mapcoordsMaxs[1] - cg.mapcoordsMins[1]));
		cg.mapcoordsMins[1] -= dif / 2.0f;
		cg.mapcoordsMaxs[1] += dif / 2.0f;
	}
	cg.mapcoordsValid = qtrue;
	CG_EffectParse(cg_atmosphere.string);

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

#ifdef MISSIONPACK
	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff
#endif

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	// CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

#ifdef MISSIONPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );
}


/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - scoreboard
      3 - hud editor
*/
#ifndef MISSIONPACK
void CG_EventHandling( cgame_event_t type ) 
{

}
#endif


void CG_SetScoreCatcher( qboolean enable )
{
	int	currentCatcher, newCatcher, old_state, new_state;
	qboolean spectator;

	currentCatcher = trap_Key_GetCatcher();

	if ( currentCatcher & KEYCATCH_CONSOLE || !cg.snap )
		return;
	
	spectator = cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW );

	if ( enable && spectator ) {
		cgs.score_key = trap_Key_GetKey( "+scores" );
		cgs.score_catched = qtrue;
		newCatcher = currentCatcher | KEYCATCH_CGAME;
	} else {
		cgs.score_catched = qfalse;
		newCatcher = currentCatcher & ~KEYCATCH_CGAME;
	}

	if ( newCatcher != currentCatcher ) {
		if ( cgs.score_key ) {
			// keycatcher change may cause reset of all pressed buttons on new engines
			// so track state of scoreboard key and ignore first upcoming keyup event for it
			old_state = trap_Key_IsDown( cgs.score_key );
			trap_Key_SetCatcher( newCatcher );
			new_state = trap_Key_IsDown( cgs.score_key );
			if ( new_state != old_state ) {
				cgs.filterKeyUpEvent = qtrue;
			}
		} else {
			trap_Key_SetCatcher( newCatcher );
		}
	}
}


#if !defined(MISSIONPACK) || defined(USE_CLASSIC_HUD)
#if defined(MISSIONPACK) && defined(USE_CLASSIC_HUD)
void CG_KeyEvent2( int key, qboolean down ) 
#else
void CG_KeyEvent( int key, qboolean down ) 
#endif
{
	// process scoreboard clicks etc.
	if ( cgs.score_catched && down ) 
	{
		if ( key == /*K_TAB*/ cgs.score_key )
			return;
		if ( key == /*K_MOUSE1*/178 )
			CG_ScoreboardClick();
		else
			CG_SetScoreCatcher( qfalse );
	}
}

#if defined(MISSIONPACK) && defined(USE_CLASSIC_HUD)
void CG_MouseEvent2( int x, int y )
#else
void CG_MouseEvent( int x, int y )
#endif
{
	cgs.cursorX += x * cgs.cursorScaleR;
	cgs.cursorY += y * cgs.cursorScaleR;

	if ( cgs.cursorX < cgs.screenXmin ) {
		cgs.cursorX = cgs.screenXmin;
	}
	else if ( cgs.cursorX > cgs.screenXmax ) {
		cgs.cursorX = cgs.screenXmax;
	}

	if ( cgs.cursorY < cgs.screenYmin ) {
		cgs.cursorY = cgs.screenYmin;
	}
	else if ( cgs.cursorY > cgs.screenYmax ) {
		cgs.cursorY = cgs.screenYmax;
	}
}
#endif
