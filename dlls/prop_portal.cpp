#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"customentity.h"
#include	"shake.h"
#include	"prop_portal.h"
#include	"const.h"

LINK_ENTITY_TO_CLASS(prop_portal, CPortal)

void CPortal::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/portal.mdl");
//	UTIL_SetSize(pev, Vector(-8, -23, -36),  Vector(8, 23, 36));

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("prop_portal");

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	if( m_bBlue )
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "portalg/portal_open1.wav", 1, ATTN_NORM);
		pev->skin = 0;
	}
	else
	{
		switch (RANDOM_LONG(0,1)) {
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "portalg/portal_open2.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "portalg/portal_open3.wav", 1, ATTN_NORM);
			break;
		}
		pev->skin = 1;
	}
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CPortal::PortalTouch);

}

void CPortal::Precache()
{
	PRECACHE_MODEL("models/portal.mdl");

	PRECACHE_SOUND("portalg/portal_open1.wav");
	PRECACHE_SOUND("portalg/portal_open2.wav");
	PRECACHE_SOUND("portalg/portal_open3.wav");

	PRECACHE_SOUND("portalg/portal_close1.wav");
	PRECACHE_SOUND("portalg/portal_close2.wav");
}


void CPortal::OpenPortal(edict_t *pevOwner, const Vector angles, const Vector vecStart, CBaseEntity *touchEnt, BOOL blue)
{
	CPortal *pPort = GetClassPtr((CPortal*)NULL);

	pPort->m_bBlue = blue;
	UTIL_SetOrigin(pPort->pev, vecStart);

	pPort->pev->owner = pevOwner;

	pPort->Spawn();
	pPort->pev->angles = angles;

	if( touchEnt )
	{
		pPort->pev->movetype = MOVETYPE_COMPOUND;
		pPort->pev->aiment = ENT(touchEnt->pev);
	}
}

void CPortal::ClosePortal()
{

	if( m_bBlue )
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "portalg/portal_close1.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "portalg/portal_close2.wav", 1, ATTN_NORM);

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

void CPortal::PortalTouch(CBaseEntity *pOther)
{

}
