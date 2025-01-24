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

#include <Engine/CurrentVersion.h>
#include "CmdLine.h"

CTString cmd_strWorld = "";  // world to load
INDEX cmd_iGoToMarker = -1;  // marker to go to
CTString cmd_strScript = ""; // script to execute
CTString cmd_strServer = ""; // server to connect to
INDEX cmd_iPort = -1;     // port to connect to
CTString cmd_strPassword = ""; // network password
BOOL cmd_bServer = FALSE;  // set to run as server
BOOL cmd_bQuickJoin = FALSE; // do not ask for players and network settings

// [Cecil] Command line options as functions
static void OptionLevel(const CommandLineArgs_t &aArgs) {
  cmd_strWorld = aArgs[0];
};

static void OptionServer(const CommandLineArgs_t &) {
  cmd_bServer = TRUE;
};

static void OptionQuickJoin(const CommandLineArgs_t &) {
  cmd_bQuickJoin = TRUE;
};

static void OptionPassword(const CommandLineArgs_t &aArgs) {
  cmd_strPassword = aArgs[0];
};

static void OptionConnect(const CommandLineArgs_t &aArgs) {
  cmd_strServer = aArgs[0];
  size_t iColon = cmd_strServer.Find(':');

  if (iColon != CTString::npos) {
    CTString strServer, strPort;
    cmd_strServer.Split((INDEX)iColon, strServer, strPort);

    cmd_strServer = strServer;
    strPort.ScanF(":%d", &cmd_iPort);
  }
};

static void OptionScript(const CommandLineArgs_t &aArgs) {
  cmd_strScript = aArgs[0];
};

static void OptionGoTo(const CommandLineArgs_t &aArgs) {
  cmd_iGoToMarker = strtol(aArgs[0].ConstData(), NULL, 10);
};

// [Cecil] Register command line functions
void SetupCommandLine(CommandLineSetup &cmd) {
  cmd.AddCommand("+level",     &OptionLevel,     1);
  cmd.AddCommand("+server",    &OptionServer,    0);
  cmd.AddCommand("+quickjoin", &OptionQuickJoin, 0);
  cmd.AddCommand("+password",  &OptionPassword,  1);
  cmd.AddCommand("+connect",   &OptionConnect,   1);
  cmd.AddCommand("+script",    &OptionScript,    1);
  cmd.AddCommand("+goto",      &OptionGoTo,      1);
};
