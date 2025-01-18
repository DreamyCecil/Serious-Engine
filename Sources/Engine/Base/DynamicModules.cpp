/* Copyright (c) 2024 Dreamy Cecil
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

#include <Engine/Base/DynamicModules.h>
#include <Engine/Templates/Stock_CEntityClass.h>

EntityClassRegistry_t *_pEntityClassRegistry = NULL;
ShaderRenderRegistry_t *_pShaderRenderRegistry = NULL;
ShaderDescRegistry_t *_pShaderDescRegistry = NULL;

// [Cecil] TEMP: List contents of all registries
void DisplayRegistryContents(void) {
  CPutString("\n--- Entity class registry ---\n");

  if (_pEntityClassRegistry != NULL) {
    EntityClassRegistry_t::const_iterator it;
    INDEX ct = 0;

    for (it = _pEntityClassRegistry->begin(); it != _pEntityClassRegistry->end(); it++) {
      ct++;
      CPrintF("%d. '%s'\n", ct, it->first.ConstData());
    }
  } else {
    CPutString("  ^cff0000Registry hasn't been created yet!\n");
  }

  CPutString("\n--- Shader render function registry ---\n");

  if (_pShaderRenderRegistry != NULL) {
    ShaderRenderRegistry_t::const_iterator it;
    INDEX ct = 0;

    for (it = _pShaderRenderRegistry->begin(); it != _pShaderRenderRegistry->end(); it++) {
      ct++;
      CPrintF("%d. '%s'\n", ct, it->first.ConstData());
    }
  } else {
    CPutString("  ^cff0000Registry hasn't been created yet!\n");
  }

  CPutString("\n--- Shader info function registry ---\n");

  if (_pShaderDescRegistry != NULL) {
    ShaderDescRegistry_t::const_iterator it;
    INDEX ct = 0;

    for (it = _pShaderDescRegistry->begin(); it != _pShaderDescRegistry->end(); it++) {
      ct++;
      CPrintF("%d. '%s'\n", ct, it->first.ConstData());
    }
  } else {
    CPutString("  ^cff0000Registry hasn't been created yet!\n");
  }
};

// [Cecil] TEMP: Verify existence of entity classes using entity class links
void CheckEntityClasses(void) {
  CDynamicStackArray<CTString> aClasses;
  MakeDirList(aClasses, "Classes\\", "*.ecl", DLI_RECURSIVE);

  const INDEX ctAll = aClasses.Count();
  INDEX ctOK = 0;

  for (INDEX i = 0; i < ctAll; i++) {
    const CTString &fnm = aClasses[i];
    CPrintF("%d. %s - ", i + 1, fnm.FileName().ConstData());

    try {
      CEntityClass* pec = _pEntityClassStock->Obtain_t(fnm);
      _pEntityClassStock->Release(pec);
      ctOK++;
      CPutString("OK!\n");

    } catch (char *strError) {
      CPrintF("%s\n", strError);
    }
  }

  CPrintF("  Usable entity classes: %d/%d\n", ctOK, ctAll);
};
