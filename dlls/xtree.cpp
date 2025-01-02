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
#include "xtree.h"

LINK_ENTITY_TO_CLASS(xtree, CXtree);

void CXtree::Precache( void )
{
	PRECACHE_MODEL("models/xtree.mdl");
}

void CXtree::Spawn(void)
{
	Precache();
	SET_MODEL( ENT( pev ), "models/xtree.mdl" );
	pev->classname = MAKE_STRING("xtree");
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor = Vector(255,0,0);
	
	DROP_TO_FLOOR(edict());
	SetThink(&CXtree::LightThink);
	pev->nextthink = gpGlobals->time;
}

void CXtree::LightThink()
{
	Vector vecSrc = pev->origin;
	static int iColor = 0;
	if( iColor == 3 ) iColor = 0;
	int rgrgiColors[3][3] =
	{
			{ 255, 0, 0 },
			{ 0, 0, 255 },
			{ 0, 255, 0 }
	};
	
	pev->rendercolor = Vector(rgrgiColors[iColor][0],rgrgiColors[iColor][1],rgrgiColors[iColor][2]);
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( vecSrc.x );        // X
			WRITE_COORD( vecSrc.y );        // Y
			WRITE_COORD( vecSrc.z );        // Z
			WRITE_BYTE( 12 );               // radius * 0.1
			WRITE_BYTE( rgrgiColors[iColor][0] );           // r
			WRITE_BYTE( rgrgiColors[iColor][1] );           // g
			WRITE_BYTE( rgrgiColors[iColor][2] );           // b
			WRITE_BYTE( 150 );              // time * 10
			WRITE_BYTE( 1 );                // decay * 0.1
	MESSAGE_END();
	iColor++;
	
	pev->nextthink = gpGlobals->time + 1;
}
