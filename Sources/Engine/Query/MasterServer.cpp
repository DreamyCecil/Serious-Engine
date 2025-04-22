/* Copyright (c) 2022-2024 Dreamy Cecil
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

#include <Engine/Query/MasterServer.h>
#include <Engine/Query/QueryManager.h>
#include <Engine/Network/CommunicationInterface.h>

// When the last heartbeat has been sent
static TICK _tckLastHeartbeat = -1;

extern INDEX ms_iProtocol;
extern INDEX net_iPort;

namespace IMasterServer {

// Get current master server protocol
INDEX GetProtocol(void) {
  if (ms_iProtocol < E_MS_LEGACY || ms_iProtocol >= E_MS_MAX) {
    return E_MS_LEGACY;
  }

  return ms_iProtocol;
};

// Start the server
void OnServerStart(void) {
  if (ms_bDebugOutput) CPutString("IMasterServer::OnServerStart()\n");

  // Initialize as a server
  IQuery::bServer = TRUE;
  IQuery::bInitialized = TRUE;

  // Send opening packet to the master server
  switch (GetProtocol()) {
    case E_MS_LEGACY: {
      CTString strPacket;
      strPacket.PrintF("\\heartbeat\\%hu\\gamename\\%s", (net_iPort + 1), SAM_MS_NAME);

      IQuery::SendPacket(strPacket);
    } break;

    case E_MS_DARKPLACES: {
      CTString strPacket;
      strPacket.PrintF("\xFF\xFF\xFF\xFFheartbeat DarkPlaces\x0A");

      IQuery::SendPacket(strPacket);
    } break;

    case E_MS_GAMEAGENT: {
      IQuery::SendPacket("q");
    } break;
  }
};

// Stop the server
void OnServerEnd(void) {
  // Not initialized
  if (!IQuery::bInitialized) {
    return;
  }

  if (ms_bDebugOutput) CPutString("IMasterServer::OnServerEnd()\n");

  const INDEX iProtocol = GetProtocol();

  // Send double heartbeat for Dark Places
  if (iProtocol == E_MS_DARKPLACES) {
    SendHeartbeat(0);
    SendHeartbeat(0);

  // Send server closing packet to anything but GameAgent
  } else if (iProtocol == E_MS_LEGACY) {
    CTString strPacket;
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\%s\\statechanged", (net_iPort + 1), SAM_MS_NAME);
    IQuery::SendPacket(strPacket);

    if (ms_bDebugOutput) {
      CPrintF("Server end:\n%s\n", strPacket.ConstData());
    }
  }

  // Close the socket
  IQuery::CloseWinsock();
  IQuery::bInitialized = FALSE;
};

// Server update step
void OnServerUpdate(void) {
  // Not usable
  if (!IQuery::IsSocketUsable()) {
    return;
  }

  //if (ms_bDebugOutput) CPutString("IMasterServer::OnServerUpdate()\n");

  // Receive new packet
  memset(&IQuery::pBuffer[0], 0, 2050);
  INDEX iLength = IQuery::ReceivePacket();

  // If there's any data
  if (iLength > 0) {
    if (ms_bDebugOutput) {
      CPrintF("Received packet (%d bytes)\n", iLength);
    }

    // Parse received packet
    _aProtocols[GetProtocol()]->ServerParsePacket(iLength);
  }

  // Send a heartbeat every 150 seconds
  if (_pTimer->GetRealTime() - _tckLastHeartbeat >= SecToTicks(150)) {
    SendHeartbeat(0);
  }
};

// Server state has changed
void OnServerStateChanged(void) {
  // Not initialized
  if (!IQuery::bInitialized) {
    return;
  }

  if (ms_bDebugOutput) CPutString("IMasterServer::OnServerStateChanged()\n");

  // Notify master server about the state change
  switch (GetProtocol()) {
    // Legacy
    case E_MS_LEGACY: {
      CTString strPacket;
      strPacket.PrintF("\\heartbeat\\%hu\\gamename\\%s\\statechanged", (net_iPort + 1), SAM_MS_NAME);

      IQuery::SendPacket(strPacket);

      if (ms_bDebugOutput) {
        CPrintF("Sending state change:\n%s\n", strPacket.ConstData());
      }
    } break;

    // Nothing for Dark Places

    // GameAgent
    case E_MS_GAMEAGENT: {
      IQuery::SendPacket("u");
    } break;
  }
};

// Send heartbeat to the master server
void SendHeartbeat(INDEX iChallenge) {
  CTString strPacket;

  // Build heartbeat packet for a specific master server
  _aProtocols[GetProtocol()]->BuildHearthbeatPacket(strPacket, iChallenge);

  if (ms_bDebugOutput) {
    CPrintF("Sending heartbeat:\n%s\n", strPacket.ConstData());
  }

  // Send heartbeat to the master server
  IQuery::SendPacket(strPacket);
  _tckLastHeartbeat = _pTimer->GetRealTime();
};

// Request server list enumeration
void EnumTrigger(BOOL bInternet) {
  // The list has changed
  if (_pNetwork->ga_bEnumerationChange) {
    return;
  }

  // Request for a specific master server
  _aProtocols[GetProtocol()]->EnumTrigger(bInternet);
};

// Update enumerations from the server
void EnumUpdate(void) {
  // Not usable
  if (!IQuery::IsSocketUsable()) {
    return;
  }

  // Call update method for a specific master server
  _aProtocols[GetProtocol()]->EnumUpdate();
};

// Cancel master server enumeration
void EnumCancel(void) {
  // Not initialized
  if (!IQuery::bInitialized) {
    return;
  }

  if (ms_bDebugOutput) CPutString("IMasterServer::EnumCancel()\n");

  // Delete server requests and close the socket
  IQuery::aRequests.Clear();
  IQuery::CloseWinsock();
};

}; // namespace
