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
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "BMOD_flyingcrowbar.h"

#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_crowbar, CCrowbar )

enum crowbar_e
{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
#ifndef CROWBAR_IDLE_ANIM	
	CROWBAR_ATTACK3HIT
#else
	CROWBAR_ATTACK3HIT,
	CROWBAR_IDLE2,
	CROWBAR_IDLE3
#endif
};

void CCrowbar::Spawn()
{
	Precache();
	m_iId = WEAPON_CROWBAR;
	SET_MODEL( ENT( pev ), "models/w_crowbar.mdl" );
	m_iClip = -1;

	FallInit();// get ready to fall down.
}

void CCrowbar::Precache( void )
{
	PRECACHE_MODEL( "models/v_crowbar.mdl" );
	PRECACHE_MODEL( "models/w_crowbar.mdl" );
	PRECACHE_MODEL( "models/p_crowbar.mdl" );
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );

	m_usCrowbar = PRECACHE_EVENT( 1, "events/crowbar.sc" );
}

int CCrowbar::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_CROWBAR;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}

int CCrowbar::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CCrowbar::Deploy()
{
	return DefaultDeploy( "models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar" );
}

void CCrowbar::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( CROWBAR_HOLSTER );
}

void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int		i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult	tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for( i = 0; i < 2; i++ )
	{
		for( j = 0; j < 2; j++ )
		{
			for( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = ( tmpTrace.vecEndPos - vecSrc ).Length();
					if( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void CCrowbar::PrimaryAttack()
{
	if( !Swing( 1 ) )
	{
#ifndef CLIENT_DLL
		SetThink( &CCrowbar::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
#endif
	}
}

void CCrowbar::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

void CCrowbar::SwingAgain( void )
{
	Swing( 0 );
}

int CCrowbar::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif
	if( fFirst )
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usCrowbar, 
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
		0, 0, 0 );
	}

	if( tr.flFraction >= 1.0 )
	{
		if( fFirst )
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
#ifdef CROWBAR_IDLE_ANIM
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ( ( m_iSwing++ ) % 2 ) + 1 )
		{
		case 0:
			SendWeaponAnim( CROWBAR_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim( CROWBAR_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim( CROWBAR_ATTACK3HIT );
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		// play thwack, smack, or dong sound
                float flVol = 1.0;
                int fHitWorld = TRUE;

		if( pEntity )
		{
			ClearMultiDamage();
			// If building with the clientside weapon prediction system,
			// UTIL_WeaponTimeBase() is always 0 and m_flNextPrimaryAttack is >= -1.0f, thus making
			// m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() always evaluate to false.
#ifdef CLIENT_WEAPONS
			if( ( m_flNextPrimaryAttack + 1 == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
			if( ( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
			{
				// first swing does full damage
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgCrowbar / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG( 0, 2 ) )
				{
				case 0:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM );
					break;
				case 1:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM );
					break;
				case 2:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM );
					break;
				}
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if( !pEntity->IsAlive() )
				{
					m_flNextPrimaryAttack = GetNextAttackDelay(0.25);
					return TRUE;
				}
				else
					flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

			if( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch( RANDOM_LONG( 0, 1 ) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) ); 
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}

			// delay the decal a bit
			m_trHit = tr;

			UTIL_Sparks(tr.vecEndPos);
			// Troll: Dont make an earthquake with the crowbar
			//UTIL_ScreenShake( tr.vecEndPos, 15.0, 20.0, 1, 50 );
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * CROWBAR_WALLHIT_VOLUME );

		SetThink( &CCrowbar::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.25 );
	}
#ifdef CROWBAR_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	return fDidHit;
}

// BMOD Begin - Flying Crowbar
void CCrowbar::CreateThink()
{
		// Get the origin, direction, and fix the angle of the throw.
		Vector vecAng = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		vecAng.z = -90;
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * 8 + gpGlobals->v_forward * 16;

		int ang = RANDOM_LONG(-1000,-500);
		// Create a flying ,crowbar.
		CFlyingCrowbar *pFCBar = (CFlyingCrowbar *)Create( "flying_crowbar", vecSrc, Vector( 0, 0, 0 ), m_pPlayer->edict() );
		// Give the crowbar its velocity, angle, and spin.
		// Lower the gravity a bit, so it flys.
		pFCBar->pev->velocity = gpGlobals->v_forward * (int)( -( ang / 2 ) ) + m_pPlayer->pev->velocity;
		pFCBar->pev->angles = vecAng;
		//ALERT(at_console,"angles %f %f %f\n",vecAng.x,vecAng.y,vecAng.z);
		//ALERT(at_console,"velocity %f %f %f\n",pFCBar->pev->velocity.x,pFCBar->pev->velocity.y,pFCBar->pev->velocity.x);
		pFCBar->pev->avelocity = Vector(ang,RANDOM_LONG(-50,50),RANDOM_LONG(-25,25));
		pFCBar->pev->gravity = 0.5;
		pFCBar->pev->dmg = -( ang / 10 );
		pFCBar->m_pPlayer = m_pPlayer;

		// Nope! take away the crowbar
		m_pPlayer->RemovePlayerItem( this, FALSE );

		// take item off hud
		m_pPlayer->pev->weapons &= ~( 1 << this->m_iId );

		// Destroy this weapon
		DestroyItem();

		// They no longer have an active item.
		m_pPlayer->m_pActiveItem = NULL;

		SetThink( NULL );
}

void CCrowbar::SecondaryAttack()
{
	// Don't throw underwater, and only throw if we were able to detatch 
	// from player.
	if( ( m_pPlayer->pev->waterlevel != 3 ) )
	{
		SetThink( &CCrowbar::CreateThink );
		pev->nextthink = gpGlobals->time + 0.2;

		// Do player weapon anim and sound effect.
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 0xF ) );
		SendWeaponAnim( CROWBAR_ATTACK1MISS );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;
	}
}
// BMOD End - Flying Crowbar

#ifdef CROWBAR_IDLE_ANIM
void CCrowbar::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9 )
		{
			iAnim = CROWBAR_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
		}
		else
		{
			if( flRand > 0.5 )
			{
				iAnim = CROWBAR_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 30.0;
			}
			else
			{
				iAnim = CROWBAR_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
#endif
