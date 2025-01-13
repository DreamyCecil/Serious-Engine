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

#ifndef SE_INCL_STOCK_CANIMSET_H
#define SE_INCL_STOCK_CANIMSET_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Ska/AnimSet.h>
#include <Engine/Templates/Stock.h>

typedef CResourceStock<CAnimSet> CStock_CAnimSet;

ENGINE_API extern CStock_CAnimSet *_pAnimSetStock;

// [Cecil] Public interface methods that check for the existence of the stock
template<> inline
CAnimSet *CStock_CAnimSet::Obtain_t(const CTFileName &fnmFileName) {
  if (_pAnimSetStock == NULL) {
    ASSERTALWAYS("No stock to obtain the resource from!");
    return NULL;
  }

  return _pAnimSetStock->Obtain_internal_t(fnmFileName);
};

template<> inline
void CStock_CAnimSet::Release(CAnimSet *ptObject) {
  if (_pAnimSetStock == NULL) return;
  _pAnimSetStock->Release_internal(ptObject);
};

template<> inline
void CStock_CAnimSet::FreeUnused(void) {
  if (_pAnimSetStock == NULL) return;
  _pAnimSetStock->FreeUnused_internal();
};

#endif // include-once check
