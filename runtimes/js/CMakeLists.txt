add_library(AtlasMP-JS SHARED
    src/JSRuntime.cpp
    src/JSBindings.cpp
)

target_include_directories(AtlasMP-JS
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/sdk/include
    # Uncomment when V8 or QuickJS is integrated:
    # PRIVATE ${CMAKE_SOURCE_DIR}/server/vendor/quickjs
)

target_link_libraries(AtlasMP-JS
    PRIVATE atlas_shared
    PRIVATE spdlog::spdlog
    # PRIVATE quickjs   -- uncomment when ready
)

set_target_properties(AtlasMP-JS PROPERTIES
    OUTPUT_NAME "AtlasMP-JS"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/runtimes"
    CXX_STANDARD 17
)
