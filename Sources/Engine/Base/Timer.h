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

#ifndef SE_INCL_TIMER_H
#define SE_INCL_TIMER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] Show error only on Windows
#if SE1_WIN && !SE1_SINGLE_THREAD && !defined(_MT)
  #error Multithreading support is required!
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/Synchronization.h>

// [Cecil] Timer value units (nanoseconds) in one second
const SQUAD _llTimerValueSecondLen = SQUAD(1e9);

/*
 * Class that holds and manipulates with high-precision timer values.
 */
class CTimerValue {
public:
  SQUAD tv_llValue;       // 64 bit integer (MSVC specific!)
  /* Constructor from quad integer. */
  inline CTimerValue(SQUAD llValue) : tv_llValue(llValue) {};
public:
  /* Constructor. */
  inline CTimerValue(void) {};
  /* Constructor from seconds. */
  inline CTimerValue(SECOND dSeconds);
  /* Clear timer value (set it to zero). */
  inline void Clear(void);
  /* Addition. */
  inline CTimerValue &operator+=(const CTimerValue &tvOther);
  inline CTimerValue operator+(const CTimerValue &tvOther) const;
  /* Substraction. */
  inline CTimerValue &operator-=(const CTimerValue &tvOther);
  inline CTimerValue operator-(const CTimerValue &tvOther) const;
  /* Comparisons. */
  inline BOOL operator<(const CTimerValue &tvOther) const;
  inline BOOL operator>(const CTimerValue &tvOther) const;
  inline BOOL operator<=(const CTimerValue &tvOther) const;
  inline BOOL operator>=(const CTimerValue &tvOther) const;
  /* Get the timer value in seconds. - use for time spans only! */
  inline SECOND GetSeconds(void);
  /* Get the timer value in milliseconds as integral value. */
  inline SQUAD GetMilliseconds(void);
};
// a base class for hooking on timer interrupt
class CTimerHandler {
public:
  CListNode th_Node;
public:
  virtual ~CTimerHandler(void) {}  /* rcg10042001 */
  /* This is called every TickQuantum seconds. */
  ENGINE_API virtual void HandleTimer(void)=0;
};

// class for an object that maintains global timer(s)
class ENGINE_API CTimer {
// implementation:
private:
  SQUAD tm_llCPUSpeedHZ;  // CPU speed in HZ

  CTimerValue tm_tvLastTimeOnTime;  // last time when timer was on time
  TICK tm_tckLastTickOnTime; // [Cecil] Last tick when timer was on time (seconds -> ticks)

  TICK tm_tckRealTimeTimer; // [Cecil] This really ticks at 1/TickQuantum frequency (seconds -> ticks)

  // [Cecil] FLOAT -> TIME
  TIME tm_tmLerpFactor;  // Factor used for lerping between frames
  TIME tm_tmLerpFactor2; // Secondary lerp-factor used for unpredicted movement

// [Cecil] For updating the timers in the same thread
#if SE1_SINGLE_THREAD
  CTimerValue tm_tvInitialUpkeep;

// [Cecil] For controlling the timer thread
#elif SE1_PREFER_SDL
  SDL_TimerID tm_TimerID; // [Cecil] SDL: Timer ID
#else
  ULONG tm_TimerID;       // windows timer ID
#endif

  CListHead         tm_lhHooks;   // a list head for timer hooks
  BOOL tm_bInterrupt;       // set if interrupt is added

// interface:
public:
  CTCriticalSection tm_csHooks;   // access to timer hooks

  // interval defining frequency of the game ticker
  static const TICK TickRate; // [Cecil] Explicit tickrate number, i.e. amount of logic steps in one game second
  static const TIME TickQuantum;

  /* Constructor. */
  CTimer(BOOL bInterrupt=TRUE);
  /* Destructor. */
  ~CTimer(void);
  /* Add a timer handler. */
  void AddHandler(CTimerHandler *pthNew);
  /* Remove a timer handler. */
  void RemHandler(CTimerHandler *pthOld);
  /* Handle timer handlers manually. */
  void HandleTimerHandlers(void);

  // [Cecil] Retrieve calculated CPU speed in Hz
  inline SQUAD GetCPUSpeedHz(void) const {
    return tm_llCPUSpeedHZ;
  };

  // [Cecil] Get real time in ticks
  inline TICK GetRealTime(void) const {
    return tm_tckRealTimeTimer;
  };

  /* NOTE: CurrentTick is local to each thread, and every thread must take
     care to increment the current tick or copy it from real time tick if
     it wants to make animations and similar to work. */

  // [Cecil] Set the current game tick used for time dependent tasks (animations etc.) in ticks
  void SetGameTick(TICK tckNewCurrentTick);

  // [Cecil] Get the current game time that is always valid for the currently active task in ticks
  const TICK GetGameTick(void) const;

  // [Cecil] Get current time between game ticks, which is essentially
  // "GetLerpedCurrentTick() - CurrentTick()" but without precision loss
  inline const TIME GetLerpedSecond(void) const {
    return tm_tmLerpFactor * TickQuantum;
  };

  /* Get lerped game time. */
  const TIME GetLerpedCurrentTick(void) const;

  // [Cecil] Deprecated wrapper methods for compatibility
  TIME GetRealTimeTick(void) const;
  void SetCurrentTick(TIME tNewCurrentTick);
  const TIME CurrentTick(void) const;

  // [Cecil] Replaced FLOAT with TIME everywhere for lerp factors
  // Set factor for lerping between ticks.
  void SetLerp(TIME fLerp);    // sets both primary and secondary
  void SetLerp2(TIME fLerp);   // sets only secondary
  // Disable lerping factor (set both factors to 1)
  void DisableLerp(void);
  // Get current factor used for lerping between game ticks.
  inline TIME GetLerpFactor(void) const { return tm_tmLerpFactor; };
  // Get current factor used for lerping between game ticks.
  inline TIME GetLerpFactor2(void) const { return tm_tmLerpFactor2; };

  // [Cecil] Get current timer value since the engine start in nanoseconds
  CTimerValue GetHighPrecisionTimer(void);

  // [Cecil] Suspend current thread execution for some time (cross-platform replacement for Sleep() from Windows API)
  void Suspend(ULONG ulMilliseconds);

  // [Cecil] Used to be CTimer_TimerFunc_internal() in global scope
  static void TimerFunc_internal(void);
};

// pointer to global timer object
ENGINE_API extern CTimer *_pTimer;

// [Cecil] Convert seconds to in-game ticks, rounded to the nearest integer
inline TICK SecToTicks(TIME tm) {
  const TIME fAddToRound = (tm < 0.0 ? -0.5 : 0.5);
  return static_cast<TICK>(floor(tm * (TIME)CTimer::TickRate + fAddToRound));
};

// [Cecil] Convert seconds to in-game ticks, rounded down to the nearest integer
// It cuts off any remaining fraction after multiplying seconds by the tickrate if the time didn't match any tick
// Use SecToTicks() to eliminate any potential float imprecision that would result in one less tick than intended
inline TICK SecToTicksDn(TIME tm) {
  return static_cast<TICK>(floor(tm * (TIME)CTimer::TickRate));
};

// [Cecil] Convert seconds to in-game ticks, rounded up to the nearest integer
// It is rounded up in order to preserve the smallest possible amount of extra delay that there may be,
// otherwise the time might end up being the same and cause inconsistencies during specific comparisons
inline TICK SecToTicksUp(TIME tm) {
  return static_cast<TICK>(ceil(tm * (TIME)CTimer::TickRate));
};

// [Cecil] Convert in-game ticks to seconds
// The resulting time may not be the most correct due to the float value imprecision with larger numbers
// but it's still better than rounding to the nearest second if the ticks are divided before the conversion
inline TIME TicksToSec(TICK tck) {
  return static_cast<TIME>(tck) * CTimer::TickQuantum;
};

// convert a time value to a printable string (hh:mm:ss)
ENGINE_API CTString TimeToString(FLOAT fTime);

#include <Engine/Base/Timer.inl>


#endif  /* include-once check. */

