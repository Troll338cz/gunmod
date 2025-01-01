/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// Base class for flying monsters.  This overrides the movement test & execution code from CBaseMonster
#pragma once
#ifndef FLYINGMONSTER_H
#define FLYINGMONSTER_H
#include "effects.h"

#define ICHTHYOSAUR_SPEED 150

class CFlyingMonster : public CBaseMonster
{
public:
	int 		CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );// check validity of a straight move through space
	BOOL		FTriangulate( const Vector &vecStart, const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApex );
	Activity	GetStoppedActivity( void );
	void		Killed( entvars_t *pevAttacker, int iGib );
	void		Stop( void );
	float		ChangeYaw( int speed );
	void		HandleAnimEvent( MonsterEvent_t *pEvent );
	void		MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void		Move( float flInterval = 0.1 );
	BOOL		ShouldAdvanceRoute( float flWaypointDist );

	inline void	SetFlyingMomentum( float momentum ) { m_momentum = momentum; }
	inline void	SetFlyingFlapSound( const char *pFlapSound ) { m_pFlapSound = pFlapSound; }
	inline void	SetFlyingSpeed( float speed ) { m_flightSpeed = speed; }
	float		CeilingZ( const Vector &position );
	float		FloorZ( const Vector &position );
	BOOL		ProbeZ( const Vector &position, const Vector &probe, float *pFraction );

	// UNDONE:  Save/restore this stuff!!!
protected:
	Vector		m_vecTravel;		// Current direction
	float		m_flightSpeed;		// Current flight speed (decays when not flapping or gliding)
	float		m_stopTime;			// Last time we stopped (to avoid switching states too soon)
	float		m_momentum;			// Weight for desired vs. momentum velocity
	const char	*m_pFlapSound;
};

// UNDONE: Save/restore here
class CIchthyosaur : public CFlyingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	CUSTOM_SCHEDULES

	int Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );

	void Killed( entvars_t *pevAttacker, int iGib );
	void BecomeDead( void );

	void EXPORT CombatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT BiteTouch( CBaseEntity *pOther );

	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );

	float ChangeYaw( int speed );
	Activity GetStoppedActivity( void );

	void Move( float flInterval );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void MonsterThink( void );
	void Stop( void );
	void Swim( void );
	Vector DoProbe(const Vector &Probe );

	float VectorToPitch( const Vector &vec );
	float FlPitchDiff( void );
	float ChangePitch( int speed );

	Vector m_SaveVelocity;
	float m_idealDist;

	float m_flBlink;

	float m_flEnemyTouched;
	BOOL  m_bOnAttack;

	float m_flMaxSpeed;
	float m_flMinSpeed;
	float m_flMaxDist;

	CBeam *m_pBeam;

	float m_flNextAlert;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];

	void IdleSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void BiteSound( void );
	void DeathSound( void );
	void PainSound( void );
};
#endif //FLYINGMONSTER_H
