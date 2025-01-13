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

#ifndef SE_INCL_STOCK_CSHADER_H
#define SE_INCL_STOCK_CSHADER_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/Shader.h>
#include <Engine/Templates/Stock.h>

typedef CResourceStock<CShader> CStock_CShader;

ENGINE_API extern CStock_CShader *_pShaderStock;

// [Cecil] Public interface methods that check for the existence of the stock
template<> inline
CShader *CStock_CShader::Obtain_t(const CTFileName &fnmFileName) {
  if (_pShaderStock == NULL) {
    ASSERTALWAYS("No stock to obtain the resource from!");
    return NULL;
  }

  return _pShaderStock->Obtain_internal_t(fnmFileName);
};

template<> inline
void CStock_CShader::Release(CShader *ptObject) {
  if (_pShaderStock == NULL) return;
  _pShaderStock->Release_internal(ptObject);
};

template<> inline
void CStock_CShader::FreeUnused(void) {
  if (_pShaderStock == NULL) return;
  _pShaderStock->FreeUnused_internal();
};

#endif // include-once check
