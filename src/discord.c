// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2018 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2012-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  discord.h
/// \brief Discord Rich Presence handling

#ifdef HAVE_DISCORDRPC

#include "i_system.h"
#include "netcode/d_clisrv.h"
#include "netcode/d_netcmd.h"
#include "netcode/i_net.h"
#include "netcode/server_connection.h"
#include "g_game.h"
#include "p_tick.h"
#include "m_menu.h" // gametype_cons_t
#include "r_things.h" // skins
#include "netcode/mserv.h" // ms_RoomId
#include "z_zone.h"

#include <time.h>

#include "discord.h"
#include "doomdef.h"

// Feel free to provide your own, if you care enough to create another Discord app for this :P
#define DISCORD_APPID "1498038818471940206"

consvar_t cv_discordrp = CVAR_INIT("discordrp", "On", CV_SAVE|CV_CALL, CV_OnOff, DRPC_UpdatePresence);

tic_t starttime = 6*TICRATE + (3*TICRATE/4);

//
// DRPC_Handle's
//
static inline void DRPC_HandleReady(const DiscordUser *user)
{
	CONS_Printf("Discord: connected to %s#%s - %s\n", user->username, user->discriminator, user->userId);
}

static inline void DRPC_HandleDisconnect(int err, const char *msg)
{
	CONS_Printf("Discord: disconnected (%d: %s)\n", err, msg);
}

static inline void DRPC_HandleError(int err, const char *msg)
{
	CONS_Printf("Discord: error (%d, %s)\n", err, msg);
}

static inline void DRPC_HandleJoin(const char *secret)
{
	CONS_Printf("Discord: connecting to %s\n", secret);
	COM_BufAddText(va("connect \"%s\"\n", secret));
}

//
// DRPC_Init: starting up the handles, call Discord_initalize
//
void DRPC_Init(void)
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	handlers.ready = DRPC_HandleReady;
	handlers.disconnected = DRPC_HandleDisconnect;
	handlers.errored = DRPC_HandleError;
	handlers.joinGame = DRPC_HandleJoin;

	Discord_Initialize(DISCORD_APPID, &handlers, 1, NULL);
	I_AddExitFunc(Discord_Shutdown);
	DRPC_UpdatePresence();
}

//
// DRPC_UpdatePresence: Called whenever anything changes about server info
//
void DRPC_UpdatePresence(void)
{
	char mapimg[8];
	char mapname[48];
	char charimg[21];
	char charname[28];
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	if (!cv_discordrp.value)
	{
		// User doesn't want to show their game information, so update with empty presence.
		// This just shows that they're playing SRB2Kart. (If that's too much, then they should disable game activity :V)
		Discord_UpdatePresence(&discordPresence);
		return;
	}

	// Server info
	if (netgame)
	{
		const char *address;

		switch (ms_RoomId)
		{
			case -1: discordPresence.state = "Private"; break; // Private server
			case 33: discordPresence.state = "Standard"; break;
			case 28: discordPresence.state = "Casual"; break;
			default: discordPresence.state = "???"; break; // How?
		}

		discordPresence.partyId = server_context; // Thanks, whoever gave us Mumble support, for implementing the EXACT thing Discord wanted for this field!
		discordPresence.partySize = D_NumPlayers(); // Players in server
		discordPresence.partyMax = cv_maxplayers.value; // Max players (TODO: another variable should hold this, so that maxplayers doesn't have to be a netvar)

		// Grab the host's IP for joining.
		if (I_GetNodeAddress && (address = I_GetNodeAddress(servernode)) != NULL)
		{
			discordPresence.joinSecret = address;
			CONS_Printf("%s\n", address);
		}

		discordPresence.partySize = D_NumPlayers(); // Players in server
		discordPresence.partyMax = cv_maxplayers.value; // Max players (turned into a netvar for this, FOR NOW!)
	}
	else if (Playing())
		discordPresence.state = "Offline";
	else if (demoplayback)
		discordPresence.state = "Watching Demo";
	else
		discordPresence.state = "Menu";

	// Gametype info
	if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
	{
		if (modeattacking)
			discordPresence.details = "Record Attack";
		else if (marathonmode)
			discordPresence.details = "Marathon Mode";
		else
			discordPresence.details = gametype_cons_t[gametype].strvalue;
	}

	if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) // Map info
		&& !(demoplayback))
	{
		if ((gamemap >= 1 && gamemap <= 60) // supported race maps
			|| (gamemap >= 136 && gamemap <= 164)) // supported battle maps
		{
			snprintf(mapimg, 8, "%sp", G_BuildMapName(gamemap));
			strlwr(mapimg);
			discordPresence.largeImageKey = mapimg; // Map image
		}
		else if (mapheaderinfo[gamemap-1]->menuflags & LF2_HIDEINMENU)
		{
			// Hell map, use the method that got you here :P
			discordPresence.largeImageKey = "miscdice";
		}
		else
		{
			// This is probably a custom map!
			discordPresence.largeImageKey = "mapcustom";
		}

		if (mapheaderinfo[gamemap-1]->menuflags & LF2_HIDEINMENU)
		{
			// Hell map, hide the name
			discordPresence.largeImageText = "Map: ???";
		}
		else
		{
			// Map name on tool tip
			char *title = G_BuildMapTitle(gamemap);
			snprintf(mapname, 48, "Map: %s", title);
			discordPresence.largeImageText = mapname;
			Z_Free(title);
		}

		if (gamestate == GS_LEVEL && Playing())
		{
			const time_t currentTime = time(NULL);
			const time_t mapTimeStart = currentTime - ((leveltime + (modeattacking ? starttime : 0)) / TICRATE);

			discordPresence.startTimestamp = mapTimeStart;

			if (timelimitintics > 0)
			{
				const time_t mapTimeEnd = mapTimeStart + ((timelimitintics + starttime + 1) / TICRATE);
				discordPresence.endTimestamp = mapTimeEnd;
			}
		}
	}
	else
	{
		discordPresence.largeImageKey = "misctitle";
		discordPresence.largeImageText = "Title Screen";
	}

	// Character info
	if (Playing() && playeringame[consoleplayer] && !players[consoleplayer].spectator)
	{
		if (players[consoleplayer].skin <= 5) // supported skins
		{
			snprintf(charimg, 21, "char%s", skins[players[consoleplayer].skin]->name);
			discordPresence.smallImageKey = charimg; // Character image
		}
		else
			discordPresence.smallImageKey = "charnull"; // Just so that you can still see the name of custom chars 

		snprintf(charname, 28, "Character: %s", skins[players[consoleplayer].skin]->realname);
		discordPresence.smallImageText = charname; // Character name
	}

	Discord_UpdatePresence(&discordPresence);
}

#endif
