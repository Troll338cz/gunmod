/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "shake.h"
#include "customweapons.h"
#include "unpredictedweapon.h"

class CShockrifle : public CBasePlayerWeaponU
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 5; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	//BOOL HasAmmo(void){return TRUE;}
	//BOOL CanDeploy(void){return TRUE;}
	BOOL IsUseable(void){return TRUE;}

	void Holster(int skiplocal = 0);
	void Reload(void);
	void WeaponIdle(void);
	float m_flRechargeTime;
	int m_iLight;

	// Troll: Charged fire
	int m_iSoundState = 0;
	float GetFullChargeTime( void );
	void SetupBeams( Vector params );
};

enum shockrifle_e {
	SHOCK_IDLE1 = 0,
	SHOCK_FIRE,
	SHOCK_DRAW,
	SHOCK_HOLSTER,
	SHOCK_IDLE3,
};

LINK_ENTITY_TO_CLASS(weapon_shockrifle, CShockrifle);

void CShockrifle::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOCKRIFLE;
	SET_MODEL(ENT(pev), "models/w_shock.mdl");

	m_iDefaultAmmo = 10;

	FallInit();// get ready to fall down.
}


void CShockrifle::Precache(void)
{
	PRECACHE_MODEL("models/v_shock.mdl");
	PRECACHE_MODEL("models/w_shock.mdl");
	PRECACHE_MODEL("models/p_shock.mdl");

	PRECACHE_SOUND("weapons/shock_discharge.wav");
	PRECACHE_SOUND("weapons/shock_draw.wav");
	PRECACHE_SOUND("weapons/shock_fire.wav");
	PRECACHE_SOUND("weapons/shock_impact.wav");
	PRECACHE_SOUND("weapons/shock_recharge.wav");

	m_iLight = PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/flare3.spr");

	UTIL_PrecacheOther("shock_beam");
}

int CShockrifle::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		// all hivehands come full. 
		pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 10;

		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CShockrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Shocks";
	p->iMaxAmmo1 = 10;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_SHOCKRIFLE;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = 10;

	return 1;
}

BOOL CShockrifle::Deploy()
{
	m_flRechargeTime = gpGlobals->time + 1;
	m_fInAttack = 0;
	return DefaultDeploy("models/v_shock.mdl", "models/p_shock.mdl", SHOCK_DRAW, "bow");
}

void CShockrifle::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim(SHOCK_HOLSTER);
	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, m_pPlayer->pev);
		WRITE_BYTE(TE_KILLBEAM);
		WRITE_SHORT(m_pPlayer->entindex());
	MESSAGE_END();
	if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
}

void CShockrifle::SetupBeams( Vector params )
{
	for( int i=2;i<=4;i++)
	{
		MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, m_pPlayer->pev);
			WRITE_BYTE(TE_BEAMENTS);
			WRITE_SHORT((m_pPlayer->entindex() & 0x0FFF) | (( i & 0xF ) << 12 ));
			WRITE_SHORT((m_pPlayer->entindex() & 0x0FFF) | (( 1 & 0xF ) << 12 ));
			WRITE_SHORT(m_iLight);
			WRITE_BYTE(0); // startframe
			WRITE_BYTE(12); // framerate
			WRITE_BYTE(params.x); // life
			WRITE_BYTE(params.y);  // width
			WRITE_BYTE(params.z);   // noise
			WRITE_BYTE(55);   // r, g, b
			WRITE_BYTE(255);   // r, g, b
			WRITE_BYTE(255);   // r, g, b
			WRITE_BYTE(225); // brightness
			WRITE_BYTE(0);		// speed
		MESSAGE_END();
	}
/*
			WRITE_BYTE(15); // life
			WRITE_BYTE(10);  // width
			WRITE_BYTE(100);   // noise
*/
}

void CShockrifle::PrimaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		int attenuation = 150 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		int dmg = 100 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_discharge.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
		RadiusDamage(m_pPlayer->pev->origin, m_pPlayer->pev, m_pPlayer->pev, dmg, attenuation, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
		CBaseEntity *pSparks = CBaseEntity::Create( "shockparticles", pev->origin, Vector(0,0,0), edict() );
		pSparks->pev->owner = m_pPlayer->edict();
		pSparks->pev->dmg = 200;
		return;
	}

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	anglesAim.x = -anglesAim.x;
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	Vector vecSrc;
	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 8 + gpGlobals->v_right * 12 + gpGlobals->v_up * -12;

	CBaseEntity *pShock = CBaseEntity::Create("shock_beam", vecSrc, anglesAim, m_pPlayer->edict());
	pShock->pev->velocity = gpGlobals->v_forward * 2000;


	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;


	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, m_pPlayer->pev);
		WRITE_BYTE(TE_KILLBEAM);
		WRITE_SHORT(m_pPlayer->entindex());
	MESSAGE_END();

	SetupBeams( Vector(15, 10, 100) );

	SendWeaponAnim( SHOCK_FIRE );
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 1, ATTN_NORM);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = gpGlobals->time + 0.33;
	m_flNextSecondaryAttack = gpGlobals->time + 0.8;
	m_flTimeWeaponIdle = gpGlobals->time + 0.33;
}


float CShockrifle::GetFullChargeTime( void )
{
	return 5.0f;
}

void CShockrifle::SecondaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 5)
		return;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		int attenuation = 150 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		int dmg = 100 * m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_discharge.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
		RadiusDamage(m_pPlayer->pev->origin, m_pPlayer->pev, m_pPlayer->pev, dmg, attenuation, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
		CBaseEntity *pSparks = CBaseEntity::Create( "shockparticles", pev->origin, Vector(0,0,0), edict() );
		pSparks->pev->owner = m_pPlayer->edict();
		pSparks->pev->dmg = 200;
		return;
	}

	if( m_fInAttack == 0 )
	{
		// spin up
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		//printf("spinup\n");
		m_fInAttack = 1;
		SendWeaponAnim( SHOCK_IDLE1 );
		m_flTimeWeaponIdle = gpGlobals->time + 0.33;
		m_pPlayer->m_flStartCharge = gpGlobals->time;
		m_pPlayer->m_flAmmoStartCharge = gpGlobals->time + GetFullChargeTime();
		SetupBeams( Vector(15, 12, 120) );
		m_iSoundState = SND_CHANGE_PITCH;
	}
	else if( m_fInAttack == 1 )
	{
		if( m_flTimeWeaponIdle < gpGlobals->time )
		{
			//printf("attack\n");
			SetupBeams( Vector(15, 14, 140) );
			m_fInAttack = 2;
		}
	}
	else
	{
		int pitch = (int)( ( gpGlobals->time - m_pPlayer->m_flStartCharge ) * ( 150 / GetFullChargeTime() ) + 100 );
		if( pitch > 250 ) 
			 pitch = 250;
		
		// ALERT( at_console, "%d %d %d\n", m_fInAttack, m_iSoundState, pitch );

		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 0x3f ) );
		SetupBeams( Vector(15, 16, 160) );
		if( m_iSoundState == 0 )
			ALERT( at_console, "sound state %d\n", m_iSoundState );
/*
		CBaseEntity *pSparks = CBaseEntity::Create( "shockparticles", pev->origin, Vector(0,0,0), edict() );
		pSparks->pev->owner = m_pPlayer->edict();
		pSparks->pev->dmg = 16;
*/
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(m_pPlayer->pev->origin.x);	// X
			WRITE_COORD(m_pPlayer->pev->origin.y);	// Y
			WRITE_COORD(m_pPlayer->pev->origin.z);	// Z
			WRITE_BYTE( 20 );		// radius * 0.1
			WRITE_BYTE( 0 );		// r
			WRITE_BYTE( 255 );		// g
			WRITE_BYTE( 255 );		// b
			WRITE_BYTE( 15 );		// time * 10
			WRITE_BYTE( 8 );		// decay * 0.1
		MESSAGE_END( );

		Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		anglesAim.x = -anglesAim.x;
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		Vector vecSrc;
		vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 8 + gpGlobals->v_right * 12 + gpGlobals->v_up * -12;

		CBaseEntity *pShock = CBaseEntity::Create("shock_beam", vecSrc, anglesAim, m_pPlayer->edict());
		pShock->pev->velocity = gpGlobals->v_forward * 2000;
		pShock->pev->flags |= FL_IMMUNE_LAVA;

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions

		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 1;

		//printf("attack 2\n");
		m_fInAttack = 0;
		m_flTimeWeaponIdle = gpGlobals->time + 0.33;

		SendWeaponAnim( SHOCK_FIRE );
	}
	m_flTimeWeaponIdle = gpGlobals->time + 0.33;
	m_flNextPrimaryAttack = gpGlobals->time + 0.33;
	m_flNextSecondaryAttack = gpGlobals->time + 0.8;
/*
	SendWeaponAnim( SHOCK_IDLE1 );
	m_flNextSecondaryAttack = gpGlobals->time + 3.3f;
	m_flTimeWeaponIdle = gpGlobals->time + 3.3f;
*/
}

void CShockrifle::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 10)
		return;

	while (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 10 && m_flRechargeTime < gpGlobals->time)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/shock_recharge.wav", 1, ATTN_NORM);

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;

		m_flRechargeTime = gpGlobals->time + 0.6;
	}
}


void CShockrifle::WeaponIdle(void)
{
	Reload();
	m_fInAttack = 0;
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 3.3f;
	SendWeaponAnim(SHOCK_IDLE3);
}
