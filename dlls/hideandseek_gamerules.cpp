/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, disribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// teamplay_gamerules.cpp
//

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"hideandseek_gamerules.h"
#include	"game.h"
#include	"shake.h"
#include	"gravgunmod.h"

static char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH] = {
	"spectators",
	"seekers",
	"hiding"
};

static int team_scores[MAX_TEAMS];
static int num_teams = MAX_TEAMS;

extern DLL_GLOBAL BOOL		g_fGameOver;


BOOL IsPlayerConnected( CBasePlayer * plr)
{
	return plr && STRING(plr->edict()->v.netname)[0] != 0 && plr->m_fGameHUDInitialized && plr->m_ggm.iState != STATE_UNINITIALIZED;
}

int CountTeam(const char *team)
{
	int c = 0;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if( IsPlayerConnected(plr) )
		{
			if(strstr(plr->m_szTeamName, team) )
				c++;
		}
	}

	return c;
}

int countplayers()
{
	int c = 0;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if( IsPlayerConnected(plr) )
			c++;
	}
	return c;
}

CGunmodHideAndSeek::CGunmodHideAndSeek()
{
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;
	m_brtimer = FALSE;
	m_roundstarted = FALSE;
	flTimerUpdate = 0;

	// Copy over the team from the server config
	RecountTeams();
}

extern cvar_t timeleft, fragsleft;

#ifndef NO_VOICEGAMEMGR
#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;
#endif
void CGunmodHideAndSeek::Think( void )
{
	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;



#ifndef NO_VOICEGAMEMGR
	g_VoiceGameMgr.Update(gpGlobals->frametime);
#endif

	if( g_fGameOver )   // someone else quit the game already
	{
		CHalfLifeMultiplay::Think();
		return;
	}

	float flTimeLimit = CVAR_GET_FLOAT( "mp_timelimit" ) * 60;


	time_remaining = (int)( flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0 );

	if( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}


	if( countplayers() >= hideandseek_minplayers.value && !m_roundstarted )
	{
		RoundStart();
	}

	if( countplayers() < hideandseek_minplayers.value && m_roundstarted )
	{
		RoundEnd();
	}

	if( m_roundstarted && m_brtimer )
	{
		if( CountTeam("hiding") == 0 )
			RoundEnd();
		else if( CountTeam("seekers") == 0)
			RoundEnd();

		if( gpGlobals->time >= flbrtime && firstseeker )
		{
			UTIL_ScreenFade(firstseeker, Vector(0, 255 ,0) , 0.1, 0.1, 125, FFADE_IN);
			UTIL_SpawnPlayer(firstseeker);
			flRoundTime = gpGlobals->time + hideandseek_roundtime.value;
			GGM_SayText("^3(SERVER)^7: seeker was spawned");
			m_brtimer = FALSE;
		}
		else if( gpGlobals->time >= flTimerUpdate )
		{
			char text[1024];
			sprintf(text, "^3(SERVER)^7: %d seconds left before seeker spawn", (int)(flbrtime-gpGlobals->time));
			GGM_SayText(text);
			flTimerUpdate = gpGlobals->time + 10;

		}
	}
	else if( m_roundstarted )
	{
		if( CountTeam("hiding") == 0 )
		{
			GGM_SayText("^3(SERVER)^7: ^2winners: seekers");
			RoundEnd();
		}
		else if( CountTeam("seekers") == 0)
		{
			GGM_SayText("^3(SERVER)^7: ^2winners: hiding");
			RoundEnd();
		}

		if( gpGlobals->time >= flRoundTime )
		{
			GGM_SayText("^3(SERVER)^7: ^2 winners: hiding");
			RoundStart();
		}
		else if( gpGlobals->time >= flTimerUpdate )
		{
			char text[1024];
			int min = (int)((flRoundTime - gpGlobals->time)/60);
			int sec = (int)(flRoundTime - gpGlobals->time - 60*min);
			sprintf(text, "^3(SERVER)^7: round time left %d:%d", min, sec);
			flTimerUpdate = gpGlobals->time + 30;
			GGM_SayText( text );
		}
	}



	float flFragLimit = fraglimit.value;
	if( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any team is over the frag limit
		for( int i = 0; i < num_teams; i++ )
		{
			if( team_scores[i] >= flFragLimit )
			{
				GoToIntermission();
				return;
			}

			remain = (int)( flFragLimit - team_scores[i] );
			if( remain < bestfrags )
			{
				bestfrags = remain;
			}
		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	// Updates once per second
	if( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

extern int gmsgGameMode;
extern int gmsgSayText;
extern int gmsgTeamInfo;
extern int gmsgTeamNames;
extern int gmsgScoreInfo;

void CGunmodHideAndSeek::UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 );  // game mode teamplay
	MESSAGE_END();
}

const char *CGunmodHideAndSeek::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
{
	RecountTeams();

	// update the current player of the team he is joining
	if( pPlayer->m_szTeamName[0] == '\0' )
	{
		if( m_brtimer )
		{
			strncpy( pPlayer->m_szTeamName, "hiding", TEAM_NAME_LENGTH );
		}
		else
		{
			strncpy( pPlayer->m_szTeamName, "spectators", TEAM_NAME_LENGTH );
			pPlayer->RemoveAllItems(TRUE);
			UTIL_BecomeSpectator(pPlayer);
			pPlayer->m_ggm.iState = STATE_SPECTATOR;
		}
	}

	return pPlayer->m_szTeamName;
}

//=========================================================
// InitHUD
//=========================================================
void CGunmodHideAndSeek::InitHUD( CBasePlayer *pPlayer )
{
	int i;

	SetDefaultPlayerTeam( pPlayer );
	CHalfLifeMultiplay::InitHUD( pPlayer );

	// Send down the team names
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict() );  
		WRITE_BYTE( num_teams );
		for( i = 0; i < num_teams; i++ )
		{
			WRITE_STRING( team_names[i] );
		}
	MESSAGE_END();

	RecountTeams();

	char text[1024];
	sprintf( text, "* assigned to team %s\n", pPlayer->m_szTeamName );
	ChangePlayerTeam( pPlayer, pPlayer->m_szTeamName, FALSE, FALSE );
	UTIL_SayText( text, pPlayer );

	if( countplayers() < hideandseek_minplayers.value )
	{
		m_brtimer = FALSE;
		m_roundstarted = FALSE;
		sprintf( text, "^3(SERVER)^7: need %d players or more to start round\n", (int)hideandseek_minplayers.value );
		UTIL_SayText(text, pPlayer);
	}

	RecountTeams();

	// update this player with all the other players team info
	// loop through all active players and send their team info to the new client
	for( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if( plr )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_STRING( plr->TeamID() );
			MESSAGE_END();
		}
	}
}

void CGunmodHideAndSeek::ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib )
{
	int damageFlags = DMG_GENERIC;
	int clientIndex = pPlayer->entindex();

	if( !bGib )
		damageFlags |= DMG_NEVERGIB;
	else
		damageFlags |= DMG_ALWAYSGIB;

	if( bKill )
	{
		// kill the player,  remove a death,  and let them start on the new team
		m_DisableDeathMessages = TRUE;
		m_DisableDeathPenalty = TRUE;

		entvars_t *pevWorld = VARS( INDEXENT( 0 ) );
		pPlayer->TakeDamage( pevWorld, pevWorld, 900, damageFlags );

		m_DisableDeathMessages = FALSE;
		m_DisableDeathPenalty = FALSE;
	}

	if( pPlayer->m_szTeamName != pTeamName )
		strncpy( pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH );

	g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", pPlayer->m_szTeamName );

	// notify everyone's HUD of the team change
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( clientIndex );
		WRITE_STRING( pPlayer->m_szTeamName );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( clientIndex );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( pPlayer->m_szTeamName ) + 1 );
	MESSAGE_END();
}

extern int gmsgDeathMsg;

//=========================================================
// Deathnotice. 
//=========================================================
void CGunmodHideAndSeek::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	if( m_DisableDeathMessages )
		return;

	if( pVictim && pKiller && pKiller->flags & FL_CLIENT )
	{
		CBasePlayer *pk = (CBasePlayer*) CBaseEntity::Instance( pKiller );

		if( pk )
		{
			if( ( pk != pVictim ) && ( PlayerRelationship( pVictim, pk ) == GR_TEAMMATE ) )
			{
				MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
					WRITE_BYTE( ENTINDEX( ENT( pKiller ) ) );		// the killer
					WRITE_BYTE( ENTINDEX( pVictim->edict() ) );	// the victim
					WRITE_STRING( "teammate" );		// flag this as a teammate kill
				MESSAGE_END();
				return;
			}
		}
	}

	CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pevInflictor );
}

//=========================================================
//=========================================================
void CGunmodHideAndSeek::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if( !m_DisableDeathPenalty )
	{
		CBasePlayer *killer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
		CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );

		if( strstr(killer->m_szTeamName, "seekers") )
		{
			ChangePlayerTeam(pVictim, "seekers", FALSE, FALSE);
		}
		RecountTeams();
	}
}

//=========================================================
// IsHideAndSeek
//=========================================================
BOOL CGunmodHideAndSeek::IsHideAndSeek( void )
{
	return TRUE;
}

BOOL CGunmodHideAndSeek::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		// my teammate hit me.
		if( ( friendlyfire.value == 0 ) && ( pAttacker != pPlayer ) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

//=========================================================
//=========================================================
int CGunmodHideAndSeek::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	if( ( *GetTeamID( pPlayer ) != '\0' ) && ( *GetTeamID( pTarget ) != '\0' ) && !stricmp( GetTeamID( pPlayer ), GetTeamID( pTarget ) ) )
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CGunmodHideAndSeek::ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	// always autoaim, unless target is a teammate
	CBaseEntity *pTgt = CBaseEntity::Instance( target );
	if( pTgt && pTgt->IsPlayer() )
	{
		if( PlayerRelationship( pPlayer, pTgt ) == GR_TEAMMATE )
			return FALSE; // don't autoaim at teammates
	}

	return CHalfLifeMultiplay::ShouldAutoAim( pPlayer, target );
}

//=========================================================
//=========================================================
int CGunmodHideAndSeek::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if( !pKilled )
		return 0;

	if( !pAttacker )
		return 1;

	if( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;

	return 1;
}

//=========================================================
//=========================================================
const char *CGunmodHideAndSeek::GetTeamID( CBaseEntity *pEntity )
{
	if( pEntity == NULL || pEntity->pev == NULL )
		return "";

	// return their team name
	return pEntity->TeamID();
}

int CGunmodHideAndSeek::GetTeamIndex( const char *pTeamName )
{
	if( pTeamName && *pTeamName != 0 )
	{
		// try to find existing team
		for( int tm = 0; tm < num_teams; tm++ )
		{
			if( !stricmp( team_names[tm], pTeamName ) )
				return tm;
		}
	}

	return -1;	// No match
}

const char *CGunmodHideAndSeek::GetIndexedTeamName( int teamIndex )
{
	if( teamIndex < 0 || teamIndex >= num_teams )
		return "";

	return team_names[teamIndex];
}
void CGunmodHideAndSeek::DistributeTeams( void )
{
	int i;

	int num = 0;
	int seeker_num = RANDOM_LONG(1, countplayers());

	// loop through all clients
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if( IsPlayerConnected(plr) )
		{
			num++;
			if( seeker_num == num )
			{
				char text[1024];
				sprintf(text, "^3(SERVER)^7 %s ^7- new seeker", GGM_PlayerName(plr));
				ChangePlayerTeam(plr, "seekers", FALSE, FALSE);
				firstseeker = plr;
				GGM_SayText(text);
				UTIL_ScreenFade(plr, Vector(125, 0 ,0) , hideandseek_brtime.value, 0, 255, FFADE_STAYOUT);
			}
			else
			{
				ChangePlayerTeam(plr, "hiding", FALSE, FALSE);
				plr->RemoveAllItems(TRUE);
				UTIL_SpawnPlayer(plr);
			}
		}
	}
}

//=========================================================
//=========================================================
void CGunmodHideAndSeek::RecountTeams( bool bResendInfo )
{
	// Sanity check
	memset( team_scores, 0, sizeof(team_scores) );

	// loop through all clients
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr )
		{
			const char *pTeamName = plr->TeamID();

			// try add to existing team
			int tm = GetTeamIndex( pTeamName );

			if( tm >= 0 )
			{
				team_scores[tm] += (int)plr->pev->frags;
			}

			if( bResendInfo ) //Someone's info changed, let's send the team info again.
			{
				if( plr )
				{
					MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo, NULL );
						WRITE_BYTE( plr->entindex() );
						WRITE_STRING( plr->TeamID() );
					MESSAGE_END();
				}
			}
		}
	}
}

void CGunmodHideAndSeek::RoundStart()
{
	flbrtime = gpGlobals->time + hideandseek_brtime.value;
	m_brtimer = TRUE;
	m_roundstarted = TRUE;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if( plr )
		{
			plr->RemoveAllItems(TRUE);
			UTIL_BecomeSpectator(plr);
			plr->m_ggm.iState = STATE_SPECTATOR;
		}
	}
	DistributeTeams();
}

void CGunmodHideAndSeek::RoundEnd()
{
	m_roundstarted = FALSE;
	m_brtimer = FALSE;


	if( countplayers() < hideandseek_minplayers.value )
	{
		char text[1024];
		sprintf( text, "^3(SERVER)^7: need %d players or more to start round\n", (int)hideandseek_minplayers.value );
		GGM_SayText(text);
	}

	if(firstseeker)
		UTIL_ScreenFade(firstseeker, Vector(0, 0 ,255) , 0.1, 0.1, 125, FFADE_IN);

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if( plr )
		{
			ChangePlayerTeam(plr, "spectators", FALSE, FALSE);
			plr->RemoveAllItems(TRUE);
			UTIL_BecomeSpectator(plr);
			plr->m_ggm.iState = STATE_SPECTATOR;
		}
	}
}
