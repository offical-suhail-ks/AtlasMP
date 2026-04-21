add_executable(AtlasMP-Injector
    src/Main.cpp
)

set_target_properties(AtlasMP-Injector PROPERTIES
    OUTPUT_NAME "AtlasInject"
    CXX_STANDARD 17
)

target_compile_definitions(AtlasMP-Injector PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
    _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES
)

target_link_libraries(AtlasMP-Injector PRIVATE
    Advapi32.lib
    Ntdll.lib
)

# ── Output to organized folder structure ─────────────────────────────────────
set_target_properties(AtlasMP-Injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ATLASMP_LAUNCHER_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${ATLASMP_LAUNCHER_DIR}"
    RUNTIME_OUTPUT_DIRECTORY         "${ATLASMP_LAUNCHER_DIR}"
)
