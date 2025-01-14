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

// [Cecil] Serious Engine configuration for a specific platform
#include <Engine/SE_Config.h>

// set this to 1 to enable checks whether somethig is deleted while iterating some array/container
#define CHECKARRAYLOCKING 0

#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <math.h>
#include <search.h>   // for qsort
#include <float.h>    // for FPU control

// [Cecil] Windows-specific
#if SE1_WIN
  // Predefine these types for D3D8
  #define POINTER_32 __ptr32
  #define POINTER_64 __ptr64

  #include <conio.h>
  #include <crtdbg.h>
  #include <winsock2.h>
  #include <windows.h>
  #include <mmsystem.h> // For timers

  // [Cecil] Intrinsics for new compilers
  #if !SE1_OLD_COMPILER
    #include <intrin.h>
  #endif
#endif // SE1_WIN

// [Cecil] SDL: Include SDL2 in its entirety
#define SDL_MAIN_HANDLED
#include <SDL2/include/SDL.h>

#pragma comment(lib, "SDL2.lib")

// [Cecil] GLEW: Include glew in its entirety
#if SE1_GLEW
  #include <glew.h>
  #include <wglew.h>

  #pragma comment(lib, "glew32.lib")
  #pragma comment(lib, "opengl32.lib")
#endif

// Engine base
#include <Engine/Base/Base.h>
#include <Engine/Base/Types.h>

// [Cecil] OS-specific interface
#include <Engine/OS/OS.h>

// [Cecil] Byte swapping functions
#include <Engine/Base/ByteSwap.h>

#include <Engine/Base/Input.h>
#include <Engine/Base/KeyNames.h>
#include <Engine/Base/Updateable.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/ErrorTable.h>
#include <Engine/Base/ReplaceFile.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Lists.h>
#include <Engine/Base/Timer.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/Console.h>
#include <Engine/Base/Console_internal.h>
#include <Engine/Base/Shell_internal.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/Statistics.h>
#include <Engine/Base/CRC.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Base/Registry.h>
#include <Engine/Base/IFeel.h>
#include <Engine/Base/DynamicModules.h> // [Cecil]
#include <Engine/Base/GameDir.h> // [Cecil]

// [Cecil] External module interfaces
#include <Engine/API/EngineGUI.h>

// [Cecil] File system interface
#include <Engine/OS/FileSystem.h>

#include <Engine/Entities/EntityClass.h>
#include <Engine/Entities/EntityCollision.h>
#include <Engine/Entities/EntityProperties.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Entities/LastPositions.h>
#include <Engine/Entities/EntityCollision.h>
#include <Engine/Entities/ShadingInfo.h>
#include <Engine/Entities/FieldSettings.h>
#include <Engine/Entities/Precaching.h>

#include <Engine/Light/LightSource.h>
#include <Engine/Light/LensFlares.h>
#include <Engine/Light/Shadows_internal.h>
#include <Engine/Light/Gradient.h>

#include <Engine/Math/Geometry.inl>
#include <Engine/Math/Clipping.inl>
#include <Engine/Math/FixInt.h>
#include <Engine/Math/Float.h>
#include <Engine/Math/Object3D.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/Quaternion.h>
#include <Engine/Math/Projection.h>
#include <Engine/Math/Projection_DOUBLE.h>

#include <Engine/Network/Network.h>
#include <Engine/Network/Server.h>
#include <Engine/Network/NetworkMessage.h>
#include <Engine/Network/PlayerSource.h>
#include <Engine/Network/PlayerBuffer.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Network/NetworkProfile.h>

#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Terrain/Terrain.h>

#include <Engine/World/World.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/World/WorldRayCasting.h>
#include <Engine/World/PhysicsProfile.h>
#include <Engine/World/WorldSettings.h>
#include <Engine/World/WorldCollision.h>

#include <Engine/Rendering/Render.h>
#include <Engine/Rendering/Render_internal.h>

#include <Engine/Models/ModelObject.h>
#include <Engine/Models/ModelData.h>
#include <Engine/Models/Model_internal.h>
#include <Engine/Models/EditModel.h>
#include <Engine/Models/RenderModel.h>

#include <Engine/Ska/ModelInstance.h>
#include <Engine/Ska/Mesh.h>
#include <Engine/Ska/Skeleton.h>
#include <Engine/Ska/AnimSet.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Ska/Render.h>

#include <Engine/Sound/SoundObject.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Sound/SoundListener.h>

#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/Font.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/ViewPort.h>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/ImageInfo.h>
#include <Engine/Graphics/RenderScene.h>
#include <Engine/Graphics/RenderPoly.h>
#include <Engine/Graphics/Fog.h>
#include <Engine/Graphics/Stereo.h>

#include <Engine/Templates/BSP.h>
#include <Engine/Templates/BSP_internal.h>
#include <Engine/Templates/DynamicStackArray.h>
#include <Engine/Templates/DynamicStackArray.cpp>
#include <Engine/Templates/LinearAllocator.h>
#include <Engine/Templates/LinearAllocator.cpp>
#include <Engine/Templates/DynamicArray.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicContainer.h>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/Selection.h>
#include <Engine/Templates/Selection.cpp>

// [Cecil] Engine setup properties
struct SeriousEngineSetup {
  // Possible application types
  enum EAppType {
    E_OTHER,  // Some other application without special functions
    E_GAME,   // Game client
    E_SERVER, // Dedicated server
    E_EDITOR, // World editor
  };

  // Current application type
  EAppType eAppType;

  // Custom game root directory (just for the setup, not necessarily the same as the eventual _fnmApplicationPath)
  CTString strSetupRootDir;

  // Constructor
  __forceinline SeriousEngineSetup(EAppType eSetType = E_OTHER, const CTString &strSetRootDir = "") :
    eAppType(eSetType), strSetupRootDir(strSetRootDir)
  {
  };

  // App type checks
  __forceinline bool IsAppOther (void) const { return eAppType == E_OTHER; };
  __forceinline bool IsAppGame  (void) const { return eAppType == E_GAME; };
  __forceinline bool IsAppServer(void) const { return eAppType == E_SERVER; };
  __forceinline bool IsAppEditor(void) const { return eAppType == E_EDITOR; };
};

// [Cecil] Pass setup properties
ENGINE_API void SE_InitEngine(const SeriousEngineSetup &engineSetup = SeriousEngineSetup());
ENGINE_API void SE_EndEngine(void);
ENGINE_API void SE_LoadDefaultFonts(void);
ENGINE_API void SE_UpdateWindowHandle(OS::Window hwndWindowed);
ENGINE_API void SE_PretouchIfNeeded(void);

// [Cecil] Separate methods for determining and restoring gamma adjustment
ENGINE_API void SE_DetermineGamma(OS::Window hwnd);
ENGINE_API void SE_RestoreGamma(OS::Window hwnd = NULL);

ENGINE_API extern CTString _strEngineBuild;  // not valid before InitEngine()!
ENGINE_API extern ULONG _ulEngineBuildMajor;
ENGINE_API extern ULONG _ulEngineBuildMinor;

// [Cecil] Engine properties after full initialization
ENGINE_API extern const SeriousEngineSetup &_SE1Setup;

// [Cecil] TEMP: Aliases for old variables (deprecated; use is discouraged)
#define _bDedicatedServer (_SE1Setup.IsAppServer())
#define _bWorldEditorApp  (_SE1Setup.IsAppEditor())

ENGINE_API extern CTString _strLogFile;

// temporary vars for adjustments
ENGINE_API extern FLOAT tmp_af[10];
ENGINE_API extern INDEX tmp_ai[10];
ENGINE_API extern INDEX tmp_i;
ENGINE_API extern INDEX tmp_fAdd;
