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
	pev->solid = SOLID_TRIGGER; // SOLID_BBOX
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
	SetThink(&CPortal::PortalDebug);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CPortal::Precache()
{
	PRECACHE_MODEL("models/portal.mdl");

	PRECACHE_SOUND("portalg/portal_open1.wav");
	PRECACHE_SOUND("portalg/portal_open2.wav");
	PRECACHE_SOUND("portalg/portal_open3.wav");

	PRECACHE_SOUND("portalg/portal_close1.wav");
	PRECACHE_SOUND("portalg/portal_close2.wav");
	m_spriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr" );
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
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "portalg/portal_close1.wav", 1, ATTN_NORM);
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( pev->origin.x );// X
			WRITE_COORD( pev->origin.y );// Y
			WRITE_COORD( pev->origin.z );// Z
			WRITE_BYTE( 4 );// radius * 0.1
			WRITE_BYTE( this->ColorBlue.x );      // r
			WRITE_BYTE( this->ColorBlue.y );      // g
			WRITE_BYTE( this->ColorBlue.z );       // b
			WRITE_BYTE( 3 );       // time * 10
			WRITE_BYTE( 2 );// decay * 0.1
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( pev->origin.x );// X
			WRITE_COORD( pev->origin.y );// Y
			WRITE_COORD( pev->origin.z );// Z
			WRITE_BYTE( 4 );// radius * 0.1
			WRITE_BYTE( this->ColorOrange.x );      // r
			WRITE_BYTE( this->ColorOrange.y );      // g
			WRITE_BYTE( this->ColorOrange.z );       // b
			WRITE_BYTE( 3 );       // time * 10
			WRITE_BYTE( 2 );// decay * 0.1
		MESSAGE_END();

		EMIT_SOUND(ENT(pev), CHAN_ITEM, "portalg/portal_close2.wav", 1, ATTN_NORM);
	}

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_IMPLOSION );
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_BYTE( 20 );		// rad
		WRITE_BYTE( 64 );		// count
		WRITE_BYTE( 4 );		// life
	MESSAGE_END();

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

void DrawDebugLaser( Vector vecSrc, Vector vecEnd, Vector color )
{
        extern DLL_GLOBAL short g_sModelIndexLaser;
        MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_BEAMPOINTS);
                WRITE_COORD( vecSrc.x );
                WRITE_COORD( vecSrc.y );
                WRITE_COORD( vecSrc.z );
                WRITE_COORD( vecEnd.x );
                WRITE_COORD( vecEnd.y );
                WRITE_COORD( vecEnd.z );
                WRITE_SHORT( g_sModelIndexLaser );
                WRITE_BYTE( 0 );        // frame start
                WRITE_BYTE( 10 );       // framerate
                WRITE_BYTE( 2 );        // life
                WRITE_BYTE( 10 );       // width
                WRITE_BYTE( 0 );        // noise
                WRITE_BYTE( color.x );        // r, g, b
                WRITE_BYTE( color.y );        // r, g, b
                WRITE_BYTE( color.z );      // r, g, b
                WRITE_BYTE( 255 );      // brightness
                WRITE_BYTE( 10 );       // speed
        MESSAGE_END();
}

Vector AngleToDirection(Vector angle) { 
	double x_angle_radians = (angle.y * ( M_PI / 180.0 ));
	double y_angle_radians = (angle.x * ( M_PI / 180.0 ));
	Vector direction; 
	direction.x = cos(y_angle_radians) * cos(x_angle_radians);
	direction.y = cos(y_angle_radians) * sin(x_angle_radians);
	direction.z = sin(y_angle_radians);
	return direction;
}


void CPortal::PortalTouch(CBaseEntity *pOther)
{
	// Ignore other portals and world
	if( FClassnameIs( pOther->pev, "prop_portal" )  || FClassnameIs( pOther->pev, "worldspawn" )  )
		return;

	if( pOther->IsPlayer() )
	{
		Vector anglez = AngleToDirection(this->pev->angles);
		pOther->pev->velocity = pOther->pev->velocity + anglez * 128;
	}
}

BOOL VerifyTeleport( Vector center )
{
	Vector DebugRed 	= {255,0,0};
	Vector DebugGreen 	= {0,255,0};
	TraceResult tr_t;
	TraceResult tr_b;
	Vector tr_top 		= center + Vector(0,0,1) * 32;
	Vector tr_bottom 	= center - Vector(0,0,1) * 32;
	UTIL_TraceLine( tr_top, center,  dont_ignore_monsters, NULL, &tr_t );
	UTIL_TraceLine( tr_bottom,center,  dont_ignore_monsters, NULL, &tr_b );
	BOOL IsInvalid = true;
	if( tr_t.fAllSolid == 0 && tr_t.fStartSolid == 0 )
	{
		DrawDebugLaser( center, tr_top, DebugGreen );
	}
	else
	{
		DrawDebugLaser( center, tr_top, DebugRed );
		IsInvalid = false;
	}
	if( tr_b.fAllSolid == 0 && tr_b.fStartSolid == 0  )
	{
		DrawDebugLaser( center, tr_bottom, DebugGreen );
	}
	else
	{
		DrawDebugLaser( center, tr_bottom, DebugRed );
		IsInvalid = false;
	}
	return IsInvalid;
}

void CPortal::PortalDebug()
{
	Vector anglez = AngleToDirection(this->pev->angles);
	Vector angle_end = this->pev->origin + anglez * 64; 	
	if( m_bBlue )
		DrawDebugLaser( this->pev->origin, angle_end, ColorBlue );
	else
		DrawDebugLaser( this->pev->origin, angle_end, ColorOrange );

	VerifyTeleport(angle_end);
	pev->nextthink = gpGlobals->time + 0.1;
}

