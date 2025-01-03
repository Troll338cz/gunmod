// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//        Tyler Lund <halflife@bubblemod.org>
//
// LICENSE                                                            
//                                                                    
//        Permission is granted to anyone to use this  software  for  
//        any purpose on any computer system, and to redistribute it  
//        in any way, subject to the following restrictions:          
//                                                                    
//        1. The author is not responsible for the  consequences  of  
//           use of this software, no matter how awful, even if they  
//           arise from defects in it.                                
//        2. The origin of this software must not be misrepresented,  
//           either by explicit claim or by omission.                 
//        3. Altered  versions  must  be plainly marked as such, and  
//           must  not  be  misrepresented  (by  explicit  claim  or  
//           omission) as being the original software.                
//        3a. It would be nice if I got  a  copy  of  your  improved  
//            version  sent to halflife@bubblemod.org. 
//        4. This notice must not be removed or altered.              
//                                                                    
// ---------------------------------------------------------------

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"

//#include "BMOD_messaging.h"
#include "BMOD_player.h"

extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgTeamInfo;
extern int gmsgSpectator;

extern void respawn(entvars_t *pev, BOOL fCopyCorpse);

// Player has become a spectator. Set it up.
// This was moved from player.cpp.
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// ??
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster( );	

	// Off tanks
	if( m_pTank != 0 )
		m_pTank->Use( this, this, USE_OFF, 0 );

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
	 	WRITE_BYTE(0);
	 	WRITE_BYTE(0XFF);
	 	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = 0;
	m_iClientFOV = -1;
	pev->fov = m_iFOV;
	SET_VIEW(edict(), edict());

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = (HIDEHUD_FLASHLIGHT | HIDEHUD_WEAPONS | HIDEHUD_HEALTH);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects |= EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NOCLIP;
	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;
	
	// Clear out the status bar
	m_fInitHUD = TRUE;

	// Update Team Status
	// pev->team = 0;

	// Remove all the player's stuff
	RemoveAllItems( FALSE );

	// Move them to the new position
	UTIL_SetOrigin( pev, vecPosition );

	// Find a player to watch
	Observer_SetMode(OBS_ROAMING);

	// Tell all clients this player is now a spectator
	//MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
	//	WRITE_BYTE( ENTINDEX( edict() ) );
	//	WRITE_BYTE( 1 );
	//MESSAGE_END();

	 //MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
	 //	WRITE_BYTE( ENTINDEX(edict()) );
	 //  WRITE_STRING( "" );
	 //MESSAGE_END();

	 //m_fMsgTimer = gpGlobals->time + .5;
	 //m_bSentMsg = FALSE;
}

// Leave observer mode
void CBasePlayer::StopObserver( void )
{
	// Turn off spectator
	if ( pev->iuser1 || pev->iuser2 )
	{
		// Tell all clients this player is not a spectator anymore
		//MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
		// 	WRITE_BYTE( ENTINDEX( edict() ) );
		// 	WRITE_BYTE( 0 );
		//MESSAGE_END();

		pev->iuser1 = pev->iuser2 = 0; 
		m_hObserverTarget = NULL;
		m_iHideHUD = 0;

		respawn( pev, !( m_afPhysicsFlags & PFLAG_OBSERVER ) );

		//MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
	 	//	WRITE_BYTE( ENTINDEX(edict()) );
	 	//	WRITE_STRING( m_szTeamName );
		//MESSAGE_END();
	}
}

// Find the next client in the game for this player to spectate
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{

	int		iStart;
	if ( m_hObserverTarget )
		iStart = ENTINDEX( m_hObserverTarget->edict() );
	else
		iStart = ENTINDEX( edict() );
	int	    iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = 1; 

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
		if ( !pEnt )
			continue;
		if ( pEnt == this )
			continue;
		// Don't spec observers or invisible players or defunct players
		if ( ((CBasePlayer*)pEnt)->IsObserver() 
			|| (pEnt->pev->effects & EF_NODRAW) 
			|| ( ( STRING( pEnt->pev->netname ) )[0] == 0 ) 
			/*|| (!((CBasePlayer*)pEnt)->m_bIsConnected)*/
			)
			continue;

		// MOD AUTHORS: Add checks on target here.

		m_hObserverTarget = pEnt;
		break;

	} while ( iCurrent != iStart );

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		// Store the target in pev so the physics DLL can get to it
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
		// Move to the target
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );
		
		// ClientPrint( pev, HUD_PRINTCENTER, UTIL_VarArgs ("Chase Camera: %s\n", STRING( m_hObserverTarget->pev->netname ) ) );
		// ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );
	}
	else
	{
		// ALERT( at_console, "No observer targets.\n" );
	}
}

// Handle buttons in observer mode
void CBasePlayer::Observer_HandleButtons()
{

	// Slow down mouse clicks
	if (  m_fMsgTimer <= gpGlobals->time  )
	{
		// Show the observer mode instructions
		if (!m_bSentMsg)
		{
			m_bSentMsg = TRUE;
			//PrintMessage( this, BMOD_CHAN_RUNE, Vector (20,250,20), Vector (.5, 86400, 2), "-Spectator Mode-\n FIRE=Spawn   JUMP=Switch Modes   ALT FIRE=Switch Targets");
			UTIL_ShowMessage("FIRE=Spawn   JUMP=Switch Modes   ALT FIRE=Switch Targets", this);
		}

		// Check to see if the observer wants to spray their logo
		if (pev->impulse == 201) {
			ImpulseCommands();
		}

		// Jump changes modes: Chase to Roaming
		if ( pev->button & IN_JUMP )
		{
			if ( pev->iuser1 == OBS_ROAMING )
				Observer_SetMode( OBS_CHASE_FREE );
			else
				Observer_SetMode( OBS_ROAMING );
			m_fMsgTimer = gpGlobals->time + 0.2;
		}

		// Attack2 cycles player targets
		if ( pev->button & IN_ATTACK2 && pev->iuser1 != OBS_ROAMING )
		{
			Observer_FindNextPlayer( false );
			m_fMsgTimer = gpGlobals->time + 0.2;
		}

		// Attack Spawns
		// if ( m_afButtonPressed & IN_ATTACK && pev->iuser1 != OBS_ROAMING )
		if ( pev->button & IN_ATTACK )
		{
//			g_engfuncs.pfnServerPrint( "Player spawned from Obs!\n" );
			StopObserver();
			this->m_flLastCommandTime[0] = gpGlobals->time + 1.0f;
			m_fMsgTimer = gpGlobals->time + 0.2;
		}		
	}

	// clear attack/use commands from player
	m_afButtonPressed = 0;
	pev->button = 0;
	m_afButtonReleased = 0;

	pev->impulse = 0;
	if ((m_hObserverTarget != 0) && (m_hObserverTarget->IsPlayer()) && (pev->iuser1 == OBS_CHASE_FREE))
	{
		pev->origin = m_hObserverTarget->pev->origin;
		pev->velocity = m_hObserverTarget->pev->velocity;
	}
}

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode( int iMode )
{
	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
		return;

	// Changing to Roaming?
	if ( iMode == OBS_ROAMING )
	{
		// MOD AUTHORS: If you don't want to allow roaming observers at all in your mod, just abort here.
		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;
		m_hObserverTarget = NULL;

		ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode3" );
		// pev->maxspeed = 320;
		pev->maxspeed = 1000;
		return;
	}

	// Changing to Chase Lock?
	if ( iMode == OBS_CHASE_LOCKED )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if ( m_hObserverTarget == 0 )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_LOCKED;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode1" );
			pev->maxspeed = 0;
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}

		return;
	}

	// Changing to Chase Freelook?
	if ( iMode == OBS_CHASE_FREE )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		// if ( m_hObserverTarget == NULL )
		if ( pev->iuser1 == OBS_ROAMING )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_FREE;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
			// ClientPrint( pev, HUD_PRINTCENTER, "Chase-Camera Mode" );
			pev->maxspeed = 0;
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}
		
		return;
	}
}

