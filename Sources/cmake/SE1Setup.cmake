# Executables expect the libraries to be in Debug
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Debug)

if(USE_CCACHE)
  find_program(CCACHE_FOUND ccache)

  if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  endif()
endif()

# [Cecil] OpenAL support
if(SE1_OPENAL_SUPPORT)
  find_package(OpenAL REQUIRED)
  include_directories(${OPENAL_INCLUDE_DIR})

else()
  # Disable OpenAL features
  add_definitions(-DSE1_SND_OPENAL=0)
  add_definitions(-DSE1_OPENAL_EFX=0)
endif()

# [Cecil] Build SDL locally, if not using system libraries
if(NOT USE_SYSTEM_SDL3)
  add_subdirectory(${CMAKE_SOURCE_DIR}/ThirdParty/SDL EXCLUDE_FROM_ALL)
else()
  find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

  if(SDL3_FOUND)
    include_directories(${SDL3_INCLUDE_DIR})
  else()
    message(FATAL_ERROR "Error USE_SYSTEM_SDL3 is set but neccessary developer files are missing")
  endif()
endif()

# [Cecil] Uses engine's zlib sources included with other third party sources, if not using system libraries
if(USE_SYSTEM_ZLIB)
  find_package(ZLIB REQUIRED)

  if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
  else()
    message(FATAL_ERROR "Error! USE_SYSTEM_ZLIB is set but neccessary developer files are missing")
  endif()
endif()

# Set install path to the project's root directory if it hasn't been set
# Only works for Linux and Windows
if(CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" OR CMAKE_INSTALL_PREFIX STREQUAL "c:/Program Files/${PROJECT_NAME}" OR CMAKE_INSTALL_PREFIX STREQUAL "")
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/../")
  set(LOCAL_INSTALL TRUE)
endif()

# OS identification
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX TRUE)
endif()

if(MSVC)
  set(WINDOWS TRUE)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

set(DEBUG FALSE)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG TRUE)
endif()

# [Cecil] Dummy values
if(SE1_DUMMY_BUILD_INFO)
  add_definitions("-DSE1_CURRENT_BRANCH_NAME=\"main\"")
  add_definitions("-DSE1_CURRENT_COMMIT_HASH=\"0000000000000000000000000000000000000000\"")
  add_definitions("-DSE1_BUILD_DATETIME=\"1970-01-01T00:00:00\"")

else()
  # [Cecil] Get the current working branch 
  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE SE1_CURRENT_BRANCH_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # [Cecil] Get the latest commit hash
  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE SE1_CURRENT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # [Cecil] Get the current date and time
  string(TIMESTAMP SE1_BUILD_DATETIME "%Y-%m-%dT%H:%M:%S" UTC)

  # [Cecil] Add preprocessor definitions
  add_definitions("-DSE1_CURRENT_BRANCH_NAME=\"${SE1_CURRENT_BRANCH_NAME}\"")
  add_definitions("-DSE1_CURRENT_COMMIT_HASH=\"${SE1_CURRENT_COMMIT_HASH}\"")
  add_definitions("-DSE1_BUILD_DATETIME=\"${SE1_BUILD_DATETIME}\"")
endif()

# Set compiler-specific options
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  # This section and the like are for flags/defines that can be shared between 
  # c and c++ compile options
  add_compile_options(-pipe)
  add_compile_options(-fPIC)
  add_compile_options(-march=native)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm.*")
    add_compile_options(-mfpu=neon)
    add_compile_options(-fsigned-char)
  endif()

  add_compile_options(-fno-strict-aliasing)

  # [Cecil] Enable certain warnings
  add_compile_options(-Wformat)

  # [Cecil] These were already disabled
  add_compile_options(-Wno-switch)
  add_compile_options(-Wno-char-subscripts)
  add_compile_options(-Wno-unknown-pragmas)
  add_compile_options(-Wno-unused-variable)
  add_compile_options(-Wno-unused-value)
  add_compile_options(-Wno-missing-braces)
  add_compile_options(-Wno-overloaded-virtual)
  add_compile_options(-Wno-invalid-offsetof)

  # [Cecil] Disable some other ones
  add_compile_options(-Wno-write-strings)
  add_compile_options(-Wno-nonnull)
  add_compile_options(-Wno-format-security)
  add_compile_options(-Wno-unused-result)
  add_compile_options(-Wno-unused-function)
  add_compile_options(-Wno-conversion-null)
  add_compile_options(-Wno-all)
  add_compile_options(-Wno-extra)

  # Multithreading
  add_definitions(-D_REENTRANT=1)
  add_definitions(-D_MT=1)

  # Static building
  add_definitions(-DSE1_STATIC_BUILD=1)

  # NOTE: Add your custom C and CXX flags on the command line like -DCMAKE_C_FLAGS=-std=c98 or -DCMAKE_CXX_FLAGS=-std=c++11

  # For C flags
  set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS} -g -D_DEBUG=1 -DDEBUG=1 -O0")
  set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS} -DNDEBUG=1 -g -O3 -fno-unsafe-math-optimizations")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -DNDEBUG=1 -g -O3 -fno-unsafe-math-optimizations")
  set(CMAKE_C_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS} -DNDEBUG=1 -Os -fno-unsafe-math-optimizations")

  # For C++ flags
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS} -g -D_DEBUG=1 -DDEBUG=1 -O0")
  set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -O3 -fno-unsafe-math-optimizations")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -g -O3 -fno-unsafe-math-optimizations")
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS} -DNDEBUG=1 -Os -fno-unsafe-math-optimizations")

  add_definitions(-DPRAGMA_ONCE=1)
  if(WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE=1)
  elseif(LINUX)
    set(CMAKE_SKIP_RPATH ON CACHE BOOL "Skip RPATH" FORCE)
    add_definitions(-D_FILE_OFFSET_BITS=64)
    add_definitions(-D_LARGEFILE_SOURCE=1)
  endif()
  
  if(LINUX)
    add_compile_options(-pthread)
    add_compile_options(-fsigned-char)
  endif()

else()
  message(FATAL_ERROR "Unsupported compiler")
endif()
