/* Copyright (c) 2002-2012 Croteam Ltd.
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "StdH.h"

#include <Engine/Query/QueryManager.h>

extern unsigned char *gsseckey(u_char *secure, u_char *key, int enctype);

// Status response for formatting
static const char *_strStatusResponseFormat =
  "\\gamename\\%s\\gamever\\%s\\location\\%s\\hostname\\%s\\hostport\\%hu"
  "\\mapname\\%s\\gametype\\%s\\activemod\\"
  "\\numplayers\\%d\\maxplayers\\%d\\gamemode\\openplaying\\difficulty\\Normal"
  "\\friendlyfire\\%d\\weaponsstay\\%d\\ammosstay\\%d"
  "\\healthandarmorstays\\%d\\allowhealth\\%d\\allowarmor\\%d\\infiniteammo\\%d\\respawninplace\\%d"
  "\\password\\0\\vipplayers\\1";

extern INDEX net_iPort;

void ILegacy::BuildHearthbeatPacket(CTString &strPacket, INDEX iChallenge) {
  strPacket.PrintF("\\heartbeat\\%hu\\gamename\\%s", (net_iPort + 1), SAM_MS_NAME);
};

void ILegacy::ServerParsePacket(INDEX iLength) {
  // End with a null terminator
  IQuery::pBuffer[iLength] = '\0';

  // String of data
  const char *strData = IQuery::pBuffer;

  // Check for packet types
  const char *pStatus  = strstr(strData, "\\status\\");
  const char *pInfo    = strstr(strData, "\\info\\");
  const char *pBasic   = strstr(strData, "\\basic\\");
  const char *pPlayers = strstr(strData, "\\players\\");
  const char *pSecure  = strstr(strData, "\\secure\\");

  if (ms_bDebugOutput) {
    CPrintF("Received data (%d bytes):\n%s\n", iLength, IQuery::pBuffer);
  }

  // Player count
  const INDEX ctPlayers = _pNetwork->ga_srvServer.GetPlayersCount();
  const INDEX ctMaxPlayers = _pNetwork->ga_sesSessionState.ses_ctMaxPlayers;

  // Status request
  if (pStatus != NULL) {
    // Get location
    extern CTString net_strLocalHost;
    CTString strLocation = net_strLocalHost;

    if (strLocation == "") {
      strLocation = "Heartland";
    }

    // Retrieve symbols once
    const INDEX symptrFF    = _pShell->GetINDEX("gam_bFriendlyFire");
    const INDEX symptrWeap  = _pShell->GetINDEX("gam_bWeaponsStay");
    const INDEX symptrAmmo  = _pShell->GetINDEX("gam_bAmmoStays");
    const INDEX symptrVital = _pShell->GetINDEX("gam_bHealthArmorStays");
    const INDEX symptrHP    = _pShell->GetINDEX("gam_bAllowHealth");
    const INDEX symptrAR    = _pShell->GetINDEX("gam_bAllowArmor");
    const INDEX symptrIA    = _pShell->GetINDEX("gam_bInfiniteAmmo");
    const INDEX symptrResp  = _pShell->GetINDEX("gam_bRespawnInPlace");

    // Compose status response
    CTString strPacket;
    strPacket.PrintF(_strStatusResponseFormat,
      sam_strGameName, _SE_VER_STRING, strLocation.ConstData(), Game_SessionName, net_iPort,
      _pNetwork->ga_pWorld->wo_strName.ConstData(), Query_GetCurrentGameTypeName().ConstData(),
      ctPlayers, ctMaxPlayers, symptrFF, symptrWeap, symptrAmmo,
      symptrVital, symptrHP, symptrAR, symptrIA, symptrResp);

    // Go through server players
    for (INDEX i = 0; i < ctPlayers; i++) {
      CPlayerTarget &plt = _pNetwork->ga_sesSessionState.ses_apltPlayers[i];
      CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[i];

      if (plt.plt_bActive) {
        // Get info about an individual player
        CTString strPlayer;
        plt.plt_penPlayerEntity->GetGameAgentPlayerInfo(plb.plb_Index, strPlayer);

        // If not enough space for the next player info
        if (strPacket.Length() + strPlayer.Length() > 2048) {
          // Send existing packet and reset it
          IQuery::SendReply(strPacket);
          strPacket = "";
        }

        // Append player info
        strPacket += strPlayer;
      }
    }

    // Send the packet
    strPacket += "\\final\\\\queryid\\333.1";
    IQuery::SendReply(strPacket);

    if (ms_bDebugOutput) {
      CPrintF("Sending status answer:\n%s\n", strPacket.ConstData());
    }

  // Information request
  } else if (pInfo != NULL) {
    // Send information response
    CTString strPacket;
    strPacket.PrintF("\\hostname\\%s\\hostport\\%hu\\mapname\\%s\\gametype\\%s"
      "\\numplayers\\%d\\maxplayers\\%d\\gamemode\\openplaying\\final\\"
      "\\queryid\\8.1",
      Game_SessionName, net_iPort,
      _pNetwork->ga_pWorld->wo_strName.ConstData(), Query_GetCurrentGameTypeName().ConstData(),
      ctPlayers, ctMaxPlayers);

    IQuery::SendReply(strPacket);

    if (ms_bDebugOutput) {
      CPrintF("Sending info answer:\n%s\n", strPacket.ConstData());
    }

  // Basic request
  } else if (pBasic != NULL) {
    // Get location
    extern CTString net_strLocalHost;
    CTString strLocation = net_strLocalHost;

    if (strLocation == "") {
      strLocation = "Heartland";
    }

    // Send basic response
    CTString strPacket;
    strPacket.PrintF("\\gamename\\%s\\gamever\\%s\\location\\EU\\final\\" "\\queryid\\1.1",
      sam_strGameName, _SE_VER_STRING, strLocation.ConstData()); // [Cecil] NOTE: Unused location

    IQuery::SendReply(strPacket);

    if (ms_bDebugOutput) {
      CPrintF("Sending basic answer:\n%s\n", strPacket.ConstData());
    }

  // Player status request
  } else if (pPlayers != NULL) {
    // Compose player status response
    CTString strPacket;
    strPacket = "";

    // Go through server players
    for (INDEX i = 0; i < ctPlayers; i++) {
      CPlayerTarget &plt = _pNetwork->ga_sesSessionState.ses_apltPlayers[i];
      CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[i];

      if (plt.plt_bActive) {
        // Get info about an individual player
        CTString strPlayer;
        plt.plt_penPlayerEntity->GetGameAgentPlayerInfo(plb.plb_Index, strPlayer);

        // If not enough space for the next player info
        if (strPacket.Length() + strPlayer.Length() > 2048) {
          // Send existing packet and reset it
          IQuery::SendReply(strPacket);
          strPacket = "";
        }

        // Append player info
        strPacket += strPlayer;
      }
    }

    // Send the packet
    strPacket += "\\final\\\\queryid\\6.1";
    IQuery::SendReply(strPacket);

    if (ms_bDebugOutput) {
      CPrintF("Sending players answer:\n%s\n", strPacket.ConstData());
    }

  // Validation request
  } else if (pSecure != NULL) {
    UBYTE *pValidateKey = gsseckey((UBYTE *)(strData + 8), (UBYTE *)SAM_MS_KEY, 0);

    // Send validation response
    CTString strPacket;
    strPacket.PrintF("\\validate\\%s\\final\\" "\\queryid\\2.1", pValidateKey);

    IQuery::SendReply(strPacket);

    if (ms_bDebugOutput) {
      CPrintF("Sending validation answer:\n%s\n", strPacket.ConstData());
    }

  // Unknown request
  } else if (ms_bDebugOutput) {
    CPrintF("Unknown query server request!\n"
            "Data (%d bytes): %s\n", iLength, strData);
  }
};
