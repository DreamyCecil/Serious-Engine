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

#ifndef SE_INCL_ENGINEGUI_INTERFACE_H
#define SE_INCL_ENGINEGUI_INTERFACE_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Predefined registry key names
#define KEY_NAME_REQUEST_FILE_DIR            "Request file directory"
#define KEY_NAME_DETAIL_TEXTURE_DIR          "Detail texture directory"
#define KEY_NAME_BASE_TEXTURE_DIR            "Base texture directory"
#define KEY_NAME_CREATE_TEXTURE_DIR          "Create texture directory"
#define KEY_NAME_CREATE_ANIMATED_TEXTURE_DIR "Create animated texture directory"
#define KEY_NAME_CREATE_EFFECT_TEXTURE_DIR   "Create effect texture directory"
#define KEY_NAME_BACKGROUND_TEXTURE_DIR      "Background texture directory"
#define KEY_NAME_REPLACE_TEXTURE_DIR         "Replace texture directory"
#define KEY_NAME_SCREEN_SHOT_DIR             "Screen shots directory"

// Predefined file filters for the file requester
#define FILTER_ALL      "All files (*.*)\0*.*\0"
#define FILTER_PICTURES "Pictures (*.pcx;*.tga)\0*.pcx;*.tga\0"
#define FILTER_3DOBJ    "3D object\0*.lwo;*.obj;*.3ds\0"
#define FILTER_LWO      "Lightwave object (*.lwo)\0*.lwo\0"
#define FILTER_OBJ      "Alias Wavefront Object (*.obj)\0*.obj\0"
#define FILTER_3DS      "3DS object (*.3ds)\0*.3ds\0"
#define FILTER_SCR      "Scripts (*.scr)\0*.scr\0"
#define FILTER_PCX      "PCX files (*.pcx)\0*.pcx\0"
#define FILTER_TGA      "TGA files (*.tga)\0*.tga\0"
#define FILTER_TEX      "Textures (*.tex)\0*.tex\0"
#define FILTER_TBN      "Thumbnails (*.tbn)\0*.tbn\0"
#define FILTER_MDL      "Models (*.mdl)\0*.mdl\0"
#define FILTER_WLD      "Worlds (*.wld)\0*.wld\0"
#define FILTER_WAV      "Sounds (*.wav)\0*.wav\0"
#define FILTER_ANI      "Animations (*.ani)\0*.ani\0"
#define FILTER_TXT      "Text files (*.txt)\0*.txt\0"
#define FILTER_LST      "List files (*.lst)\0*.lst\0"
#define FILTER_TGA      "TGA files (*.tga)\0*.tga\0"
#define FILTER_SMC      "SMC files (*.smc)\0*.smc\0"
#define FILTER_BMC      "BMC files (*.bmc)\0*.bmc\0" // [Cecil] Binary model configs
#define FILTER_MODELCFG "Model config files (*.smc;*.bmc)\0*.smc;*.bmc\0" // [Cecil] All model configs
#define FILTER_END      "\0"

// SKA studio
#define FILTER_ASCII         "ASCII files (*.aal,*.asl,*.aml)\0*.aal;*.aml;*.asl\0"
#define FILTER_MESH          "Mesh ASCII files (*.am)\0*.am;\0"
#define FILTER_SKELETON      "Skeleton ASCII files (*.as)\0*.as;\0"
#define FILTER_ANIMATION     "Animation ASCII files (*.aa)\0*.aa;\0"
#define FILTER_MESH_LIST     "Mesh list files (*.aml)\0*.aml;\0"
#define FILTER_SKELETON_LIST "Skeleton list files (*.asl)\0*.asl;\0"
#define FILTER_ANIMSET_LISTS "AnimSet list files (*.aal)\0*.aal;\0"

class IEngineGUI {
  public:
    ENGINE_API IEngineGUI();
    ENGINE_API ~IEngineGUI();

  public:
    // Functions used by application for getting and setting registry keys concerning modes
    virtual void GetFullScreenModeFromRegistry(CTString strSectionName, CDisplayMode &dm, enum GfxAPIType &gat) = 0;
    virtual void SetFullScreenModeToRegistry(  CTString strSectionName, CDisplayMode  dm, enum GfxAPIType  gat) = 0;

    // Call select mode dialog
    virtual void SelectMode(CDisplayMode &dm, enum GfxAPIType &gat) = 0;

    // Call create texture dialog
    virtual CTFileName CreateTexture(CTFileName fnTexFileToRecreate = CTString(""),
      CDynamicArray<CTFileName> *pafnCreatedTextures = NULL) = 0;

    // File requester with thumbnail display
    virtual CTFileName FileRequester(
      const char *pchrTitle = "Choose file",
      const char *pchrFilters = FILTER_ALL FILTER_END,
      const char *pchrRegistry = KEY_NAME_REQUEST_FILE_DIR,
      CTString strDefaultDir = "",
      CTString strFileSelectedByDefault = "",
      CDynamicArray<CTFileName> *pafnCreatedTextures = NULL,
      BOOL bIfOpen = TRUE) = 0;

    // [Cecil] Simplified requester for replacing files
    virtual CTFileName ReplaceFileRequester(
      const char *pchrTitle,
      const char *pchrFilters,
      const char *pchrRegistry,
      const char *pchrFileSelectedByDefault) = 0;

    // Call browse texture requester
    virtual CTFileName BrowseTexture(
      CTFileName fnDefaultSelected = CTString(""),
      const char *pchrIniKeyName = KEY_NAME_REQUEST_FILE_DIR,
      const char *pchrWindowTitle = "Choose texture",
      BOOL bIfOpen = TRUE) = 0;
};

ENGINE_API extern IEngineGUI *_pEngineGuiApi;

#endif // include-once check
