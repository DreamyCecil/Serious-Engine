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

#include <Engine/Base/Updateable.h>
#include <Engine/Base/Timer.h>

// Constructor
template<bool bRealTime> inline
TChangeable<bRealTime>::TChangeable(void)
{
  ch_tckLastChangeTime = -1;
};

// Mark that something has changed in this object
template<bool bRealTime> inline
void TChangeable<bRealTime>::MarkChanged(void)
{
  // [Cecil] Unified logic
  if (bRealTime) {
    ch_tckLastChangeTime = _pTimer->GetRealTime();
  } else {
    ch_tckLastChangeTime = _pTimer->GetGameTick();
  }
};

// Test if some updateable object is up to date with this changeable
template<bool bRealTime> inline
BOOL TChangeable<bRealTime>::IsUpToDate(const TUpdateable<bRealTime> &ud) const
{
  return ch_tckLastChangeTime < ud.LastUpdateTick();
};
