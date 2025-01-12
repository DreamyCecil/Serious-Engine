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

#ifndef SE_INCL_SYNCHRONIZATION_H
#define SE_INCL_SYNCHRONIZATION_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] C++11 multithreading
#if !SE1_INCOMPLETE_CPP11
  #include <mutex>
#endif

// [Cecil] Thread mutex types for specific classes
enum EThreadMutexType {
  E_MTX_INVALID = -2, // Not yet set
  E_MTX_IGNORE  = -1, // Mutex with no deadlock control
  E_MTX_DEFAULT =  0, // Reserved as "no value"

  E_MTX_TIMER       = 1000, // CTimer
  E_MTX_NETWORK     = 2000, // CNetworkLibrary
  E_MTX_TEMPNETWORK = 2001, // Temporary CNetworkLibrary during default state creation
  E_MTX_SOUND       = 3000, // CSoundLibrary
  E_MTX_INPUT       = 4000, // CInput under SDL
};

// intra-process mutex (only used by thread of same process)
// NOTE: mutex has no interface - it is locked using CTSingleLock
class CTCriticalSection {
public:
  EThreadMutexType cs_eIndex; // Mutex type (index) used to prevent deadlock with assertions

  // use numbers from 1 and above for deadlock control, or -1 for no deadlock control
  ENGINE_API CTCriticalSection(void);
  ENGINE_API ~CTCriticalSection(void);

private:
#if SE1_INCOMPLETE_CPP11
  void *cs_pvObject; // Object is internal to implementation

  INDEX Lock(void);
  INDEX TryToLock(void);
  INDEX Unlock(void);

#else
  std::recursive_mutex *cs_pMutex; // [Cecil] Mutex object with recursive locking mechanism
#endif

  // [Cecil] Give access to private methods
  friend class CTSingleLock;
};

// lock object for locking a mutex with automatic unlocking
class CTSingleLock {
private:
#if SE1_INCOMPLETE_CPP11
  CTCriticalSection &sl_cs;   // the mutex this object refers to
  BOOL sl_bLocked;            // set while locked
  EThreadMutexType sl_eLastLockedIndex; // index of mutex that was locked before this lock

#else
  std::unique_lock<std::recursive_mutex> sl_lock; // [Cecil] Mutex lock
#endif

public:
  ENGINE_API CTSingleLock(CTCriticalSection *pcs, BOOL bLock);
  ENGINE_API ~CTSingleLock(void);
  ENGINE_API void Lock(void);
  ENGINE_API BOOL TryToLock(void);
  ENGINE_API BOOL IsLocked(void);
  ENGINE_API void Unlock(void);
};

#endif  /* include-once check. */
