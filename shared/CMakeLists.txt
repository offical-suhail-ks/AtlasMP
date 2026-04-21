add_library(atlas_shared STATIC
    src/Math.cpp
    src/PacketCodec.cpp
    src/Utils.cpp
)

target_include_directories(atlas_shared
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(atlas_shared PUBLIC
    _USE_MATH_DEFINES
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
)

# NO spdlog here — spdlog pulls in <cmath> which breaks MSVC 19.50
# when math.h ordering is wrong. Server links spdlog directly.
target_link_libraries(atlas_shared
    PUBLIC nlohmann_json::nlohmann_json
)

set_target_properties(atlas_shared PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
)
