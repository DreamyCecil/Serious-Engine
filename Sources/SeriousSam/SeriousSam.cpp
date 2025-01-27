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

#if SE1_WIN
  #include <process.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <GameMP/Game.h>
#include "resource.h"
#include "MainWindow.h"
#include "GLSettings.h"
#include "LevelInfo.h"
#include "CmdLine.h"
#include "Credits.h"

// [Cecil] Screen resolution lists and window modes
#include "ScreenResolutions.h"

// application state variables
BOOL _bRunning = TRUE;
BOOL _bQuitScreen = TRUE;
BOOL bMenuActive = FALSE;
BOOL bMenuRendering = FALSE;

extern BOOL _bDefiningKey;
static BOOL _bReconsiderInput = FALSE;

// [Cecil] Computer screen resolution
PIX2D _vpixScreenRes = PIX2D(0, 0);

static INDEX sam_iMaxFPSActive   = 500;
static INDEX sam_iMaxFPSInactive = 10;
static INDEX sam_bPauseOnMinimize = TRUE; // auto-pause when window has been minimized
INDEX sam_bWideScreen = FALSE;
FLOAT sam_fPlayerOffset = 0.0f;

// display mode settings
INDEX sam_iWindowMode = E_WM_BORDERLESS; // [Cecil] Different window modes; borderless window by default
// [Cecil] Set monitor resolution by default
INDEX sam_iScreenSizeI = -1;  // current size of the window
INDEX sam_iScreenSizeJ = -1;  // current size of the window
INDEX sam_iDisplayDepth  = 0;  // 0==default, 1==16bit, 2==32bit
INDEX sam_iDisplayAdapter = 0; 
INDEX sam_iGfxAPI = 0;         // 0==OpenGL
INDEX sam_bFirstStarted = FALSE;
FLOAT sam_tmDisplayModeReport = 5.0f;
INDEX sam_bShowAllLevels = FALSE;
INDEX sam_bMentalActivated = FALSE;

// network settings
CTString sam_strNetworkSettings = "";
// command line
CTString sam_strCommandLine = "";

// 0...app started for the first time
// 1...all ok
// 2...automatic fallback
static INDEX _iDisplayModeChangeFlag = 0;
static TIME _tmDisplayModeChanged = 100.0f; // when display mode was last changed

// rendering preferences for automatic settings
INDEX sam_iVideoSetup = 1;  // 0==speed, 1==normal, 2==quality, 3==custom
// automatic adjustment of audio quality
BOOL sam_bAutoAdjustAudio = TRUE;

INDEX sam_bAutoPlayDemos = TRUE;
static INDEX _bInAutoPlayLoop = TRUE;

// menu calling
INDEX sam_bMenuSave     = FALSE;
INDEX sam_bMenuLoad     = FALSE;
INDEX sam_bMenuControls = FALSE;
INDEX sam_bMenuHiScore  = FALSE;
INDEX sam_bToggleConsole = FALSE;
INDEX sam_iStartCredits = FALSE;

// for mod re-loading
CTFileName _fnmModToLoad = CTString("");
CTString _strModServerJoin = CTString("");
CTString _strURLToVisit = CTString("");


// state variables fo addon execution
// 0 - nothing
// 1 - start (invoke console)
// 2 - console invoked, waiting for one redraw
INDEX _iAddonExecState = 0;
CTFileName _fnmAddonToExec = CTString("");

// logo textures
static CTextureObject  _toLogoCT;
static CTextureObject  _toLogoODI;
static CTextureObject  _toLogoEAX;
CTextureObject *_ptoLogoCT  = NULL;
CTextureObject *_ptoLogoODI = NULL;
CTextureObject *_ptoLogoEAX = NULL;

CTString sam_strVersion = _SE_VER_STRING;
CTString sam_strModName = TRANS("-   O P E N   S O U R C E   -");

CTString sam_strFirstLevel = "Levels\\LevelsMP\\1_0_InTheLastEpisode.wld";
CTString sam_strIntroLevel = "Levels\\LevelsMP\\Intro.wld";
CTString sam_strGameName = "serioussamse";

CTString sam_strTechTestLevel = "Levels\\LevelsMP\\TechTest.wld";
CTString sam_strTrainingLevel = "Levels\\KarnakDemo.wld";

ENGINE_API extern INDEX snd_iFormat;


// main window canvas
CDrawPort *pdp = NULL;
CDrawPort *pdpNormal = NULL;
CDrawPort *pdpWideScreen = NULL;
CViewPort *pvpViewPort = NULL;
HINSTANCE _hInstance = NULL;


static void PlayDemo(void* pArgs)
{
  CTString strDemoFilename = *NEXTARGUMENT(CTString*);
  _gmMenuGameMode = GM_DEMO;
  CTFileName fnDemo = "demos\\" + strDemoFilename + ".dem";
  extern BOOL LSLoadDemo(const CTFileName &fnm);
  LSLoadDemo(fnDemo);
}

static void ApplyRenderingPreferences(void)
{
  ApplyGLSettings(TRUE);
}

extern void ApplyVideoMode(void)
{
  StartNewMode( (GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ,
                (enum DisplayDepth)sam_iDisplayDepth, sam_iWindowMode);
}

static void BenchMark(void)
{
  _pGfx->Benchmark(pvpViewPort, pdp);
}


static void QuitGame(void)
{
  _bRunning = FALSE;
  _bQuitScreen = FALSE;
}

#if SE1_WIN

// check if another app is already running
static HANDLE _hLock = NULL;
static void DirectoryLockOn(void)
{
  // create lock filename
  static CTFileName _fnmLock;
  _fnmLock = _fnmApplicationPath+"SeriousSam.loc";
  // try to open lock file
  _hLock = CreateFileA(
    _fnmLock.ConstData(), 
    GENERIC_WRITE,
    0/*no sharing*/,
    NULL, // pointer to security attributes
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,  // file attributes
    NULL);
  // if failed
  if (_hLock==NULL || GetLastError()!=0) {
    // report warning
    CPrintF(TRANS("WARNING: SeriousSam didn't shut down properly last time!\n"));
  }
}
static void DirectoryLockOff(void)
{
  // if lock is open
  if (_hLock!=NULL) {
    // close it
    CloseHandle(_hLock);
  }
}

#else

static void DirectoryLockOn(void) {};
static void DirectoryLockOff(void) {};

#endif // SE1_WIN

void End(void);


// automaticaly manage input enable/disable toggling
static BOOL _bInputEnabled = FALSE;
void UpdateInputEnabledState(void)
{
  // do nothing if window is invalid
  if( _hwndMain==NULL) return;

  // input should be enabled if application is active
  // and no menu is active and no console is active
  BOOL bShouldBeEnabled = (!OS::IsIconic(_hwndMain) && !bMenuActive && _pGame->gm_csConsoleState == CS_OFF
                       && (_pGame->gm_csComputerState == CS_OFF || _pGame->gm_csComputerState == CS_ONINBACKGROUND))
                       || _bDefiningKey;

  // if should be turned off
  if( (!bShouldBeEnabled && _bInputEnabled) || _bReconsiderInput) {
    // disable it and remember new state
    _pInput->DisableInput(_hwndMain);
    _bInputEnabled = FALSE;
  }
  // if should be turned on
  if( bShouldBeEnabled && !_bInputEnabled) {
    // enable it and remember new state
    _pInput->EnableInput(_hwndMain);
    _bInputEnabled = TRUE;
  }
  _bReconsiderInput = FALSE;
}


// automaticaly manage pause toggling
static void UpdatePauseState(void)
{
  BOOL bShouldPause = (_gmRunningGameMode==GM_SINGLE_PLAYER) && (bMenuActive || 
                       _pGame->gm_csConsoleState ==CS_ON || _pGame->gm_csConsoleState ==CS_TURNINGON || _pGame->gm_csConsoleState ==CS_TURNINGOFF ||
                       _pGame->gm_csComputerState==CS_ON || _pGame->gm_csComputerState==CS_TURNINGON || _pGame->gm_csComputerState==CS_TURNINGOFF);
  _pNetwork->SetLocalPause(bShouldPause);
}


// limit current frame rate if neeeded
void LimitFrameRate(void)
{
  // measure passed time for each loop
  static CTimerValue tvLast(-1.0f);
  CTimerValue tvNow   = _pTimer->GetHighPrecisionTimer();
  TIME tmCurrentDelta = (tvNow-tvLast).GetSeconds();

  // limit maximum frame rate
  sam_iMaxFPSActive   = ClampDn( (INDEX)sam_iMaxFPSActive,   1L);
  sam_iMaxFPSInactive = ClampDn( (INDEX)sam_iMaxFPSInactive, 1L);
  INDEX iMaxFPS = sam_iMaxFPSActive;
  if (OS::IsIconic(_hwndMain)) iMaxFPS = sam_iMaxFPSInactive;
  if(_pGame->gm_CurrentSplitScreenCfg==CGame::SSC_DEDICATED) {
    iMaxFPS = ClampDn(iMaxFPS, 60L); // never go very slow if dedicated server
  }
  TIME tmWantedDelta = 1.0f / iMaxFPS;
  if( tmCurrentDelta<tmWantedDelta) _pTimer->Suspend( (tmWantedDelta-tmCurrentDelta)*1000.0f);
  
  // remember new time
  tvLast = _pTimer->GetHighPrecisionTimer();
}

// load first demo
void StartNextDemo(void)
{
  if (!sam_bAutoPlayDemos || !_bInAutoPlayLoop) {
    _bInAutoPlayLoop = FALSE;
    return;
  }

  // skip if no demos
  if(_lhAutoDemos.IsEmpty()) {
    _bInAutoPlayLoop = FALSE;
    return;
  }

  // get first demo level and cycle the list
  CLevelInfo *pli = LIST_HEAD(_lhAutoDemos, CLevelInfo, li_lnNode);
  pli->li_lnNode.Remove();
  _lhAutoDemos.AddTail(pli->li_lnNode);

  // if intro
  if (pli->li_fnLevel==sam_strIntroLevel) {
    // start intro
    _gmRunningGameMode = GM_NONE;
    _pGame->gm_aiStartLocalPlayers[0] = 0;
    _pGame->gm_aiStartLocalPlayers[1] = -1;
    _pGame->gm_aiStartLocalPlayers[2] = -1;
    _pGame->gm_aiStartLocalPlayers[3] = -1;
    _pGame->gm_strNetworkProvider = "Local";
    _pGame->gm_StartSplitScreenCfg = CGame::SSC_PLAY1;

    _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_NORMAL);
    _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_FLYOVER);

    CUniversalSessionProperties sp;
    _pGame->SetSinglePlayerSession(sp);

    _pGame->gm_bFirstLoading = TRUE;

    if (_pGame->NewGame( sam_strIntroLevel, sam_strIntroLevel, sp)) {
      _gmRunningGameMode = GM_INTRO;
    }
  // if not intro
  } else {
    // start the demo
    _pGame->gm_StartSplitScreenCfg = CGame::SSC_OBSERVER;
    _pGame->gm_aiStartLocalPlayers[0] = -1;
    _pGame->gm_aiStartLocalPlayers[1] = -1;
    _pGame->gm_aiStartLocalPlayers[2] = -1;
    _pGame->gm_aiStartLocalPlayers[3] = -1;
    // play the demo
    _pGame->gm_strNetworkProvider = "Local";
    _gmRunningGameMode = GM_NONE;
    if( _pGame->StartDemoPlay( pli->li_fnLevel)) {
      _gmRunningGameMode = GM_DEMO;
      CON_DiscardLastLineTimes();
    }
  }

  if (_gmRunningGameMode==GM_NONE) {
    _bInAutoPlayLoop = FALSE;
  }
}

// run web browser and view an url
void RunBrowser(const char *strUrl)
{
#if SE1_PREFER_SDL
  // [Cecil] SDL: Open URL in browser
  if (!SDL_OpenURL(strUrl)) {
    NOTHING;
  }

#else
  INT_PTR iResult = (INT_PTR)ShellExecuteA(_hwndMain, "OPEN", strUrl, NULL, NULL, SW_SHOWMAXIMIZED);
  if (iResult<32) {
    // should report error?
    NOTHING;
  }
#endif
}

void LoadAndForceTexture(CTextureObject &to, CTextureObject *&pto, const CTFileName &fnm)
{
  try {
    to.SetData_t(fnm);
    CTextureData *ptd = (CTextureData*)to.GetData();
    ptd->Force( TEX_CONSTANT);
    ptd = ptd->td_ptdBaseTexture;
    if( ptd!=NULL) ptd->Force( TEX_CONSTANT);
    pto = &to;
  } catch( char *pchrError) {
    (void*)pchrError;
    pto = NULL;
  }
}

BOOL Init(HINSTANCE hInstance, const CommandLineSetup &cmd) {
  // [Cecil] NOTE: Used in ShowSplashScreen() and for creating new game windows under Windows platforms
  _hInstance = hInstance;

  extern void ShowSplashScreen(void);
  ShowSplashScreen();

  // prepare main window
  MainWindow_Init();
  OpenMainWindowInvisible();

  // [Cecil] Parse command line arguments
  SE_ParseCommandLine(cmd);

  // initialize engine
  SeriousEngineSetup se1setup("Serious Sam");
  se1setup.eAppType = SeriousEngineSetup::E_GAME;
  SE_InitEngine(se1setup);

  // [Cecil] Determine adjustable gamma
  SE_DetermineGamma();

  // [Cecil] Get screen resolution
  _vpixScreenRes = _pGfx->GetMonitorResolution();

  SE_LoadDefaultFonts();

  // lock the directory
  DirectoryLockOn();

  // load all translation tables
  InitTranslation();
  try {
    AddTranslationTablesDir_t(CTString("Data\\Translations\\"), CTString("*.txt"));
    FinishTranslationTable();
  } catch (char *strError) {
    FatalError("%s", strError);
  }

  // always disable all warnings when in serious sam
  _pShell->Execute( "con_bNoWarnings=1;");

  // declare shell symbols
  _pShell->DeclareSymbol("user void PlayDemo(CTString);", &PlayDemo);
  _pShell->DeclareSymbol("persistent INDEX sam_iWindowMode;",   &sam_iWindowMode); // [Cecil] Window modes
  _pShell->DeclareSymbol("persistent INDEX sam_iScreenSizeI;",  &sam_iScreenSizeI);
  _pShell->DeclareSymbol("persistent INDEX sam_iScreenSizeJ;",  &sam_iScreenSizeJ);
  _pShell->DeclareSymbol("persistent INDEX sam_iDisplayDepth;", &sam_iDisplayDepth);
  _pShell->DeclareSymbol("persistent INDEX sam_iDisplayAdapter;", &sam_iDisplayAdapter);
  _pShell->DeclareSymbol("persistent INDEX sam_iGfxAPI;",         &sam_iGfxAPI);
  _pShell->DeclareSymbol("persistent INDEX sam_bFirstStarted;", &sam_bFirstStarted);
  _pShell->DeclareSymbol("persistent INDEX sam_bAutoAdjustAudio;", &sam_bAutoAdjustAudio);
  _pShell->DeclareSymbol("persistent user INDEX sam_bWideScreen;", &sam_bWideScreen);
  _pShell->DeclareSymbol("persistent user FLOAT sam_fPlayerOffset;",  &sam_fPlayerOffset);
  _pShell->DeclareSymbol("persistent user INDEX sam_bAutoPlayDemos;", &sam_bAutoPlayDemos);
  _pShell->DeclareSymbol("persistent user INDEX sam_iMaxFPSActive;",    &sam_iMaxFPSActive);
  _pShell->DeclareSymbol("persistent user INDEX sam_iMaxFPSInactive;",  &sam_iMaxFPSInactive);
  _pShell->DeclareSymbol("persistent user INDEX sam_bPauseOnMinimize;", &sam_bPauseOnMinimize);
  _pShell->DeclareSymbol("persistent user FLOAT sam_tmDisplayModeReport;",   &sam_tmDisplayModeReport);
  _pShell->DeclareSymbol("persistent user CTString sam_strNetworkSettings;", &sam_strNetworkSettings);
  _pShell->DeclareSymbol("persistent user CTString sam_strIntroLevel;",      &sam_strIntroLevel);
  _pShell->DeclareSymbol("persistent user CTString sam_strGameName;",      &sam_strGameName);
  _pShell->DeclareSymbol("user CTString sam_strVersion;",    &sam_strVersion);
  _pShell->DeclareSymbol("user CTString sam_strFirstLevel;", &sam_strFirstLevel);
  _pShell->DeclareSymbol("user CTString sam_strModName;", &sam_strModName);
  _pShell->DeclareSymbol("persistent INDEX sam_bShowAllLevels;", &sam_bShowAllLevels);
  _pShell->DeclareSymbol("persistent INDEX sam_bMentalActivated;", &sam_bMentalActivated);

  _pShell->DeclareSymbol("user CTString sam_strTechTestLevel;", &sam_strTechTestLevel);
  _pShell->DeclareSymbol("user CTString sam_strTrainingLevel;", &sam_strTrainingLevel);
  
  _pShell->DeclareSymbol("user void Quit(void);", &QuitGame);

  _pShell->DeclareSymbol("persistent user INDEX sam_iVideoSetup;",     &sam_iVideoSetup);
  _pShell->DeclareSymbol("user void ApplyRenderingPreferences(void);", &ApplyRenderingPreferences);
  _pShell->DeclareSymbol("user void ApplyVideoMode(void);",            &ApplyVideoMode);
  _pShell->DeclareSymbol("user void Benchmark(void);", &BenchMark);

  _pShell->DeclareSymbol("user INDEX sam_bMenuSave;",     &sam_bMenuSave);
  _pShell->DeclareSymbol("user INDEX sam_bMenuLoad;",     &sam_bMenuLoad);
  _pShell->DeclareSymbol("user INDEX sam_bMenuControls;", &sam_bMenuControls);
  _pShell->DeclareSymbol("user INDEX sam_bMenuHiScore;",  &sam_bMenuHiScore);
  _pShell->DeclareSymbol("user INDEX sam_bToggleConsole;",&sam_bToggleConsole);
  _pShell->DeclareSymbol("INDEX sam_iStartCredits;", &sam_iStartCredits);

  _pGame->Initialize("UserData\\Game\\SeriousSam.gms"); // [Cecil]
  _pGame->LCDInit();

  if( sam_bFirstStarted) {
    InfoMessage("%s", TRANS(
      "SeriousSam is starting for the first time.\n"
      "If you experience any problems, please consult\n"
      "ReadMe file for troubleshooting information."));
  }

  // initialize sound library
  snd_iFormat = Clamp( snd_iFormat, (INDEX)CSoundLibrary::SF_NONE, (INDEX)CSoundLibrary::SF_44100_16);
  _pSound->SetFormat( (enum CSoundLibrary::SoundFormat)snd_iFormat);

  if (sam_bAutoAdjustAudio) {
    _pShell->Execute("include \"Scripts\\Addons\\SFX-AutoAdjust.ini\"");
  }

  // execute script given on command line
  if (cmd_strScript!="") {
    CPrintF("Command line script: '%s'\n", cmd_strScript.ConstData());
    CTString strCmd;
    strCmd.PrintF("include \"%s\"", cmd_strScript.ConstData());
    _pShell->Execute(strCmd);
  }
  
  // load logo textures
  LoadAndForceTexture(_toLogoCT,   _ptoLogoCT,   CTFILENAME("Textures\\Logo\\LogoCT.tex"));
  LoadAndForceTexture(_toLogoODI,  _ptoLogoODI,  CTFILENAME("Textures\\Logo\\GodGamesLogo.tex"));
  LoadAndForceTexture(_toLogoEAX,  _ptoLogoEAX,  CTFILENAME("Textures\\Logo\\LogoEAX.tex"));

  // !! NOTE !! Re-enable these to allow mod support.
  LoadStringVar(CTString("Data\\Var\\Sam_Version.var"), sam_strVersion);
  LoadStringVar(CTString("Data\\Var\\ModName.var"), sam_strModName);
  CPrintF(TRANS("Serious Sam version: %s\n"), sam_strVersion.ConstData());
  CPrintF(TRANS("Active mod: %s\n"), sam_strModName.ConstData());
  InitializeMenus();      
  
  // if there is a mod
  if (_fnmMod!="") {
    // execute the mod startup script
    _pShell->Execute(CTString("include \"Scripts\\Mod_startup.ini\";"));
  }

  // init gl settings module
  InitGLSettings();

  // init level-info subsystem
  LoadLevelsList();
  LoadDemosList();

  // [Cecil] NOTE: Needs to be hidden *before* creating a game window, otherwise it breaks under Linux somehow
  extern void HideSplashScreen(void);
  HideSplashScreen();

  // apply application mode
  StartNewMode( (GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ,
                (enum DisplayDepth)sam_iDisplayDepth, sam_iWindowMode);

  // set default mode reporting
  if( sam_bFirstStarted) {
    _iDisplayModeChangeFlag = 0;
    sam_bFirstStarted = FALSE;
  }

  if (cmd_strPassword!="") {
    _pShell->SetString("net_strConnectPassword", cmd_strPassword);
  }

  // if connecting to server from command line
  if (cmd_strServer!="") {
    CTString strPort = "";
    if (cmd_iPort>0) {
      _pShell->SetINDEX("net_iPort", cmd_iPort);
      strPort.PrintF(":%d", cmd_iPort);
    }
    CPrintF(TRANS("Command line connection: '%s%s'\n"), cmd_strServer.ConstData(), strPort.ConstData());
    // go to join menu
    _pGame->gam_strJoinAddress = cmd_strServer;
    if (cmd_bQuickJoin) {
      extern void JoinNetworkGame(void);
      JoinNetworkGame();
    } else {
      StartMenus("join");
    }
  // if starting world from command line
  } else if (cmd_strWorld!="") {
    CPrintF(TRANS("Command line world: '%s'\n"), cmd_strWorld.ConstData());
    // try to start the game with that level
    try {
      if (cmd_iGoToMarker>=0) {
        CPrintF(TRANS("Command line marker: %d\n"), cmd_iGoToMarker);
        CTString strCommand;
        strCommand.PrintF("cht_iGoToMarker = %d;", cmd_iGoToMarker);
        _pShell->Execute(strCommand);
      }
      _pGame->gam_strCustomLevel = cmd_strWorld;
      if (cmd_bServer) {
        extern void StartNetworkGame(void);
        StartNetworkGame();
      } else {
        extern void StartSinglePlayerGame(void);
        StartSinglePlayerGame();
      }
    } catch (char *strError) {
      CPrintF(TRANS("Cannot start '%s': '%s'\n"), cmd_strWorld.ConstData(), strError);
    }
  // if no relevant starting at command line
  } else {
    StartNextDemo();
  }
  return TRUE;
}


void End(void)
{
  _pGame->DisableLoadingHook();
  // cleanup level-info subsystem
  ClearLevelsList();
  ClearDemosList();

  // destroy the main window and its canvas
  if (pvpViewPort!=NULL) {
    _pGfx->DestroyWindowCanvas( pvpViewPort);
    pvpViewPort = NULL;
    pdpNormal   = NULL;
  }

  // [Cecil] Restore gamma adjustments
  SE_RestoreGamma();

  CloseMainWindow();
  MainWindow_End();
  DestroyMenus();
  _pGame->End();
  _pGame->LCDEnd();
  // unlock the directory
  DirectoryLockOff();
  SE_EndEngine();
}


// print display mode info if needed
void PrintDisplayModeInfo(void)
{
  // skip if timed out
  if( _pTimer->GetRealTimeTick() > (_tmDisplayModeChanged+sam_tmDisplayModeReport)) return;

  // cache some general vars
  SLONG slDPWidth  = pdp->GetWidth();
  SLONG slDPHeight = pdp->GetHeight();
  if( pdp->IsDualHead()) slDPWidth/=2;

  CDisplayMode dm;
  dm.dm_pixSizeI = slDPWidth;
  dm.dm_pixSizeJ = slDPHeight;
  // determine proper text scale for statistics display
  FLOAT fTextScale = (FLOAT)slDPHeight / 480.0f; // [Cecil] Height ratio

  // get resolution
  CTString strRes;
  extern CTString _strPreferencesDescription;
  strRes.PrintF( "%dx%dx%s", slDPWidth, slDPHeight, _pGfx->gl_dmCurrentDisplayMode.DepthString().ConstData());
  if( dm.IsDualHead())   strRes += TRANS(" DualMonitor");
  if( dm.IsWideScreen()) strRes += TRANS(" WideScreen");

  // [Cecil] API name
  strRes += CTString(0, " (%s)", _pGfx->GetApiName(_pGfx->GetCurrentAPI()).ConstData());

  CTString strDescr;
  strDescr.PrintF("\n%s (%s)\n", _strPreferencesDescription.ConstData(), RenderingPreferencesDescription(sam_iVideoSetup));
  strRes+=strDescr;
  // tell if application is started for the first time, or failed to set mode
  if( _iDisplayModeChangeFlag==0) {
    strRes += TRANS("Display mode set by default!");
  } else if( _iDisplayModeChangeFlag==2) {
    strRes += TRANS("Last mode set failed!");
  }

  // print it all
  pdp->SetFont( _pfdDisplayFont);
  pdp->SetTextScaling( fTextScale);
  pdp->SetTextAspect( 1.0f);
  pdp->PutText( strRes, slDPWidth*0.05f, slDPHeight*0.85f, _pGame->LCDGetColor(C_GREEN|255, "display mode"));
}

// do the main game loop and render screen
void DoGame(void)
{
#if SE1_SINGLE_THREAD
  // [Cecil] Run timer logic in the same thread
  _pTimer->HandleTimerHandlers();
#endif

  // set flag if not in game
  if( !_pGame->gm_bGameOn) _gmRunningGameMode = GM_NONE;

  if ((_gmRunningGameMode == GM_DEMO  && _pNetwork->IsDemoPlayFinished())
   || (_gmRunningGameMode == GM_INTRO && _pNetwork->IsGameFinished())) {
    _pGame->StopGame();
    _gmRunningGameMode = GM_NONE;

    // load next demo
    StartNextDemo();
    if (!_bInAutoPlayLoop) {
      // start menu
      StartMenus();
    }
  }

  // do the main game loop
  if( _gmRunningGameMode != GM_NONE) {
    _pGame->GameMainLoop();
  // if game is not started
  } else {
    // just handle broadcast messages
    _pNetwork->GameInactive();
  }

  if (sam_iStartCredits>0) {
    Credits_On(sam_iStartCredits);
    sam_iStartCredits = 0;
  }
  if (sam_iStartCredits<0) {
    Credits_Off();
    sam_iStartCredits = 0;
  }
  if( _gmRunningGameMode==GM_NONE) {
    Credits_Off();
    sam_iStartCredits = 0;
  }

  // redraw the view
  if (!OS::IsIconic(_hwndMain) && pdp != NULL && pdp->Lock())
  {
    if( _gmRunningGameMode!=GM_NONE && !bMenuActive ) {
      // handle pretouching of textures and shadowmaps
      pdp->Unlock();
      _pGame->GameRedrawView( pdp, (_pGame->gm_csConsoleState!=CS_OFF || bMenuActive)?0:GRV_SHOWEXTRAS);
      pdp->Lock();
      _pGame->ComputerRender(pdp);
      pdp->Unlock();
      CDrawPort dpScroller(pdp, TRUE);
      dpScroller.Lock();
      if (Credits_Render(&dpScroller)==0) {
        Credits_Off();
      }
      dpScroller.Unlock();
      pdp->Lock();
    } else {
      pdp->Fill( _pGame->LCDGetColor(C_dGREEN|CT_OPAQUE, "bcg fill"));
    }

    // do menu
    if( bMenuRendering) {
      // clear z-buffer
      pdp->FillZBuffer( ZBUF_BACK);
      // remember if we should render menus next tick
      bMenuRendering = DoMenu(pdp);
    }

    // print display mode info if needed
    PrintDisplayModeInfo();

    // render console
    _pGame->ConsoleRender(pdp);

    // done with all
    pdp->Unlock();

    // clear upper and lower parts of screen if in wide screen mode
    if( pdp==pdpWideScreen && pdpNormal->Lock()) {
      const PIX pixWidth  = pdpWideScreen->GetWidth();
      const PIX pixHeight = (pdpNormal->GetHeight() - pdpWideScreen->GetHeight()) /2;
      const PIX pixJOfs   = pixHeight + pdpWideScreen->GetHeight()-1;
      pdpNormal->Fill( 0, 0,       pixWidth, pixHeight, C_BLACK|CT_OPAQUE);
      pdpNormal->Fill( 0, pixJOfs, pixWidth, pixHeight, C_BLACK|CT_OPAQUE);
      pdpNormal->Unlock();
    }
    // show
    pvpViewPort->SwapBuffers();
  }
}


void TeleportPlayer(int iPosition)
{
  CTString strCommand;
  strCommand.PrintF( "cht_iGoToMarker = %d;", iPosition);
  _pShell->Execute(strCommand);
}

// [Cecil] Check any significant controller button
static BOOL AnyControllerButton(const OS::SE1Event &event) {
  if (event.type == WM_CTRLBUTTONDOWN) {
    switch (event.ctrl.action) {
      case SDL_GAMEPAD_BUTTON_EAST: case SDL_GAMEPAD_BUTTON_SOUTH:
      case SDL_GAMEPAD_BUTTON_WEST: case SDL_GAMEPAD_BUTTON_NORTH:
      case SDL_GAMEPAD_BUTTON_BACK: case SDL_GAMEPAD_BUTTON_GUIDE: case SDL_GAMEPAD_BUTTON_START:
        return TRUE;
    }
  }

  return FALSE;
};

CTextureObject _toStarField;
static FLOAT _fLastVolume = 1.0f;
void RenderStarfield(CDrawPort *pdp, FLOAT fStrength)
{
  CTextureData *ptd = (CTextureData *)_toStarField.GetData();
  // skip if no texture
  if(ptd==NULL) return;

  PIX pixSizeI = pdp->GetWidth();
  PIX pixSizeJ = pdp->GetHeight();
  FLOAT fStretch = (FLOAT)pixSizeJ / 480.0f; // [Cecil] Height ratio
  fStretch*=FLOAT(ptd->GetPixWidth())/ptd->GetWidth();

  PIXaabbox2D boxScreen(PIX2D(0,0), PIX2D(pixSizeI, pixSizeJ));
  MEXaabbox2D boxTexture(MEX2D(0, 0), MEX2D(pixSizeI/fStretch, pixSizeJ/fStretch));
  pdp->PutTexture(&_toStarField, boxScreen, boxTexture, LerpColor(C_BLACK, C_WHITE, fStrength)|CT_OPAQUE);
}


FLOAT RenderQuitScreen(CDrawPort *pdp, CViewPort *pvp)
{
  CDrawPort dpQuit(pdp, TRUE);
  CDrawPort dpWide;
  dpQuit.MakeWideScreen(&dpWide);
  // redraw the view
  if (!dpWide.Lock()) {
    return 0;
  }

  dpWide.Fill(C_BLACK|CT_OPAQUE);
  RenderStarfield(&dpWide, _fLastVolume);
  
  FLOAT fVolume = Credits_Render(&dpWide);
  _fLastVolume = fVolume;

  dpWide.Unlock();
  pvp->SwapBuffers();

  return fVolume;
}
void QuitScreenLoop(void)
{

  Credits_On(3);
  CSoundObject soMusic;
  try {
    _toStarField.SetData_t(CTFILENAME("Textures\\Background\\Night01\\Stars01.tex"));
    soMusic.Play_t(CTFILENAME("Music\\Credits.mp3"), SOF_NONGAME|SOF_MUSIC|SOF_LOOP);
  } catch (char *strError) {
    CPrintF("%s\n", strError);
  }
  // while it is still running
  FOREVER {
    FLOAT fVolume = RenderQuitScreen(pdp, pvpViewPort);
    if (fVolume<=0) {
      return;
    }
    // assure we can listen to non-3d sounds
    soMusic.SetVolume(fVolume, fVolume);
    _pSound->UpdateSounds();

    // [Cecil] Cross-platform events
    OS::SE1Event event;

    // Quit the quit screen on any button/key press
    while (OS::PollEvent(event)) {
      if (event.type == WM_LBUTTONDOWN || event.type == WM_RBUTTONDOWN || event.type == WM_KEYDOWN
       || AnyControllerButton(event)) {
        return;
      }
    }

    //_pTimer->Suspend(5);

  #if SE1_SINGLE_THREAD
    // [Cecil] Run timer logic in the same thread
    _pTimer->HandleTimerHandlers();
  #endif
  }
}

// [Cecil] Game application is DPI-aware
static BOOL _bDPIAware = FALSE;

// [Cecil] Make game application be aware of the DPI scaling on Windows Vista and later
static void SetDPIAwareness(void) {
#if SE1_WIN
  // Load the library
  HMODULE hUser = LoadLibraryA("User32.dll");

  if (hUser == NULL) return;

  // Try to find the DPI awareness method
  typedef BOOL (*CSetAwarenessFunc)(void);
  CSetAwarenessFunc pFunc = (CSetAwarenessFunc)GetProcAddress(hUser, "SetProcessDPIAware");

  if (pFunc == NULL) return;

  // Mark game application as DPI-aware
  _bDPIAware = pFunc();

  // Free the library
  FreeLibrary(hUser);
#endif // SE1_WIN
};

int SubMain(HINSTANCE hInstance, const CommandLineSetup &cmd) {
  // [Cecil] Set DPI awareness
  SetDPIAwareness();

  if (!Init(hInstance, cmd)) return FALSE;

  // [Cecil] Disable SDL joystick events to handle them manually alongside Windows API
  #if !SE1_PREFER_SDL
    SDL_SetJoystickEventsEnabled(false);
  #endif

  // initialy, application is running and active, console and menu are off
  _bRunning    = TRUE;
  _bQuitScreen = TRUE;
  _pGame->gm_csConsoleState  = CS_OFF;
  _pGame->gm_csComputerState = CS_OFF;
//  bMenuActive    = FALSE;
//  bMenuRendering = FALSE;
  // while it is still running
  while( _bRunning && _fnmModToLoad=="")
  {
    // [Cecil] Cross-platform events
    OS::SE1Event event;

    // While there are any messages in the message queue
    while (OS::PollEvent(event))
    {
      // [Cecil] Remap controller axes to buttons
      if (event.type == WM_CTRLAXISMOTION) {
        switch (event.ctrl.action) {
          case SDL_GAMEPAD_AXIS_LEFTX: {
            event.type = WM_CTRLBUTTONDOWN;
            event.ctrl.action = (event.ctrl.dir < 0 ? SDL_GAMEPAD_BUTTON_DPAD_LEFT : SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
            event.ctrl.dir = TRUE;
          } break;

          case SDL_GAMEPAD_AXIS_LEFTY: {
            event.type = WM_CTRLBUTTONDOWN;
            event.ctrl.action = (event.ctrl.dir < 0 ? SDL_GAMEPAD_BUTTON_DPAD_UP : SDL_GAMEPAD_BUTTON_DPAD_DOWN);
            event.ctrl.dir = TRUE;
          } break;
        }
      }

      // Stop running the game
      if (_pGame->ShouldStopRunning(event, FALSE)) {
        _bRunning = FALSE;
        _bQuitScreen = FALSE;
        break;
      }

      // Game window commands (also sent by the application itself)
      if (event.type == WM_SYSCOMMAND) {
        switch (event.window.event) {
          // Minimize the window
          case SDL_EVENT_WINDOW_MINIMIZED: {
            if (_bWindowChanging) break;
            _bWindowChanging  = TRUE;
            _bReconsiderInput = TRUE;

            // Pause game in single player
            if (sam_bPauseOnMinimize && !_pNetwork->IsPaused() && _gmRunningGameMode == GM_SINGLE_PLAYER) {
              _pNetwork->TogglePause();
            }

          #if !SE1_PREFER_SDL
            // [Cecil] Reset display mode, if in fullscreen
            if (sam_iWindowMode == E_WM_FULLSCREEN) {
              _pGfx->ResetDisplayMode();
            }

            ShowWindow(_hwndMain, SW_MINIMIZE);
          #endif
          } break;

        #if SE1_PREFER_SDL
          // If application is deactivated or minimized
          case SDL_EVENT_WINDOW_MOUSE_LEAVE:
          case SDL_EVENT_WINDOW_FOCUS_LOST: {
            // If application is running and in full screen mode
            if (!_bWindowChanging && _bRunning) {
            #if !SE1_PREFER_SDL
              // [Cecil] Minimize if in fullscreen
              if (sam_iWindowMode == E_WM_FULLSCREEN) {
                PostMessage(NULL, WM_SYSCOMMAND, SC_MINIMIZE, 0);

              // Just disable input if not in fullscreen 
              } else
            #endif
              {
                _pInput->DisableInput(_hwndMain);
              }
            }
          } break;

          // If application is activated or maximized
          case SDL_EVENT_WINDOW_MOUSE_ENTER:
          case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            // Enable input back again if needed
            _bReconsiderInput = TRUE;
          } break;

          case SDL_EVENT_WINDOW_CLOSE_REQUESTED: break;

        #else
          // Restore the window
          case SDL_EVENT_WINDOW_RESTORED: {
            if (_bWindowChanging) break;
            _bWindowChanging  = TRUE;
            _bReconsiderInput = TRUE;

            ShowWindow(_hwndMain, SW_SHOWNORMAL);

            // [Cecil] Set display mode again, if in fullscreen
            if (sam_iWindowMode == E_WM_FULLSCREEN) {
              StartNewMode((GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ,
                (DisplayDepth)sam_iDisplayDepth, sam_iWindowMode);
            }
          } break;

          // Maximize the window
          case SDL_EVENT_WINDOW_MAXIMIZED: {
            if (_bWindowChanging) break;
            _bWindowChanging  = TRUE;
            _bReconsiderInput = TRUE;

            // Open in fullscreen
            StartNewMode((GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ,
              (DisplayDepth)sam_iDisplayDepth, TRUE);

            ShowWindow(_hwndMain, SW_SHOWNORMAL);
          } break;
        #endif // !SE1_PREFER_SDL
        }
      }

      // Toggle fullscreen on Alt+Enter
      if (event.type == WM_SYSKEYDOWN && event.key.code == SE1K_RETURN && !OS::IsIconic(_hwndMain)) {
        // [Cecil] Switch between windowed and fullscreen
        StartNewMode((GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ,
          (DisplayDepth)sam_iDisplayDepth, (sam_iWindowMode != E_WM_FULLSCREEN ? E_WM_FULLSCREEN : E_WM_WINDOWED));
      }

    #if SE1_WIN && !SE1_PREFER_SDL
      // Teleport player around the level from an external application (for TechTest from Help)
      if (event.type == WM_COMMAND && event.window.event == 1001) {
        TeleportPlayer(event.window.data);

        // Restore the game window
        PostMessage(NULL, WM_SYSCOMMAND, SC_RESTORE, 0);
        continue;
      }
    #endif

      // Close chat on Escape before anything else
      if (_pGame->gm_csConsoleState == CS_TALK && _pGame->IsEscapeKeyPressed(event)) {
        _pGame->gm_csConsoleState = CS_OFF;
        continue;
      }

      // [Cecil] Moved here from the bottom of the loop to take priority
      // If any demo is playing
      if (_gmRunningGameMode == GM_DEMO || _gmRunningGameMode == GM_INTRO)
      {
        // Space, Enter, Left Click, Right Click
        const BOOL bAnyKey = ((event.type == WM_KEYDOWN && (event.key.code == SE1K_SPACE || event.key.code == SE1K_RETURN))
          || event.type == WM_LBUTTONDOWN || event.type == WM_RBUTTONDOWN);

        // [Cecil] Stop demo on some controller buttons
        const BOOL bCtrlStopDemo = AnyControllerButton(event);

        // Stop demo on escape
        if (bCtrlStopDemo || _pGame->IsEscapeKeyPressed(event)) {
          _pGame->StopGame();
          _bInAutoPlayLoop = FALSE;
          _gmRunningGameMode = GM_NONE;

          event.type = WM_NULL;

        // Skip to the next demo on any key (other than console)
        } else if (bAnyKey && !_pGame->IsConsoleKeyPressed(event)) {
          // Only if there's no menu or console in the way
          if (!bMenuActive && !bMenuRendering && _pGame->gm_csConsoleState == CS_OFF) {
            _pGame->StopGame();
            _gmRunningGameMode = GM_NONE;
            StartNextDemo();

            event.type = WM_NULL;
          }
        }
      }

      // No game is running and the console is closed/closing
      const BOOL bMenuForced = (_gmRunningGameMode == GM_NONE && (_pGame->gm_csConsoleState == CS_OFF || _pGame->gm_csConsoleState == CS_TURNINGOFF));

      // Want to open the menu and the computer isn't in the way
      const BOOL bMenuToggle = (_pGame->IsEscapeKeyPressed(event) && (_pGame->gm_csComputerState == CS_OFF || _pGame->gm_csComputerState == CS_ONINBACKGROUND));

      // If currently in game
      if (!bMenuActive) {
        // And want to be in the menu
        if (bMenuForced || bMenuToggle) {
          // Deactivate the console
          if (_pGame->gm_csConsoleState == CS_ON || _pGame->gm_csConsoleState == CS_TURNINGON) {
            _pGame->gm_csConsoleState = CS_TURNINGOFF;
            _iAddonExecState = 0;
          }

          // Clear key down message and start the menu
          event.type = WM_NULL;
          StartMenus();
        }

        // And if the console isn't active (or has been deactivated just now)
        if (_pGame->gm_csConsoleState == CS_OFF || _pGame->gm_csConsoleState == CS_TURNINGOFF) {
          // Start the current menu if it's not the root one
          if (!IsMenusInRoot()) StartMenus();
        }

      // If currently in the menu and wanting to return to the previous menu
      } else if (bMenuForced && bMenuToggle && pgmCurrentMenu->gm_pgmParentMenu == NULL) {
        // Delete key down message because there's no previous menu
        if (event.type != WM_CTRLBUTTONDOWN) {
          event.type = WM_NULL;
        }
      }

      // Quick-open specific menus
      {
        if (sam_bMenuSave) {
          sam_bMenuSave = FALSE;
          StartMenus("save");
        }

        if (sam_bMenuLoad) {
          sam_bMenuLoad = FALSE;
          StartMenus("load");
        }

        if (sam_bMenuControls) {
          sam_bMenuControls = FALSE;
          StartMenus("controls");
        }

        if (sam_bMenuHiScore) {
          sam_bMenuHiScore = FALSE;
          StartMenus("hiscore");
        }
      }

      // Pass key and mouse messages to the menu if it's active with no input on
      if (bMenuActive && !_pInput->IsInputEnabled()) {
        if (event.type == WM_KEYDOWN) {
          MenuOnKeyDown(PressedMenuButton(event.key.code, -1, -1));

        } else if (event.type == WM_CTRLBUTTONDOWN) {
          MenuOnKeyDown(PressedMenuButton(-1, -1, event.ctrl.action));

        } else if (event.type == WM_LBUTTONDOWN || event.type == WM_RBUTTONDOWN
                || event.type == WM_MBUTTONDOWN || event.type == WM_XBUTTONDOWN) {
          MenuOnKeyDown(PressedMenuButton(-1, event.mouse.button, -1));

        } else if (event.type == WM_MOUSEMOVE) {
          MenuOnMouseMove(event.mouse.x, event.mouse.y);

        } else if (event.type == WM_MOUSEWHEEL) {
          if (event.mouse.y > 0) {
            MenuOnKeyDown(PressedMenuButton(-1, MOUSEWHEEL_UP, -1));
          } else if (event.mouse.y < 0) {
            MenuOnKeyDown(PressedMenuButton(-1, MOUSEWHEEL_DN, -1));
          }

        } else if (event.type == WM_CHAR) {
          MenuOnChar(event);
        }
      }

      // Toggle console forcefully or on console key (or on Escape when done running addon scripts)
      const BOOL bToggleConsole = (sam_bToggleConsole || _pGame->IsConsoleKeyPressed(event)
        || (_pGame->IsEscapeKeyPressed(event) && _iAddonExecState == 3));

      // Wanting to toggle console on key when not defining keys for controls
      if (bToggleConsole && !_bDefiningKey) {
        // Reset different states
        sam_bToggleConsole = FALSE;
        if (_iAddonExecState == 3) _iAddonExecState = 0;

        // [Cecil] If console is opened
        if (_pGame->ToggleConsole()) {
          // Stop all IFeel effects and close the menu
          IFeel_StopEffect(NULL);
          if (bMenuActive) StopMenus(FALSE);
        }
      }

      // If not running any addon script
      if (_iAddonExecState == 0) {
        _pGame->HandleConsoleAndComputer(event); // [Cecil]
      }

      _pGame->HandlePause(event); // [Cecil]
    } // loop while there are messages

    // when all messages are removed, window has surely changed
    _bWindowChanging = FALSE;

    // get real cursor position
    if( _pGame->gm_csComputerState!=CS_OFF && _pGame->gm_csComputerState!=CS_ONINBACKGROUND) {
      float fMouseX, fMouseY;
      OS::GetMouseState(&fMouseX, &fMouseY);
      _pGame->ComputerMouseMove(fMouseX, fMouseY);
    }

    // if addon is to be executed
    if (_iAddonExecState==1) {
      // print header and start console
      CPrintF(TRANS("---- Executing addon: '%s'\n"), _fnmAddonToExec.ConstData());
      sam_bToggleConsole = TRUE;
      _iAddonExecState = 2;
    // if addon is ready for execution
    } else if (_iAddonExecState==2 && _pGame->gm_csConsoleState == CS_ON) {
      // execute it
      CTString strCmd;
      strCmd.PrintF("include \"%s\"", _fnmAddonToExec.ConstData());
      _pShell->Execute(strCmd);
      CPrintF(TRANS("Addon done, press Escape to close console\n"));
      _iAddonExecState = 3;
    }

    // automaticaly manage input enable/disable toggling
    UpdateInputEnabledState();
    // automaticaly manage pause toggling
    UpdatePauseState();
    // notify game whether menu is active
    _pGame->gm_bMenuOn = bMenuActive;

    // do the main game loop and render screen
    DoGame();

    // limit current frame rate if neeeded
    LimitFrameRate();

  } // end of main application loop

  _pInput->DisableInput(_hwndMain);
  _pGame->StopGame();

#if SE1_WIN
  if (_fnmModToLoad != "") {
    STARTUPINFOA cif;
    ZeroMemory(&cif, sizeof(STARTUPINFOA));
    PROCESS_INFORMATION pi;

    const CTString strMod = _fnmModToLoad.FileName();

    // [Cecil] Use executable filename
    CTString strCmd = _fnmApplicationPath + _fnmApplicationExe;
    CTString strParam = " +game " + strMod;

    if (_strModServerJoin != "") {
      strParam += " +connect " + _strModServerJoin + " +quickjoin";
    }

    if (!CreateProcessA(strCmd.ConstData(), strParam.Data(), NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &cif, &pi)) {
      // [Cecil] Proper error message
      ErrorMessage(TRANS("Cannot start '%s' mod:\n%s"), strMod.ConstData(), GetWindowsError(GetLastError()).ConstData());
    }
  }
#endif

  // invoke quit screen if needed
  if( _bQuitScreen && _fnmModToLoad=="") QuitScreenLoop();
  
  End();
  return TRUE;
}

void CheckBrowser(void)
{
  if (_strURLToVisit!="") {
    RunBrowser(_strURLToVisit.ConstData());
  }
}

// [Cecil] Used to be WinMain()
int GameEntryPoint(HINSTANCE hInstance, CommandLineSetup &cmd)
{
  int iResult = 1;

  CTSTREAM_BEGIN {
    // [Cecil] Register command line functions
    SetupCommandLine(cmd);
    iResult = SubMain(hInstance, cmd);
  } CTSTREAM_END;

  CheckBrowser();

  return iResult;
};

#if SE1_WIN

// Entry point
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  (void)hPrevInstance;
  (void)nCmdShow;

  // [Cecil] Setup the command line
  CommandLineSetup cmd(lpCmdLine);
  return GameEntryPoint(hInstance, cmd);
};

#else

// Entry point
int main(int argc, char **argv) {
  // [Cecil] Setup the command line
  CommandLineSetup cmd(argc, argv);
  return GameEntryPoint(NULL, cmd);
};

#endif // SE1_WIN

// try to start a new display mode
BOOL TryToSetDisplayMode( enum GfxAPIType eGfxAPI, INDEX iAdapter, PIX pixSizeI, PIX pixSizeJ,
                          enum DisplayDepth eColorDepth, EWindowModes eWindowMode)
{
  CDisplayMode dmTmp;
  dmTmp.dm_ddDepth = eColorDepth;

  // [Cecil] Window mode name
  CTString strWindowMode = _astrWindowModes[eWindowMode];
  CPrintF(TRANS("  Starting display mode: %dx%dx%s (%s)\n"), pixSizeI, pixSizeJ, dmTmp.DepthString().ConstData(), strWindowMode.ConstData());

  // mark to start ignoring window size/position messages until settled down
  _bWindowChanging = TRUE;
  
  // destroy canvas if existing
  _pGame->DisableLoadingHook();
  if( pvpViewPort!=NULL) {
    _pGfx->DestroyWindowCanvas( pvpViewPort);
    pvpViewPort = NULL;
    pdpNormal = NULL;
  }

  // close the application window
  CloseMainWindow();

  // try to set new display mode
  BOOL bSuccess;

  // [Cecil] Fullscreen mode
  const BOOL bFullscreen = (eWindowMode == E_WM_FULLSCREEN);

  // [Cecil] Main window opening methods per type
  static void (*apWindowMethods[3])(PIX, PIX) = {
    &OpenMainWindowNormal,     // Windowed/normal
    &OpenMainWindowBorderless, // Borderless
    &OpenMainWindowFullScreen, // Fullscreen
  };
  
#if SE1_DIRECT3D
  // [Cecil] Open main window for Direct3D
  if (eGfxAPI == GAT_D3D) {
    (apWindowMethods[eWindowMode])(pixSizeI, pixSizeJ);
  }
#endif

  // [Cecil] Set display mode
  if (bFullscreen) {
    bSuccess = _pGfx->SetDisplayMode(eGfxAPI, iAdapter, pixSizeI, pixSizeJ, eColorDepth);
  } else {
    bSuccess = _pGfx->ResetDisplayMode(eGfxAPI);
  }

  // [Cecil] Open main window for OpenGL
  if (bSuccess && eGfxAPI == GAT_OGL) {
    (apWindowMethods[eWindowMode])(pixSizeI, pixSizeJ);
  }
  
#if SE1_DIRECT3D
  // [Cecil] Reset main window for Direct3D for non-fullscreen modes
  if (bSuccess && !bFullscreen && eGfxAPI == GAT_D3D) {
    ResetMainWindowNormal();
  }
#endif

  // if new mode was set
  if( bSuccess) {
    // create canvas
    ASSERT( pvpViewPort==NULL);
    ASSERT( pdpNormal==NULL);
    _pGfx->CreateWindowCanvas( _hwndMain, &pvpViewPort, &pdpNormal);

    // erase context of both buffers (for the sake of wide-screen)
    pdp = pdpNormal;
    if( pdp!=NULL && pdp->Lock()) {
      pdp->Fill(C_BLACK|CT_OPAQUE);
      pdp->Unlock();
      pvpViewPort->SwapBuffers();
      pdp->Lock();
      pdp->Fill(C_BLACK|CT_OPAQUE);
      pdp->Unlock();
      pvpViewPort->SwapBuffers();
    }

    // lets try some wide screen screaming :)
    const PIX pixYBegAdj = pdp->GetHeight() * 21/24;
    const PIX pixYEndAdj = pdp->GetHeight() * 3/24;
    const PIX pixXEnd    = pdp->GetWidth();
    pdpWideScreen = new CDrawPort( pdp, PIXaabbox2D( PIX2D(0,pixYBegAdj), PIX2D(pixXEnd, pixYEndAdj)));
    if( sam_bWideScreen) pdp = pdpWideScreen;

    // initial screen fill and swap, just to get context running
    BOOL bSuccess = FALSE;
    if( pdp!=NULL && pdp->Lock()) {
      pdp->Fill( _pGame->LCDGetColor( C_dGREEN|CT_OPAQUE, "bcg fill"));
      pdp->Unlock();
      pvpViewPort->SwapBuffers();
      bSuccess = TRUE;
    }
    _pGame->EnableLoadingHook(pdp);

    // if the mode is not working, or is not accelerated
    if( !bSuccess || !_pGfx->IsCurrentModeAccelerated())
    { // report error
      CPrintF( TRANS("This mode does not support hardware acceleration.\n"));
      // destroy canvas if existing
      if( pvpViewPort!=NULL) {
        _pGame->DisableLoadingHook();
        _pGfx->DestroyWindowCanvas( pvpViewPort);
        pvpViewPort = NULL;
        pdpNormal = NULL;
      }
      // close the application window
      CloseMainWindow();
      // report failure
      return FALSE;
    }

    // remember new settings
    sam_iWindowMode = eWindowMode; // [Cecil]
    sam_iScreenSizeI = pixSizeI;
    sam_iScreenSizeJ = pixSizeJ;
    sam_iDisplayDepth = eColorDepth;
    sam_iDisplayAdapter = iAdapter;
    sam_iGfxAPI = eGfxAPI;

    // report success
    return TRUE;
  // if couldn't set new mode
  } else {
    // close the application window
    CloseMainWindow();
    // report failure
    return FALSE;
  }
}


// list of possible display modes for recovery 
const INDEX aDefaultModes[][3] =
{ // color, API, adapter
  { DD_DEFAULT, GAT_OGL, 0},
  { DD_16BIT,   GAT_OGL, 0},

#if SE1_3DFX
  { DD_16BIT,   GAT_OGL, 1}, // 3dfx Voodoo2
#endif

#if SE1_DIRECT3D
  { DD_DEFAULT, GAT_D3D, 0},
  { DD_16BIT,   GAT_D3D, 0},
  { DD_16BIT,   GAT_D3D, 1},
#endif // SE1_DIRECT3D
};
const INDEX ctDefaultModes = ARRAYCOUNT(aDefaultModes);

// start new display mode
void StartNewMode( enum GfxAPIType eGfxAPI, INDEX iAdapter, PIX pixSizeI, PIX pixSizeJ,
                   enum DisplayDepth eColorDepth, INDEX iWindowMode)
{
  CPrintF( TRANS("\n* START NEW DISPLAY MODE ...\n"));

  // [Cecil] Reset invalid resolutions to the monitor one
  if (pixSizeI <= 0 || pixSizeJ <= 0) {
    pixSizeI = _vpixScreenRes(1);
    pixSizeJ = _vpixScreenRes(2);
    iWindowMode = E_WM_BORDERLESS;
    CPrintF(TRANS("Invalid resolution detected! Resetting to fullscreen monitor resolution...\n"));
  }

  // try to set the mode
  BOOL bSuccess = TryToSetDisplayMode( eGfxAPI, iAdapter, pixSizeI, pixSizeJ, eColorDepth, (EWindowModes)iWindowMode);

  // if failed
  if( !bSuccess)
  {
    // report failure and reset to default resolution
    _iDisplayModeChangeFlag = 2;  // failure
    CPrintF( TRANS("Requested display mode could not be set!\n"));

    // [Cecil] 3Dfx compatibility mode
    #if SE1_3DFX
      pixSizeI = 640;
      pixSizeJ = 480;
      iWindowMode = E_WM_FULLSCREEN;
    #else
      // [Cecil] 640x480 was too small by default
      pixSizeI = 1024;
      pixSizeJ = 768;
      iWindowMode = E_WM_WINDOWED;
    #endif

    // try to revert to one of recovery modes
    for( INDEX iMode=0; iMode<ctDefaultModes; iMode++) {
      eColorDepth = (DisplayDepth)aDefaultModes[iMode][0];
      eGfxAPI     = (GfxAPIType)  aDefaultModes[iMode][1];
      iAdapter    =               aDefaultModes[iMode][2];
      CPrintF(TRANS("\nTrying recovery mode %d...\n"), iMode);
      bSuccess = TryToSetDisplayMode( eGfxAPI, iAdapter, pixSizeI, pixSizeJ, eColorDepth, (EWindowModes)iWindowMode);
      if( bSuccess) break;
    }
    // if all failed
    if( !bSuccess) {
      FatalError(TRANS(
        "Cannot set display mode!\n"
        "Serious Sam was unable to find display mode with hardware acceleration.\n"
        "Make sure you install proper drivers for your video card as recommended\n"
        "in documentation and set your desktop to 16 bit (65536 colors).\n"
        "Please see ReadMe file for troubleshooting information.\n"));
    }

  // if succeeded
  } else {
    _iDisplayModeChangeFlag = 1;  // all ok
  }

  // apply 3D-acc settings
  ApplyGLSettings(FALSE);

  // remember time of mode setting
  _tmDisplayModeChanged = _pTimer->GetRealTimeTick();
}
