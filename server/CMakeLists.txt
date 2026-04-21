add_executable(AtlasMP-Server
    src/core/Main.cpp
    src/core/Server.cpp
    src/core/Config.cpp
    src/core/Logger.cpp
    src/core/EventBus.cpp
    src/core/PlayerManager.cpp
    src/core/AntiCheat.cpp
    src/network/NetworkServer.cpp
    src/network/PacketHandler.cpp
    src/resources/ResourceLoader.cpp
    src/resources/ResourceManifest.cpp
    src/sync/SyncManager.cpp
    src/scripting/ScriptHost.cpp
    src/database/Database.cpp
)

target_include_directories(AtlasMP-Server
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/resources
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/sync
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/scripting
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    PRIVATE ${CMAKE_SOURCE_DIR}/sdk/include
    PRIVATE ${CMAKE_SOURCE_DIR}/shared/include
    PRIVATE ${CMAKE_SOURCE_DIR}/shared/src
)

if(DEFINED enet_SOURCE_DIR)
    target_include_directories(AtlasMP-Server PRIVATE ${enet_SOURCE_DIR}/include)
endif()

target_link_libraries(AtlasMP-Server
    PRIVATE atlas_shared
    PRIVATE enet
    PRIVATE spdlog::spdlog
    PRIVATE tomlplusplus::tomlplusplus
    PRIVATE nlohmann_json::nlohmann_json
)

# Windows-specific
if(WIN32)
    target_link_libraries(AtlasMP-Server PRIVATE ws2_32 winmm)
endif()

set_target_properties(AtlasMP-Server PROPERTIES
    OUTPUT_NAME "AtlasMP-Server"
    CXX_STANDARD 17
)

# Copy server.toml next to the built executable for each configuration
add_custom_command(TARGET AtlasMP-Server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/server.toml
        $<TARGET_FILE_DIR:AtlasMP-Server>/server.toml
)

# ── Output to organized folder structure ─────────────────────────────────────
set_target_properties(AtlasMP-Server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ATLASMP_SERVER_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${ATLASMP_SERVER_DIR}"
    RUNTIME_OUTPUT_DIRECTORY         "${ATLASMP_SERVER_DIR}"
)
