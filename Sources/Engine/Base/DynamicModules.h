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

#ifndef SE_INCL_DYNAMICMODULES_H
#define SE_INCL_DYNAMICMODULES_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Entities/EntityProperties.h>
#include <Engine/Graphics/Shader.h>

#include <unordered_map>

// Entity class registry under class names specified in ECL files
typedef std::unordered_map<CTString, CDLLEntityClass *> EntityClassRegistry_t;
ENGINE_API extern EntityClassRegistry_t *_pEntityClassRegistry;

// Registries of shader methods under method names specified in SHA files
typedef std::unordered_map<CTString, FShaderRender> ShaderRenderRegistry_t;
ENGINE_API extern ShaderRenderRegistry_t *_pShaderRenderRegistry;

typedef std::unordered_map<CTString, FShaderDesc> ShaderDescRegistry_t;
ENGINE_API extern ShaderDescRegistry_t *_pShaderDescRegistry;

// Structure for registering classes from static/dynamic modules
struct ClassRegistrar
{
  // Constructor wrapper for registering classes upon initialization
  template<class Type>
  inline ClassRegistrar(const char *strName, Type pToAdd) {
    AddToRegistry(strName, pToAdd);
  };

  // Method for registering library entity classes
  static void AddToRegistry(const char *strName, CDLLEntityClass *pToAdd)
  {
    // Create a new registry
    if (_pEntityClassRegistry == NULL) _pEntityClassRegistry = new EntityClassRegistry_t;

    // Add new entity class (or replace an existing one)
    (*_pEntityClassRegistry)[strName] = pToAdd;
  };

  // Method for registering shader render functions
  static void AddToRegistry(const char *strName, FShaderRender pToAdd)
  {
    // Create a new registry
    if (_pShaderRenderRegistry == NULL) _pShaderRenderRegistry = new ShaderRenderRegistry_t;

    // Add new entity class (or replace an existing one)
    (*_pShaderRenderRegistry)[strName] = pToAdd;
  };

  // Method for registering shader description functions
  static void AddToRegistry(const char *strName, FShaderDesc pToAdd)
  {
    // Create a new registry
    if (_pShaderDescRegistry == NULL) _pShaderDescRegistry = new ShaderDescRegistry_t;

    // Add new entity class (or replace an existing one)
    (*_pShaderDescRegistry)[strName] = pToAdd;
  };
};

#endif // include-once check
