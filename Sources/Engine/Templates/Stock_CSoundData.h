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

#ifndef SE_INCL_STOCK_CSOUNDDATA_H
#define SE_INCL_STOCK_CSOUNDDATA_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Sound/SoundData.h>
#include <Engine/Templates/Stock.h>

typedef CResourceStock<CSoundData> CStock_CSoundData;

ENGINE_API extern CStock_CSoundData *_pSoundStock;

// [Cecil] Public interface methods that check for the existence of the stock
template<> inline
CSoundData *CStock_CSoundData::Obtain_t(const CTFileName &fnmFileName) {
  if (_pSoundStock == NULL) {
    ASSERTALWAYS("No stock to obtain the resource from!");
    return NULL;
  }

  return _pSoundStock->Obtain_internal_t(fnmFileName);
};

template<> inline
void CStock_CSoundData::Release(CSoundData *ptObject) {
  if (_pSoundStock == NULL) return;
  _pSoundStock->Release_internal(ptObject);
};

template<> inline
void CStock_CSoundData::FreeUnused(void) {
  if (_pSoundStock == NULL) return;
  _pSoundStock->FreeUnused_internal();
};

#endif // include-once check
