/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to 
//				   have one without a hardcoded player.mdl in tf_client.cpp
/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
#include "pm_shared.h"
#include "admin.h" // Admin cmds

extern DLL_GLOBAL ULONG		g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern DLL_GLOBAL int		g_iSkillLevel;
extern DLL_GLOBAL ULONG		g_ulFrameCount;

extern void CopyToBodyQue(entvars_t* pev);
extern BOOL IsPlayerConnected( CBasePlayer * plr);
extern int giPrecacheGrunt;
extern int gmsgSayText;
extern int gmsgBhopcap;
extern int gmsgScoreInfo;

extern cvar_t allow_spectators;

extern int g_teamplay;

extern cvar_t bhopcap;
extern "C" int g_bhopcap;

void LinkUserMessages( void );
void UTIL_BecomeSpectator( CBasePlayer *pPlayer );
/*
 * used by kill command and disconnect command
 * ROBIN: Moved here from player.cpp, to allow multiple player models
 */
void set_suicide_frame(entvars_t* pev)
{
	if (!FStrEq(STRING(pev->model), "models/player.mdl"))
		return; // allready gibbed

//	pev->frame		= $deatha11;
	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_TOSS;
	pev->deadflag	= DEAD_DEAD;
	pev->nextthink	= -1;
}


/*
===========
ClientConnect

called when a player connects to a server
============
*/
BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{
	CBasePlayer *pl = (CBasePlayer *)CBaseEntity::Instance( pEntity ) ;

	if( mp_coop.value && pEntity )
	{
		if( pl && pl->m_ggm.iState != STATE_LOAD_FIX )
		{
			pl->m_ggm.iState = STATE_UNINITIALIZED;
			pl->RemoveAllItems( TRUE );
			UTIL_BecomeSpectator( pl );
		}
	}

	if( pl ) pl->m_gm.score = NULL;

	return g_pGameRules->ClientConnected( pEntity, pszName, pszAddress, szRejectReason );

// a client connecting during an intermission can cause problems
//	if (intermission_running)
//		ExitIntermission ();

}


/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
void ClientDisconnect( edict_t *pEntity )
{

	CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pEntity );

	if( pPlayer && pPlayer->IsPlayer() )
	{
		GGM_SaveState( pPlayer );
		pPlayer->m_ggm.iState = STATE_UNINITIALIZED;
}

	if (g_fGameOver)
		return;

	char text[256] = "";

	if( pEntity->v.netname )
		_snprintf( text, sizeof(text), "- %s has left the game\n", STRING( pEntity->v.netname ) );

	MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();

	CSound *pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEntity ) );

	// since this client isn't around to think anymore, reset their sound. 
	if( pSound )
	{

		pSound->Reset();
	}

// since the edict doesn't get deleted, fix it so it doesn't interfere.
	pEntity->v.takedamage = DAMAGE_NO;// don't attract autoaim
	pEntity->v.solid = SOLID_NOT;// nonsolid
	pEntity->v.effects = 0;// clear any effects
	UTIL_SetOrigin( &pEntity->v, pEntity->v.origin );

	g_pGameRules->ClientDisconnected( pEntity );

}


// called by ClientKill and DeadThink
void respawn(entvars_t* pev, BOOL fCopyCorpse)
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			CopyToBodyQue(pev);
		}

		// respawn player
		GetClassPtr( (CBasePlayer *)pev)->Spawn( );
	}
	else
	{       // restart the entire server
		SERVER_COMMAND("reload\n");
	}
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;

	if( pev->flags & FL_SPECTATOR )
	{
		pev->health = 1;
		return;
	}

	CBasePlayer *pl = (CBasePlayer*) CBasePlayer::Instance( pev );

	if( !pl )
		return;

	if ( pl->m_fNextSuicideTime > gpGlobals->time )
		return;  // prevent suiciding too ofter

	pl->m_fNextSuicideTime = gpGlobals->time + 1;  // don't let them suicide for 5 seconds after suiciding

	// have the player kill themself
	pev->health = 0;
	pl->Killed( pev, GIB_NEVER );

//	pev->modelindex = g_ulModelIndexPlayer;
//	pev->frags -= 2;		// extra penalty
//	respawn( pev );
}

void KickCheater( CBasePlayer *player, const char *CheatType )
{
	FILE *flch;
	flch = fopen("Cheaters.txt", "a");
	fprintf( flch , "name: %s id: %s %s\n", GGM_PlayerName(player), GETPLAYERAUTHID(player->edict()), CheatType);
	GM_KickPlayer( player, "Skill issue" );
	fclose( flch );
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer( edict_t *pEntity )
{
	CBasePlayer *pPlayer;

	entvars_t *pev = &pEntity->v;

	pPlayer = GetClassPtr((CBasePlayer *)pev);
	pPlayer->SetCustomDecalFrames(-1); // Assume none;

	GGM_ClientPutinServer( pEntity, pPlayer );

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn();

	GGM_ClientFirstSpawn( pPlayer );

	pPlayer->m_flCheckCvars = gpGlobals->time + 10;
	g_engfuncs.pfnQueryClientCvarValue2( pEntity, "host_ver", 116 );

	// Reset interpolation during first frame
	pPlayer->pev->effects |= EF_NOINTERP;

	pPlayer->pev->iuser1 = 0;
	pPlayer->pev->iuser2 = 0;

	GM_Login(pEntity);
}

#ifndef NO_VOICEGAMEMGR
#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;
#endif

//-----------------------------------------------------------------------------
// Purpose: determine if a uchar32 represents a valid Unicode code point
//-----------------------------------------------------------------------------
bool Q_IsValidUChar32( unsigned int uVal )
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return ( ( uVal - 0x0u ) < 0x110000u ) && ( (uVal - 0x00D800u) > 0x7FFu ) && ( (uVal & 0xFFFFu) < 0xFFFEu ) && ( ( uVal - 0x00FDD0u ) > 0x1Fu );
}

// Decode one character from a UTF-8 encoded string. Treats 6-byte CESU-8 sequences
// as a single character, as if they were a correctly-encoded 4-byte UTF-8 sequence.
int Q_UTF8ToUChar32( const char *pUTF8_, unsigned int &uValueOut, bool &bErrorOut )
{
	const unsigned char *pUTF8 = (const unsigned char*)pUTF8_;

	int nBytes = 1;
	unsigned int uValue = pUTF8[0];
	unsigned int uMinValue = 0;

	// 0....... single byte
	if( uValue < 0x80 )
		goto decodeFinishedNoCheck;

	// Expecting at least a two-byte sequence with 0xC0 <= first <= 0xF7 (110...... and 11110...)
	if( ( uValue - 0xC0u ) > 0x37u || ( pUTF8[1] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = ( uValue << 6 ) - ( 0xC0 << 6 ) + pUTF8[1] - 0x80;
	nBytes = 2;
	uMinValue = 0x80;

	// 110..... two-byte lead byte
	if( !( uValue & ( 0x20 << 6 ) ) )
		goto decodeFinished;

	// Expecting at least a three-byte sequence
	if( ( pUTF8[2] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = ( uValue << 6 ) - ( 0x20 << 12 ) + pUTF8[2] - 0x80;
	nBytes = 3;
	uMinValue = 0x800;

	// 1110.... three-byte lead byte
decodeFinished:
	if( uValue >= uMinValue && Q_IsValidUChar32( uValue ) )
	{
decodeFinishedNoCheck:
		uValueOut = uValue;
		bErrorOut = false;
		return nBytes;
	}
decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;

decodeFinishedMaybeCESU8:
	// Do we have a full UTF-16 surrogate pair that's been UTF-8 encoded afterwards?
	// That is, do we have 0xD800-0xDBFF followed by 0xDC00-0xDFFF? If so, decode it all.
	if( ( uValue - 0xD800u ) < 0x400u && pUTF8[3] == 0xED && (unsigned char)( pUTF8[4] - 0xB0 ) < 0x10 && ( pUTF8[5] & 0xC0 ) == 0x80 )
	{
		uValue = 0x10000 + ( ( uValue - 0xD800u ) << 10 ) + ( (unsigned char)( pUTF8[4] - 0xB0 ) << 6 ) + pUTF8[5] - 0x80;
		nBytes = 6;
		uMinValue = 0x10000;
	}
	goto decodeFinished;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-8 string contains invalid sequences.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate( const char *pUTF8 )
{
	bool bError = false;
	while( *pUTF8 )
	{
		unsigned int uVal;
		// Our UTF-8 decoder silently fixes up 6-byte CESU-8 (improperly re-encoded UTF-16) sequences.
		// However, these are technically not valid UTF-8. So if we eat 6 bytes at once, it's an error.
		int nCharSize = Q_UTF8ToUChar32( pUTF8, uVal, bError );
		if( bError || nCharSize == 6 )
			return false;
		pUTF8 += nCharSize;
	}
	return true;
}


//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say( edict_t *pEntity, int teamonly )
{
	CBasePlayer *client;
	int		j;
	char	*p, *pc;
	char	text[128];
	char    szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = CMD_ARGV(0);
	BOOL adminsonly = FALSE;

	// We can get a raw string now, without the "say " prepended
	if ( CMD_ARGC() == 0 )
		return;

	entvars_t *pev = &pEntity->v;
	CBasePlayer* player = GetClassPtr((CBasePlayer *)pev);

	//Not yet.
	if ( player->m_flNextChatTime > gpGlobals->time )
		 return;

	if( player && GM_IsPlayerFucked( player ) )
		return;

	if ( !stricmp( pcmd, cpSay) || !stricmp( pcmd, cpSayTeam ) )
	{
		if ( CMD_ARGC() >= 2 )
		{
			p = (char *)CMD_ARGS();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if ( CMD_ARGC() >= 2 )
		{
			sprintf( szTemp, "%s %s", ( char * )pcmd, (char *)CMD_ARGS() );
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf( szTemp, "%s", ( char * )pcmd );
		}
		p = szTemp;
	}

	// remove quotes if present
	if( p && *p == '"' )
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	if( !p || !p[0] || !Q_UnicodeValidate ( p ) )
		return;  // no character found, so say nothing

	// turn on color set 2  (color on,  no sound)
	if( player->IsObserver() && ( teamonly ) )
		sprintf( text, "%c(SPEC) %s: ", 2, STRING( pEntity->v.netname ) );
	else if( teamonly )
	{
		if( player->m_gm.IsAdmin() && !g_pGameRules->IsTeamplay() )
		{
			adminsonly = TRUE;
			sprintf( text, "%c(ADMIN) %s: ", 2, STRING( pEntity->v.netname ) );
		}
		else
			sprintf( text, "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
	}
	else
		sprintf( text, "%c%s: ", 2, STRING( pEntity->v.netname ) );

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ( (int)strlen(p) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );

	// Troll: No delay for admins
	if( !player->m_gm.IsAdmin() )
		player->m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
	{
		if ( !client->pev )
			continue;
		
		if ( client->edict() == pEntity )
			continue;

		if ( !(client->IsNetClient()) )	// Not a client ? (should never be true)
			continue;

		// can the receiver hear the sender? or has he muted him?
#ifndef NO_VOICEGAMEMGR
		if ( g_VoiceGameMgr.PlayerHasBlockedPlayer( client, player ) )
			continue;
#endif

		if( g_pGameRules->IsTeamplay() )
		{
			if(  !player->IsObserver() && teamonly && g_pGameRules->PlayerRelationship( client, CBaseEntity::Instance( pEntity ) ) != GR_TEAMMATE )
				continue;
		}
		else if( adminsonly )
		{
			if( !player->m_gm.IsAdmin())
				continue;
		}
		else if( teamonly )
			continue;


		// Spectators can only talk to other specs
		if( player->IsObserver() && teamonly )
			if ( !client->IsObserver() )
				continue;

		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, client->pev );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

	}

	// print to the sending client
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, &pEntity->v );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();
	// echo to server console
	g_engfuncs.pfnServerPrint( text );

	if( adminsonly )
		return;

	GM_ChatLog( pEntity, text );

	const char *temp;
	if( teamonly )
		temp = "say_team";
	else
		temp = "say";

	// team match?
	if ( g_teamplay )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"\n", 
			STRING( pEntity->v.netname ), 
			GETPLAYERUSERID( pEntity ),
			GETPLAYERAUTHID( pEntity ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model" ),
			temp,
			p );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%i>\" %s \"%s\"\n", 
			STRING( pEntity->v.netname ), 
			GETPLAYERUSERID( pEntity ),
			GETPLAYERAUTHID( pEntity ),
			GETPLAYERUSERID( pEntity ),
			temp,
			p );
	}
}

/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern float g_flWeaponCheat;

// Use CMD_ARGV,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
void ClientCommand( edict_t *pEntity )
{
	const char *pcmd = CMD_ARGV(0);
	const char *pstr;

	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);
	CBasePlayer *client;

	GM_LogCmd(pEntity, pcmd, CMD_ARGC() > 1 ? CMD_ARGS() : "" );

	if ( FStrEq(pcmd, "say" ) )
	{
		Host_Say( pEntity, 0 );
		GGM_Say(pEntity);
	}
	else if ( FStrEq(pcmd, "say_team" ) )
	{
		Host_Say( pEntity, 1 );
	}
	else if ( FStrEq(pcmd, "fullupdate" ) )
	{
		GetClassPtr((CBasePlayer *)pev)->ForceClientDllUpdate(); 
	}
	else if ( FStrEq(pcmd, "give" ) )
	{
		if ( CMD_ARGC() < 2 )
		{
			return;
		}

		if ( g_flWeaponCheat != 0.0 || pPlayer->m_gm.IsAdmin()  )
		{
			const char* iszItem = CMD_ARGV(1);
			if(!GM_IsAllowedClassname(iszItem))
				return;
			GetClassPtr((CBasePlayer *)pev)->GiveNamedItem( iszItem );
		}
	}
	else if ( FStrEq(pcmd, "fire") )
	{
		if ( g_flWeaponCheat != 0.0 || pPlayer->m_gm.IsAdmin() )
		{
			if (CMD_ARGC() > 1)
			{
				FireTargets(CMD_ARGV(1), pPlayer, pPlayer, USE_TOGGLE, 0);
			}
			else
			{
				TraceResult tr;
				UTIL_MakeVectors(pev->v_angle);
				UTIL_TraceLine(
					pev->origin + pev->view_ofs,
					pev->origin + pev->view_ofs + gpGlobals->v_forward * 1000,
					dont_ignore_monsters, pEntity, &tr
				);

				if (tr.pHit)
				{
					CBaseEntity *pHitEnt = CBaseEntity::Instance(tr.pHit);
					if (pHitEnt)
					{
						pHitEnt->Use(pPlayer, pPlayer, USE_TOGGLE, 0);
						ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Fired %s \"%s\"\n", STRING(pHitEnt->pev->classname), STRING(pHitEnt->pev->targetname) ) );
					}
				}
			}
		}
	}
	else if ( FStrEq(pcmd, "drop" ) )
	{
		// player is dropping an item. 
		if((gpGlobals->time >= GetClassPtr( (CBasePlayer *)pev )->m_flLastCommandTime[1]))
		{
			GetClassPtr( (CBasePlayer *)pev )->DropPlayerItem( (char *)CMD_ARGV( 1 ) );
			GetClassPtr( (CBasePlayer *)pev )->m_flLastCommandTime[1] = gpGlobals->time + 1.0f;
		}
	}
	else if ( FStrEq(pcmd, "fov" ) )
	{
		if ( (g_flWeaponCheat || pPlayer->m_gm.IsAdmin()) && CMD_ARGC() > 1)
		{
			pPlayer->m_iFOV = atoi( CMD_ARGV(1) );
		}
		else
		{
			CLIENT_PRINTF( pEntity, print_console, UTIL_VarArgs( "\"fov\" is \"%d\"\n", (int)pPlayer->m_iFOV ) );
		}
	}
	else if ( FStrEq(pcmd, "use" ) )
	{
		pPlayer->SelectItem((char *)CMD_ARGV(1));
	}
	else if (((pstr = strstr(pcmd, "weapon_")) != NULL)  && (pstr == pcmd))
	{
		pPlayer->SelectItem(pcmd);
	}
	else if (FStrEq(pcmd, "lastinv" ))
	{
		pPlayer->SelectLastItem();
	}
	else if( FStrEq( pcmd, "specmode" ) ) // new spectator mode
	{
		CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pev );

		if( pPlayer->IsObserver() )
			pPlayer->Observer_SetMode( atoi( CMD_ARGV( 1 ) ) );
	}
	else if( FStrEq( pcmd, "spectate" ) ) // clients wants to become a spectator
	{
		CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pev );
		if( mp_coop.value )
		{
			pPlayer->RemoveAllItems(TRUE);
			UTIL_BecomeSpectator(pPlayer);
			pPlayer->m_ggm.iState = STATE_SPECTATOR;
		}
		else
		{
			if( !pPlayer->IsObserver() )
			{
				// always allow admins to become a spectator
				if( ( allow_spectators.value || pPlayer->m_gm.IsAdmin() ) && (gpGlobals->time >= pPlayer->m_flLastCommandTime[0]))
				{
					edict_t *pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot( pPlayer );
					pPlayer->StartObserver( pev->origin, VARS( pentSpawnSpot )->angles );
					pPlayer->m_bSentMsg = FALSE;
					// notify other clients of player switching to spectator mode
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s switched to spectator mode\n",
							( pev->netname && ( STRING( pev->netname ) )[0] != 0 ) ? STRING( pev->netname ) : "unconnected" ) );
					pPlayer->m_flLastCommandTime[0] = gpGlobals->time + 10.0f;
				}
				else
					ClientPrint( pev, HUD_PRINTCONSOLE, "Spectator mode is disabled. (Or you are spamming it)\n" );
			}
			else
			{
				pPlayer->StopObserver();
				pPlayer->m_bSentMsg = FALSE;
				// notify other clients of player left spectators
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has left spectator mode\n",
						( pev->netname && ( STRING( pev->netname ) )[0] != 0 ) ? STRING( pev->netname ) : "unconnected" ) );
			}
		}
	}
	else if( FStrEq( pcmd, "closemenus" ) )
	{
		// just ignore it
	}
	else if( FStrEq( pcmd, "follownext" ) )	// follow next player
	{
		CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pev );

		if( pPlayer->IsObserver() )
			pPlayer->Observer_FindNextPlayer( atoi( CMD_ARGV( 1 ) ) ? true : false );
	}
	else if( g_pGameRules->ClientCommand( GetClassPtr( (CBasePlayer *)pev ), pcmd ) )
	{
		// MenuSelect returns true only if the command is properly handled,  so don't print a warning
	}
	else if( FStrEq( pcmd, "VModEnable" ) )
	{
		// clear 'Unknown command: VModEnable' in singleplayer
		return;
	}
	else if( FStrEq(pcmd, "+bhop"))
	{
		pPlayer->m_bBhop = TRUE;
	}
	else if( FStrEq(pcmd, "-bhop"))
	{
		pPlayer->m_bBhop = FALSE;
	}
	else if( !GGM_ClientCommand( GetClassPtr( (CBasePlayer *)pev ), pcmd ) && !GM_ClientCommand( GetClassPtr( (CBasePlayer *)pev ), pcmd ) && ADMIN_ClientCommand( GetClassPtr( (CBasePlayer *)pev ), pcmd ) )
	{
		// tell the user they entered an unknown command
		char command[128];

		// check the length of the command (prevents crash)
		// max total length is 192 ...and we're adding a string below ("Unknown command: %s\n")
		strncpy( command, pcmd, 127 );
		command[127] = '\0';

		// tell the user they entered an unknown command
		ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Unknown command: %s\n", command ) );
	}
}


/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	const char *name =  g_engfuncs.pfnInfoKeyValue( infobuffer, "name" );

	// prevent keeping other's uid on saverestore
	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)&pEntity->v);
	const char *uid = GGM_GetAuthID( pPlayer );
	if( !pPlayer->m_ggm.pState || !pPlayer->m_ggm.pState->fRegistered || pPlayer->m_ggm.iState != STATE_SPAWNED )
	{
		GGMPlayerState *pState = GGM_GetState(uid, name);

		if( pState != pPlayer->m_ggm.pState )
		{

			if( pPlayer->m_ggm.iState == STATE_LOAD_FIX )
			{
				pEntity->v.netname = pEntity->v.frags = 0;
				return;
			}
			GGM_SaveState( pPlayer );
			pEntity->v.netname = pEntity->v.frags = 0;
			pPlayer->m_ggm.pState = pState;
			pPlayer->m_ggm.iState = STATE_UNINITIALIZED;
			GGM_RestoreState( pPlayer );
		}
	}

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if( pEntity->v.netname && ( STRING( pEntity->v.netname ) )[0] != 0 && !FStrEq( STRING( pEntity->v.netname ), name ) )
	{
//		if( pPlayer && pPlayer->IsPlayer() && GM_IsPlayerFucked( pPlayer ) )
//			return;

		char sName[256];
		char *pName = g_engfuncs.pfnInfoKeyValue( infobuffer, "name" );
		strncpy( sName, pName, sizeof(sName) - 1 );
		sName[ sizeof(sName) - 1 ] = '\0';

		// First parse the name and remove any %'s
		for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
		{
			// Replace it with a space
			if ( *pApersand == '%' )
				*pApersand = ' ';
		}

		// Set the name
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX(pEntity), infobuffer, "name", sName );

		// prevent phantom nickname changed messages
//		if( !IsPlayerConnected( pPlayer ) )
//			return;

		if( gpGlobals->maxClients > 1 )
		{
			char text[256];
			_snprintf( text, 256, "* %s changed name to %s\n", STRING( pEntity->v.netname ), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
			MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
				WRITE_BYTE( ENTINDEX( pEntity ) );
				WRITE_STRING( text );
			MESSAGE_END();
		}

		// team match?
		if ( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed name to \"%s\"\n", 
				STRING( pEntity->v.netname ), 
				GETPLAYERUSERID( pEntity ), 
				GETPLAYERAUTHID( pEntity ),
				g_engfuncs.pfnInfoKeyValue( infobuffer, "model" ), 
				g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" changed name to \"%s\"\n", 
				STRING( pEntity->v.netname ), 
				GETPLAYERUSERID( pEntity ), 
				GETPLAYERAUTHID( pEntity ),
				GETPLAYERUSERID( pEntity ), 
				g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		}
	}

	g_pGameRules->ClientUserInfoChanged( GetClassPtr((CBasePlayer *)&pEntity->v), infobuffer );
}

static int g_serveractive = 0;

void ServerDeactivate( void )
{
//	ALERT( at_console, "ServerDeactivate()\n" );

	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate 
	if ( g_serveractive != 1 )
	{
		return;
	}

	g_serveractive = 0;

	// Peform any shutdown operations here...
	//
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	int				i;
	CBaseEntity		*pClass;

//	ALERT( at_console, "ServerActivate()\n" );

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

	// Clients have not been initialized yet
	for ( i = 0; i < edictCount; i++ )
	{
		if ( pEdictList[i].free )
			continue;
		
		// Clients aren't necessarily initialized until ClientPutInServer()
		if ( i < clientMax || !pEdictList[i].pvPrivateData )
			continue;

		pClass = CBaseEntity::Instance( &pEdictList[i] );
		// Activate this entity if it's got a class & isn't dormant
		if ( pClass && !(pClass->pev->flags & FL_DORMANT) )
		{
			pClass->Activate();
		}
		else
		{
			ALERT( at_console, "Can't instance %s\n", STRING(pEdictList[i].v.classname) );
		}
	}

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();
	GGM_ServerActivate();
}


/*
================
PlayerPreThink

Called every frame before physics are run
================
*/

void PlayerPreThink( edict_t *pEntity )
{
//	ALERT( at_console, "PreThink( %g, frametime %g )\n", gpGlobals->time, gpGlobals->frametime );

	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pEntity );


	if( pPlayer->m_bBhop && ( pPlayer->pev->flags & FL_ONGROUND ))
	{
		pPlayer->Jump();
		pPlayer->pev->velocity.z = sqrt( 2 * 800 * 45.0 );
	}
	if( pPlayer->m_flCheckCvars <= gpGlobals->time )
	{
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "r_drawentities", 112 );
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "gl_wh", 114 );
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "cl_wh", 115 );
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "host_build", 117 );
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "cscl_ver", 118 );
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "drm_weaponglow", 119 );
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "ogc", 120 );
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "bae", 121 );
		pPlayer->m_flCheckCvars = gpGlobals->time + 100;
	}

	if( !pPlayer )
		ClientPutInServer( pEntity );

	if (pPlayer)
		pPlayer->PreThink( );
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink( edict_t *pEntity )
{
//	ALERT( at_console, "PostThink( %g, frametime %g )\n", gpGlobals->time, gpGlobals->frametime );

	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pEntity );

	if (pPlayer)
	{
		pPlayer->PostThink( );
		GM_PlayerPostThink( pEntity );
	}
}

void ParmsNewLevel( void )
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if ( pSaveData )
	{
		pSaveData->connectionCount = BuildChangeList( pSaveData->levelList, MAX_LEVEL_CONNECTIONS );
	}

}



void ParmsChangeLevel( void )
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if ( pSaveData )
	{
		pSaveData->connectionCount = BuildChangeList( pSaveData->levelList, MAX_LEVEL_CONNECTIONS );
	}
}


//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
void StartFrame( void )
{
//	ALERT( at_console, "SV_Physics( %g, frametime %g )\n", gpGlobals->time, gpGlobals->frametime );

	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = teamplay.value;
	g_ulFrameCount++;

	int oldBhopcap = g_bhopcap;
	g_bhopcap = ( g_pGameRules->IsMultiplayer() && bhopcap.value != 0.0f ) ? 1 : 0;
	if( g_bhopcap != oldBhopcap )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgBhopcap, NULL );
			WRITE_BYTE( g_bhopcap );
		MESSAGE_END();
	}
}

void ClientPrecache( void )
{
	// setup precaches always needed
	PRECACHE_SOUND("player/sprayer.wav");			// spray paint sound for PreAlpha
	
	// PRECACHE_SOUND("player/pl_jumpland2.wav");		// UNDONE: play 2x step sound
	
	PRECACHE_SOUND("player/pl_fallpain2.wav");		
	PRECACHE_SOUND("player/pl_fallpain3.wav");		
	
	PRECACHE_SOUND("player/pl_step1.wav");		// walk on concrete
	PRECACHE_SOUND("player/pl_step2.wav");
	PRECACHE_SOUND("player/pl_step3.wav");
	PRECACHE_SOUND("player/pl_step4.wav");

	PRECACHE_SOUND("common/npc_step1.wav");		// NPC walk on concrete
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");

	PRECACHE_SOUND("player/pl_metal1.wav");		// walk on metal
	PRECACHE_SOUND("player/pl_metal2.wav");
	PRECACHE_SOUND("player/pl_metal3.wav");
	PRECACHE_SOUND("player/pl_metal4.wav");

	PRECACHE_SOUND("player/pl_dirt1.wav");		// walk on dirt
	PRECACHE_SOUND("player/pl_dirt2.wav");
	PRECACHE_SOUND("player/pl_dirt3.wav");
	PRECACHE_SOUND("player/pl_dirt4.wav");

	PRECACHE_SOUND("player/pl_duct1.wav");		// walk in duct
	PRECACHE_SOUND("player/pl_duct2.wav");
	PRECACHE_SOUND("player/pl_duct3.wav");
	PRECACHE_SOUND("player/pl_duct4.wav");

	PRECACHE_SOUND("player/pl_grate1.wav");		// walk on grate
	PRECACHE_SOUND("player/pl_grate2.wav");
	PRECACHE_SOUND("player/pl_grate3.wav");
	PRECACHE_SOUND("player/pl_grate4.wav");

	PRECACHE_SOUND("player/pl_slosh1.wav");		// walk in shallow water
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");

	PRECACHE_SOUND("player/pl_tile1.wav");		// walk on tile
	PRECACHE_SOUND("player/pl_tile2.wav");
	PRECACHE_SOUND("player/pl_tile3.wav");
	PRECACHE_SOUND("player/pl_tile4.wav");
	PRECACHE_SOUND("player/pl_tile5.wav");

	PRECACHE_SOUND("player/pl_swim1.wav");		// breathe bubbles
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");

	PRECACHE_SOUND("player/pl_ladder1.wav");	// climb ladder rung
	PRECACHE_SOUND("player/pl_ladder2.wav");
	PRECACHE_SOUND("player/pl_ladder3.wav");
	PRECACHE_SOUND("player/pl_ladder4.wav");

	PRECACHE_SOUND("player/pl_wade1.wav");		// wade in water
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");

	PRECACHE_SOUND("debris/wood1.wav");			// hit wood texture
	PRECACHE_SOUND("debris/wood2.wav");
	PRECACHE_SOUND("debris/wood3.wav");

	PRECACHE_SOUND("plats/train_use1.wav");		// use a train

	PRECACHE_SOUND("buttons/spark5.wav");		// hit computer texture
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/glass1.wav");
	PRECACHE_SOUND("debris/glass2.wav");
	PRECACHE_SOUND("debris/glass3.wav");

	PRECACHE_SOUND( SOUND_FLASHLIGHT_ON );
	PRECACHE_SOUND( SOUND_FLASHLIGHT_OFF );

// player gib sounds
	PRECACHE_SOUND("common/bodysplat.wav");			       

// player pain sounds
	PRECACHE_SOUND("player/pl_pain2.wav");
	PRECACHE_SOUND("player/pl_pain4.wav");
	PRECACHE_SOUND("player/pl_pain5.wav");
	PRECACHE_SOUND("player/pl_pain6.wav");
	PRECACHE_SOUND("player/pl_pain7.wav");

	PRECACHE_MODEL("models/player.mdl");

	// hud sounds

	PRECACHE_SOUND("common/wpn_hudoff.wav");
	PRECACHE_SOUND("common/wpn_hudon.wav");
	PRECACHE_SOUND("common/wpn_moveselect.wav");
	PRECACHE_SOUND("common/wpn_select.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");


	// geiger sounds

	PRECACHE_SOUND("player/geiger6.wav");
	PRECACHE_SOUND("player/geiger5.wav");
	PRECACHE_SOUND("player/geiger4.wav");
	PRECACHE_SOUND("player/geiger3.wav");
	PRECACHE_SOUND("player/geiger2.wav");
	PRECACHE_SOUND("player/geiger1.wav");

	if (giPrecacheGrunt)
		UTIL_PrecacheOther("monster_human_grunt");
}

/*
===============
GetGameDescription

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Half-Life";
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error( const char *error_string )
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pEntity );

	if (!pPlayer)
	{
		ALERT(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		ALERT(at_console, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2); // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		ALERT(at_console, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect( edict_t *pEntity )
{
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE( pEntity );

	if (pPlayer)
		pPlayer->SpectatorConnect( );
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect( edict_t *pEntity )
{
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE( pEntity );

	if (pPlayer)
		pPlayer->SpectatorDisconnect( );
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink( edict_t *pEntity )
{
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE( pEntity );

	if (pPlayer)
		pPlayer->SpectatorThink( );
}

////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
	Vector org;
	edict_t *pView = pClient;

	// Find the client's PVS
	if ( pViewEntity )
	{
		pView = pViewEntity;
	}

	if ( pClient->v.flags & FL_PROXY )
	{
		*pvs = NULL;	// the spectator proxy sees
		*pas = NULL;	// and hears everything
		return;
	}

	if( pView->v.effects & EF_MERGE_VISIBILITY )
	{
		if( FClassnameIs( pView, "env_sky" ))
		{
			org = pView->v.origin;
		}
		else return; // don't merge pvs
	}
	else
	{
		org = pView->v.origin + pView->v.view_ofs;
		if ( pView->v.flags & FL_DUCKING )
		{
			org = org + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		}
	}

	*pvs = ENGINE_SET_PVS ( (float *)&org );
	*pas = ENGINE_SET_PAS ( (float *)&org );
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet )
{
	int					i;
	static int counter; // XASH3D MAX_VISIBLE_PACKET == 512
	bool hide = false;

	if( ( ent == host || player ) && counter > gpGlobals->maxClients  + 1 )
		counter = 0;

	// don't send if flagged for NODRAW and it's not the host getting the message
	if( ( ent->v.effects & EF_NODRAW ) && ( ent != host ) )
		return 0;

	// Ignore ents without valid / visible models
	if ( !ent->v.modelindex || !STRING( ent->v.model ) )
		return 0;

	// Don't send spectators to other players
	if ( ( ent->v.flags & FL_SPECTATOR ) && ( ent != host ) )
	{
		return 0;
	}

	if( mp_serverdistclip.value )
	{
		bool isbmodel = false, istrash = false, ismonster = false;
		const char *classname = "";

		if( ent->v.model && STRING(ent->v.model) && *STRING(ent->v.model) == '*' )
			isbmodel = true;

		if( ent->v.classname && STRING(ent->v.classname) )
			classname = STRING( ent->v.classname );

		if( !strcmp( classname, "gib" ) || !strncmp( classname, "weapon_", 7) || !strncmp( classname, "ammo_", 5) || !strncmp( classname, "item_", 5)  )
			istrash = true;
		else if( !strncmp( classname, "monster_", 8 ) )
			ismonster = true;

		Vector delta = ( ent->v.absmin + ent->v.absmax ) / 2 - host->v.origin;

		float size = 0, dist = 0;

		if( ent->v.size.x > size )
			size = ent->v.size.x;
		if( ent->v.size.y > size )
			size = ent->v.size.y;
		if( ent->v.size.z > size )
			size = ent->v.size.z;

		if( size > 512 ) // big brushes may be rotated, but dist check does not cover this
			dist = abs(delta.z), size = ent->v.size.z;
		else
		{
			if( abs(delta.x) > dist )
				dist = abs(delta.x);
			if( abs(delta.y) > dist )
				dist = abs(delta.y);
			if( abs(delta.z) > dist )
				dist = abs(delta.z);
		}
		dist -= size / 2;

		if( size <= 32 && ismonster )
			istrash = true; // we can hide small monsters like trash

		//printf("%s %s %f %f %f %f %f\n", classname, STRING(ent->v.model), size, dist, delta.z,ent->v.origin.z, host->v.origin.z );

		if( isbmodel ) // brushes needed to pmove prediction
		{
			// water renders too slow
			if( ent->v.skin == -3 && dist > mp_maxwaterdist.value  )
				hide = true;
			else if( dist > mp_maxbmodeldist.value )
				hide  = true;
		}
		else if( istrash && ( dist > mp_maxtrashdist.value || counter > 448 ) ) // reserve 64 visents
			hide = true;	// low priority models.
							// Nothing will break if we hide it, so hide if packet full
		else if( ismonster && dist > mp_maxmonsterdist.value )
			hide = true;
		else if( dist > mp_maxotherdist.value )
			hide = true; // other entities. May break beams, etc...

		if( hide )
		{
			// printf("%s %d\n", classname, counter );
			// trash class does not have any important
			// attachments/sounds, so we may just skip adding it to packet
			// but left it just hidden if we have enough visents to prevent
			// sending big delta packets, but reserve 256 slots for visible
			if( counter > 128 && !istrash || counter > 256 )
				return 0;
		}

	}

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if ( ent != host )
	{
		if ( !ENGINE_CHECK_VISIBILITY( (const struct edict_s *)ent, pSet ) )
		{
			// env_sky is visible always
			if( !FClassnameIs( ent, "env_sky" ))
			{
				return 0;
			}
		}
	}


	// Don't send entity to local client if the client says it's predicting the entity itself.
	if ( ent->v.flags & FL_SKIPLOCALHOST )
	{
		if ( hostflags & 4 ) return 0; // it's a portal pass
		if ( ( hostflags & 1 ) && ( ent->v.owner == host ) )
			return 0;
	}
	
	if ( host->v.groupinfo )
	{
		UTIL_SetGroupTrace( host->v.groupinfo, GROUP_OP_AND );

		// Should always be set, of course
		if ( ent->v.groupinfo )
		{
			if ( g_groupop == GROUP_OP_AND )
			{
				if ( !(ent->v.groupinfo & host->v.groupinfo ) )
					return 0;
			}
			else if ( g_groupop == GROUP_OP_NAND )
			{
				if ( ent->v.groupinfo & host->v.groupinfo )
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	memset( state, 0, sizeof( *state ) );

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number	  = e;
	state->entityType = ENTITY_NORMAL;
	
	// Flag custom entities.
	if ( ent->v.flags & FL_CUSTOMENTITY )
	{
		state->entityType = ENTITY_BEAM;
	}

	// 
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime   = (int)(1000.0 * ent->v.animtime ) / 1000.0;

	memcpy( state->origin, ent->v.origin, 3 * sizeof( float ) );
	memcpy( state->angles, ent->v.angles, 3 * sizeof( float ) );
	memcpy( state->mins, ent->v.mins, 3 * sizeof( float ) );
	memcpy( state->maxs, ent->v.maxs, 3 * sizeof( float ) );

	memcpy( state->startpos, ent->v.startpos, 3 * sizeof( float ) );
	memcpy( state->endpos, ent->v.endpos, 3 * sizeof( float ) );
	memcpy( state->velocity, ent->v.velocity, 3 * sizeof( float ) );

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;
		
	state->frame      = ent->v.frame;

	state->skin       = ent->v.skin;
	state->effects    = ent->v.effects;
	if( hide )
		state->effects |= EF_NODRAW;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	if ( !player &&
		 ent->v.animtime &&
		 ent->v.velocity[ 0 ] == 0 && 
		 ent->v.velocity[ 1 ] == 0 && 
		 ent->v.velocity[ 2 ] == 0 )
	{
		state->eflags |= EFLAG_SLERP;
	}

	state->scale	  = ent->v.scale;
	state->solid	  = ent->v.solid;
	state->colormap   = ent->v.colormap;

	state->movetype   = ent->v.movetype;
	state->sequence   = ent->v.sequence;
	state->framerate  = ent->v.framerate;
	state->body       = ent->v.body;

	for (i = 0; i < 4; i++)
	{
		state->controller[i] = ent->v.controller[i];
	}

	for (i = 0; i < 2; i++)
	{
		state->blending[i]   = ent->v.blending[i];
	}

	state->rendermode	= ent->v.rendermode;
	state->renderamt	= (int)ent->v.renderamt; 
	state->renderfx		= ent->v.renderfx;
	state->rendercolor.r	= (byte)ent->v.rendercolor.x;
	state->rendercolor.g	= (byte)ent->v.rendercolor.y;
	state->rendercolor.b	= (byte)ent->v.rendercolor.z;

	state->aiment = 0;
	if ( ent->v.aiment )
	{
		state->aiment = ENTINDEX( ent->v.aiment );
	}

	state->owner = 0;
	if ( ent->v.owner )
	{
		int owner = ENTINDEX( ent->v.owner );
		
		// Only care if owned by a player
		if ( owner >= 1 && owner <= gpGlobals->maxClients )
		{
			state->owner = owner;	
		}
	}

	state->onground = 0;
	if ( ent->v.groundentity )
	{
		state->onground = ENTINDEX( ent->v.groundentity );
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	if ( !player )
	{
		state->playerclass  = ent->v.playerclass;
		if( ent->v.deadflag == DEAD_DEAD )
			state->solid = SOLID_NOT;
		//if( ent->v.movetype == MOVETYPE_WALK || ent->v.movetype == MOVETYPE_STEP )
		//	state->effects |= EF_NOINTERP;
		//	state->movetype = MOVETYPE_TOSS;


		// gravgun hacks
		if( ent->pvPrivateData && CBaseEntity::Instance( ent)->m_fireState == ENTINDEX(host) )
		{
			static float times[32];
			float tdiff = gpGlobals->time - times[ENTINDEX(host) - 1];
			if( tdiff > 1 )
				tdiff = 0;
			int ping, loss;


			state->solid = SOLID_NOT;
			//state->effects |= EF_NOINTERP;
			// update rate correction (will be interpolated on client)
			PLAYER_CNX_STATS( host, &ping, &loss );
			if( ping > 300 )
				ping = 300;
			state->origin = state->origin + host->v.velocity * tdiff + host->v.velocity *  0.001 *ping;

			//state->movetype = MOVETYPE_WALK;
			times[ENTINDEX(host) - 1] = gpGlobals->time;
		}
	}

	// Special stuff for players only
	if ( player )
	{
		memcpy( state->basevelocity, ent->v.basevelocity, 3 * sizeof( float ) );

		state->weaponmodel  = MODEL_INDEX( STRING( ent->v.weaponmodel ) );
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction     = ent->v.friction;

		state->gravity		= ent->v.gravity;
		//state->team		= ent->v.team;

		state->usehull		= ( ent->v.flags & FL_DUCKING ) ? 1 : 0;
		state->health		= (int)ent->v.health;

		// semclip prediction
		if( mp_semclip.value )
		{
			state->solid = SOLID_NOT;
			if( !ent->v.velocity[0] && !ent->v.velocity[1]  )
			if( ent != host )
				state->movetype = MOVETYPE_NONE;
		}
	}

	counter++;

	return 1;
}

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	28

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
	baseline->origin		= entity->v.origin;
	baseline->angles		= entity->v.angles;
	baseline->frame			= entity->v.frame;
	baseline->skin			= (short)entity->v.skin;

	// render information
	baseline->rendermode	= (byte)entity->v.rendermode;
	baseline->renderamt		= (byte)entity->v.renderamt;
	baseline->rendercolor.r	= (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g	= (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b	= (byte)entity->v.rendercolor.z;
	baseline->renderfx		= (byte)entity->v.renderfx;

	if ( player )
	{
		baseline->mins			= player_mins;
		baseline->maxs			= player_maxs;

		baseline->colormap		= eindex;
		baseline->modelindex	= playermodelindex;
		baseline->friction		= 1.0;
		baseline->movetype		= MOVETYPE_WALK;

		baseline->scale			= entity->v.scale;
		baseline->solid			= SOLID_SLIDEBOX;
		baseline->framerate		= 1.0;
		baseline->gravity		= 1.0;

	}
	else
	{
		baseline->mins			= entity->v.mins;
		baseline->maxs			= entity->v.maxs;

		baseline->colormap		= 0;
		baseline->modelindex	= entity->v.modelindex;//SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype		= entity->v.movetype;

		baseline->scale			= entity->v.scale;
		baseline->solid			= entity->v.solid;
		baseline->framerate		= entity->v.framerate;
		baseline->gravity		= entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int	 field;
} entity_field_alias_t;

#define FIELD_ORIGIN0			0
#define FIELD_ORIGIN1			1
#define FIELD_ORIGIN2			2
#define FIELD_ANGLES0			3
#define FIELD_ANGLES1			4
#define FIELD_ANGLES2			5

static entity_field_alias_t entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
};

void Entity_FieldInit( struct delta_s *pFields )
{
	entity_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN0 ].name );
	entity_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN1 ].name );
	entity_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN2 ].name );
	entity_field_alias[ FIELD_ANGLES0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES0 ].name );
	entity_field_alias[ FIELD_ANGLES1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES1 ].name );
	entity_field_alias[ FIELD_ANGLES2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES2 ].name );
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network. 
FIXME:  Move to script
==================
*/
void Entity_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->impacttime != 0 ) && ( t->starttime != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );

		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

static entity_field_alias_t player_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
};

void Player_FieldInit( struct delta_s *pFields )
{
	player_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN0 ].name );
	player_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN1 ].name );
	player_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN2 ].name );
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network. 
==================
*/
void Player_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Player_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

#define CUSTOMFIELD_ORIGIN0			0
#define CUSTOMFIELD_ORIGIN1			1
#define CUSTOMFIELD_ORIGIN2			2
#define CUSTOMFIELD_ANGLES0			3
#define CUSTOMFIELD_ANGLES1			4
#define CUSTOMFIELD_ANGLES2			5
#define CUSTOMFIELD_SKIN			6
#define CUSTOMFIELD_SEQUENCE		7
#define CUSTOMFIELD_ANIMTIME		8

entity_field_alias_t custom_entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
	{ "skin",				0 },
	{ "sequence",			0 },
	{ "animtime",			0 },
};

void Custom_Entity_FieldInit( struct delta_s *pFields )
{
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].name );
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
void Custom_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int beamType;
	static int initialized = 0;

	if ( !initialized )
	{
		Custom_Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	beamType = t->rendermode & 0x0f;
		
	if ( beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field );
	}

	if ( beamType != BEAM_POINTS )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field );
	}

	if ( beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field );
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ( (int)f->animtime == (int)t->animtime )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field );
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders( void )
{
	DELTA_ADDENCODER( "Entity_Encode", Entity_Encode );
	DELTA_ADDENCODER( "Custom_Encode", Custom_Encode );
	DELTA_ADDENCODER( "Player_Encode", Player_Encode );
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
#if defined( CLIENT_WEAPONS )
	int i;
	weapon_data_t *item;
	entvars_t *pev = &player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );
	CBasePlayerWeapon *gun;
	
	ItemInfo II;

	memset( info, 0, 32 * sizeof( weapon_data_t ) );

	if ( !pl )
		return 1;

	// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( pl->m_rgpPlayerItems[ i ] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = pl->m_rgpPlayerItems[ i ];

			while ( pPlayerItem )
			{
				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();
				if ( gun && gun->UseDecrement() )
				{
					// Get The ID.
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );

					if ( II.iId >= 0 && II.iId < 32 )
					{
						item = &info[ II.iId ];
					 	
						item->m_iId						= II.iId;
						item->m_iClip					= gun->m_iClip;

						item->m_flTimeWeaponIdle	= Q_max( gun->m_flTimeWeaponIdle, -0.001 );
						item->m_flNextPrimaryAttack	= Q_max( gun->m_flNextPrimaryAttack, -0.001 );
						item->m_flNextSecondaryAttack	= Q_max( gun->m_flNextSecondaryAttack, -0.001 );
						item->m_fInReload		= gun->m_fInReload;
						item->m_fInSpecialReload	= gun->m_fInSpecialReload;
						item->fuser1			= Q_max( gun->pev->fuser1, -0.001 );
						item->fuser2			= gun->m_flStartThrow;
						item->fuser3			= gun->m_flReleaseThrow;
						item->iuser1			= gun->m_chargeReady;
						item->iuser2			= gun->m_fInAttack;
						item->iuser3			= gun->m_fireState;

						//item->m_flPumpTime		= max( gun->m_flPumpTime, -0.001 );
					}
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}
#else
	memset( info, 0, 32 * sizeof( weapon_data_t ) );
#endif
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData ( const struct edict_s *ent, int sendweapons, struct clientdata_s *cd )
{
	if( !ent || !ent->pvPrivateData )
		return;
	entvars_t *pev = (entvars_t *)&ent->v;
	CBasePlayer *pl = (CBasePlayer *)( CBasePlayer::Instance( pev ) );
	entvars_t *pevOrg = NULL;

	// if user is spectating different player in First person, override some vars
	if( pl && pl->pev->iuser1 == OBS_IN_EYE )
	{
		if( pl->m_hObserverTarget )
		{
			pevOrg = pev;
			pev = pl->m_hObserverTarget->pev;
			pl = (CBasePlayer *)(CBasePlayer::Instance( pev ) );
		}
	}

	cd->flags		= pev->flags;
	cd->health		= pev->health;

	cd->viewmodel		= MODEL_INDEX( STRING( pev->viewmodel ) );

	cd->waterlevel		= pev->waterlevel;
	cd->watertype		= pev->watertype;
	cd->weapons		= pev->weapons;

	// Vectors
	cd->origin		= pev->origin;
	cd->velocity		= pev->velocity;
	cd->view_ofs		= pev->view_ofs;
	cd->punchangle		= pev->punchangle;

	cd->bInDuck		= pev->bInDuck;
	cd->flTimeStepSound	= pev->flTimeStepSound;
	cd->flDuckTime		= pev->flDuckTime;
	cd->flSwimTime		= pev->flSwimTime;
	cd->waterjumptime	= pev->teleport_time;

	strcpy( cd->physinfo, ENGINE_GETPHYSINFO( ent ) );

	cd->maxspeed		= pev->maxspeed;
	cd->fov			= pev->fov;
	cd->weaponanim		= pev->weaponanim;

	cd->pushmsec		= pev->pushmsec;

	// Spectator mode
	if( pevOrg != NULL )
	{
		// don't use spec vars from chased player
		cd->iuser1		= pevOrg->iuser1;
		cd->iuser2		= pevOrg->iuser2;
	}
	else
	{
		cd->iuser1		= pev->iuser1;
		cd->iuser2		= pev->iuser2;
	}
#if defined( CLIENT_WEAPONS )
	if ( sendweapons )
	{
		if( pl )
		{
			cd->m_flNextAttack	= pl->m_flNextAttack;
			cd->fuser2			= pl->m_flNextAmmoBurn;
			cd->fuser3			= pl->m_flAmmoStartCharge;
			cd->vuser1.x		= pl->ammo_9mm;
			cd->vuser1.y		= pl->ammo_357;
			cd->vuser1.z		= pl->ammo_argrens;
			cd->ammo_nails		= pl->ammo_bolts;
			cd->ammo_shells		= pl->ammo_buckshot;
			cd->ammo_rockets	= pl->ammo_rockets;
			cd->ammo_cells		= pl->ammo_uranium;
			cd->vuser2.x		= pl->ammo_hornets;
			cd->vuser2.y		= pl->ammo_556;
			cd->vuser2.z		= pl->ammo_762;

			if ( pl->m_pActiveItem )
			{
				CBasePlayerWeapon *gun;
				gun = (CBasePlayerWeapon *)pl->m_pActiveItem->GetWeaponPtr();
				if ( gun && gun->UseDecrement() )
				{
					ItemInfo II;
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );

					cd->m_iId = II.iId;

					cd->vuser3.z	= gun->m_iSecondaryAmmoType;
					cd->vuser4.x	= gun->m_iPrimaryAmmoType;
					cd->vuser4.y	= pl->m_rgAmmo[gun->m_iPrimaryAmmoType];
					cd->vuser4.z	= pl->m_rgAmmo[gun->m_iSecondaryAmmoType];
					
					if ( pl->m_pActiveItem->m_iId == WEAPON_RPG )
					{
						cd->vuser2.y = ( ( CRpg * )pl->m_pActiveItem)->m_fSpotActive;
						cd->vuser2.z = ( ( CRpg * )pl->m_pActiveItem)->m_cActiveRockets;
					}
				}
			}
		}
	}
#endif
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

	if( !pl )
		return;

	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_SetGroupTrace( pl->pev->groupinfo, GROUP_OP_AND );
	}

	pl->random_seed = random_seed;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd ( const edict_t *player )
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

	if( !pl )
		return;
	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_UnsetGroupTrace();
	}
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	//int max_buffer_size = *response_buffer_size;
	if( GGM_ConnectionlessPacket( net_from, args, response_buffer, response_buffer_size ) )
		return 1;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
		VEC_HULL_MIN.CopyToArray(mins);
		VEC_HULL_MAX.CopyToArray(maxs);
		iret = 1;
		break;
	case 1:				// Crouched player
		VEC_DUCK_HULL_MIN.CopyToArray(mins);
		VEC_DUCK_HULL_MAX.CopyToArray(maxs);
		iret = 1;
		break;
	case 2:				// Point based hull
		Vector( 0, 0, 0 ).CopyToArray(mins);
		Vector( 0, 0, 0 ).CopyToArray(maxs);
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines ( void )
{
	/*int iret = 0;
	entity_state_t state;

	memset( &state, 0, sizeof(state) );*/

	// Create any additional baselines here for things like grendates, etc.
	// iret = ENGINE_INSTANCE_BASELINE( pc->pev->classname, &state );

	// Destroy objects.
	//UTIL_Remove( pc );
}

void CvarValue2( const edict_t *pEnt, int requestID, const char *cvarName, const char *value )
{

	CBasePlayer *player = (CBasePlayer * ) CBaseEntity::Instance( (edict_t*)pEnt );

	if( pEnt && requestID == 112 && FStrEq( cvarName , "r_drawentities" ) && atoi(value) != 1 )
		KickCheater( player, "wh" );

	if( pEnt && requestID == 113 && FStrEq( cvarName , "r_lockpvs" ) && atoi(value)  )
		KickCheater( player, "wh" );

	if( pEnt && requestID == 114 && FStrEq( cvarName , "gl_wh" ) && atoi(value) )
		KickCheater( player, "wh" );

	if( pEnt && requestID == 115 && FStrEq( cvarName , "cl_wh" ) && atoi(value) )
		KickCheater( player, "wh" );

	if( pEnt && requestID == 116 && FStrEq( cvarName , "host_ver" ) )
	{
		if( !strcmp( value , "eee764" ) )
			KickCheater( player, "cheats" );
		if( !strcmp( value , "7a3ffb" ) )
			KickCheater( player, "cheats" );
		if( !strcmp( value , "235225" ) )
			KickCheater( player, "cheats" );
		if( !strcmp( value , "5fc42f" ) )
			KickCheater( player, "cheats" );
	}
	if( pEnt && requestID == 117 && FStrEq( cvarName , "host_build" ) )
	{
		if( !strcmp( value , "3295" ) )
			KickCheater( player, "cheats" );
	}
	if( pEnt && requestID == 119 && FStrEq( cvarName , "drm_weaponglow" ) && isdigit(value[0]) )
	{
		KickCheater( player, "cheats" );
	}
	/* CSQQ */
	if( pEnt && requestID == 118 && FStrEq( cvarName , "cscl_ver" ) && isdigit(value[0]) )
	{
		KickCheater( player, "cheats" );
	}
	if( pEnt && requestID == 118 && FStrEq( cvarName , "nocs_ver" ) && isdigit(value[0]) )
	{
		KickCheater( player, "cheats" );
	}
	if( pEnt && requestID == 120 && FStrEq( cvarName , "ogc" ) && isdigit(value[0]) )
	{
		KickCheater( player, "cheats" );
	}
	if( pEnt && requestID == 121 && FStrEq( cvarName , "bae" ) && isdigit(value[0]) )
	{
		KickCheater( player, "cheats" );
	}

	GM_CvarValue2( pEnt, requestID, cvarName, value );
	GGM_CvarValue2( pEnt, requestID, cvarName, value );
}


/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int	InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
	// Server doesn't care?
	if ( CVAR_GET_FLOAT( "mp_consistency" ) != 1 )
		return 0;

	// Default behavior is to kick the player
	sprintf( disconnect_message, "Server is enforcing file consistency for %s\n", filename );

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too, 
  if you want.
================================
*/
int AllowLagCompensation( void )
{
	return 1;
}
