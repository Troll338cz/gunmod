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
#include "santahat.h"

extern int gmsgSayText;

LINK_ENTITY_TO_CLASS(santa_hat, CSantaHat);

void CSantaHat::Precache( void )
{
	PRECACHE_MODEL("models/santa_hat.mdl");
}

void CSantaHat::Spawn(void)
{
	Precache();
	SET_MODEL( ENT( pev ), "models/santa_hat.mdl" );
}

CSantaHat *CSantaHat::CreateSantaHat(CBaseEntity *aiment)
{
	CSantaHat *pHat = GetClassPtr( (CSantaHat *)NULL );
	pHat->Spawn();
	pHat->pev->aiment = aiment->edict();
	pHat->pev->movetype = MOVETYPE_FOLLOW;
	return pHat;
}
