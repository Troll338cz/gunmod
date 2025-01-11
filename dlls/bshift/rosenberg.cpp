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
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"game.h"
#include	"scientist.h"

//=========================================================
// Dr. Rosenberg
//=========================================================
class CRosenberg : public CScientist
{
public:
	void Spawn( void );
	void Precache( void );
	void DeclineFollowing( void );
	void Scream( void );
	void StartTask( Task_t *pTask );
	void TalkInit( void );
	void DeathSound( void );
	void PainSound( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	Schedule_t *GetSchedule( void );
private:	
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS( monster_rosenberg, CRosenberg )

extern Task_t tlHeal[];
extern Schedule_t slHeal[];
extern Task_t tlFear[];
extern Schedule_t slFear[];
extern Task_t tlScientistCover[];
extern Schedule_t slScientistCover[];

void CRosenberg::DeclineFollowing( void )
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( "RO_POK", 2, VOL_NORM, ATTN_NORM );
}

void CRosenberg::Scream( void )
{
	if( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "RO_SCREAM", RANDOM_FLOAT( 3.0f, 6.0f ), VOL_NORM, ATTN_NORM );
	}
}

void CRosenberg::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:
		//if( FOkToSpeak() )
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "RO_HEAL", 2, VOL_NORM, ATTN_IDLE );
		TaskComplete();
		break;
	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;
	case TASK_RANDOM_SCREAM:
		if( RANDOM_FLOAT( 0.0f, 1.0f ) < pTask->flData )
			Scream();
		TaskComplete();
		break;
	case TASK_SAY_FEAR:
		if( FOkToSpeak() )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;
			PlaySentence( "RO_FEAR", 5, VOL_NORM, ATTN_NORM );
		}
		TaskComplete();
		break;
	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if( ( m_hTargetEnt->pev->origin - pev->origin ).Length() < 1.0f )
			{
				TaskComplete();
			}
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
					TaskFail();
			}
		}
		break;
	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CRosenberg::Spawn( void )
{
	// We need to set it before precache so the right voice will be chosen
	if( pev->body == -1 )
	{
		// -1 chooses a random head
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );// pick a head, any head
	}

	Precache();

	SET_MODEL( ENT( pev ), "models/scientist_bs.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth * 2;
	pev->view_ofs = Vector( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//m_flDistTooFar = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	// Luther is black, make his hands black
	if( pev->body == HEAD_LUTHER )
		pev->skin = 1;

	MonsterInit();
	SetUse( &CTalkMonster::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRosenberg::Precache( void )
{
	PRECACHE_MODEL( "models/scientist_bs.mdl");
	PRECACHE_SOUND( "rosenberg/ro_pain0.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain1.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain2.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain3.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain4.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain5.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain6.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain7.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain8.wav" );

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CRosenberg::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)
	m_szGrp[TLK_ANSWER] = "RO_ANSWER";
	m_szGrp[TLK_QUESTION] = "RO_QUESTION";
	m_szGrp[TLK_IDLE] = "RO_IDLE";
	m_szGrp[TLK_STARE] = "RO_STARE";
	m_szGrp[TLK_USE] = "RO_OK";
	m_szGrp[TLK_UNUSE] = "RO_WAIT";
	m_szGrp[TLK_STOP] = "RO_STOP";
	m_szGrp[TLK_NOSHOOT] = "RO_SCARED";
	m_szGrp[TLK_HELLO] = "RO_HELLO";

	m_szGrp[TLK_PLHURT1] = "!RO_CUREA";
	m_szGrp[TLK_PLHURT2] = "!RO_CUREB";
	m_szGrp[TLK_PLHURT3] = "!RO_CUREC";

	m_szGrp[TLK_PHELLO] = "RO_PHELLO";
	m_szGrp[TLK_PIDLE] = "RO_PIDLE";
	m_szGrp[TLK_PQUESTION] = "RO_PQUEST";
	m_szGrp[TLK_SMELL] = "RO_SMELL";

	m_szGrp[TLK_WOUND] = "RO_WOUND";
	m_szGrp[TLK_MORTAL] = "RO_MORTAL";


	// get voice for head
	switch( pev->body % NUM_SCIENTIST_HEADS )
	{
	default:
	case HEAD_GLASSES:
		m_voicePitch = 105;
		break;	//glasses
	case HEAD_EINSTEIN:
		m_voicePitch = 100;
		break;	//einstein
	case HEAD_LUTHER:
		m_voicePitch = 95;
		break;	//luther
	case HEAD_SLICK:
		m_voicePitch = 100;
		break;	//slick
	}
}

int CRosenberg::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// make sure friends talk about it if player hurts scientist...
	return CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// PainSound
//=========================================================
void CRosenberg::PainSound( void )
{
	const char *pszSound;
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 0.75f );

	switch( RANDOM_LONG( 0, 8 ) )
	{
		case 0:
			pszSound = "rosenberg/ro_pain0.wav";
			break;
		case 1:
                        pszSound = "rosenberg/ro_pain1.wav";
                        break;
		case 2:
                        pszSound = "rosenberg/ro_pain2.wav";
                        break;
		case 3:
                        pszSound = "rosenberg/ro_pain3.wav";
                        break;
		case 4:
                        pszSound = "rosenberg/ro_pain4.wav";
                        break;
		case 5:
                        pszSound = "rosenberg/ro_pain5.wav";
                        break;
		case 6:
                        pszSound = "rosenberg/ro_pain6.wav";
                        break;
		case 7:
			pszSound = "rosenberg/ro_pain7.wav";
			break;
		case 8:
			pszSound = "rosenberg/ro_pain8.wav";
			break;
	}

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pszSound, 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CRosenberg::DeathSound( void )
{
	PainSound();
}

Schedule_t *CRosenberg::GetSchedule( void )
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if( pEnemy )
		{
			if( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = 0;
				pEnemy = 0;
			}
		}

		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// Cower when you hear something scary
		if( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT( pSound != NULL );
			if( pSound )
			{
				if( pSound->m_iType & ( bits_SOUND_DANGER | bits_SOUND_COMBAT ) )
				{
					if( gpGlobals->time - m_fearTime > 3 )	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType( SCHED_STARTLE );	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if( IsFollowing() )
		{
			if( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if( pEnemy != NULL )
				relationship = IRelationship( pEnemy );

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if( relationship != R_DL && relationship != R_HT )
			{
				// If I'm already close enough to my target
				if( TargetDistance() <= 128 )
				{
					if( CanHeal() )	// Heal opportunistically
						return slHeal;
					if( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
						return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
		}

		if( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if( HasConditions( bits_COND_NEW_ENEMY ) )
			return slFear;					// Point and scream!
		if( HasConditions( bits_COND_SEE_ENEMY ) )
			return slScientistCover;		// Take Cover

		if( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slScientistCover;			// Run & Cower
		break;
	default:
		break;
	}
	
	return CTalkMonster::GetSchedule();
}