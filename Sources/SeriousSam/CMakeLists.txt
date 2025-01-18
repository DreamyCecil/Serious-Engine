cmake_minimum_required(VERSION 3.7.2)
project(SeriousSam)

set(SERIOUSSAM_SRCS
  "GUI/Components/MenuGadget.cpp"
  "GUI/Components/MGArrow.cpp"
  "GUI/Components/MGButton.cpp"
  "GUI/Components/MGChangePlayer.cpp"
  "GUI/Components/MGEdit.cpp"
  "GUI/Components/MGFileButton.cpp"
  "GUI/Components/MGHighScore.cpp"
  "GUI/Components/MGKeyDefinition.cpp"
  "GUI/Components/MGLevelButton.cpp"
  "GUI/Components/MGModel.cpp"
  "GUI/Components/MGServerList.cpp"
  "GUI/Components/MGSlider.cpp"
  "GUI/Components/MGTitle.cpp"
  "GUI/Components/MGTrigger.cpp"
  "GUI/Components/MGVarButton.cpp"
  "GUI/Menus/GameMenu.cpp"
  "GUI/Menus/MAudioOptions.cpp"
  "GUI/Menus/MConfirm.cpp"
  "GUI/Menus/MControls.cpp"
  "GUI/Menus/MCustomizeAxis.cpp"
  "GUI/Menus/MCustomizeKeyboard.cpp"
  "GUI/Menus/MDisabled.cpp"
  "GUI/Menus/Menu.cpp"
  "GUI/Menus/MenuActions.cpp"
  "GUI/Menus/MenuManager.cpp"
  "GUI/Menus/MenuPrinting.cpp"
  "GUI/Menus/MenuStarters.cpp"
  "GUI/Menus/MenuStartersAF.cpp"
  "GUI/Menus/MenuStuff.cpp"
  "GUI/Menus/MHighScore.cpp"
  "GUI/Menus/MInGame.cpp"
  "GUI/Menus/MLevels.cpp"
  "GUI/Menus/MLoadSave.cpp"
  "GUI/Menus/MMain.cpp"
  "GUI/Menus/MNetwork.cpp"
  "GUI/Menus/MNetworkJoin.cpp"
  "GUI/Menus/MNetworkOpen.cpp"
  "GUI/Menus/MNetworkStart.cpp"
  "GUI/Menus/MOptions.cpp"
  "GUI/Menus/MPlayerProfile.cpp"
  "GUI/Menus/MSelectPlayers.cpp"
  "GUI/Menus/MServers.cpp"
  "GUI/Menus/MSinglePlayer.cpp"
  "GUI/Menus/MSinglePlayerNew.cpp"
  "GUI/Menus/MSplitScreen.cpp"
  "GUI/Menus/MSplitStart.cpp"
  "GUI/Menus/MVar.cpp"
  "GUI/Menus/MVideoOptions.cpp"
  "CmdLine.cpp"
  "Credits.cpp"
  "GLSettings.cpp"
  "LevelInfo.cpp"
  "MainWindow.cpp"
  "ScreenResolutions.cpp"
  "SeriousSam.cpp"
  "SplashScreen.cpp"
  "VarList.cpp"
)

add_executable(SeriousSam ${SERIOUSSAM_SRCS})
add_dependencies(SeriousSam ${GAMELIB} Engine)

# For SeriousSam/StdH.h access
target_include_directories(SeriousSam PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(SeriousSam Engine ${ENTITIESLIB} ${GAMELIB} ${SHADERSLIB})
#set_target_properties(SeriousSam PROPERTIES ENABLE_EXPORTS ON)

if(LINUX)
  # For preserving global class registrars in static modules
  target_link_options(SeriousSam PRIVATE -Wl,--whole-archive ../EntitiesMP/lib${ENTITIESLIB}.a ../Shaders/lib${SHADERSLIB}.a -Wl,--no-whole-archive)

  set_target_properties(SeriousSam PROPERTIES LINK_FLAGS "-Wl,-rpath,$ORIGIN")
  target_link_libraries(SeriousSam "m")
  target_link_libraries(SeriousSam "dl")
  target_link_libraries(SeriousSam "pthread")
  target_link_libraries(SeriousSam ${SDL2_LIBRARY} ${ZLIB_LIBRARIES})
endif()
