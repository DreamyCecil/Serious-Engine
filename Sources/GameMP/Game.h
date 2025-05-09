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

#ifndef __GAME_H
#define __GAME_H 1

// [Cecil] Directly export from the library
#ifdef GAME_EXPORTS
  #define GAME_API SE1_API_EXPORT
#else
  #define GAME_API SE1_API_IMPORT

  #ifdef NDEBUG
    #pragma comment(lib, "GameMP.lib")
  #else
    #pragma comment(lib, "GameMPD.lib")
  #endif
#endif

#include <GameMP/PlayerSettings.h>
#include <GameMP/SessionProperties.h>

// [Cecil] V012 -> V013 because of variable amounts of local players per client
#define GAME_SHELL_VER "V013"

#define AXIS_ACTIONS_CT 9
#define SENSITIVITY_SLIDER_POSITIONS 25

// all available axis-actions are listed here
#define AXIS_MOVE_UD 0
#define AXIS_MOVE_LR 1
#define AXIS_MOVE_FB 2
#define AXIS_TURN_UD 3
#define AXIS_TURN_LR 4
#define AXIS_TURN_BK 5
#define AXIS_LOOK_UD 6
#define AXIS_LOOK_LR 7
#define AXIS_LOOK_BK 8

enum ConsoleState {
  CS_OFF,
  CS_ON,
  CS_TURNINGON,
  CS_TURNINGOFF,
  CS_ONINBACKGROUND,
  CS_TALK,
};

class CGameTimerHandler : public CTimerHandler
{
public:
  /* This is called every TickQuantum seconds. */
  virtual void HandleTimer(void);
};


class CAxisAction {
public:
  INDEX aa_iAxisAction;       // to which axis this object referes to
  FLOAT aa_fSensitivity;      // percentage of maximum sensitivity (0..100)
  FLOAT aa_fDeadZone;         // percentage of deadzone (0..100)
  BOOL aa_bInvert;            // if controler's axis should be inverted
  BOOL aa_bRelativeControler; // if this is controler of type "relative"
  BOOL aa_bSmooth;            // if controler's axis should be smoothed
  // this is value for applying to "angle" or "movement" and it
  // is calculated from invert flag and sensitivity attributes
  // (i.e. for rotation colud be: AXIS_ROTATION_SPEED*sensitivity*(-1)(if should invert axis)
  FLOAT aa_fAxisInfluence;

  FLOAT aa_fLastReading;    // last reading of this axis (for smoothing)
  FLOAT aa_fAbsolute;       // absolute value of the axis (integrated from previous readings)
};

class CButtonAction {
public:
  // default constructor
  CButtonAction();
  CListNode ba_lnNode;
  INDEX ba_iFirstKey;
  BOOL ba_bFirstKeyDown;
  INDEX ba_iSecondKey;
  BOOL ba_bSecondKeyDown;
  CTString ba_strName;
  CTString ba_strCommandLineWhenPressed;
  CTString ba_strCommandLineWhenReleased;
  // Assignment operator.
  virtual CButtonAction &operator=(CButtonAction &baOriginal);
  virtual void Read_t( CTStream &istrm);    // throw char*
  virtual void Write_t( CTStream &ostrm);    // throw char*
};

/*
 * Class containing information concerning controls system
 */
class CControls {
public:
  // list of mounted button actions
  CListHead ctrl_lhButtonActions;
  // objects describing mounted controler's axis (mouse L/R, joy U/D) and their
  // attributes (sensitivity, intvertness, type (relative/absolute) ...)
  CAxisAction ctrl_aaAxisActions[ AXIS_ACTIONS_CT];
  FLOAT ctrl_fSensitivity;    // global sensitivity for all axes
  BOOL ctrl_bInvertLook;      // inverts up/down looking
  BOOL ctrl_bSmoothAxes;      // smooths axes movements
  INDEX ctrl_iDeviceSlot;     // [Cecil] Which slot to use for polling input devices

// operations
  CControls(void);   // default constructor
  virtual ~CControls(void);   // default destructor
  // Assignment operator.
  virtual CControls &operator=(CControls &ctrlOriginal);

  // depending on axis attributes and type (rotation or translation), calculates axis
  // influence factors for all axis actions
  virtual void CalculateInfluencesForAllAxis(void);
  // get current reading of an axis
  virtual FLOAT GetAxisValue(INDEX iAxis);
  // check if these controls use any joystick
  virtual BOOL UsesJoystick(void);

  // [Cecil] Check if these controls use any mouse
  virtual BOOL UsesMouse(void);

  // switches button and axis action mounters to defaults
  virtual void SwitchAxesToDefaults(void);
  virtual void SwitchToDefaults(void);
  virtual void DoButtonActions(void);
  virtual void CreateAction(const CPlayerCharacter &pc, CPlayerAction &paAction, BOOL bPreScan);
  virtual CButtonAction &AddButtonAction(void);
  virtual void RemoveButtonAction( CButtonAction &baButtonAction);
  virtual void Load_t( CTFileName fnFile); // throw char *
  virtual void Save_t( CTFileName fnFile); // throw char *
};

// [Cecil] TEMP: Maximum amount of player profiles per game
#define MAX_PLAYER_PROFILES 8

class CLocalPlayer
{
public:
// Attributes
  BOOL lp_bActive;
  INDEX lp_iPlayer;
  CPlayerSource *lp_pplsPlayerSource;
  UBYTE lp_ubPlayerControlsState[2048]; // current state of player controls that are local to the player
// Construction
  CLocalPlayer( void)
  {
    lp_pplsPlayerSource = NULL; 
    lp_bActive = FALSE; 
    memset(lp_ubPlayerControlsState, 0, sizeof(lp_ubPlayerControlsState)) ;
  };
};


#define HIGHSCORE_COUNT 10
class CHighScoreEntry {
public:
  CTString hse_strPlayer;
  enum CSessionProperties::GameDifficulty hse_gdDifficulty;
  TIME hse_tmTime;
  INDEX hse_ctKills;
  INDEX hse_ctScore;
public:
  CHighScoreEntry(void);
};

/*
 * Class responsible for handling game interface
 */
class CGame {
public:
  enum ConsoleState gm_csConsoleState;
  enum ConsoleState gm_csComputerState;

  CTFileName gm_fnSaveFileName;

  CTString gam_strCustomLevel;
  CTString gam_strSessionName;
  CTString gam_strJoinAddress;
  CTString gam_strConsoleInputBuffer;

  CTString gm_astrAxisNames[AXIS_ACTIONS_CT];

  CHighScoreEntry gm_ahseHighScores[HIGHSCORE_COUNT];
  INDEX gm_iLastSetHighScore;

  CPlayerCharacter gm_apcPlayers[MAX_PLAYER_PROFILES];
  CControls gm_actrlControls[MAX_PLAYER_PROFILES];
  CControls gm_ctrlControlsExtra;
  INDEX gm_iSinglePlayer;
  INDEX gm_iWEDSinglePlayer;

  enum SplitScreenCfg {
    SSC_DEDICATED = -2,
    SSC_OBSERVER = -1,

    // [Cecil] Removed 1-4 types because local player amount can now be variable
    // This value should now be used purely as offset
    SSC_PLAY = 0,
  };
  enum SplitScreenCfg gm_MenuSplitScreenCfg;
  enum SplitScreenCfg gm_StartSplitScreenCfg;
  enum SplitScreenCfg gm_CurrentSplitScreenCfg;

  // Attributes
  CGameTimerHandler m_gthGameTimerHandler;
  BOOL gm_bGameOn;
  BOOL gm_bMenuOn;        // set by serioussam.exe to notify that menu is active
  BOOL gm_bFirstLoading;  // set by serioussam.exe to notify first loading
  BOOL gm_bProfileDemo;   // demo profiling required

  // network provider itself
  CNetworkProvider gm_npNetworkProvider;
  // network provider's description
  CTString gm_strNetworkProvider;

  // controls that are local to each player
  SLONG gm_slPlayerControlsSize;
  void *gm_pvGlobalPlayerControls;
  // index of local player
  // (-1) if not active
  INDEX gm_aiMenuLocalPlayers[NET_MAXLOCALPLAYERS];
  INDEX gm_aiStartLocalPlayers[NET_MAXLOCALPLAYERS];
  // players that are currently playing on local machine
  CLocalPlayer gm_lpLocalPlayers[NET_MAXLOCALPLAYERS];

  // Operations
  void InitInternal(void);
  void EndInternal(void);
  BOOL StartProviderFromName(void);
  void SetupLocalPlayers( void);
  BOOL AddPlayers(void);
  SLONG PackHighScoreTable(void);
  void RecordHighScore(void);
  void UnpackHighScoreTable(SLONG slSize);
  void SaveThumbnail(const CTFileName &fnm);
  CTFileName GetQuickSaveName(BOOL bSave);
  void GameHandleTimer(void);
  virtual void LoadPlayersAndControls(void);
  virtual void SavePlayersAndControls(void);
  virtual void Load_t( void);
  virtual void Save_t( void);

  // set properties for a quick start session
  virtual void SetQuickStartSession(CSessionProperties &sp);
  // set properties for a single player session
  virtual void SetSinglePlayerSession(CSessionProperties &sp);
  // set properties for a multiplayer session
  virtual void SetMultiPlayerSession(CSessionProperties &sp);

  // game loop functions
#define GRV_SHOWEXTRAS  (1L<<0)   // add extra stuff like console, weapon, pause
  virtual void GameRedrawView(CDrawPort *pdpDrawport, ULONG ulFlags);
  virtual void GameMainLoop(void);

  // console functions
  virtual void ConsoleKeyDown(const OS::SE1Event &event);
  virtual void ConsoleChar(const OS::SE1Event &event);
  virtual void ConsoleRender(CDrawPort *pdpDrawport);
  virtual void ConsolePrintLastLines(CDrawPort *pdpDrawport);

  // computer functions
  virtual void ComputerMouseMove(PIX pixX, PIX pixY);
  virtual void ComputerKeyDown(const OS::SE1Event &event);
  virtual void ComputerRender(CDrawPort *pdpDrawport);
  virtual void ComputerForceOff();

  // loading hook functions
  virtual void EnableLoadingHook(CDrawPort *pdpDrawport);
  virtual void DisableLoadingHook(void);

  // get default description for a game (for save games/demos)
  virtual CTString GetDefaultGameDescription(BOOL bWithInfo);

  // game start/end functions
  virtual BOOL NewGame(const CTString &strSessionName, const CTFileName &fnWorld,
    class CSessionProperties &sp);
  virtual BOOL JoinGame(const CNetworkSession &session);
  virtual BOOL LoadGame(const CTFileName &fnGame);
  virtual BOOL SaveGame(const CTFileName &fnGame);
  virtual void StopGame(void);
  virtual BOOL StartDemoPlay(const CTFileName &fnDemo);
  virtual BOOL StartDemoRec(const CTFileName &fnDemo);
  virtual void StopDemoRec(void);
  virtual INDEX GetPlayersCount(void);
  virtual INDEX GetLivePlayersCount(void);

  // printout and dump extensive demo profile report
  virtual CTString DemoReportFragmentsProfile( INDEX iRate);
  virtual CTString DemoReportAnalyzedProfile(void);

  // functions called from world editor
  virtual void Initialize(const CTFileName &fnGameSettings);
  virtual void End(void);
  virtual void QuickTest(const CTFileName &fnMapName, 
    CDrawPort *pdpDrawport, CViewPort *pvpViewport);

  // [Cecil] Pass key presses to console and computer
  virtual void HandleConsoleAndComputer(const OS::SE1Event &event);

  // [Cecil] Manually toggle in-game pause
  virtual void HandlePause(const OS::SE1Event &event);

  // [Cecil] Should the application stop running or not
  virtual BOOL ShouldStopRunning(const OS::SE1Event &event, BOOL bOnDeactivation);

  // [Cecil] Check if pressed the key to go back
  virtual BOOL IsEscapeKeyPressed(const OS::SE1Event &event);

  // [Cecil] Check if pressed any of the console opening keys
  virtual BOOL IsConsoleKeyPressed(const OS::SE1Event &event);

  // [Cecil] Toggle console state and return TRUE if console is being opened
  virtual BOOL ToggleConsole(void);

  // interface rendering functions
  virtual void LCDInit(void);
  virtual void LCDEnd(void);
  virtual void LCDPrepare(FLOAT fFade);
  virtual void LCDSetDrawport(CDrawPort *pdp);
  virtual void LCDDrawBox(PIX pixUL, PIX pixDR, const PIXaabbox2D &box, COLOR col);
  virtual void LCDScreenBox(COLOR col);
  virtual void LCDScreenBoxOpenLeft(COLOR col);
  virtual void LCDScreenBoxOpenRight(COLOR col);
  virtual void LCDRenderClouds1(void);
  virtual void LCDRenderClouds2(void);
          void LCDRenderCloudsForComp(void);
          void LCDRenderCompGrid(void);
  virtual void LCDRenderGrid(void);
  virtual void LCDDrawPointer(PIX pixI, PIX pixJ);
  virtual COLOR LCDGetColor(COLOR colDefault, const char *strName);
  virtual COLOR LCDFadedColor(COLOR col);
  virtual COLOR LCDBlinkingColor(COLOR col0, COLOR col1);

  // menu interface functions
  virtual void MenuPreRenderMenu(const char *strMenuName);
  virtual void MenuPostRenderMenu(const char *strMenuName);
};

// [Cecil] Export the interface from here
GAME_API extern CGame *_pGame;

// [Cecil] Load shell symbols from the persistent symbols script
GAME_API void LoadPersistentSymbols(void);

// [Cecil] Shell functions used by SeriousSam and GameAgent
GAME_API CTString GetGameTypeName(INDEX iMode);
GAME_API CTString GetCurrentGameTypeName(void);
GAME_API ULONG GetSpawnFlagsForGameType(INDEX iGameType);
GAME_API BOOL IsMenuEnabled(const CTString &strMenuName);
CTString GetGameTypeNameCfunc(void *pArgs);
ULONG GetSpawnFlagsForGameTypeCfunc(void *pArgs);
BOOL IsMenuEnabledCfunc(void *pArgs);

#endif
