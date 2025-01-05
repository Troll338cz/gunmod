/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#ifndef GENERICMONSTER_H
#define GENERICMONSTER_H

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
class CGenericMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask( void );
	void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );
	void IdleHeadTurn( Vector &vecFriend );
	void EXPORT MonsterThink();

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;
};

#endif // GENERICMONSTER_H