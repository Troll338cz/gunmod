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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "game.h"
#include "unpredictedweapon.h"

#define WEAPON_SNIPERRIFLE 28
#define SNIPERRIFLE_WEIGHT 10
#define SNIPERRIFLE_MAX_CLIP 5
#define SNIPERRIFLE_DEFAULT_GIVE 5
#define _762_MAX_CARRY 15
#define AMMO_762BOX_GIVE 5

class CSniperrifle : public CBasePlayerWeaponU
{
public:
        void Spawn(void);
        void Precache(void);
        int iItemSlot(void) { return 3; }
        int GetItemInfo(ItemInfo *p);
        int AddToPlayer(CBasePlayer *pPlayer);
        void PrimaryAttack(void);
        void SecondaryAttack(void);
        BOOL Deploy(void);
        void Holster(int skiplocal = 0);
        void Reload(void);
        void WeaponIdle(void);
        //void ItemPostFrame(void);

        BOOL ShouldWeaponIdle(void) { return TRUE; };

        BOOL m_fInZoom;// don't save this.

        BOOL m_fNeedAjustBolt;
        int      m_iBoltState;

        enum SNIPER_BOLTSTATE { BOLTSTATE_FINE = 0, BOLTSTATE_ADJUST, BOLTSTATE_ADJUSTING, };
};


enum sniper_e {
	SNIPER_DRAW = 0,
	SNIPER_SLOWIDLE1,
	SNIPER_FIRE,
	SNIPER_FIRELASTROUND,
	SNIPER_RELOAD1,
	SNIPER_RELOAD2,
	SNIPER_RELOAD3,
	SNIPER_SLOWIDLE2,
	SNIPER_HOLSTER,
};

LINK_ENTITY_TO_CLASS( weapon_sniperrifle, CSniperrifle );

void CSniperrifle::Spawn( )
{

	pev->classname = MAKE_STRING("weapon_sniperrifle"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_SNIPERRIFLE;
	SET_MODEL(ENT(pev), "models/w_m40a1.mdl");

	m_iDefaultAmmo = SNIPERRIFLE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSniperrifle::Precache( void )
{
	PRECACHE_MODEL("models/v_m40a1.mdl");
	PRECACHE_MODEL("models/w_m40a1.mdl");
	PRECACHE_MODEL("models/p_m40a1.mdl");

	PRECACHE_SOUND ("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND ("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND ("weapons/sniper_fire.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload_second_seq.wav");
	PRECACHE_SOUND ("weapons/sniper_reload3.wav");
	PRECACHE_SOUND ("weapons/sniper_zoom.wav");

//	m_usSniper = PRECACHE_EVENT( 1, "events/sniper.sc" );
}

int CSniperrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762";
	p->iMaxAmmo1 = _762_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 5;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SNIPERRIFLE;
	p->iWeight = SNIPERRIFLE_WEIGHT;

	return 1;
}

BOOL CSniperrifle::Deploy( )
{
	return DefaultDeploy( "models/v_m40a1.mdl", "models/p_m40a1.mdl", SNIPER_DRAW, "bow", 0 );
}

int CSniperrifle::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CSniperrifle::Holster( int skiplocal )
{
	m_fInReload = FALSE;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	SendWeaponAnim( SNIPER_HOLSTER );

	if ( m_fInZoom )
	{
		SecondaryAttack( );
	}
}

void CSniperrifle::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
	}
	else if ( m_pPlayer->pev->fov != 15 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 15;
		m_fInZoom = 1;
	}

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);

	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}
void CSniperrifle::PrimaryAttack()
{
	if ( m_fInSpecialReload )
		return;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	float flSpread = 0.001;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;

	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MP5, 0, 100, m_pPlayer->pev, m_pPlayer->random_seed ); // damage is 100
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.75;

	if( m_iClip == 0 )
		SendWeaponAnim( SNIPER_FIRELASTROUND );
	else
		SendWeaponAnim( SNIPER_FIRE );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM);
	//PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	// HEV suit - indicate out of ammo condition
	m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 68.0 / 38.0;
}


void CSniperrifle::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 5)
		return;

	int iResult;

	if ( m_pPlayer->pev->fov != 0 )
	{
		SecondaryAttack();
	}

	if (m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0 )
	{
		iResult = DefaultReload( 5, SNIPER_RELOAD1, 80 / 34 );
		m_fInSpecialReload = 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.25;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.25;
	}
	else
	{
		iResult = DefaultReload( 5, SNIPER_RELOAD3, 2.25 );
	}
}
void CSniperrifle::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if ( m_fInSpecialReload )
		{
			m_fInSpecialReload = 0;
			SendWeaponAnim( SNIPER_RELOAD2 );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 49.0 / 27.0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 27.0;
		}
		else
		{
			m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );
			if (m_iClip <= 0)
			{
				iAnim = SNIPER_SLOWIDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
			}
			else
			{
				iAnim = SNIPER_SLOWIDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 67.5 / 16;
			}
			SendWeaponAnim( iAnim, 1 );
		}
	}
}

class CSniperrifleAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m40a1clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m40a1clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 5, "762", _762_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_762, CSniperrifleAmmo )
