/* Copyright (c) 2025 Dreamy Cecil
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

#ifndef SE_INCL_SKA_MODELCONFIG_H
#define SE_INCL_SKA_MODELCONFIG_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Serial.h>
#include <Engine/Ska/ModelInstance.h>

// Class for loading preconfigured models from specific model configs (SMC, BMC)
class ENGINE_API CModelConfig : public CSerial {
  public:
    CModelInstance *mc_pModelInstance;

  public:
    CModelConfig();
    ~CModelConfig();

  // CSerial overrides
  public:

    // Get the description of this object
    virtual CTString GetDescription(void);

    // Clear the object
    virtual void Clear(void);

    // Write to stream
    virtual void Write_t(CTStream *ostrFile);

    // Read from stream
    virtual void Read_t(CTStream *istrFile);

    // Check if this kind of objects is auto-freed
    virtual BOOL IsAutoFreed(void);

    // Get amount of memory used by this object
    virtual SLONG GetUsedMemory(void);
};

// Load model from the stock into some model instance
ENGINE_API void ObtainModelInstance_t(CModelInstance *pmi, const CTString &fnSmcFile);

// Obtain model instance from the stock
ENGINE_API CModelInstance *ObtainModelInstance_t(const CTString &fnSmcFile);

#endif // include-once check
