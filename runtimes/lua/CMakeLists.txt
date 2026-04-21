set(ATLAS_LUA_INCLUDE_HINTS
    ${CMAKE_SOURCE_DIR}/server/vendor/luajit/src
)

if(DEFINED luajit_SOURCE_DIR)
    list(APPEND ATLAS_LUA_INCLUDE_HINTS ${luajit_SOURCE_DIR}/src)
endif()

find_path(ATLAS_LUA_INCLUDE_DIR lua.h PATHS ${ATLAS_LUA_INCLUDE_HINTS})

set(ATLAS_LUA_LINK_LIB "")
if(TARGET luajit)
    set(ATLAS_LUA_LINK_LIB luajit)
else()
    find_library(ATLAS_LUA_LINK_LIB NAMES luajit-5.1 luajit lua51 lua5.1 lua)
endif()

if(NOT ATLAS_LUA_INCLUDE_DIR OR NOT ATLAS_LUA_LINK_LIB)
    message(WARNING "[Lua Runtime] Lua headers/library not found - skipping AtlasMP-Lua")
    return()
endif()

add_library(AtlasMP-Lua SHARED
    src/LuaRuntime.cpp
    src/LuaBindings.cpp
    src/LuaResource.cpp
)

target_include_directories(AtlasMP-Lua
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/sdk/include
    PRIVATE ${ATLAS_LUA_INCLUDE_DIR}
)

target_link_libraries(AtlasMP-Lua
    PRIVATE atlas_shared
    PRIVATE spdlog::spdlog
    PRIVATE ${ATLAS_LUA_LINK_LIB}
)

set_target_properties(AtlasMP-Lua PROPERTIES
    OUTPUT_NAME "AtlasMP-Lua"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/runtimes"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/runtimes"
    CXX_STANDARD 17
)

target_compile_definitions(AtlasMP-Lua PRIVATE ATLAS_RUNTIME_BUILD)
