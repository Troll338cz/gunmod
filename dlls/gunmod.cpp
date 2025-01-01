#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gunmod.h"
#include "gravgunmod.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

// There is no sauce original code :trollface:

cvar_t mp_gunmod = { "mp_gunmod", "1", FCVAR_SERVER };
cvar_t mp_tolchock = { "mp_tolchock", "0", FCVAR_SERVER };
cvar_t mp_toilet = { "mp_toilet", "", FCVAR_SERVER };
cvar_t mp_santahat = { "mp_santahat", "0", FCVAR_SERVER };

cvar_t hideandseek = { "mp_hideandseek", "0", FCVAR_SERVER };
cvar_t hideandseek_minplayers = { "mp_hideandseek_minplayers", "2", FCVAR_SERVER };
cvar_t hideandseek_brtime = { "mp_hideandseek_brtime", "60", FCVAR_SERVER }; // time before round in seconds
cvar_t hideandseek_roundtime = { "mp_roundtime", "300", FCVAR_SERVER };

struct wcvar{
        char *name;
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
        {"weapon_portalgun", 0}
};

struct acvar{
        char *name;
        int count;
        int max_count;
} ammolist[]={
        {"AR2", 0, 180},
        {"9mm", 68, _9MM_MAX_CARRY},
        {"rockets", 0, ROCKET_MAX_CARRY},
        {"uranium", 0, URANIUM_MAX_CARRY},
        {"bolts", 0, BOLT_MAX_CARRY},
        {"357", 0, _357_MAX_CARRY},
        {"556", 0, 250},
        {"ARgrenades", 0, M203_GRENADE_MAX_CARRY},
        {"AR2grenades", 0, 3},
        {"buckshot", 0, BUCKSHOT_MAX_CARRY}
};

void GM_RegisterCVars()
{
        CVAR_REGISTER( &mp_gunmod );
        CVAR_REGISTER( &mp_santahat );
        CVAR_REGISTER( &mp_tolchock );
        CVAR_REGISTER( &mp_toilet );

        CVAR_REGISTER( &hideandseek );
        CVAR_REGISTER( &hideandseek_minplayers );
        CVAR_REGISTER( &hideandseek_brtime );
        CVAR_REGISTER( &hideandseek_roundtime );

}

void GM_AllowWeapon( void ) {}
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

void GM_AllowAmmo( void ) {}
void GM_AddAmmo( CBasePlayer *pPlayer )
{
        for(int i = 0; i < ARRAYSIZE(ammolist); i++)
        {
                if(ammolist[i].count)
                        pPlayer->GiveAmmo( ammolist[i].count, ammolist[i].name, ammolist[i].max_count );
        }
}
void GM_InitAdmins( void ) {}
BOOL GM_IsAdmin( edict_t *player ) { return FALSE; }
void GM_AddAdmin( edict_t *player ) {}
void GM_Login( edict_t *player ) {}
void GM_ChatLog(char *text) {}
BOOL GM_ClientCommand( CBasePlayer *pPlayer, const char *pCmd ) { return FALSE; }
void GM_LogCmd(edict_t *pEntity, const char *cmd, const char *argv) {}
BOOL GM_IsAuthidValid( CBasePlayer *player ) { return TRUE; }
char *GM_GetRandomString(int length) { return ""; }
void GM_SpawnIfValid(CBasePlayer *player) {}
void GM_PlayerPostThink( edict_t *pEntity ) {}
void GM_CvarValue2( const edict_t *pEnt, int requestID, const char *cvarName, const char *value ) {}
void GM_KickPlayer( CBasePlayer *player, const char *reason ) {}
void GM_ScoreUpdate( CBasePlayer *pVictim, int deaths, CBasePlayer *pKiller, int frags ) {}
void GM_MultiplayThink( void ) {}
void GM_UpdateScoreLeaderboard( bool motd_only ) {}
void GM_InitScores( void ) {}
const char *GM_GetAuthUID( CBasePlayer *player ) {}
BOOL GM_IsPlayerFucked( CBasePlayer *plr ) { return FALSE; }
void GM_WriteMutedID( void ) {};

