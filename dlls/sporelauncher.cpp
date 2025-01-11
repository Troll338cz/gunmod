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
//#include "xen.h"
#include "sporegrenade.h"
#include "unpredictedweapon.h"
#include "customweapons.h"

class CSporelauncher : public CBasePlayerWeaponU
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void Poop( Vector vecSrc );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy();
	void Reload( void );
	void WeaponIdle( void );

	int m_iSquidSpitSprite;
	float m_flNextReload;
};


// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum sporelauncher_e {
	SPLAUNCHER_IDLE = 0,
	SPLAUNCHER_FIDGET,
	SPLAUNCHER_RELOAD_REACH ,
	SPLAUNCHER_RELOAD_LOAD,
	SPLAUNCHER_RELOAD_AIM,
	SPLAUNCHER_FIRE,
	SPLAUNCHER_HOLSTER1,
	SPLAUNCHER_DRAW1,
	SPLAUNCHER_IDLE2
};

LINK_ENTITY_TO_CLASS(weapon_sporelauncher, CSporelauncher);

void CSporelauncher::Spawn()
{
	Precache();
	m_iId = WEAPON_SPORELAUNCHER;
	SET_MODEL(ENT(pev), "models/w_spore_launcher.mdl");

	m_iDefaultAmmo = 5;

	FallInit();// get ready to fall
}


void CSporelauncher::Precache(void)
{
	PRECACHE_MODEL("models/v_spore_launcher.mdl");
	PRECACHE_MODEL("models/w_spore_launcher.mdl");
	PRECACHE_MODEL("models/p_spore_launcher.mdl");

	PRECACHE_SOUND("weapons/splauncher_altfire.wav");
	PRECACHE_SOUND("weapons/splauncher_bounce.wav");
	PRECACHE_SOUND("weapons/splauncher_fire.wav");
	PRECACHE_SOUND("weapons/splauncher_impact.wav");
	PRECACHE_SOUND("weapons/splauncher_pet.wav");
	PRECACHE_SOUND("weapons/splauncher_reload.wav");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_MODEL("sprites/bigspit.spr");
	m_iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");
	UTIL_PrecacheOther("spore");
}

int CSporelauncher::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}


int CSporelauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "spores";
	p->iMaxAmmo1 = 20;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 5;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SPORELAUNCHER;
	p->iWeight = 15;

	return 1;
}

void CSporelauncher::Poop( Vector vecSrc )
{
	// spew the spittle temporary ents.
//	gEngfuncs.pEfxAPI->R_Sprite_Spray( (float*)&vecSpitOffset, (float*)&vecSpitDir, iSpitModelIndex, 8, 210, 25 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, m_pPlayer->pev);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(vecSrc.x);
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		WRITE_COORD(gpGlobals->v_forward[0]);
		WRITE_COORD(gpGlobals->v_forward[1]);
		WRITE_COORD(gpGlobals->v_forward[2]);
		WRITE_SHORT(m_iSquidSpitSprite);
		WRITE_BYTE(8);
		WRITE_BYTE(210);
		WRITE_BYTE(25);
	MESSAGE_END();
//	m_iSquidSpitSprite
}


BOOL CSporelauncher::Deploy()
{
	return DefaultDeploy("models/v_spore_launcher.mdl", "models/p_spore_launcher.mdl", SPLAUNCHER_DRAW1, "rpg");
}

void CSporelauncher::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		if (m_iClip == 0)
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	// m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

	CSpore *pSpore = CSpore::CreateSporeRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1500;

	SendWeaponAnim(SPLAUNCHER_FIRE);
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/splauncher_altfire.wav", 1, ATTN_NORM, 0, 100);
	Poop(vecSrc);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 1.0;
	else
		m_flTimeWeaponIdle = gpGlobals->time + 0.75;
	m_fInSpecialReload = 0;
}


void CSporelauncher::SecondaryAttack(void)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	//m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;


	CSpore *pSpore = CSpore::CreateSporeGrenade( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, FALSE );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 700;

	SendWeaponAnim(SPLAUNCHER_FIRE);
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/splauncher_fire.wav", 1, ATTN_NORM, 0, 100);
	Poop(vecSrc);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 1.0;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInSpecialReload = 0;

}


void CSporelauncher::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 5)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(SPLAUNCHER_RELOAD_REACH);
		m_fInSpecialReload = 1;
//		m_pPlayer->m_flNextAttack = gpGlobals->time + 0.7;
		m_flTimeWeaponIdle = gpGlobals->time + 0.7;
		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flNextSecondaryAttack = gpGlobals->time + 1.0;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->time)
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		// Play reload sound.
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/splauncher_reload.wav", 1, ATTN_NORM, 0, 100);

		SendWeaponAnim(SPLAUNCHER_RELOAD_LOAD);

		m_flNextReload = gpGlobals->time + 1.0;
		m_flTimeWeaponIdle = gpGlobals->time + 1.0;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CSporelauncher::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle <  gpGlobals->time)
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != 5 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim(SPLAUNCHER_RELOAD_AIM);

				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = gpGlobals->time + 0.8;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.5)
			{
				iAnim = SPLAUNCHER_IDLE;
				m_flTimeWeaponIdle = gpGlobals->time + 2.0f;
			}
			else
			{
				iAnim = SPLAUNCHER_IDLE2;
				m_flTimeWeaponIdle = gpGlobals->time + 4.0f;
			}

			SendWeaponAnim(iAnim);
		}
	}
}
