//#include <math.h>
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "player.h"
#include "explode.h"
#include "gamerules.h"
#include "shake.h"
#include "prop.h"
#include "gunmod.h"
#include "gravgunmod.h"

class CDust : public CProp
{
	void Spawn();
	virtual void PropRespawn( );
	void Precache();

	void EXPORT FuckThink( );
	BOOL IsFaggot( CBaseEntity *player );

	virtual float TouchGravGun( CBaseEntity *attacker, int stage )
	{
		return 0;
	}
	void EXPORT TolchokTouch( CBaseEntity *pOther );
	float deg;
// turn
	float yaw;
	float VecToYaw( Vector vecDir );
};

void CDust::Precache()
{
	PRECACHE_MODEL("models/xash/toilet.mdl");
}

void CDust::Spawn(void)
{

	Precache();

	if( minsH == g_vecZero )
	{
		// default barrel parameters
		minsV = Vector(-10, -10, -17);
		maxsV = Vector(10, 10, 18);
		minsH = Vector(-10, -10, -10);
		maxsH = Vector(10, 10, 13);
 	}
	m_flCollideFriction = 0.7;
	m_flFloorFriction = 0.5;
	if( !(pev->spawnflags & SF_PROP_FIXED ) )
	{
		UTIL_SetSize( pev, minsH, maxsH );
		if( m_shape < 3 )
			CheckRotate();

		UTIL_SetOrigin( pev, pev->origin );

		DROP_TO_FLOOR( edict() );
	}
	spawnOrigin = pev->origin;
	spawnAngles = pev->angles;
	m_flSpawnHealth = pev->health;
	if( m_flSpawnHealth <= 0 )
		m_flSpawnHealth = 30;
	if( !m_flRespawnTime )
		m_flRespawnTime = 80;
	pev->dmg = 100;
	pev->nextthink = gpGlobals->time + 0.1;
	PropRespawn();
}

//=========================================================
// VecToYaw - turns a directional vector into a yaw value
// that points down that vector.
//=========================================================
float CDust::VecToYaw( Vector vecDir )
{
	if( vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0 )
		return pev->angles.y;

	return UTIL_VecToYaw( vecDir );
}

void CDust::PropRespawn()
{
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );
	pev->effects |= EF_NODRAW;
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_TRIGGER;
	pev->takedamage = DAMAGE_NO;
	pev->health = m_flSpawnHealth;
	pev->velocity = pev->avelocity = g_vecZero;
	pev->angles = spawnAngles;
	pev->deadflag = DEAD_NO;
	SET_MODEL( ENT(pev), "models/xash/toilet.mdl" );
	pev->rendermode = 1;
	pev->renderamt = 255;
	pev->rendercolor = Vector(1, 1, 1);

	m_oldshape = (PropShape)-1;
	CheckRotate();

	SetUse( NULL );
	SetThink( &CDust::FuckThink );

	SetTouch( &CDust::TolchokTouch );
	pev->framerate = 1.0f;
	UTIL_SetOrigin( pev, spawnOrigin );
	m_owner2 = NULL;
	m_attacker = NULL;
}

BOOL CDust::IsFaggot( CBaseEntity *player )
{
	if( mp_toilet.string[0] != 0 && strstr(GGM_PlayerName(player),mp_toilet.string) )
		return TRUE;
	else
		return FALSE;
}

void CDust::FuckThink( )
{
	BOOL k = FALSE;
	CBasePlayer *plr;
	CBasePlayer *faggot;
	
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
         plr = (CBasePlayer*)UTIL_PlayerByIndex( i );
         if(plr && IsFaggot(plr) )
         {
			faggot = plr;
			if( !faggot->IsAlive() )
				continue;

			if( pev->effects & EF_NODRAW )
				pev->effects &= ~EF_NODRAW;
			if( pev->solid == SOLID_NOT )
				pev->solid = SOLID_TRIGGER;
			
			k = TRUE;
			Vector direction = plr->pev->origin - pev->origin;
			direction.z = -direction.z;
			Vector angles = UTIL_VecToAngles( direction );
			UTIL_MakeVectors( angles );
			pev->velocity = gpGlobals->v_forward * mp_toilet_speed.value;

			if(mp_toilet_turn.value)
			{
		            	yaw = VecToYaw( plr->pev->origin - pev->origin );
		
				if( yaw > 180 )
					yaw -= 360;
				if( yaw < -180 )
					yaw += 360;
				pev->angles.y = yaw;
			}
		}
	}
	if(!k)
	{
        	if( !(pev->effects & EF_NODRAW) )
			pev->effects |= EF_NODRAW;
		if( pev->solid != SOLID_NOT )
			pev->solid = SOLID_NOT;
        	pev->velocity = Vector(0,0,0);
	}
    
	pev->nextthink = gpGlobals->time + 0.001;
}


void CDust::TolchokTouch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() && (pOther->IsPlayer() && !IsFaggot(pOther)) )
		return;

	TraceResult tr = UTIL_GetGlobalTrace( );
	ClearMultiDamage();
	pOther->TraceAttack(pev, 500, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
	ApplyMultiDamage(pev, pev);
	//SetThink( &CBaseEntity::SUB_Remove );
	//pev->nextthink = gpGlobals->time;
}

LINK_ENTITY_TO_CLASS(toilet, CDust);
