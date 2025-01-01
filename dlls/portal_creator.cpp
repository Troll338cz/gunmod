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
#include	"portal_creator.h"
#include	"prop_portal.h"
#include	"player.h"

LINK_ENTITY_TO_CLASS(portal_creator, CPortalCreator)

void CPortalCreator::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("portal_creator");
	SET_MODEL(ENT(pev), "sprites/glow01.spr");
	UTIL_SetOrigin(pev, pev->origin);

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	pev->scale = 0.2;
	if( m_bBlue )
		SetTransparency( kRenderTransAdd, 60, 190, 240, 200, kRenderFxNoDissipation );
	else
		SetTransparency( kRenderTransAdd, 230, 130, 30, 200, kRenderFxNoDissipation );

	CreateEffects();
	SetThink( &CPortalCreator::FlyThink );
	pev->nextthink = gpGlobals->time;
}

void CPortalCreator::Precache()
{
	PRECACHE_MODEL("sprites/glow01.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("models/shock_effect.mdl");

	PRECACHE_SOUND("portalg/portal_fizzle2.wav");
	PRECACHE_SOUND("portalg/portal_ambient_loop1.wav");
	PRECACHE_SOUND("portalg/portalgun_powerup1.wav");
	PRECACHE_SOUND("portalg/portal_invalid_surface3.wav");


	m_iTrail = PRECACHE_MODEL( "sprites/lgtning.spr" );
	m_iBeam = PRECACHE_MODEL( "sprites/smoke.spr" );
	m_iBlue = PRECACHE_MODEL( "sprites/portals/blue.spr" );
	m_iOrange = PRECACHE_MODEL( "sprites/portals/orange.spr" );
}

void CPortalCreator::FlyThink()
{
	if (pev->waterlevel == 3)
	{
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
	else
		pev->nextthink = gpGlobals->time + 0.05;
}

void CPortalCreator::Shoot(edict_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity, BOOL blue)
{
	CPortalCreator *pPort = GetClassPtr((CPortalCreator *)NULL);
	pPort->m_bBlue = blue;
	pPort->Spawn();

	UTIL_SetOrigin(pPort->pev, vecStart);
	pPort->pev->velocity = vecVelocity;
	pPort->pev->owner = pevOwner;
	pPort->pev->angles = angles;

	pPort->pev->nextthink = gpGlobals->time;
}

void CPortalCreator::OpenError()
{
	TraceResult tr = UTIL_GetGlobalTrace();
	Vector ang = pev->angles;
	ang.x = -ang.x;
	UTIL_MakeVectors(ang);
	Vector from = pev->origin;
	Vector fwd = pev->origin - tr.vecPlaneNormal - gpGlobals->v_forward*4;

	EMIT_SOUND(ENT(pev), CHAN_BODY, "portalg/portal_invalid_surface3.wav", VOL_NORM, ATTN_NORM);

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_SPRITETRAIL );
		WRITE_COORD( from.x );
		WRITE_COORD( from.y );
		WRITE_COORD( from.z );
		WRITE_COORD( fwd.x );
		WRITE_COORD( fwd.y );
		WRITE_COORD( fwd.z );
		WRITE_SHORT(m_bBlue ? m_iBlue : m_iOrange);
		WRITE_BYTE(25);
		WRITE_BYTE(1);
		WRITE_BYTE(1);
		WRITE_BYTE(20);
		WRITE_BYTE(100);
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

BOOL TestSurface(Vector &vecPos, Vector &vecAngles, Vector vecPlaneNormal, CBaseEntity *touchEnt)
{
	TraceResult tr, tr2;
	Vector aimAngles;
	aimAngles = vecAngles;
	aimAngles.x = -aimAngles.x; // stupid quake bug
	UTIL_MakeVectors(aimAngles);
	float flRightSpace, flLeftSpace, flUpSpace, flDownSpace, flLength;
	flRightSpace = flLeftSpace = PORTAL_WIDTH;
	flUpSpace = flDownSpace = PORTAL_HEIGHT;

	for(int i = 1; i <= PORTAL_HEIGHT; i++)
	{
		Vector pos = vecPos + gpGlobals->v_up*i + gpGlobals->v_forward;
		UTIL_TraceLine(pos, pos + gpGlobals->v_forward*(-5), ignore_monsters, NULL, &tr);
		flLength = (tr.vecEndPos - pos).Length();
		if( flLength >= 2 || tr.pHit != touchEnt->edict() )
		{
			flUpSpace = i;
			break;
		}
	}

	for(int i = 1; i <= PORTAL_HEIGHT; i++)
	{
		Vector pos = vecPos - gpGlobals->v_up*i + gpGlobals->v_forward;
		UTIL_TraceLine(pos, pos + gpGlobals->v_forward*(-5), ignore_monsters, NULL, &tr);
		flLength = (tr.vecEndPos - pos).Length();
		if( flLength >= 2 || tr.pHit != touchEnt->edict() )
		{
			flDownSpace = i;
			break;
		}
	}

	for(int i = 1; i <= PORTAL_WIDTH; i++)
	{
		Vector pos = vecPos + gpGlobals->v_right*i + gpGlobals->v_forward;
		UTIL_TraceLine(pos, pos + gpGlobals->v_forward*(-5), ignore_monsters, NULL, &tr);
		flLength = (tr.vecEndPos - pos).Length();
		if( flLength >= 2 || tr.pHit != touchEnt->edict() )
		{
			flRightSpace = i;
			break;
		}
	}

	for(int i = 1; i <= PORTAL_WIDTH; i++)
	{
		Vector pos = vecPos - gpGlobals->v_right*i + gpGlobals->v_forward;
		UTIL_TraceLine(pos, pos + gpGlobals->v_forward*(-5), ignore_monsters, NULL, &tr);
		flLength = (tr.vecEndPos - pos).Length();
		if( flLength >= 2 || tr.pHit != touchEnt->edict() )
		{
			flLeftSpace = i;
			break;
		}
	}

	// i like sex
	UTIL_TraceLine(vecPos, vecPos + gpGlobals->v_right*PORTAL_WIDTH*2, ignore_monsters, NULL, &tr);
	flLength = ( tr.vecEndPos - vecPos ).Length();
	if( flLength < flRightSpace )
		flRightSpace = flLength;
	UTIL_TraceLine(vecPos, vecPos + gpGlobals->v_right*(-PORTAL_WIDTH*2), ignore_monsters, NULL, &tr);
	flLength = ( tr.vecEndPos - vecPos ).Length();
	if( flLength < flLeftSpace )
		flLeftSpace = flLength;
	UTIL_TraceLine(vecPos, vecPos + gpGlobals->v_up*PORTAL_HEIGHT*2, ignore_monsters, NULL, &tr);
	flLength = ( tr.vecEndPos - vecPos ).Length();
	if( flLength < flUpSpace )
		flUpSpace = flLength;
	UTIL_TraceLine(vecPos, vecPos + gpGlobals->v_up*(-PORTAL_HEIGHT*2), ignore_monsters, NULL, &tr);
	flLength = ( tr.vecEndPos - vecPos ).Length();
	if( flLength < flDownSpace )
		flDownSpace = flLength;

	if( flRightSpace + flLeftSpace < PORTAL_WIDTH)
		return FALSE;

	if( flRightSpace < PORTAL_WIDTH/2 )
		vecPos = vecPos + gpGlobals->v_right*(-(PORTAL_WIDTH/2-flRightSpace));
	else if( flLeftSpace < PORTAL_WIDTH/2 )
		vecPos = vecPos + gpGlobals->v_right*(PORTAL_WIDTH/2-flLeftSpace);

	if( flUpSpace + flDownSpace < PORTAL_HEIGHT)
		return FALSE;

	if( flUpSpace < PORTAL_HEIGHT/2 )
		vecPos = vecPos + gpGlobals->v_up*(-(PORTAL_HEIGHT/2-flUpSpace));
	else if( flDownSpace < PORTAL_HEIGHT/2 )
		vecPos = vecPos + gpGlobals->v_up*(PORTAL_HEIGHT/2-flDownSpace);

	return TRUE;
}

void CPortalCreator::Touch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner)
		return;

	Vector vecSrc = pev->origin;
	TraceResult tr = UTIL_GetGlobalTrace();
	Vector aimAngles = UTIL_VecToAngles(tr.vecPlaneNormal);
	if( (aimAngles.x >= 269 && aimAngles.x <= 271) || (aimAngles.x >= 89 && aimAngles.x <= 91))
	{
		aimAngles.y = pev->angles.y;
		aimAngles.z = 0;
	}

	CBaseEntity *owner = CBaseEntity::Instance(pev->owner);

	if( !pOther->IsBSPModel() ||
			!TestSurface(vecSrc, aimAngles, tr.vecPlaneNormal, pOther) )
	{
		OpenError();
		return;
	}

	CBaseEntity *pObject = NULL;
	while( ( pObject = UTIL_FindEntityByClassname(pObject, "prop_portal") ) != NULL )
	{
		CPortal *pPort = (CPortal*)pObject;
		if( pev->owner == pPort->pev->owner && pPort->m_bBlue == this->m_bBlue)
			pPort->ClosePortal();
	}

	CPortal::OpenPortal( pev->owner, aimAngles, vecSrc, pOther, m_bBlue );

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}


void CPortalCreator::CreateEffects()
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );     // entity
		WRITE_SHORT( m_iTrail );        // model
		WRITE_BYTE( 5 ); // life
		WRITE_BYTE( 2 ); // width
		if(m_bBlue)
		{
			WRITE_BYTE( 60 ); // r
			WRITE_BYTE( 190 ); // g
			WRITE_BYTE( 240 ); // b
		}
		else
		{
			WRITE_BYTE( 230 ); // r
			WRITE_BYTE( 130 ); // g
			WRITE_BYTE( 30 ); // b
		}
		WRITE_BYTE( 255 ); // brightness
	MESSAGE_END();
}
