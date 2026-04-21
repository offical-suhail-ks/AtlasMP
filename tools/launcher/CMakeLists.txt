# tools/launcher/CMakeLists.txt
add_executable(AtlasMP-Launcher
    src/Main.cpp
    src/DllInjector.cpp
)

target_include_directories(AtlasMP-Launcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(AtlasMP-Launcher PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE
)

if(MSVC)
    target_link_options(AtlasMP-Launcher PRIVATE
        /SUBSYSTEM:CONSOLE /DYNAMICBASE /HIGHENTROPYVA /MANIFEST:NO
    )
    target_compile_options(AtlasMP-Launcher PRIVATE
        /W3 /MP /utf-8 /wd4100 /wd4244 /wd4996
        "$<$<CONFIG:Release>:/O2>"
    )
    set_target_properties(AtlasMP-Launcher PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

target_link_libraries(AtlasMP-Launcher PRIVATE
    comdlg32 shell32 advapi32 user32 ole32 Psapi
)

set_target_properties(AtlasMP-Launcher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ATLASMP_ROOT}/.output"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${ATLASMP_ROOT}/.output"
    RUNTIME_OUTPUT_DIRECTORY         "${ATLASMP_ROOT}/.output"
)
