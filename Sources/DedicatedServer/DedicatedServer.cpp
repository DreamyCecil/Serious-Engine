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

#include "StdAfx.h"
#include <GameMP/Game.h>

#if SE1_UNIX
  #include <signal.h>
#endif

// application state variables
BOOL _bRunning = TRUE;
static BOOL _bForceRestart = FALSE;
static BOOL _bForceNextMap = FALSE;

CTString _strSamVersion = "no version information";
INDEX ded_iMaxFPS = 100;
CTString ded_strConfig = "";
CTString ded_strLevel = "";
INDEX ded_bRestartWhenEmpty = TRUE;
FLOAT ded_tmTimeout = -1;
CTString sam_strFirstLevel = "Levels\\KarnakDemo.wld";
CTString sam_strIntroLevel = "Levels\\Intro.wld";
CTString sam_strGameName = "serioussam";

CTimerValue _tvLastLevelEnd = SQUAD(-1);

static void QuitGame(void)
{
  _bRunning = FALSE;
}

static void RestartGame(void)
{
  _bForceRestart = TRUE;
}
static void NextMap(void)
{
  _bForceNextMap = TRUE;
}


void End(void);

// limit current frame rate if neeeded

void LimitFrameRate(void)
{
  // measure passed time for each loop
  static CTimerValue tvLast(-1.0f);
  CTimerValue tvNow   = _pTimer->GetHighPrecisionTimer();
  TIME tmCurrentDelta = (tvNow-tvLast).GetSeconds();

  // limit maximum frame rate
  ded_iMaxFPS = ClampDn( ded_iMaxFPS,   1L);
  TIME tmWantedDelta  = 1.0f / ded_iMaxFPS;

  if (tmCurrentDelta < tmWantedDelta) {
    _pTimer->Suspend(ULONG((tmWantedDelta - tmCurrentDelta) * 1000.0f));
  }

  // remember new time
  tvLast = _pTimer->GetHighPrecisionTimer();
}

#if SE1_WIN

// break/close handler
BOOL WINAPI HandlerRoutine(
  DWORD dwCtrlType   //  control signal type
)
{
  if (dwCtrlType == CTRL_C_EVENT
  || dwCtrlType == CTRL_BREAK_EVENT
  || dwCtrlType == CTRL_CLOSE_EVENT
  || dwCtrlType == CTRL_LOGOFF_EVENT
  || dwCtrlType == CTRL_SHUTDOWN_EVENT) {
    _bRunning = FALSE;
  }
  return TRUE;
}

#else

// [Cecil] Stop running the server on certain signals
void UnixSignalHandler(int iSignal) {
  _bRunning = FALSE;
};

#endif // SE1_WIN

#define REFRESHTIME (0.1f)

static void LoadingHook_t(CProgressHookInfo *pphi)
{
  // measure time since last call
  static CTimerValue tvLast = SQUAD(0);
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

  if (!_bRunning) {
    ThrowF_t(TRANS("User break!"));
  }
  // if not first or final update, and not enough time passed
  if (pphi->phi_fCompleted!=0 && pphi->phi_fCompleted!=1 &&
     (tvNow-tvLast).GetSeconds() < REFRESHTIME) {
    // do nothing
    return;
  }
  tvLast = tvNow;

  // print status text
  CTString strRes;

#if SE1_WIN
  printf("\r                                                                      ");
  printf("\r%s : %3.0f%%\r", pphi->phi_strDescription.ConstData(), pphi->phi_fCompleted*100);

#else
  // [Cecil] This isn't ideal but I don't know how to make it update the same line
  printf("%s : %3.0f%%\n", pphi->phi_strDescription.ConstData(), pphi->phi_fCompleted * 100);
#endif
}

// loading hook functions
void EnableLoadingHook(void)
{
  printf("\n");
  SetProgressHook(LoadingHook_t);
}

void DisableLoadingHook(void)
{
  SetProgressHook(NULL);
  printf("\n");
}

BOOL StartGame(CTString &strLevel)
{
  _pGame->gm_aiStartLocalPlayers[0] = -1;
  _pGame->gm_aiStartLocalPlayers[1] = -1;
  _pGame->gm_aiStartLocalPlayers[2] = -1;
  _pGame->gm_aiStartLocalPlayers[3] = -1;

  _pGame->gam_strCustomLevel = strLevel;

  _pGame->gm_strNetworkProvider = "TCP/IP Server";
  CUniversalSessionProperties sp;
  _pGame->SetMultiPlayerSession(sp);
  return _pGame->NewGame( _pGame->gam_strSessionName, strLevel, sp);
}
 
void ExecScript(const CTString &str)
{
  CPrintF("Executing: '%s'\n", str.ConstData());
  CTString strCmd;
  strCmd.PrintF("include \"%s\"", str.ConstData());
  _pShell->Execute(strCmd);
}

BOOL Init(int argc, char* argv[])
{
  if (argc!=1+1 && argc!=2+1) {
    // NOTE: this cannot be translated - translations are not loaded yet
    printf("Usage: DedicatedServer <configname> [<modname>]\n"
      "This starts a server reading configs from directory 'Scripts\\Dedicated\\<configname>\\'\n");

  #if SE1_WIN
    getch();
  #endif

    exit(0);
  }

#if SE1_WIN
  SetConsoleTitleA(argv[1]);
#endif

  ded_strConfig = CTString("Scripts\\Dedicated\\")+argv[1]+"\\";

  if (argc==2+1) {
    _fnmMod = CTString("Mods\\")+argv[2]+"\\";
  }


  _strLogFile = CTString("Dedicated_")+argv[1];

  // initialize engine
  SE_InitEngine(SeriousEngineSetup(SeriousEngineSetup::E_SERVER));

//  ParseCommandLine(strCmdLine);

  // load all translation tables
  InitTranslation();
  CTFileName fnmTransTable;
  try {
    fnmTransTable = CTFILENAME("Data\\Translations\\Engine.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\Game.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\Entities.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\SeriousSam.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\Levels.txt");
    AddTranslationTable_t(fnmTransTable);

    FinishTranslationTable();
  } catch (char *strError) {
    FatalError("%s %s", fnmTransTable.ConstData(), strError);
  }

  // always disable all warnings when in serious sam
  _pShell->Execute( "con_bNoWarnings=1;");

  // declare shell symbols
  _pShell->DeclareSymbol("persistent user INDEX ded_iMaxFPS;", &ded_iMaxFPS);
  _pShell->DeclareSymbol("user void Quit(void);", &QuitGame);
  _pShell->DeclareSymbol("user CTString ded_strLevel;", &ded_strLevel);
  _pShell->DeclareSymbol("user FLOAT ded_tmTimeout;", &ded_tmTimeout);
  _pShell->DeclareSymbol("user INDEX ded_bRestartWhenEmpty;", &ded_bRestartWhenEmpty);
  _pShell->DeclareSymbol("user void Restart(void);", &RestartGame);
  _pShell->DeclareSymbol("user void NextMap(void);", &NextMap);
  _pShell->DeclareSymbol("persistent user CTString sam_strIntroLevel;",      &sam_strIntroLevel);
  _pShell->DeclareSymbol("persistent user CTString sam_strGameName;",      &sam_strGameName);
  _pShell->DeclareSymbol("user CTString sam_strFirstLevel;", &sam_strFirstLevel);

  // init game - this will load persistent symbols
  _pGame->Initialize("UserData\\Game\\DedicatedServer.gms"); // [Cecil]

  LoadStringVar(CTString("Data\\Var\\Sam_Version.var"), _strSamVersion);
  CPrintF(TRANS("Serious Sam version: %s\n"), _strSamVersion.ConstData());

#if SE1_WIN
  SetConsoleCtrlHandler(HandlerRoutine, TRUE);

#else
  signal(SIGINT, UnixSignalHandler);
  signal(SIGHUP, UnixSignalHandler);
  signal(SIGQUIT, UnixSignalHandler);
  signal(SIGTERM, UnixSignalHandler);
#endif

  // if there is a mod
  if (_fnmMod!="") {
    // execute the mod startup script
    _pShell->Execute(CTString("include \"Scripts\\Mod_startup.ini\";"));
  }

  return TRUE;
}
void End(void)
{

  // cleanup level-info subsystem
//  ClearDemosList();

  // end game
  _pGame->End();

  // end engine
  SE_EndEngine();
}

static INDEX iRound = 1;
static BOOL _bHadPlayers = 0;
static BOOL _bRestart = 0;
CTString strBegScript;
CTString strEndScript;

void RoundBegin(void)
{
  // repeat generate script names
  FOREVER {
    strBegScript.PrintF("%s%d_begin.ini", ded_strConfig.ConstData(), iRound);
    strEndScript.PrintF("%s%d_end.ini",   ded_strConfig.ConstData(), iRound);
    // if start script exists
    if (FileExists(strBegScript)) {
      // stop searching
      break;

    // if start script doesn't exist
    } else {
      // if this is first round
      if (iRound==1) {
        // error
        CPrintF(TRANS("No scripts present!\n"));
        _bRunning = FALSE;
        return;
      }
      // try again with first round
      iRound = 1;
    }
  }
  
  // run start script
  ExecScript(strBegScript);

  // start the level specified there
  if (ded_strLevel=="") {
    CPrintF(TRANS("ERROR: No next level specified!\n"));
    _bRunning = FALSE;
  } else {
    EnableLoadingHook();
    StartGame(ded_strLevel);
    _bHadPlayers = 0;
    _bRestart = 0;
    DisableLoadingHook();
    _tvLastLevelEnd = SQUAD(-1);
    CPrintF(TRANS("\nALL OK: Dedicated server is now running!\n"));
    CPrintF(TRANS("Use Ctrl+C to shutdown the server.\n"));
    CPrintF(TRANS("DO NOT use the 'Close' button, it might leave the port hanging!\n\n"));
  }
}

void ForceNextMap(void)
{
  EnableLoadingHook();
  StartGame(ded_strLevel);
  _bHadPlayers = 0;
  _bRestart = 0;
  DisableLoadingHook();
  _tvLastLevelEnd = SQUAD(-1);
}

void RoundEnd(void)
{
  CPrintF("end of round---------------------------\n");

  ExecScript(strEndScript);
  iRound++;
}

// do the main game loop and render screen
void DoGame(void)
{
#if SE1_SINGLE_THREAD
  // [Cecil] Run timer logic in the same thread
  _pTimer->HandleTimerHandlers();
#endif

  // do the main game loop
  if( _pGame->gm_bGameOn) {
    _pGame->GameMainLoop();

    // if any player is connected
    if (_pGame->GetPlayersCount()) {
      if (!_bHadPlayers) {
        // unpause server
        if (_pNetwork->IsPaused()) {
          _pNetwork->TogglePause();
        }
      }
      // remember that
      _bHadPlayers = 1;
    // if no player is connected, 
    } else {
      // if was before
      if (_bHadPlayers) {
        // make it restart
        _bRestart = TRUE;
      // if never had any player yet
      } else {
        // keep the server paused
        if (!_pNetwork->IsPaused()) {
          _pNetwork->TogglePause();
        }
      }
    }

  // if game is not started
  } else {
    // just handle broadcast messages
    _pNetwork->GameInactive();
  }

  // limit current frame rate if needed
  LimitFrameRate();
}

int SubMain(int argc, char* argv[])
{

  // initialize
  if( !Init(argc, argv)) {
    End();
    return -1;
  }

  // initialy, application is running
  _bRunning = TRUE;

  // execute dedicated server startup script
  ExecScript(CTFILENAME("Scripts\\Dedicated_startup.ini"));
  // execute startup script for this config
  ExecScript(ded_strConfig+"init.ini");
  // start first round
  RoundBegin();

  // while it is still running
  while( _bRunning)
  {
    // do the main game loop
    DoGame();

    // if game is finished
    if (_pNetwork->IsGameFinished()) {
      // if not yet remembered end of level
      if (_tvLastLevelEnd.tv_llValue<0) {
        // remember end of level
        _tvLastLevelEnd = _pTimer->GetHighPrecisionTimer();
        // finish this round
        RoundEnd();
      // if already remembered
      } else {
        // if time is out
        if ((_pTimer->GetHighPrecisionTimer()-_tvLastLevelEnd).GetSeconds()>ded_tmTimeout) {
          // start next round
          RoundBegin();
        }
      }
    }

    if (_bRestart||_bForceRestart) {
      if (ded_bRestartWhenEmpty||_bForceRestart) {
        _bForceRestart = FALSE;
        _bRestart = FALSE;
        RoundEnd();
        CPrintF(TRANS("\nNOTE: Restarting server!\n\n"));
        RoundBegin();
      } else {
        _bRestart = FALSE;
        _bHadPlayers = FALSE;
      }
    }
    if (_bForceNextMap) {
      ForceNextMap();
      _bForceNextMap = FALSE;
    }

  } // end of main application loop

  _pGame->StopGame();

  End();

  return 0;
}


int main(int argc, char* argv[])
{
  int iResult;
  CTSTREAM_BEGIN {
    iResult = SubMain(argc, argv);
  } CTSTREAM_END;

  return iResult;
}

