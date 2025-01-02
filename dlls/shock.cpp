
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
#include	"shock.h"

//=========================================================
// shockparticles - effects on player hit
//=========================================================

class CSparkEffects:public CPointEntity
{
	void Spawn( void );
	void Think( void );
private:
	int iFrameCounter;
	Vector BeamColor;
};
LINK_ENTITY_TO_CLASS( shockparticles, CSparkEffects )

void CSparkEffects::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	if(pev->dmg == 0) // Default amount
		pev->dmg = 100;

	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 64) );
	pev->nextthink = gpGlobals->time + 0.2;
}

void CSparkEffects::Think( void )
{
	if(iFrameCounter > pev->dmg)
	{
		SetThink( NULL );
		UTIL_Remove(this);
	}

	if( pev->owner )
	{
		pev->aiment = pev->owner;
		pev->movetype = MOVETYPE_FOLLOW; 
	}
	else
	{
		UTIL_Remove(this);
	}

	switch(RANDOM_LONG(0,2))
	{
	case 0:
		BeamColor = Vector(0, 198, 252);
		break;
	case 1:
		BeamColor = Vector(74, 126, 247);
		break;
	case 2:
		BeamColor = Vector(173, 255, 251);
		break;
	}

	Vector rand1 = Vector( RANDOM_FLOAT(-this->pev->size.x, this->pev->size.x), RANDOM_FLOAT(-this->pev->size.y, this->pev->size.y), RANDOM_FLOAT(-this->pev->size.z, this->pev->size.z) );
	Vector rand2 = Vector( RANDOM_FLOAT(-this->pev->size.x, this->pev->size.x), RANDOM_FLOAT(-this->pev->size.y, this->pev->size.y), RANDOM_FLOAT(-this->pev->size.z, this->pev->size.z) );

        MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_BEAMPOINTS );
                WRITE_COORD( pev->origin.x + rand1.x / 2 );
                WRITE_COORD( pev->origin.y + rand1.y / 2 );
                WRITE_COORD( pev->origin.z + rand1.z );
                WRITE_COORD( pev->origin.x + rand2.x / 2 );
                WRITE_COORD( pev->origin.y + rand2.y / 2 );
                WRITE_COORD( pev->origin.z + rand2.z );
                WRITE_SHORT( g_sModelIndexLaser ); // model
                WRITE_BYTE( 0 ); // framestart?
                WRITE_BYTE( 0 ); // framerate?
                WRITE_BYTE( 5 ); // life
                WRITE_BYTE( 5 ); // width
                WRITE_BYTE( 20 ); // noise
                WRITE_BYTE( BeamColor.x ); // r, g, b
                WRITE_BYTE( BeamColor.y ); // r, g, b
                WRITE_BYTE( BeamColor.z ); // r, g, b
                WRITE_BYTE( 200 ); // brightness
                WRITE_BYTE( 0 ); // speed?
        MESSAGE_END();

	UTIL_Sparks( pev->origin + Vector( RANDOM_FLOAT(-this->pev->size.x, this->pev->size.x), RANDOM_FLOAT(-this->pev->size.y, this->pev->size.y), RANDOM_FLOAT(-this->pev->size.z, this->pev->size.z)) );

//	printf("%d\n",iFrameCounter);
	pev->nextthink = gpGlobals->time + 0.001;
	iFrameCounter++;
}


const char *sparktrailtargets[] = 
{
	"player",
	"monster_tripmine",
	"monster_snark",
	"monster_satchel",
	"monster_bloater",
	"monster_snark",
	"monster_rat",
	"monster_babycrab",
	"monster_cockroach",
	"monster_flyer_flock",
	"monster_headcrab",
	"monster_leech",
	"prop",
	"monster_alien_controller",
	"monster_alien_slave",
	"monster_barney",
	"monster_bullchicken",
	"monster_houndeye",
	"monster_human_assassin",
	"monster_human_grunt",
	"monster_scientist",
	"monster_zombie",
	"monster_alien_grunt",
	"monster_bigmomma",
	"monster_gargantua",
	"monster_ichthyosaur",
	"ammo_spore"
};

//=========================================================
// lightning1 - First stage of lightning chain - Chain
//=========================================================
class CSparkTrail1:public CPointEntity
{
	void Spawn( void );
	void Think( void );
private:
	Vector BeamColor;
};

void CSparkTrail1::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.2;// let targets spawn!
}

void CSparkTrail1::Think( void )
{
	if( pev->owner )
	{
		CBaseEntity *searchtargets = NULL;
		while( ( searchtargets = UTIL_FindEntityInSphere( searchtargets, pev->origin, 200 ) ) != NULL )
		{
			switch(RANDOM_LONG(0,2))
			{
			case 0:
				BeamColor = Vector(0, 198, 252);
				break;
			case 1:
				BeamColor = Vector(74, 126, 247);
				break;
			case 2:
				BeamColor = Vector(173, 255, 251);
				break;
			}
	
			for( size_t uiIndex = 0; uiIndex < ARRAYSIZE( sparktrailtargets ); ++uiIndex )
			{
				if( strcmp( STRING(searchtargets->pev->classname), sparktrailtargets[ uiIndex ] ) == 0 && this->pev != searchtargets->pev )
				{
				        MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				                WRITE_BYTE( TE_BEAMPOINTS );
				                WRITE_COORD( pev->origin.x );
				                WRITE_COORD( pev->origin.y );
				                WRITE_COORD( pev->origin.z );
				                WRITE_COORD( searchtargets->pev->origin.x );
				                WRITE_COORD( searchtargets->pev->origin.y );
				                WRITE_COORD( searchtargets->pev->origin.z );
				                WRITE_SHORT( g_sModelIndexLaser ); // model
				                WRITE_BYTE( 0 ); // framestart?
				                WRITE_BYTE( 10 ); // framerate?
				                WRITE_BYTE( 15 ); // life
				                WRITE_BYTE( 20 ); // width
				                WRITE_BYTE( 100 ); // noise
				                WRITE_BYTE( BeamColor.x ); // r, g, b
				                WRITE_BYTE( BeamColor.y ); // r, g, b
				                WRITE_BYTE( BeamColor.z ); // r, g, b
				                WRITE_BYTE( 200 ); // brightness
				                WRITE_BYTE( 0 ); // speed?
	        			MESSAGE_END();

					if( strcmp( STRING(searchtargets->pev->classname), "monster_satchel" ) != 0 ) // Dont spark around satchels
					{
						CSparkEffects *pSparks = (CSparkEffects *)Create( "shockparticles", searchtargets->pev->origin, Vector(0,0,0), edict() );
						pSparks->pev->owner = searchtargets->edict();
						pSparks->pev->dmg = RANDOM_LONG( 30, 80 );
					}
					else
					{
						searchtargets->Use( this, this, USE_ON, 0 );
					}


					CBaseEntity *pLight2 = CBaseEntity::Create( "lightning2", searchtargets->pev->origin, Vector(0,0,0), NULL );
					pLight2->pev->owner = this->pev->owner;

					//printf("%s\n", STRING(searchtargets->pev->classname));
					break;
				}
			}
		}
	}
	else
	{
		UTIL_Remove(this);
	}
}

LINK_ENTITY_TO_CLASS( lightning1, CSparkTrail1 )

//=========================================================
// lightning2 - Second stage of lightning chain - Damage
//=========================================================
class CSparkTrail2:public CPointEntity
{
	void Spawn( void );
	void Think( void );
};
LINK_ENTITY_TO_CLASS( lightning2, CSparkTrail2 )

void CSparkTrail2::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.2;// let targets spawn!
}

void CSparkTrail2::Think( void )
{
	if( pev->owner )
	{
		::RadiusDamage( pev->origin, pev, VARS( pev->owner ), 15, 32, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
	}
	else
	{
		UTIL_Remove(this);
	}
}

LINK_ENTITY_TO_CLASS(shock_beam, CShock)

TYPEDESCRIPTION	CShock::m_SaveData[] =
{
	DEFINE_FIELD(CShock, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pNoise, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pSprite, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CShock, CBaseAnimating)

void CShock::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("shock_beam");
	SET_MODEL(ENT(pev), "models/shock_effect.mdl");
	UTIL_SetOrigin(pev, pev->origin);

	pev->dmg = 10;
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	if( pev->flags & FL_IMMUNE_LAVA )
		pev->dmg = 35;

	CreateEffects();
	SetThink( &CShock::FlyThink );
	pev->nextthink = gpGlobals->time;
}

void CShock::Precache()
{
	PRECACHE_MODEL("sprites/flare3.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("models/shock_effect.mdl");
	PRECACHE_SOUND("weapons/shock_impact.wav");
	m_iTrail = PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CShock::FlyThink()
{
	if (pev->waterlevel == 3)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", VOL_NORM, ATTN_NORM);
		RadiusDamage(pev->origin, pev, pevOwner ? pevOwner : pev, 100, 150, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
		ClearEffects();
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.01;
	}
	else
		pev->nextthink = gpGlobals->time + 0.05;
}

void CShock::Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity)
{
	CShock *pShock = GetClassPtr((CShock *)NULL);
	pShock->Spawn();

	UTIL_SetOrigin(pShock->pev, vecStart);
	pShock->pev->velocity = vecVelocity;
	pShock->pev->owner = ENT(pevOwner);
	pShock->pev->angles = angles;

	pShock->pev->nextthink = gpGlobals->time;
}

void CShock::Touch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner)
		return;

	TraceResult tr = UTIL_GetGlobalTrace( );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 8 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );

	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", VOL_NORM, ATTN_NORM);

	ClearEffects();
	
	if( pOther->IsPlayer() )
		UTIL_ScreenFade( pOther, Vector( 0, 200, 200 ), 0.3, 0.3, 150, FFADE_IN ); // 255
	
	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0, 2));

		int iContents = UTIL_PointContents(pev->origin);

		// Create sparks
		if (iContents != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}
	}
	else
	{
		ClearMultiDamage();
		entvars_t *pevOwner = VARS(pev->owner);

		CBaseMonster* pMonster = pOther->MyMonsterPointer();
		if (pMonster)
			pMonster->GlowShellOn( Vector( 0, 220, 255 ), .5f );

		pOther->TraceAttack(pev, pev->dmg, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
		ApplyMultiDamage(pev, pevOwner ? pevOwner : pev);
		if (pOther->IsPlayer() && (UTIL_PointContents(pev->origin) != CONTENTS_WATER))
		{
			const Vector position = tr.vecEndPos;
			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOther->pev );
				WRITE_BYTE( TE_SPARKS );
				WRITE_COORD( position.x );
				WRITE_COORD( position.y );
				WRITE_COORD( position.z );
			MESSAGE_END();
		}
		if(pev->flags & FL_IMMUNE_LAVA) // Charged shot
		{
			for( int i = 0; i < RANDOM_LONG( 1, 4 ); i++ )
				CBaseEntity::Create( "spark_shower", pev->origin, Vector(0,0,0), NULL );
			CSparkEffects *pSparks = (CSparkEffects *)Create( "shockparticles", pev->origin, Vector(0,0,0), edict() );
			pSparks->pev->owner = pOther->edict();
			pSparks->pev->dmg = RANDOM_LONG( 30, 80 );
			CBaseEntity *pLight = CBaseEntity::Create( "lightning1", pev->origin, Vector(0,0,0), NULL );
			pLight->pev->owner = pOther->edict();
		}
	}
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CShock::CreateEffects()
{
	m_pSprite = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 1 );
	m_pSprite->pev->scale = 0.35;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 70, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;

	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pBeam)
	{
		m_pBeam->EntsInit( entindex(), entindex() );
		m_pBeam->SetStartAttachment( 1 );
		m_pBeam->SetEndAttachment( 2 );
		m_pBeam->SetBrightness( 190 );
		m_pBeam->SetScrollRate( 20 );
		m_pBeam->SetNoise( 20 );
		m_pBeam->SetFlags( BEAM_FSHADEOUT );
		m_pBeam->SetColor( 0, 255, 255 );
		m_pBeam->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pBeam->RelinkBeam();
	}

	m_pNoise = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pNoise)
	{
		m_pNoise->EntsInit( entindex(), entindex() );
		m_pNoise->SetStartAttachment( 1 );
		m_pNoise->SetEndAttachment( 2 );
		m_pNoise->SetBrightness( 190 );
		m_pNoise->SetScrollRate( 20 );
		m_pNoise->SetNoise( 65 );
		m_pNoise->SetFlags( BEAM_FSHADEOUT );
		m_pNoise->SetColor( 255, 255, 173 );
		m_pNoise->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pNoise->RelinkBeam();
	}

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( (entindex() & 0x0FFF) | (( 2 & 0xF) << 12 ) );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 2 );  // width
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
}

void CShock::ClearEffects()
{
	if( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if( m_pNoise )
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}
	if( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
}