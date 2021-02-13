/* Copyright (c) 2020 Dreamy Cecil
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

#ifndef SE_INCL_TIMERTICK_H
#define SE_INCL_TIMERTICK_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include "Engine/Base/Types.h"
#include "Engine/Base/Assert.h"

// [Cecil] New timer: Ticks (signed 64-bit integer)
typedef long long TICK;

// [Cecil] New timer: Tick with the fraction
struct FTICK {
  TICK llTick;
  FLOAT fFrac;

  // Constructors
  FTICK()                 : llTick(0),  fFrac(0.0f) {};
  FTICK(TICK ll)          : llTick(ll), fFrac(0.0f) {};
  FTICK(TICK ll, FLOAT f) : llTick(ll), fFrac(f) {};

  // Type conversion
  operator TICK() {
    return llTick;
  };

  operator FLOAT() {
    return FLOAT(llTick) + fFrac;
  };
  
  // Operations (FTICK)
  FTICK operator+(FTICK ft) {
    llTick += ft.llTick;
    fFrac += ft.fFrac;
    return *this;
  };

  FTICK operator-(FTICK ft) {
    llTick -= ft.llTick;
    fFrac -= ft.fFrac;
    return *this;
  };

  FTICK operator*(FTICK ft) {
    llTick *= ft.llTick;
    fFrac *= ft.fFrac;
    return *this;
  };

  FTICK operator/(FTICK ft) {
    ASSERT(ft.llTick != 0);
    ASSERT(ft.fFrac != 0.0f);

    llTick /= ft.llTick;
    fFrac /= ft.fFrac;
    return *this;
  };

  // Operations (TICK)
  FTICK operator+(TICK ll) {
    llTick += ll;
    return *this;
  };

  FTICK operator-(TICK ll) {
    llTick -= ll;
    return *this;
  };

  FTICK operator*(TICK ll) {
    llTick *= ll;
    return *this;
  };

  FTICK operator/(TICK ll) {
    ASSERT(ll != 0);

    llTick /= ll;
    return *this;
  };

  // Operations (FLOAT)
  FTICK operator+(FLOAT f) {
    fFrac += f;
    return *this;
  };

  FTICK operator-(FLOAT f) {
    fFrac -= f;
    return *this;
  };

  FTICK operator*(FLOAT f) {
    fFrac *= f;
    return *this;
  };

  FTICK operator/(FLOAT f) {
    ASSERT(f != 0.0f);

    fFrac /= f;
    return *this;
  };
  
  // Conditions (FTICK)
  BOOL operator>(FTICK ft) {
    if (llTick == ft.llTick) {
      return (fFrac > ft.fFrac);
    }
    return (llTick > ft.llTick);
  };

  BOOL operator<(FTICK ft) {
    if (llTick == ft.llTick) {
      return (fFrac < ft.fFrac);
    }
    return (llTick < ft.llTick);
  };

  BOOL operator>=(FTICK ft) {
    if (llTick == ft.llTick) {
      return (fFrac >= ft.fFrac);
    }
    return (llTick >= ft.llTick);
  };

  BOOL operator<=(FTICK ft) {
    if (llTick == ft.llTick) {
      return (fFrac <= ft.fFrac);
    }
    return (llTick <= ft.llTick);
  };

  BOOL operator==(FTICK ft) {
    if (llTick == ft.llTick) {
      return (fFrac == ft.fFrac);
    }
    return (llTick > ft.llTick);
  };

  // Conditions (TICK)
  BOOL operator> (TICK ll) { return (llTick >  ll); };
  BOOL operator< (TICK ll) { return (llTick <  ll); };
  BOOL operator>=(TICK ll) { return (llTick >= ll); };
  BOOL operator<=(TICK ll) { return (llTick <= ll); };
  BOOL operator==(TICK ll) { return (llTick == ll); };
  
  // Conditions (int)
  BOOL operator> (int i) { return (llTick >  i); };
  BOOL operator< (int i) { return (llTick <  i); };
  BOOL operator>=(int i) { return (llTick >= i); };
  BOOL operator<=(int i) { return (llTick <= i); };
  BOOL operator==(int i) { return (llTick == i); };
};

#endif