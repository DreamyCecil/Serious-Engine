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

#include <Engine/Base/Timer.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Translation.h>

#include <Engine/Base/Registry.h>
#include <Engine/Base/Profiling.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Statistics_internal.h>

#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/Priority.inl>

#if SE1_UNIX
  #include <chrono>
  #include <thread>
  #include <x86intrin.h>
#endif

// [Cecil] TODO: Get rid of __rdtsc(), which is only used in a fallback scenario in DetermineCPUSpeedHz() in the entire codebase
// Read the Pentium TimeStampCounter
static inline SQUAD ReadTSC(void)
{
// [Cecil] Prioritize old compiler
#if SE1_OLD_COMPILER || SE1_USE_ASM
  SQUAD mmRet;
  __asm {
    rdtsc
    mov   dword ptr [mmRet+0],eax
    mov   dword ptr [mmRet+4],edx
  }
  return mmRet;

#else
  return __rdtsc();
#endif
}


// link with Win-MultiMedia
#pragma comment(lib, "winmm.lib")

// current game time always valid for the currently active task
static SE1_THREADLOCAL TICK _tckCurrentTickTimer = 0; // [Cecil] Ticks instead of seconds

// CTimer implementation

// pointer to global timer object
CTimer *_pTimer = NULL;

// [Cecil] Explicit tickrate number, i.e. amount of logic steps in one game second
const TICK CTimer::TickRate = 20;
const TIME CTimer::TickQuantum = TIME(1.0 / (SECOND)CTimer::TickRate);

/*
 * Timer interrupt callback function.
 */
/*
  NOTE:
  This function is a bit more complicated than it could be, because
  it has to deal with a feature in the windows multimedia timer that
  is undesired here.
  That is the fact that, if the timer function is stalled for a while,
  because some other thread or itself took too much time, the timer function
  is called more times to catch up with the hardware clock.
  This can cause complete lockout if timer handlers constantly consume more
  time than is available between two calls of timer function.
  As a workaround, this function measures hardware time and refuses to call
  the handlers if it is not on time.

  In effect, if some timer handler starts spending too much time, the
  handlers are called at lower frequency until the application (hopefully)
  stabilizes.

  When such a catch-up situation occurs, 'real time' timer still keeps
  more or less up to date with the hardware time, but the timer handlers
  skip some ticks. E.g. if timer handlers start spending twice more time
  than is tick quantum, they get called approx. every two ticks.

  EXTRA NOTE:
  Had to disable that, because it didn't work well (caused jerking) on 
  Win95 osr2 with no patches installed!
*/
void CTimer::TimerFunc_internal(void)
{
// Access to stream operations might be invoked in timer handlers, but
// this is disabled for now. Should also synchronize access to list of
// streams and to group file before enabling that!
//  CTSTREAM_BEGIN {

  #if SE1_SINGLE_THREAD
    static const SECOND tmQuantum = _pTimer->TickQuantum;
    static CTimerValue tvTickQuantum(tmQuantum);

    CTimerValue tvUpkeepDiff = _pTimer->GetHighPrecisionTimer() - _pTimer->tm_tvInitialUpkeep;
    const SECOND tmDiff = tvUpkeepDiff.GetSeconds();

    // No need to update timers yet
    if (tmDiff < tmQuantum) return;

    while (tmDiff >= tmQuantum) {
      _pTimer->tm_tvInitialUpkeep += tvTickQuantum;
      _pTimer->tm_tckRealTimeTimer++;
      tmDiff -= tmQuantum;
    }

  #else
    // increment the 'real time' timer
    _pTimer->tm_tckRealTimeTimer++;
  #endif // SE1_SINGLE_THREAD

    // get the current time for real and in ticks
    CTimerValue tvTimeNow = _pTimer->GetHighPrecisionTimer();
    TICK tckTickNow = _pTimer->tm_tckRealTimeTimer;
    // calculate how long has passed since we have last been on time
    TIME tmTimeDelay = (TIME)(tvTimeNow - _pTimer->tm_tvLastTimeOnTime).GetSeconds();
    TICK tckTickDelay = (tckTickNow - _pTimer->tm_tckLastTickOnTime);

    _sfStats.StartTimer(CStatForm::STI_TIMER);
    // if we are keeping up to time (more or less)
//    if (tmTimeDelay>=_pTimer->TickQuantum*0.9f) {

      // for all hooked handlers
      FOREACHINLIST(CTimerHandler, th_Node, _pTimer->tm_lhHooks, itth) {
        // handle
        itth->HandleTimer();
      }
//    }
    _sfStats.StopTimer(CStatForm::STI_TIMER);

    // remember that we have been on time now
    _pTimer->tm_tvLastTimeOnTime = tvTimeNow;
    _pTimer->tm_tckLastTickOnTime = tckTickDelay;

//  } CTSTREAM_END;
}

#if !SE1_SINGLE_THREAD
#if SE1_PREFER_SDL

// [Cecil] SDL: Timer tick function
Uint32 CTimer_TimerFunc(void *pUserData, SDL_TimerID iTimerID, Uint32 interval) {
  (void)pUserData;
  (void)iTimerID;

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // handle all timers
  CTimer::TimerFunc_internal();

  return interval;
};

#else

void __stdcall CTimer_TimerFunc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // handle all timers
  CTimer::TimerFunc_internal();
}

#endif // SE1_PREFER_SDL
#endif // !SE1_SINGLE_THREAD

// [Cecil] Get CPU speed using platform-specific methods
static BOOL DetermineCPUSpeedFromOS(SQUAD &llSpeedHz) {
#if SE1_WIN
  // Get CPU speed from the Windows registry
  ULONG ulSpeedReg = 0;

  if (REG_GetLong("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\\~MHz", ulSpeedReg)) {
    llSpeedHz = (SQUAD)ulSpeedReg * 1000000;
    return TRUE;
  }

  llSpeedHz = 0;
  return FALSE;

#elif SE1_UNIX
  // Determine CPU speed on Unix
  FILE *fileCPU = fopen("/proc/cpuinfo", "rb");

  if (fileCPU == NULL) {
    CPrintF(TRANS("Couldn't open '/proc/cpuinfo' for reading:\n%s"), strerror(errno));
    return FALSE;
  }

  char *pBuffer = (char *)AllocMemory(10240);

  if (pBuffer != NULL) {
    fread(pBuffer, 10240, 1, fileCPU);
    char *pMHz = strstr(pBuffer, "cpu MHz");

    if (pMHz != NULL) {
      pMHz = strchr(pMHz, ':');

      if (pMHz != NULL) {
        do {
          pMHz++;
        } while (isspace(static_cast<UBYTE>(*pMHz)));

        llSpeedHz = (SQUAD)atof(pMHz) * 1000000;
      }
    }

    FreeMemory(pBuffer);
  }

  fclose(fileCPU);

  return (llSpeedHz != 0);

#else
  ASSERTALWAYS("Implement CPU speed determination for this platform!");
  llSpeedHz = 0;
  return FALSE;
#endif
};

#pragma inline_depth()

// Get processor speed in Hertz
static SQUAD DetermineCPUSpeedHz(void)
{
  // [Cecil] NOTE: When measuring CPU speed manually with the code below, the value is always
  // going to be ever-so-slightly different compared to any other time, which is inconsistent.
  // So, I decided to just rely on the OS first and do manual measurements second as a fallback.
  SQUAD llPlatformHz;
  if (DetermineCPUSpeedFromOS(llPlatformHz)) return llPlatformHz;

  // [Cecil] Proceed with a manual measurement of the CPU speed if it couldn't be read from the OS
  CPrintF(TRANS("  CPU speed not found in registry, using calculated value\n\n"));

  #define MAX_MEASURE_TRIES 5
  static SQUAD _aTries[MAX_MEASURE_TRIES];

  // [Cecil] SDL: Get precision timer frequency
  const SQUAD llTimerFrequency = SDL_GetPerformanceFrequency();

  INDEX iTry;
  SQUAD llTimeLast, llTimeNow;
  SQUAD llCPUBefore, llCPUAfter; 
  SQUAD llTimeBefore, llTimeAfter;

  // try to measure 10 times
  INDEX iSet = 0;

  for (; iSet < 10; iSet++)
  {
    // one time has several tries
    for (iTry = 0; iTry < MAX_MEASURE_TRIES; iTry++)
    {
      // wait the state change on the timer
      llTimeNow = SDL_GetPerformanceCounter(); // [Cecil] SDL

      do {
        llTimeLast = llTimeNow;
        llTimeNow = SDL_GetPerformanceCounter(); // [Cecil] SDL
      } while( llTimeLast==llTimeNow);

      // wait for some time, and count the CPU clocks passed
      llCPUBefore  = ReadTSC();
      llTimeBefore = llTimeNow;
      llTimeAfter  = llTimeNow + llTimerFrequency / 4;

      do {
        llTimeNow = SDL_GetPerformanceCounter(); // [Cecil] SDL
      } while (llTimeNow < llTimeAfter);

      llCPUAfter = ReadTSC();

      // calculate the CPU clock frequency from gathered data
      _aTries[iTry] = (llCPUAfter - llCPUBefore) * llTimerFrequency / (llTimeNow - llTimeBefore);
    }

    // see if we had good measurement
    INDEX ctFaults = 0;
    SQUAD llSpeed = _aTries[0];
    const SQUAD llTolerance = llSpeed / 100; // 1% tolerance should be enough

    for (iTry = 1; iTry < MAX_MEASURE_TRIES; iTry++) {
      if (Abs(llSpeed - _aTries[iTry]) > llTolerance) ctFaults++;
    }

    // done if no faults
    if (ctFaults == 0) break;

    _pTimer->Suspend(1000);
  }

  // fail if couldn't readout CPU speed
  if( iSet==10) {
    CPrintF( TRANS("PerformanceTimer is not vaild!\n"));
    //return 1; 
    // NOTE: this function must never fail, or the engine will crash! 
    // if this failed, the speed will be read from registry (only happens on Win2k)
  }

  // [Cecil] Simply return the measured value
  return _aTries[0];
};

/*
 * Constructor.
 */
CTimer::CTimer(BOOL bInterrupt /*=TRUE*/)
{
#if SE1_SINGLE_THREAD
  bInterrupt = FALSE;
#endif

  tm_csHooks.cs_eIndex = EThreadMutexType::E_MTX_TIMER;
  // set global pointer
  ASSERT(_pTimer == NULL);
  _pTimer = this;
  tm_bInterrupt = bInterrupt;

  {
  #if SE1_WIN
    // this part of code must be executed as precisely as possible
    CSetPriority sp(REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL);
  #endif // SE1_WIN

    tm_llCPUSpeedHZ = DetermineCPUSpeedHz();

    // measure profiling errors and set epsilon corrections
    CProfileForm::CalibrateProfilingTimers();
  }

  // clear counters
  _tckCurrentTickTimer = 0;
  tm_tckRealTimeTimer = 0;

  tm_tckLastTickOnTime = 0;
  tm_tvLastTimeOnTime = GetHighPrecisionTimer();
  // disable lerping by default
  DisableLerp();

#if SE1_SINGLE_THREAD
  tm_tvInitialUpkeep = GetHighPrecisionTimer();

#else
  // start interrupt (eventually)
  if( tm_bInterrupt)
  {
  #if SE1_PREFER_SDL
    // [Cecil] SDL: Add timer
    tm_TimerID = SDL_AddTimer(ULONG(TickQuantum * (TIME)1000.0), &CTimer_TimerFunc, NULL);

  #else
    tm_TimerID = timeSetEvent(
      ULONG(TickQuantum*1000.0f),	  // period value [ms]
      0,	                          // resolution (0==max. possible)
      &CTimer_TimerFunc,	          // callback
      0,                            // user
      TIME_PERIODIC);               // event type
  #endif

    // check that interrupt was properly started
    if (tm_TimerID == 0) FatalError(TRANS("Cannot initialize multimedia timer!"));

    // make sure that timer interrupt is ticking
    INDEX iTry=1;
    for( ; iTry<=3; iTry++) {
      const TICK tckTickBefore = GetRealTime();
      Suspend(1000 * iTry * 3 * TickQuantum);
      const TICK tckTickAfter = GetRealTime();
      if (tckTickBefore != tckTickAfter) break;
      Suspend(1000 * iTry);
    }
    // report fatal
    if( iTry>3) FatalError(TRANS("Problem with initializing multimedia timer - please try again."));
  }
#endif // SE1_SINGLE_THREAD
}

/*
 * Destructor.
 */
CTimer::~CTimer(void)
{
  ASSERT(_pTimer == this);

#if !SE1_SINGLE_THREAD
#if SE1_PREFER_SDL
  // [Cecil] SDL: Remove timer
  SDL_RemoveTimer(tm_TimerID);

#else
  // destroy timer
  if (tm_bInterrupt) {
    ASSERT(tm_TimerID!=NULL);
    ULONG rval = timeKillEvent(tm_TimerID);
    ASSERT(rval == TIMERR_NOERROR);
  }
  // check that all handlers have been removed
  ASSERT(tm_lhHooks.IsEmpty());
#endif
#endif // !SE1_SINGLE_THREAD

  // clear global pointer
  _pTimer = NULL;
}

/*
 * Add a timer handler.
 */
void CTimer::AddHandler(CTimerHandler *pthNew)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&tm_csHooks, TRUE);
  tm_lhHooks.AddTail(pthNew->th_Node);
}

/*
 * Remove a timer handler.
 */
void CTimer::RemHandler(CTimerHandler *pthOld)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&tm_csHooks, TRUE);
  pthOld->th_Node.Remove();
}

/* Handle timer handlers manually. */
void CTimer::HandleTimerHandlers(void)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // handle all timers
  CTimer::TimerFunc_internal();
}

// [Cecil] Set the current game tick used for time dependent tasks (animations etc.) in ticks
void CTimer::SetGameTick(TICK tckNewCurrentTick) {
  _tckCurrentTickTimer = tckNewCurrentTick;
};

// [Cecil] Get the current game time that is always valid for the currently active task in ticks
const TICK CTimer::GetGameTick(void) const {
  return _tckCurrentTickTimer;
};

/*
 * Get current game time, always valid for the currently active task.
 */
const TIME CTimer::GetLerpedCurrentTick(void) const {
  return TicksToSec(_tckCurrentTickTimer) + GetLerpedSecond();
}

// [Cecil] Deprecated wrapper methods for compatibility
TIME CTimer::GetRealTimeTick(void) const {
  return TicksToSec(tm_tckRealTimeTimer);
};

void CTimer::SetCurrentTick(TIME tNewCurrentTick) {
  _tckCurrentTickTimer = SecToTicks(tNewCurrentTick);
};

const TIME CTimer::CurrentTick(void) const {
  return TicksToSec(_tckCurrentTickTimer);
};

// Set factor for lerping between ticks.
void CTimer::SetLerp(TIME fFactor) // sets both primary and secondary
{
  tm_tmLerpFactor  = fFactor;
  tm_tmLerpFactor2 = fFactor;
}
void CTimer::SetLerp2(TIME fFactor)  // sets only secondary
{
  tm_tmLerpFactor2 = fFactor;
}
// Disable lerping factor (set both factors to 1)
void CTimer::DisableLerp(void)
{
  tm_tmLerpFactor  = 1.0;
  tm_tmLerpFactor2 = 1.0;
}

// [Cecil] Get current timer value since the engine start in nanoseconds
CTimerValue CTimer::GetHighPrecisionTimer(void) {
  // Measure precise time in nanoseconds instead of the current CPU cycles
  return static_cast<SQUAD>(SDL_GetTicksNS());
};

// [Cecil] Suspend current thread execution for some time (cross-platform replacement for Sleep() from Windows API)
void CTimer::Suspend(ULONG ulMilliseconds) {
#if SE1_PREFER_SDL
  // [Cecil] SDL: Delay
  SDL_Delay(ulMilliseconds);

#elif SE1_WIN
  ::Sleep(ulMilliseconds);

#else
  std::this_thread::sleep_for(std::chrono::milliseconds(ulMilliseconds));
#endif
};

// convert a time value to a printable string (hh:mm:ss)
CTString TimeToString(FLOAT fTime)
{
  CTString strTime;
  int iSec = floor(fTime);
  int iMin = iSec/60;
  iSec = iSec%60;
  int iHou = iMin/60;
  iMin = iMin%60;
  strTime.PrintF("%02d:%02d:%02d", iHou, iMin, iSec);
  return strTime;
}
