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
#include "player.h"
#include "effects.h"
#include "weapons.h"
#include "items.h"

extern int gmsgItemPickup;

class CItemArmorVest : public CItem
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/barney_vest.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/barney_vest.mdl" );
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( ( pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY ) &&
			( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		{
			pPlayer->pev->armorvalue += 60;
			pPlayer->pev->armorvalue = Q_min( pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY );

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_armorvest, CItemArmorVest )

class CItemHelmet : public CItem
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/barney_helmet.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/barney_helmet.mdl" );
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( ( pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY ) &&
			( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		{
			pPlayer->pev->armorvalue += 40;
			pPlayer->pev->armorvalue = Q_min( pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY );

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_helmet, CItemHelmet )