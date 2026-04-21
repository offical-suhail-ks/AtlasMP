cmake_minimum_required(VERSION 3.21)
project(AtlasMP VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Output structure ───────────────────────────────────────────────────────────
# All build artifacts go to .output/  (build cache, libs, temp)
# Final binaries go to their respective hidden folders at project root:
#   .client\   <- AtlasMP-Client.dll
#   .server\   <- AtlasMP-Server.exe
#   .launcher\ <- AtlasMP-Launcher.exe + AtlasInject.exe

# The "project root" is one level up from src/
set(ATLASMP_ROOT "${CMAKE_SOURCE_DIR}/..")

# Everything goes into .output\ — DLLs, EXEs, libs, all build artifacts
set(ATLASMP_OUT              "${ATLASMP_ROOT}/.output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ATLASMP_OUT}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ATLASMP_OUT}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ATLASMP_OUT}")

# All targets output to the same .output\ folder
set(ATLASMP_CLIENT_DIR   "${ATLASMP_OUT}")
set(ATLASMP_SERVER_DIR   "${ATLASMP_OUT}")
set(ATLASMP_LAUNCHER_DIR "${ATLASMP_OUT}")

# ── Options ───────────────────────────────────────────────────────────────────
option(ATLAS_BUILD_CLIENT  "Build AtlasMP-Client.dll"      ON)
option(ATLAS_BUILD_SERVER  "Build AtlasMP-Server.exe"      ON)
option(ATLAS_BUILD_RUNTIME_LUA "Build Lua scripting runtime" ON)
option(ATLAS_BUILD_RUNTIME_JS  "Build JS scripting runtime"  ON)
option(ATLAS_BUILD_RPFLIB  "Build RPF archive library"     ON)
option(ATLAS_BUILD_TESTS   "Build unit tests"              ON)

include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# ── Subdirectories ────────────────────────────────────────────────────────────
add_subdirectory(shared)
add_subdirectory(server)

if(WIN32 AND ATLAS_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(ATLAS_BUILD_RUNTIME_LUA)
    add_subdirectory(runtimes/lua)
endif()

if(ATLAS_BUILD_RUNTIME_JS)
    add_subdirectory(runtimes/js)
endif()

if(ATLAS_BUILD_RPFLIB)
    add_subdirectory(tools/rpflib)
endif()

add_subdirectory(tools/launcher)
add_subdirectory(tools/injector)

if(ATLAS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Summary ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  AtlasMP ${PROJECT_VERSION} build configuration")
message(STATUS "═══════════════════════════════════════")
message(STATUS "  Source root  : ${CMAKE_SOURCE_DIR}")
message(STATUS "  Project root : ${ATLASMP_ROOT}")
message(STATUS "  Build cache  : ${ATLASMP_ROOT}/.output")
message(STATUS "  Client DLL   : ${ATLASMP_CLIENT_DIR}")
message(STATUS "  Server EXE   : ${ATLASMP_SERVER_DIR}")
message(STATUS "  Launcher     : ${ATLASMP_LAUNCHER_DIR}")
message(STATUS "═══════════════════════════════════════")
message(STATUS "")
