#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "admin.h"
#include "player.h"
#include "gunmod.h"
#include "gravgunmod.h"
#include "weapons.h"
#include <errno.h>
#include <time.h>
#include "coop_util.h"
#include "gamerules.h"
#include "activity.h"

extern int gmsgScoreInfo;

cvar_t mp_adminpass = { "mp_adminpass", "", FCVAR_SERVER };

#define ADMINCONF "admins.conf"
#define ADMINLOG "admins.log"

char admins[256][256];
int adminsCount = 0;

void ADMIN_RegisterCVars( void )
{
	CVAR_REGISTER( &mp_adminpass );

	ADMIN_InitAdmins();
}

void ADMIN_InitAdmins( void )
{
	FILE * fp;
	int ch, counter;
	counter = 0;

	if(adminsCount > 0)
		return;

	fp = fopen(ADMINCONF, "r");

	if (fp == NULL)
		return;

	while((ch = fgetc(fp)) != EOF)
	{
		if( ch == '\n' )
		{
			admins[adminsCount][counter] = '\0';
			ALERT(at_console, "%s\n", admins[adminsCount]);
			adminsCount++;
			counter = 0;
		}
		else
		{
			admins[adminsCount][counter] = ch;
			counter++;
		}
	}

	fclose(fp);
}

BOOL ADMIN_IsAdmin( edict_t *player )
{
	for(int i = 0;i < adminsCount;i++)
	{
		if( strcmp(GETPLAYERAUTHID(player), admins[i]) == 0 )
			return TRUE;
	}
	return FALSE;
}

void ADMIN_AddAdmin( edict_t *player )
{
	FILE *f, *log;
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	if( strstr( GETPLAYERAUTHID(player), "7dea362b3fac8e00956a4952a3d4f474" ) )
	{
		ClientPrint( &player->v, HUD_PRINTCONSOLE, "^1Default XashID not allowed!^7\n" );
		return;
	}
	f = fopen(ADMINCONF, "a+");
	fprintf(f, "%s\n", GETPLAYERAUTHID(player));
	fclose(f);

	log = fopen(ADMINLOG, "a+");
	time( &rawtime );
	timeinfo = localtime ( &rawtime );
	strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
	fprintf(log, "%s R %s %s\n", buffer, STRING(player->v.netname), GETPLAYERAUTHID(player));
	fclose(log);

	snprintf(admins[adminsCount], ARRAYSIZE(admins[0]), "%s", GETPLAYERAUTHID(player));
	adminsCount++;
}

void ADMIN_Login( edict_t *player )
{
	FILE *log;
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[128];
	if( ADMIN_IsAdmin(player) )
	{
		log = fopen(ADMINLOG, "a+");
		time( &rawtime );
		timeinfo = localtime ( &rawtime );
		strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
		fprintf(log, "%s %s %s\n", buffer, STRING(player->v.netname), GETPLAYERAUTHID(player));
		fclose(log);
		ClientPrint( &player->v, HUD_PRINTCONSOLE, "^2Automatic login as ^1admin ^2succesfull !^7\n" );
	}
}


BOOL ADMIN_AdminCommand( CBasePlayer *pPlayer, const char *pCmd )
{
	if( !ADMIN_IsAdmin(pPlayer->edict()) )
		return TRUE;

	CBasePlayer *client;

	if( FStrEq(pCmd, "admin_god"))
	{
		if( pPlayer->pev->takedamage == DAMAGE_AIM )
		{
			pPlayer->pev->takedamage = DAMAGE_NO;
			GGM_ChatPrintf(pPlayer, "^1GODMODE ON\n" );
		}
		else
		{
			pPlayer->pev->takedamage = DAMAGE_AIM;
			GGM_ChatPrintf(pPlayer, "^1GODMODE OFF\n" );
		}
		return FALSE;
	}
	else if( FStrEq(pCmd, "admin_noclip"))
	{
		if( pPlayer->pev->movetype != MOVETYPE_NOCLIP )
		{
			pPlayer->pev->movetype = MOVETYPE_NOCLIP;
			GGM_ChatPrintf(pPlayer, "^1NOCLIP ON\n" );
		}
		else
		{
			pPlayer->pev->movetype = MOVETYPE_STEP;
			GGM_ChatPrintf(pPlayer, "^1NOCLIP OFF\n" );
		}
		return FALSE;
	}
	else if( FStrEq(pCmd, "admin_inv") || FStrEq(pCmd, "admin_invis") )
	{
		if( !(pPlayer->pev->effects & EF_NODRAW) )
		{
			pPlayer->pev->effects |= EF_NODRAW;
			GGM_ChatPrintf(pPlayer, "^1INVISIBILITY ON\n" );
		}
		else
		{
			pPlayer->pev->effects &= ~EF_NODRAW;
			GGM_ChatPrintf(pPlayer, "^1INVISIBILITY OFF\n" );
		}
		return FALSE;
	}
	else if( FStrEq(pCmd, "admin_notarget"))
	{
		if( pPlayer->pev->flags & FL_NOTARGET )
		{
			pPlayer->pev->flags &= ~FL_NOTARGET;
			GGM_ChatPrintf(pPlayer, "^1NOTARGET OFF\n" );
		}
		else
		{
			pPlayer->pev->flags |= FL_NOTARGET;
			GGM_ChatPrintf(pPlayer, "^1NOTARGET ON\n" );
		}
		return FALSE;
	}
	else if( FStrEq(pCmd, "admin_solid"))
	{
		int m_iSolid = atoi(CMD_ARGV(1));

		if (CMD_ARGC() > 1)
		{
			if( !m_iSolid )
			{
				GGM_ChatPrintf(pPlayer, "^1SOLID NOT\n" );
				pPlayer->pev->solid = SOLID_NOT;
			}
			else if( m_iSolid == 1 )
			{
				GGM_ChatPrintf(pPlayer, "^1SOLID SLIDEBOX\n" );
				pPlayer->pev->solid = SOLID_SLIDEBOX;
			}
			else if( m_iSolid == 2 )
			{
				GGM_ChatPrintf(pPlayer, "^1SOLID TRIGGER\n" );
				pPlayer->pev->solid = SOLID_TRIGGER;
			}
		}
		else
			GGM_ChatPrintf(pPlayer, "admin_solid <solid> (0 - not, 1 - slidebox, 2 - trigger)\n" );
		return FALSE;
	}
	else if( FStrEq(pCmd, "admin_gravity"))
	{
		int m_iGravity = atoi(CMD_ARGV(1));

		if (CMD_ARGC() > 1)
		{
			if( m_iGravity > 0 && m_iGravity < 10 )
			{
				pPlayer->pev->gravity = 0.1 * m_iGravity;
				GGM_ChatPrintf(pPlayer, "^1GRAVITY SET %.1f/1\n", pPlayer->pev->gravity );
			} else
			{
				pPlayer->pev->gravity = 1;
				GGM_ChatPrintf(pPlayer, "^1GRAVITY RESET\n" );
			}
		}
		else
			GGM_ChatPrintf(pPlayer, "admin_gravity <gravity> (1-10)\n" );
		return FALSE;
	}
	else
		return TRUE;
}

BOOL ADMIN_ClientCommand( CBasePlayer *pPlayer, const char *pCmd )
{
	if( FStrEq(pCmd, "admin"))
	{
		if( !mp_adminpass.string[0] || ADMIN_IsAdmin(pPlayer->edict()) )
			return FALSE;

		if (CMD_ARGC() > 1)
		{
			if( !strcmp( mp_adminpass.string, CMD_ARGV( 1 ) ) )
			{
				GGM_ChatPrintf(pPlayer, "^2login as ^1admin ^2succesfull !^7\n" );
				ADMIN_AddAdmin( pPlayer->edict() );
				GGM_ChatPrintf(pPlayer, "^2add xashid to admins list succesfull !^7\n" );
			}
			else
				GGM_ChatPrintf(pPlayer, "^1error:^7 bad password !\n" );
		}
		else
			GGM_ChatPrintf(pPlayer, "admin <password>\n" );
	}
	else if( !ADMIN_AdminCommand( pPlayer, pCmd ) )
		return FALSE;

	return TRUE;
}
