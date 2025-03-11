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

#include <Engine/Engine.h>

#ifdef ENGINEGUI_EXPORTS
  #define ENGINEGUI_API SE1_API_EXPORT
#else
  #define ENGINEGUI_API SE1_API_IMPORT

  #ifdef NDEBUG
    #pragma comment(lib, "EngineGUI.lib")
  #else
    #pragma comment(lib, "EngineGUID.lib")
  #endif
#endif

// [Cecil] Define Engine GUI interface
class ENGINEGUI_API CEngineGUI : public IEngineGUI {
  public:
    // Functions used by application for getting and setting registry keys concerning modes
    virtual void GetFullScreenModeFromRegistry(CTString strSectionName, CDisplayMode &dm, GfxAPIType &gat);
    virtual void SetFullScreenModeToRegistry(  CTString strSectionName, CDisplayMode  dm, GfxAPIType  gat);

    // Call select mode dialog
    virtual void SelectMode(CDisplayMode &dm, GfxAPIType &gat);

    // Call create texture dialog
    virtual CTFileName CreateTexture(CTFileName fnTexFileToRecreate = CTString(""),
      CDynamicArray<CTFileName> *pafnCreatedTextures = NULL);

    // File requester with thumbnail display
    virtual CTFileName FileRequester(
      const char *pchrTitle = "Choose file",
      const char *pchrFilters = FILTER_ALL FILTER_END,
      const char *pchrRegistry = KEY_NAME_REQUEST_FILE_DIR,
      CTString strDefaultDir = "",
      CTString strFileSelectedByDefault = "",
      CDynamicArray<CTFileName> *pafnCreatedTextures = NULL,
      BOOL bIfOpen = TRUE);

    // [Cecil] Simplified requester for replacing files
    virtual CTFileName ReplaceFileRequester(
      const char *pchrTitle,
      const char *pchrFilters,
      const char *pchrRegistry,
      const char *pchrFileSelectedByDefault);

    // Call browse texture requester
    virtual CTFileName BrowseTexture(
      CTFileName fnDefaultSelected = CTString(""),
      const char *pchrIniKeyName = KEY_NAME_REQUEST_FILE_DIR,
      const char *pchrWindowTitle = "Choose texture",
      BOOL bIfOpen = TRUE);
};

// global engine gui handling object
ENGINEGUI_API extern CEngineGUI _EngineGUI;

// [Cecil] Define function for converting strings
#include <EngineGUI/MfcStringToCT.inl>
