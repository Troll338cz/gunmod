#ifndef GUNMOD_H
#define GUNMOD_H

#define CHECK_COUNT 2

extern cvar_t mp_gunmod;
extern cvar_t mp_tolchock;
extern cvar_t adminpass;
extern cvar_t hideandseek;
extern cvar_t hideandseek_minplayers;
extern cvar_t hideandseek_brtime;
extern cvar_t hideandseek_roundtime;
extern cvar_t mp_toilet;
extern cvar_t mp_toilet_speed;
extern cvar_t mp_toilet_turn;
extern cvar_t mp_santahat;

struct score_s
{
	char szNickname[32];
	char szUID[33];
	int frags;
	int deaths;
};
typedef struct score_s score_t;

struct GMData
{
	BOOL bBhop = FALSE;
	float flCheckCvars;
	BOOL bSitted = FALSE;
	BOOL iClientValid = 0;
	BOOL bAdmin = FALSE;
	BOOL fSpectate;
	float flSpectateWait = 0;
	BOOL IsAdmin()	// Troll: ??
	{
		return bAdmin;
	};

	char szKey[64];

	score_t *score;
};

void GM_RegisterCVars( void );
void GM_AllowWeapon( void );
void GM_AddWeapons( CBasePlayer *pPlayer );
void GM_AllowAmmo( void );
void GM_AddAmmo( CBasePlayer *pPlayer );
void GM_InitAdmins( void );
BOOL GM_IsAdmin( edict_t *player );
void GM_AddAdmin( edict_t *player );
void GM_Login( edict_t *player );
void GM_ChatLog(edict_t *pEntity, char *text);
BOOL GM_ClientCommand( CBasePlayer *pPlayer, const char *pCmd );
void GM_LogCmd(edict_t *pEntity, const char *cmd, const char *argv);
BOOL GM_IsAuthidValid( CBasePlayer *player );
char *GM_GetRandomString(int length);
void GM_SpawnIfValid(CBasePlayer *player);
void GM_PlayerPostThink( edict_t *pEntity );
void GM_CvarValue2( const edict_t *pEnt, int requestID, const char *cvarName, const char *value );
void GM_KickPlayer( CBasePlayer *player, const char *reason );
void GM_ScoreUpdate( CBasePlayer *pVictim, int deaths, CBasePlayer *pKiller, int frags );
void GM_MultiplayThink( void );
void GM_UpdateScoreLeaderboard( bool motd_only );
void GM_InitScores( void );
const char *GM_GetAuthUID( CBasePlayer *player );
BOOL GM_IsPlayerFucked( CBasePlayer *plr );
void GM_WriteMutedID( void );
BOOL GM_IsAllowedClassname( const char *classname );
#endif
