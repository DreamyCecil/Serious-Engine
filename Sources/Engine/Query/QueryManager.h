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

#ifndef CECIL_INCL_QUERYMANAGER_H
#define CECIL_INCL_QUERYMANAGER_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Query/MasterServer.h>
#include <Engine/Query/ServerRequest.h>

#if SE1_WIN
  typedef int socklen_t;
#else
  #include <fcntl.h>
  #include <netdb.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <errno.h>

  #define INVALID_SOCKET -1
  #define SOCKET_ERROR   -1
  #define closesocket close

  typedef int SOCKET;
  typedef struct hostent HOSTENT, *PHOSTENT;
  typedef struct sockaddr_in SOCKADDR_IN;
  typedef struct sockaddr    SOCKADDR;

  #define WSALastError() errno
  #define WSACleanup()
#endif

// Master server protocols
enum EMasterServers {
  E_MS_LEGACY     = 0, // GameSpy emulation (default)
  E_MS_DARKPLACES = 1, // Dark Places
  E_MS_GAMEAGENT  = 2, // GameAgent from 1.10

  E_MS_MAX,
};

// Abstract protocol base
class IAbstractProtocol {
  public:
    CTString m_strMS; // Configurable master server address

  public:
    // Get master server address
    inline const CTString &GetMS(void) const {
      return m_strMS;
    };

    virtual UWORD GetPort(void) = 0;

    virtual void BuildHearthbeatPacket(CTString &strPacket, INDEX iChallenge) = 0;
    virtual void EnumTrigger(BOOL bInternet) = 0;
    virtual void EnumUpdate(void) = 0;
    virtual void ServerParsePacket(INDEX iLength) = 0;
};

// Legacy protocol
class ILegacy : public IAbstractProtocol {
  public:
    ILegacy();

    virtual UWORD GetPort(void) {
      return 27900;
    };

    virtual void BuildHearthbeatPacket(CTString &strPacket, INDEX iChallenge);
    virtual void EnumTrigger(BOOL bInternet);
    virtual void EnumUpdate(void);
    virtual void ServerParsePacket(INDEX iLength);
};

// DarkPlaces protocol
class IDarkPlaces : public IAbstractProtocol {
  public:
    IDarkPlaces();

    virtual UWORD GetPort(void) {
      return 27950;
    };

    virtual void BuildHearthbeatPacket(CTString &strPacket, INDEX iChallenge);
    virtual void EnumTrigger(BOOL bInternet);
    virtual void EnumUpdate(void);
    virtual void ServerParsePacket(INDEX iLength);
};

// GameAgent protocol
class IGameAgent : public IAbstractProtocol {
  public:
    IGameAgent();

    virtual UWORD GetPort(void) {
      return 9005;
    };

    virtual void BuildHearthbeatPacket(CTString &strPacket, INDEX iChallenge);
    virtual void EnumTrigger(BOOL bInternet);
    virtual void EnumUpdate(void);
    virtual void ServerParsePacket(INDEX iLength);
};

// Available protocols
extern IAbstractProtocol *_aProtocols[E_MS_MAX];

// Debug output for query
ENGINE_API extern INDEX ms_bDebugOutput;

// Internal query functionality
namespace IQuery {

#pragma pack(push, 1)

// Structure housing an IP address with a port
struct Address {
  union {
    UBYTE aIP[4]; // IP address byte by byte
    ULONG ulIP; // Full IP address
  };
  UWORD uwPort; // Port

  // Add new server request from a received address
  void AddServerRequest(const char **ppBuffer, INDEX &iLength, const UWORD uwSetPort, const char *strPacket, SOCKET iSocketUDP = INVALID_SOCKET);
};

#pragma pack(pop)

extern sockaddr_in sinFrom;
extern char *pBuffer;

extern BOOL bServer;
extern BOOL bInitialized;

extern CDynamicStackArray<SServerRequest> aRequests;

// Initialize the socket
void InitWinsock(void);

// Close the socket
void CloseWinsock();

// Check if the socket is usable
BOOL IsSocketUsable(void);

// Send data packet
void SendPacket(const char *pBuffer, int iLength = -1);

// Send data packet to a specific socket address
void SendPacketTo(sockaddr_in *psin, const char *pBuffer, int iLength, SOCKET iSocket = INVALID_SOCKET);

// Send reply packet with a message
void SendReply(const CTString &strMessage);

// Receive some packet
int ReceivePacket(void);

// Set enumeration status
void SetStatus(const CTString &strStatus);

}; // namespace

// Game key and game name for the master server
#define SAM_MS_KEY "AKbna4\0"

// [Cecil] TODO: Use "serioussam" for The First Encounter
#define SAM_MS_NAME "serioussamse"

// [Cecil] FIXME: Create an interface on engine level for these variables instead of pulling them from the shell
#define sam_strGameName (_pShell->GetString("sam_strGameName").ConstData())
#define Game_SessionName (_pShell->GetString("gam_strSessionName").ConstData())
CTString Query_GetCurrentGameTypeName(void);

#endif
