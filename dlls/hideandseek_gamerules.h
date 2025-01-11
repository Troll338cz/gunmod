/***
*
Created by nillerusr
*
****/

//
// hideandseek_gamerules.h
//

#pragma once
#ifndef HIDEANDSEEK_GAMERULES_H
#define HIDEANDSEEK_GAMERULES_H

#define MAX_TEAMNAME_LENGTH		16
#define MAX_TEAMS			3

#define TEAMPLAY_TEAMLISTLENGTH		MAX_TEAMS*MAX_TEAMNAME_LENGTH

class CGunmodHideAndSeek: public CHalfLifeMultiplay
{
public:
	CGunmodHideAndSeek();

	virtual BOOL IsHideAndSeek( void );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual const char *GetTeamID( CBaseEntity *pEntity );
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void InitHUD( CBasePlayer *pl );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor );
	virtual const char *GetGameDescription( void ) { return "Gunmod Hide And Seek"; }  // this is the game name that gets seen in the server browser
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void Think( void );
	virtual int GetTeamIndex( const char *pTeamName );
	virtual const char *GetIndexedTeamName( int teamIndex );
	const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer );
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib );

private:
	CBasePlayer *firstseeker;
	float flTimerUpdate;
	float flbrtime;
	float flRoundTime;
	BOOL m_brtimer;
	BOOL m_roundstarted;

	void RecountTeams( bool bResendInfo = FALSE );
	void DistributeTeams( void );
	void RoundStart( void );
	void RoundEnd();

	BOOL m_DisableDeathMessages;
	BOOL m_DisableDeathPenalty;
	BOOL m_teamLimit;				// This means the server set only some teams as valid
};
#endif // TEAMPLAY_GAMERULES_H
