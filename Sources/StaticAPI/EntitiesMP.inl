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

#if SE1_WIN && defined(SE1_STATIC_BUILD)

#ifdef NDEBUG
  #pragma comment(lib, "EntitiesMP.lib")
#else
  #pragma comment(lib, "EntitiesMPD.lib")
#endif

#define ES_CLASS_INLINE(_Class) \
  extern ClassRegistrar _Class##_AddToRegistry; \
  void *__##_Class##_Include__ = &_Class##_AddToRegistry;

#include "Lists/Entities.inl"

#undef ES_CLASS_INLINE

#endif // SE1_WIN && SE1_STATIC_BUILD
