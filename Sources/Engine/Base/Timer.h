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
  inline CTimerValue(double dSeconds);
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
  inline DOUBLE GetSeconds(void);
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
public:

  SQUAD tm_llCPUSpeedHZ;  // CPU speed in HZ

  CTimerValue tm_tvLastTimeOnTime;  // last time when timer was on time
  TIME        tm_tmLastTickOnTime;  // last tick when timer was on time

  TIME tm_RealTimeTimer;  // this really ticks at 1/TickQuantum frequency
  FLOAT tm_fLerpFactor;   // factor used for lerping between frames
  FLOAT tm_fLerpFactor2;  // secondary lerp-factor used for unpredicted movement

// [Cecil] For updating the timers in the same thread
#if SE1_SINGLE_THREAD
  CTimerValue tm_tvInitialUpkeep;

// [Cecil] For controlling the timer thread
#elif SE1_PREFER_SDL
  SDL_TimerID tm_TimerID; // [Cecil] SDL: Timer ID
#else
  ULONG tm_TimerID;       // windows timer ID
#endif

  CTCriticalSection tm_csHooks;   // access to timer hooks
  CListHead         tm_lhHooks;   // a list head for timer hooks
  BOOL tm_bInterrupt;       // set if interrupt is added

// interface:
public:
  // interval defining frequency of the game ticker
  static const TIME TickQuantum;     // 20 ticks per second

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

  /* Set the real time tick value. */
  void SetRealTimeTick(TIME tNewRealTimeTick);
  /* Get the real time tick value. */
  TIME GetRealTimeTick(void) const;

  /* NOTE: CurrentTick is local to each thread, and every thread must take
     care to increment the current tick or copy it from real time tick if
     it wants to make animations and similar to work. */

  /* Set the current game tick used for time dependent tasks (animations etc.). */
  void SetCurrentTick(TIME tNewCurrentTick);
  /* Get current game time, always valid for the currently active task. */
  const TIME CurrentTick(void) const;
  /* Get lerped game time. */
  const TIME GetLerpedCurrentTick(void) const;

  // Set factor for lerping between ticks.
  void SetLerp(FLOAT fLerp);    // sets both primary and secondary
  void SetLerp2(FLOAT fLerp);   // sets only secondary
  // Disable lerping factor (set both factors to 1)
  void DisableLerp(void);
  // Get current factor used for lerping between game ticks.
  inline FLOAT GetLerpFactor(void) const { return tm_fLerpFactor; };
  // Get current factor used for lerping between game ticks.
  inline FLOAT GetLerpFactor2(void) const { return tm_fLerpFactor2; };

  // [Cecil] Get current timer value since the engine start in nanoseconds
  CTimerValue GetHighPrecisionTimer(void);

  // [Cecil] Suspend current thread execution for some time (cross-platform replacement for Sleep() from Windows API)
  void Suspend(ULONG ulMilliseconds);
};

// pointer to global timer object
ENGINE_API extern CTimer *_pTimer;

// convert a time value to a printable string (hh:mm:ss)
ENGINE_API CTString TimeToString(FLOAT fTime);

#include <Engine/Base/Timer.inl>


#endif  /* include-once check. */

