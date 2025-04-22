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

#include <Engine/Base/Timer.h>

// Constructor
template<bool bRealTime> inline
TUpdateable<bRealTime>::TUpdateable(void)
{
  ud_tckLastUpdateTime = -1;
};

// Mark that the object has been updated
template<bool bRealTime> inline
void TUpdateable<bRealTime>::MarkUpdated(void)
{
  // [Cecil] Unified logic
  if (bRealTime) {
    ud_tckLastUpdateTime = _pTimer->GetRealTime();
  } else {
    ud_tckLastUpdateTime = _pTimer->GetGameTick();
  }
};

// Get time when last updated
template<bool bRealTime> inline
TICK TUpdateable<bRealTime>::LastUpdateTick(void) const
{
  return ud_tckLastUpdateTime;
};

// Mark that the object has become invalid in spite of its time stamp
template<bool bRealTime> inline
void TUpdateable<bRealTime>::Invalidate(void)
{
  ud_tckLastUpdateTime = -1;
};
