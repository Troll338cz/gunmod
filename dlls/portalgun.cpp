#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "sprite.h"
#include "com_model.h"
#include "customweapons.h"
#include "unpredictedweapon.h"
#include "portal_creator.h"
#include "prop_portal.h"

enum PortalGunAnim
{
	PORTALGUN_IDLE = 0,
	PORTALGUN_SHOOT1,
	PORTALGUN_SHOOT2,
	PORTALGUN_DRAW
};

class CPortalGun : public CBasePlayerWeaponU
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 0; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	int num = 0;
};

LINK_ENTITY_TO_CLASS(weapon_portalgun, CPortalGun);

void CPortalGun::Spawn()
{
	Precache();
	m_iId = WEAPON_PORTALGUN;

	SET_MODEL( ENT( pev ), "models/w_portalbruh.mdl" );

	FallInit();// get ready to fall down.
}

int CPortalGun::AddToPlayer( CBasePlayer *pPlayer )
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

void CPortalGun::Precache( void )
{
	PRECACHE_MODEL( "models/v_portalbruh.mdl" );
	PRECACHE_MODEL( "models/p_portalbruh.mdl" );
	PRECACHE_MODEL( "models/w_portalbruh.mdl" );

	PRECACHE_SOUND("portalg/portalgun_shoot_red1.wav");
	PRECACHE_SOUND("portalg/portalgun_shoot_blue1.wav");

	UTIL_PrecacheOther("portal_creator");
	UTIL_PrecacheOther("prop_portal");
}

int CPortalGun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = WEAPON_NOCLIP;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = WEAPON_PORTALGUN;
	p->iWeight = 11;
	p->iFlags = 0;
	return 1;
}

BOOL CPortalGun::Deploy()
{
	return DefaultDeploy( "models/v_portalbruh.mdl", "models/p_portalbruh.mdl", PORTALGUN_DRAW, "gauss" );
}

void CPortalGun::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.01;
	CPortalCreator::ClosePortals( m_pPlayer->edict() );
	//SendWeaponAnim( PORTALGUN_HOLSTER );
}

void CPortalGun::PrimaryAttack( void )
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	TraceResult tr;

	SendWeaponAnim(PORTALGUN_SHOOT1);
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "portalg/portalgun_shoot_blue1.wav", 1, ATTN_NORM);

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	anglesAim.x	= -anglesAim.x;

	Vector vecSrc;
	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 6 + gpGlobals->v_right * 8 - gpGlobals->v_up * 12;

	UTIL_TraceLine(m_pPlayer->GetGunPosition(), m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16384, dont_ignore_monsters, m_pPlayer->edict(), &tr);
	Vector aim = tr.vecEndPos - m_pPlayer->GetGunPosition() - gpGlobals->v_right * 8 + gpGlobals->v_up * 12;
	aim.z = -aim.z;
	Vector ang = UTIL_VecToAngles(aim);
	UTIL_MakeVectors(ang);

	CPortalCreator::Shoot(m_pPlayer->edict(), anglesAim, vecSrc, gpGlobals->v_forward * 1000, TRUE);

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
	m_flTimeWeaponIdle = gpGlobals->time + 1;
}

void CPortalGun::SecondaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	TraceResult tr;

	SendWeaponAnim(PORTALGUN_SHOOT1);

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "portalg/portalgun_shoot_red1.wav", 1, ATTN_NORM);

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	anglesAim.x	= -anglesAim.x;

	Vector vecSrc;
	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 6 + gpGlobals->v_right * 8 - gpGlobals->v_up * 12;

	UTIL_TraceLine(m_pPlayer->GetGunPosition(), m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16384, dont_ignore_monsters, m_pPlayer->edict(), &tr);
	Vector aim = tr.vecEndPos - m_pPlayer->GetGunPosition() - gpGlobals->v_right * 8 + gpGlobals->v_up * 12;
	aim.z = -aim.z;
	Vector ang = UTIL_VecToAngles(aim);
	UTIL_MakeVectors(ang);

	CPortalCreator::Shoot(m_pPlayer->edict(), anglesAim, vecSrc, gpGlobals->v_forward * 1000, FALSE);

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
	m_flTimeWeaponIdle = gpGlobals->time + 1;
}


void CPortalGun::Reload( void )
{
	;
}

void CPortalGun::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle >= gpGlobals->time )
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 2;
	SendWeaponAnim( PORTALGUN_IDLE );
}
