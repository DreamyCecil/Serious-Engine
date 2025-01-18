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
  #pragma comment(lib, "Shaders.lib")
#else
  #pragma comment(lib, "ShadersD.lib")
#endif

#define SHADER_FUNC_INLINE(_ShaderName) \
  extern ClassRegistrar Shader_##_ShaderName##_AddToRegistry; \
  void *__##_ShaderName##_FuncInclude__ = &Shader_##_ShaderName##_AddToRegistry;

#define SHADER_DESC_INLINE(_ShaderName) \
  extern ClassRegistrar Shader_Desc_##_ShaderName##_AddToRegistry; \
  void *__##_ShaderName##_DescInclude__ = &Shader_Desc_##_ShaderName##_AddToRegistry;

#include "Lists/Shaders.inl"

#undef SHADER_FUNC_INLINE
#undef SHADER_DESC_INLINE

#endif // SE1_WIN && SE1_STATIC_BUILD
