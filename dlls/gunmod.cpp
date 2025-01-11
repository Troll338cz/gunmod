#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gunmod.h"
#include "gravgunmod.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "admin.h"
// ChatLog
#include <time.h>


// There is no sauce original code :trollface:

cvar_t mp_gunmod = { "mp_gunmod", "1", FCVAR_SERVER };
cvar_t mp_tolchock = { "mp_tolchock", "0", FCVAR_SERVER };
cvar_t mp_toilet = { "mp_toilet", "", FCVAR_SERVER };
cvar_t mp_toilet_speed = { "mp_toilet_speed", "400", FCVAR_SERVER };
cvar_t mp_toilet_turn = { "mp_toilet_turn", "0", FCVAR_SERVER };
cvar_t mp_santahat = { "mp_santahat", "0", FCVAR_SERVER };
cvar_t mp_logchat = { "mp_logchat", "0", FCVAR_SERVER };
cvar_t mp_logcmd = { "mp_logcmd", "0", FCVAR_SERVER };

cvar_t hideandseek = { "mp_hideandseek", "0", FCVAR_SERVER };
cvar_t hideandseek_minplayers = { "mp_hideandseek_minplayers", "2", FCVAR_SERVER };
cvar_t hideandseek_brtime = { "mp_hideandseek_brtime", "60", FCVAR_SERVER }; // time before round in seconds
cvar_t hideandseek_roundtime = { "mp_roundtime", "300", FCVAR_SERVER };
#define W_count 23
#define A_count 14

const char *GM_BadClassNames[] =
{
	"worldspawn",
	"player",
	"monstermaker",
	"grenade",
	"crossbow_bolt",
	"exit",
	"abort",
	"mkdir",
	"system",
};

struct wcvar{
        const char *name;
        bool enable;
} weaponlist[]={
        {"weapon_crowbar", 1},
        {"weapon_gravgun", 0},
        {"weapon_glock", 1},
        {"weapon_python", 0},
        {"weapon_mp5", 0},
        {"weapon_shotgun", 0},
        {"weapon_crossbow", 0},
        {"weapon_ar2", 0},
        {"weapon_m249", 0},
        {"weapon_rpg", 0},
        {"weapon_gauss", 0},
        {"weapon_egon", 0},
        {"weapon_hornetgun", 0},
        {"weapon_displacer", 0},
        {"weapon_handgrenade", 0},
        {"weapon_satchel", 0},
        {"weapon_tripmine", 0},
        {"weapon_snark", 0},
        {"weapon_shockrifle", 0},
        {"weapon_big_cock", 0},
        {"weapon_portalgun", 0},
        {"weapon_knife", 0},
        {"weapon_sniperrifle", 0}
};

struct acvar{
        const char *name;
        int count;
        int max_count;
} ammolist[]={
        {"AR2", 0, 180},				// AR2 primary
        {"9mm", 68, _9MM_MAX_CARRY},			// MP5/glock 
        {"rockets", 0, ROCKET_MAX_CARRY},		// RPG rockets
        {"uranium", 0, URANIUM_MAX_CARRY},		// Uranium
        {"bolts", 0, BOLT_MAX_CARRY},			// Crossbow
        {"357", 0, _357_MAX_CARRY},			// 357 
        {"556", 0, 250},				// M249Ammo
        {"ARgrenades", 0, M203_GRENADE_MAX_CARRY},	// MP5 Grenade
        {"AR2grenades", 0, 3}, 				// AR2 altfire
        {"Snarks", 0, SNARK_MAX_CARRY},			// Snarks
        {"762", 0, 15},					// Sniper
        {"Satchel Charge", 0, SATCHEL_MAX_CARRY},	// Satchels
        {"spores", 0, 20} 				// Spore
};

void GM_RegisterCVars()
{
        CVAR_REGISTER( &mp_gunmod );
        CVAR_REGISTER( &mp_santahat );
        CVAR_REGISTER( &mp_tolchock );
        CVAR_REGISTER( &mp_toilet );
        CVAR_REGISTER( &mp_toilet_speed );
        CVAR_REGISTER( &mp_toilet_turn );
        CVAR_REGISTER( &mp_logchat );
        CVAR_REGISTER( &mp_logcmd );

        CVAR_REGISTER( &hideandseek );
        CVAR_REGISTER( &hideandseek_minplayers );
        CVAR_REGISTER( &hideandseek_brtime );
        CVAR_REGISTER( &hideandseek_roundtime );

	g_engfuncs.pfnAddServerCommand( "mp_allow_weapon", GM_AllowWeapon );
	g_engfuncs.pfnAddServerCommand( "mp_allow_ammo", GM_AllowAmmo );
	
	ADMIN_RegisterCVars(); // GM_InitAdmins
}

void GM_AllowWeapon( void )
{
    bool exists = FALSE;

    if(CMD_ARGC() < 3)
    {
	ALERT(at_console,"Usage: mp_allow_weapon <weapon> <state>\n");
        for(int i=0;i<W_count;i++)
	    ALERT(at_console,"%s = %d\n",weaponlist[i].name,weaponlist[i].enable);
	return;
    }

    for(int i=0;i<W_count;i++)
    {
	if(!strcmp(weaponlist[i].name,CMD_ARGV(1)))
	{
	    weaponlist[i].enable = atoi(CMD_ARGV(2));
	    exists = TRUE;
	}
    }
    if( !exists )
	ALERT(at_error,"%s doesn\'t exit\n",CMD_ARGV(1));
}

void GM_AddWeapons( CBasePlayer *pPlayer )
{
        if( g_pGameRules->IsHideAndSeek() )
        {
                if( strstr(pPlayer->m_szTeamName, "seekers") )
                {
                        pPlayer->GiveNamedItem( "weapon_mp5" );
                        pPlayer->GiveNamedItem( "weapon_python" );
                }
                pPlayer->GiveNamedItem( "weapon_portalgun" );
                return;
        }

        for(int i = 0; i < ARRAYSIZE(weaponlist); i++)
        {
                if(weaponlist[i].enable)
                        pPlayer->GiveNamedItem( weaponlist[i].name );
        }

}

void GM_AllowAmmo( void )
{
    bool exists = FALSE;
    
    if(CMD_ARGC() < 3)
    {
	ALERT(at_console,"Usage: mp_allow_ammo <ammo> <count>\n");
        for(int i=0;i<A_count;i++)
	    ALERT(at_console,"%s = %d\n",ammolist[i].name,ammolist[i].count);
	return;
    }
    
    for(int i=0;i<A_count;i++)
    {
	if(!strcmp(ammolist[i].name,CMD_ARGV(1)))
	{
	    ammolist[i].count = atoi(CMD_ARGV(2));
	    exists = TRUE;
	}
    }
    if( !exists )
	ALERT(at_error,"ammo %s doesn\'t exit\n",CMD_ARGV(1));
}

void GM_AddAmmo( CBasePlayer *pPlayer )
{
        for(int i = 0; i < ARRAYSIZE(ammolist); i++)
        {
                if(ammolist[i].count)
                        pPlayer->GiveAmmo( ammolist[i].count, ammolist[i].name, ammolist[i].max_count );
        }
}

// Forward to admin.cpp
// void GM_InitAdmins( void ) 		{ ADMIN_InitAdmins(); }
BOOL GM_IsAdmin( edict_t *player ) 	{ return ADMIN_IsAdmin(player); }
void GM_AddAdmin( edict_t *player ) 	{ ADMIN_AddAdmin(player); }
void GM_Login( edict_t *player ) 	{ ADMIN_Login(player); }

void GM_ChatLog(edict_t *pEntity, char *text) 
{
	if( mp_logchat.value )
	{
		FILE *fllogchat = fopen("chat.log", "a");
		time_t mytime = time(NULL);
		char * time_str = ctime(&mytime);
		time_str[strlen(time_str)-1] = '\0';
		CBasePlayer *pl = (CBasePlayer *)CBaseEntity::Instance( pEntity );
		const char *ip = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pl->edict() ), "ip" );
		if( !ip[0] || !ip )
		{
			ip = "UNKNOWN";
		}

		fprintf( fllogchat, "%s %s %s %s: %s\n", time_str, GETPLAYERAUTHID( pEntity ), ip, GGM_PlayerName( pl ), text );
		fclose( fllogchat );
	}
}
void GM_LogCmd(edict_t *pEntity, const char *cmd, const char *argv ) {

	if( mp_logcmd.value )
	{
		FILE *fllogcmd = fopen("cmds.log", "a");
		CBasePlayer *pl = (CBasePlayer *)CBaseEntity::Instance( pEntity );
		time_t mytime = time(NULL);
		char * time_str = ctime(&mytime);
		time_str[strlen(time_str)-1] = '\0';

		const char *ip = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pl->edict() ), "ip" );
		if( !ip[0] || !ip )
		{
			ip = "UNKNOWN";
		}

		fprintf( fllogcmd, "%s %s %s %s: %s\n", time_str, GETPLAYERAUTHID( pEntity ), ip, GGM_PlayerName( pl ), cmd );
		fclose( fllogcmd );
	}
}
BOOL GM_IsAllowedClassname( const char *classname )
{
	for( int i = 0; i < ARRAYSIZE( GM_BadClassNames ); i++ )
        { if( !stricmp( GM_BadClassNames[i], classname ) ) { return FALSE; } }
	return TRUE;
}
void GM_KickPlayer( CBasePlayer *player, const char *reason )
{
	SERVER_COMMAND( UTIL_VarArgs("kick #%d \"%s\" \n", GETPLAYERUSERID( player->edict() ), reason ) );
}
// TODO: Restore nulled code
BOOL GM_ClientCommand( CBasePlayer *pPlayer, const char *pCmd ) { return FALSE; }
BOOL GM_IsAuthidValid( CBasePlayer *player ) { return TRUE; }
char *GM_GetRandomString(int length) { return ""; }
void GM_SpawnIfValid(CBasePlayer *player) {}
void GM_PlayerPostThink( edict_t *pEntity ) {}
void GM_CvarValue2( const edict_t *pEnt, int requestID, const char *cvarName, const char *value ) {}
void GM_ScoreUpdate( CBasePlayer *pVictim, int deaths, CBasePlayer *pKiller, int frags ) { }
void GM_MultiplayThink( void ) {}
void GM_UpdateScoreLeaderboard( bool motd_only ) {}
void GM_InitScores( void ) {}
const char *GM_GetAuthUID( CBasePlayer *player ) { return ""; }
BOOL GM_IsPlayerFucked( CBasePlayer *plr ) { return FALSE; }
void GM_WriteMutedID( void ) {};

