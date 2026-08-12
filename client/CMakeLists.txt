add_library(AtlasMP-Client SHARED
    src/core/dllmain.cpp
    src/core/Client.cpp
    src/core/Logger.cpp
    src/core/Memory.cpp
    src/hooks/HookManager.cpp
    src/hooks/NativeInvoker.cpp
    src/hooks/ScriptThread.cpp
    src/network/NetworkClient.cpp
    src/network/PacketQueue.cpp
    src/sync/SyncManager.cpp
    src/sync/LocalPlayer.cpp
    src/sync/RemotePlayer.cpp
    src/sync/RemotePed_1604.cpp
    src/ui/NUIManager.cpp
    src/ui/WindowBranding.cpp
    src/scripting/ClientScriptHost.cpp
)

target_include_directories(AtlasMP-Client
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/sdk/include
    PRIVATE ${CMAKE_SOURCE_DIR}/shared/include
    PRIVATE ${CMAKE_SOURCE_DIR}/client/vendor/minhook/include
    PRIVATE ${CMAKE_SOURCE_DIR}/client/vendor/plugin-sdk/plugin_SA/sdk
    PRIVATE ${CMAKE_SOURCE_DIR}/client/vendor/ScriptHookV/inc
)

if(DEFINED enet_SOURCE_DIR)
    target_include_directories(AtlasMP-Client PRIVATE ${enet_SOURCE_DIR}/include)
endif()

target_link_libraries(AtlasMP-Client
    PRIVATE atlas_shared
    PRIVATE enet
    PRIVATE minhook
    PRIVATE spdlog::spdlog
    PRIVATE nlohmann_json::nlohmann_json
)

target_link_libraries(AtlasMP-Client PRIVATE
    d3d11.lib
    dxgi.lib
    ws2_32.lib
    winmm.lib
    ${CMAKE_SOURCE_DIR}/client/vendor/ScriptHookV/lib/ScriptHookV.lib
)

set_target_properties(AtlasMP-Client PROPERTIES
    OUTPUT_NAME "AtlasMP-Client"
    SUFFIX ".asi"
    CXX_STANDARD 17
)

# No default entry point — we use DllMain
target_compile_definitions(AtlasMP-Client PRIVATE
    ATLAS_CLIENT_BUILD
    _USRDLL
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

# ── Output to organized folder structure ─────────────────────────────────────
set_target_properties(AtlasMP-Client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ATLASMP_CLIENT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${ATLASMP_CLIENT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY         "${ATLASMP_CLIENT_DIR}"
)
