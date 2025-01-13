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

#ifndef SE_INCL_STOCK_CTEXTUREDATA_H
#define SE_INCL_STOCK_CTEXTUREDATA_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/Texture.h>
#include <Engine/Templates/Stock.h>

typedef CResourceStock<CTextureData> CStock_CTextureData;

ENGINE_API extern CStock_CTextureData *_pTextureStock;

// [Cecil] Public interface methods that check for the existence of the stock
template<> inline
CTextureData *CStock_CTextureData::Obtain_t(const CTFileName &fnmFileName) {
  if (_pTextureStock == NULL) {
    ASSERTALWAYS("No stock to obtain the resource from!");
    return NULL;
  }

  return _pTextureStock->Obtain_internal_t(fnmFileName);
};

template<> inline
void CStock_CTextureData::Release(CTextureData *ptObject) {
  if (_pTextureStock == NULL) return;
  _pTextureStock->Release_internal(ptObject);
};

template<> inline
void CStock_CTextureData::FreeUnused(void) {
  if (_pTextureStock == NULL) return;
  _pTextureStock->FreeUnused_internal();
};

#endif // include-once check
