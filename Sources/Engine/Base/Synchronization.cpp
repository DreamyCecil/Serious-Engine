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

#include <Engine/Base/Synchronization.h>

#if SE1_SINGLE_THREAD // [Cecil] Disable all synchronization on the same thread

CTCriticalSection::CTCriticalSection(void) {};
CTCriticalSection::~CTCriticalSection(void) {};

#if SE1_INCOMPLETE_CPP11

INDEX CTCriticalSection::Lock(void) { return 1; };
INDEX CTCriticalSection::TryToLock(void) { return 1; };
INDEX CTCriticalSection::Unlock(void) { return 0; };

CTSingleLock::CTSingleLock(CTCriticalSection *pcs, BOOL bLock) : sl_cs(*pcs) {};

#else

CTSingleLock::CTSingleLock(CTCriticalSection *pcs, BOOL bLock) {};

#endif // !SE1_INCOMPLETE_CPP11

CTSingleLock::~CTSingleLock(void) {};
void CTSingleLock::Lock(void) {};
BOOL CTSingleLock::TryToLock(void) { return TRUE; };
BOOL CTSingleLock::IsLocked(void) { return TRUE; };
void CTSingleLock::Unlock(void) {};

#elif !SE1_INCOMPLETE_CPP11 // [Cecil] Use std::recursive_mutex from C++11

CTCriticalSection::CTCriticalSection(void) : cs_eIndex(EThreadMutexType::E_MTX_INVALID)
{
  cs_pMutex = new std::recursive_mutex;
};

CTCriticalSection::~CTCriticalSection(void) {
  delete cs_pMutex;
};

CTSingleLock::CTSingleLock(CTCriticalSection *pcs, BOOL bLock) : sl_lock(*pcs->cs_pMutex, std::defer_lock)
{
  if (bLock) {
    Lock();
  }
};

CTSingleLock::~CTSingleLock(void) {
  if (IsLocked()) {
    Unlock();
  }
};

void CTSingleLock::Lock(void) {
  ASSERT(!IsLocked());

  if (!IsLocked()) {
    sl_lock.lock();
  }
};

BOOL CTSingleLock::TryToLock(void) {
  ASSERT(!IsLocked());

  if (!IsLocked()) {
    return sl_lock.try_lock();
  }

  return TRUE;
};

// [Cecil] NOTE: Usage of this method seems redundant during locking/unlocking and I'm not sure if owns_lock() is what I should be
// using here but I just don't know whether the internal mechanism of std::unique_lock is the exact same as vanilla CTSingleLock
BOOL CTSingleLock::IsLocked(void) {
  return sl_lock.owns_lock();
};

void CTSingleLock::Unlock(void) {
  ASSERT(IsLocked());

  if (IsLocked()) {
    sl_lock.unlock();
  }
};

#else // [Cecil] OS-specific implementation (before C++11)

/*
This is implementation of OPTEX (optimized mutex), 
originally from MSDN Periodicals 1996, by Jeffrey Richter.

It is updated for clearer comments, shielded with tons of asserts,
and modified to support TryToEnter() function. The original version
had timeout parameter, but it didn't work.

NOTES: 
- TryToEnter() was not tested with more than one thread, and perhaps
  there might be some problems with the final decrementing and eventual event resetting
  when lock fails. Dunno.

- take care to center the lock tests around 0 (-1 means not locked). that is
  neccessary because win95 returns only <0, ==0 and >0 results from interlocked 
  functions, so testing against any other number than 0 doesn't work.
*/

#if !SE1_WIN // [Cecil] POSIX threads wrapped into Win32 API functions

#include <pthread.h>

#define WAIT_OBJECT_0 0
#define INFINITE 0xFFFFFFFF

struct HandlePrivate {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  BOOL state;
};

LONG InterlockedIncrement(LONG *Addend) {
  return __sync_add_and_fetch(Addend, 1);
};

LONG InterlockedDecrement(LONG volatile *Addend) {
  return __sync_sub_and_fetch(Addend, 1);
};

UQUAD GetCurrentThreadId() {
  static_assert(sizeof(pthread_t) == sizeof(size_t), "");
  return (UQUAD)pthread_self();
};

HANDLE CreateEvent(void *attr, BOOL bManualReset, BOOL initial, const char *lpName) {
  ASSERT(!bManualReset);
  HandlePrivate *handle = new HandlePrivate;

  pthread_mutex_init(&handle->mutex, nullptr);
  pthread_cond_init(&handle->cond, nullptr);
  handle->state = initial;

  return (HANDLE) handle;
};

BOOL CloseHandle(HANDLE hObject) {
  delete (HandlePrivate *) hObject;
  return true;
};

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
  ASSERT(dwMilliseconds == INFINITE);
  HandlePrivate *handle = (HandlePrivate *) hHandle;

  pthread_mutex_lock(&handle->mutex);
  while (!handle->state) {
    pthread_cond_wait(&handle->cond, &handle->mutex);
  }
  handle->state = false;
  pthread_mutex_unlock(&handle->mutex);
  return WAIT_OBJECT_0;
};

BOOL SetEvent(HANDLE hEvent) {
  HandlePrivate *handle = (HandlePrivate *) hEvent;

  pthread_mutex_lock(&handle->mutex);
  handle->state = true;
  pthread_cond_signal(&handle->cond);
  pthread_mutex_unlock(&handle->mutex);

  return true;
};

BOOL ResetEvent(HANDLE hEvent) {
  HandlePrivate *handle = (HandlePrivate *) hEvent;

  pthread_mutex_lock(&handle->mutex);
  handle->state = false;
  pthread_mutex_unlock(&handle->mutex);

  return true;
};

#endif // !SE1_WIN

// The opaque OPTEX data structure
typedef struct {
   LONG   lLockCount; // note: must center all tests around 0 for win95 compatibility!
   DWORD  dwThreadId;
   LONG   lRecurseCount;
   HANDLE hEvent;
} OPTEX, *POPTEX;

SE1_THREADLOCAL EThreadMutexType _eLastLockedMutex = EThreadMutexType::E_MTX_DEFAULT;

BOOL OPTEX_Initialize (POPTEX poptex) {
  
  poptex->lLockCount = -1;   // No threads have enterred the OPTEX
  poptex->dwThreadId = 0;    // The OPTEX is unowned
  poptex->lRecurseCount = 0; // The OPTEX is unowned
  poptex->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  return(poptex->hEvent != NULL);  // TRUE if the event is created
}

void OPTEX_Delete (POPTEX poptex) {
  // No in-use check
  CloseHandle(poptex->hEvent);  // Close the event
}

INDEX OPTEX_Enter (POPTEX poptex) 
{
  
  DWORD dwThreadId = GetCurrentThreadId();  // The calling thread's ID
  
  // increment lock counter
  INDEX ctLocked = InterlockedIncrement(&poptex->lLockCount);
  ASSERT(poptex->lLockCount>=0);

  // if this is first thread that entered
  if (ctLocked == 0) {
    
    // mark that we own it, exactly once
    ASSERT(poptex->dwThreadId==0);
    ASSERT(poptex->lRecurseCount==0);
    poptex->dwThreadId = dwThreadId;
    poptex->lRecurseCount = 1;
    
  // if already owned
  } else {
    
    // if owned by this thread
    if (poptex->dwThreadId == dwThreadId) {
      // just mark that we own it once more
      poptex->lRecurseCount++;
      ASSERT(poptex->lRecurseCount>1);
      
    // if owned by some other thread
    } else {
      
      // wait for the owning thread to release the OPTEX
      DWORD dwRet = WaitForSingleObject(poptex->hEvent, INFINITE);
      ASSERT(dwRet == WAIT_OBJECT_0);
  
      // mark that we own it, exactly once
      ASSERT(poptex->dwThreadId==0);
      ASSERT(poptex->lRecurseCount==0);
      poptex->dwThreadId = dwThreadId;
      poptex->lRecurseCount = 1;
    }
  }
  ASSERT(poptex->lRecurseCount>=1);
  ASSERT(poptex->lLockCount>=0);
  return poptex->lRecurseCount;
}

INDEX OPTEX_TryToEnter (POPTEX poptex) 
{
  ASSERT(poptex->lLockCount>=-1);
  DWORD dwThreadId = GetCurrentThreadId();  // The calling thread's ID
  
  // increment lock counter
  INDEX ctLocked = InterlockedIncrement(&poptex->lLockCount);
  ASSERT(poptex->lLockCount>=0);

  // if this is first thread that entered
  if (ctLocked == 0) {
    
    // mark that we own it, exactly once
    ASSERT(poptex->dwThreadId==0);
    ASSERT(poptex->lRecurseCount==0);
    poptex->dwThreadId = dwThreadId;
    poptex->lRecurseCount = 1;
    // lock succeeded
    return poptex->lRecurseCount;
    
  // if already owned
  } else {
    
    // if owned by this thread
    if (poptex->dwThreadId == dwThreadId) {
      
      // just mark that we own it once more
      poptex->lRecurseCount++;
      ASSERT(poptex->lRecurseCount>=1);

      // lock succeeded
      return poptex->lRecurseCount;
      
    // if owned by some other thread
    } else {

      // give up taking it
      INDEX ctLocked = InterlockedDecrement(&poptex->lLockCount);
      ASSERT(poptex->lLockCount>=-1);

      // if unlocked in the mean time
      /*if (ctLocked<0) {
        // NOTE: this has not been tested!
        // ignore sent the signal
        ResetEvent(poptex->hEvent);
      }*/

      // lock failed
      return 0;
    }
  }
}

INDEX OPTEX_Leave (POPTEX poptex) 
{

  ASSERT(poptex->dwThreadId==GetCurrentThreadId());
  
  // we own in one time less
  poptex->lRecurseCount--;
  ASSERT(poptex->lRecurseCount>=0);
  INDEX ctResult = poptex->lRecurseCount;

  // if more multiple locks from this thread
  if (poptex->lRecurseCount > 0) {
    
    // just decrement the lock count
    InterlockedDecrement(&poptex->lLockCount);
    //ASSERT(poptex->lLockCount>=-1);
    
  // if no more multiple locks from this thread
  } else {
    
    // mark that this thread doesn't own it
    poptex->dwThreadId = 0;
    // decrement the lock count
    INDEX ctLocked = InterlockedDecrement(&poptex->lLockCount);
    ASSERT(poptex->lLockCount>=-1);
    // if some threads are waiting for it
    if ( ctLocked >= 0) {
      // wake one of them
      SetEvent(poptex->hEvent);
    }
  }
  
  ASSERT(poptex->lRecurseCount>=0);
  ASSERT(poptex->lLockCount>=-1);
  return ctResult;
}

// these are just wrapper classes for locking/unlocking

CTCriticalSection::CTCriticalSection(void)
{
  // index must be set before using the mutex
  cs_eIndex = EThreadMutexType::E_MTX_INVALID;
  cs_pvObject = new OPTEX;
  OPTEX_Initialize((OPTEX*)cs_pvObject);
}
CTCriticalSection::~CTCriticalSection(void)
{
  OPTEX_Delete((OPTEX*)cs_pvObject);
  delete (OPTEX*)cs_pvObject;
}
INDEX CTCriticalSection::Lock(void)
{
  return OPTEX_Enter((OPTEX*)cs_pvObject);
}
INDEX CTCriticalSection::TryToLock(void)
{
  return OPTEX_TryToEnter((OPTEX*)cs_pvObject);
}
INDEX CTCriticalSection::Unlock(void)
{
  return OPTEX_Leave((OPTEX*)cs_pvObject);
}

CTSingleLock::CTSingleLock(CTCriticalSection *pcs, BOOL bLock) : sl_cs(*pcs)
{
  // initially not locked
  sl_bLocked = FALSE;
  sl_eLastLockedIndex = EThreadMutexType::E_MTX_INVALID;

  // critical section must have index assigned
  ASSERT(sl_cs.cs_eIndex >= 1 || sl_cs.cs_eIndex == EThreadMutexType::E_MTX_IGNORE);

  // if should lock immediately
  if (bLock) {
    Lock();
  }
}
CTSingleLock::~CTSingleLock(void)
{
  // if locked
  if (sl_bLocked) {
    // unlock
    Unlock();
  }
}
void CTSingleLock::Lock(void)
{
  // must not be locked
  ASSERT(!sl_bLocked);
  ASSERT(sl_eLastLockedIndex == EThreadMutexType::E_MTX_INVALID);

  // if not locked
  if (!sl_bLocked) {
    // lock
    INDEX ctLocks = sl_cs.Lock();
    // if this mutex was not locked already
    if (ctLocks==1) {
      // check that locking in given order
      if (sl_cs.cs_eIndex != EThreadMutexType::E_MTX_IGNORE) {
        ASSERT(_eLastLockedMutex < sl_cs.cs_eIndex);
        sl_eLastLockedIndex = _eLastLockedMutex;
        _eLastLockedMutex = sl_cs.cs_eIndex;
      }
    }
  }
  sl_bLocked = TRUE;
}

BOOL CTSingleLock::TryToLock(void)
{
  // must not be locked
  ASSERT(!sl_bLocked);
  // if not locked
  if (!sl_bLocked) {
    // if can lock
    INDEX ctLocks = sl_cs.TryToLock();
    if (ctLocks>=1) {
      sl_bLocked = TRUE;

      // if this mutex was not locked already
      if (ctLocks==1) {
        // check that locking in given order
        if (sl_cs.cs_eIndex != EThreadMutexType::E_MTX_IGNORE) {
          ASSERT(_eLastLockedMutex < sl_cs.cs_eIndex);
          sl_eLastLockedIndex = _eLastLockedMutex;
          _eLastLockedMutex = sl_cs.cs_eIndex;
        }
      }
    }
  }
  return sl_bLocked;
}
BOOL CTSingleLock::IsLocked(void)
{
  return sl_bLocked;
}

void CTSingleLock::Unlock(void)
{
  // must be locked
  ASSERT(sl_bLocked);
  // if locked
  if (sl_bLocked) {
    // unlock
    INDEX ctLocks = sl_cs.Unlock();
    // if unlocked completely
    if (ctLocks==0) {
      // check that unlocking in exact reverse order
      if (sl_cs.cs_eIndex != EThreadMutexType::E_MTX_IGNORE) {
        ASSERT(_eLastLockedMutex == sl_cs.cs_eIndex);
        _eLastLockedMutex = sl_eLastLockedIndex;
        sl_eLastLockedIndex = EThreadMutexType::E_MTX_INVALID;
      }
    }
  }
  sl_bLocked = FALSE;
}

#endif // SE1_SINGLE_THREAD
