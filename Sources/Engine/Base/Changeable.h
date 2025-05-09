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

#ifndef SE_INCL_CHANGEABLE_H
#define SE_INCL_CHANGEABLE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Object that can change in time
// [Cecil] Unified logic of old CChangeable & CChangeableRT classes behind a template
template<bool bRealTime>
class TChangeable {
  private:
    TICK ch_tckLastChangeTime; // last time this object has been changed

  public:
    // Constructor
    TChangeable(void);
    // Mark that something has changed in this object
    void MarkChanged(void);
    // Test if some updateable object is up to date with this changeable
    BOOL IsUpToDate(const TUpdateable<bRealTime> &ud) const;
};

// [Cecil] Define template methods
#include <Engine/Base/Changeable.inl>

#endif  /* include-once check. */
