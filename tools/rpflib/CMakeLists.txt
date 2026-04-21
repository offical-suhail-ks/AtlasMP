add_library(AtlasMP-RPFLib STATIC
    src/RPFArchive.cpp
    src/RPFEntry.cpp
    src/RPFDecrypt.cpp
)

target_include_directories(AtlasMP-RPFLib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(AtlasMP-RPFLib PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
)

target_link_libraries(AtlasMP-RPFLib
    PRIVATE spdlog::spdlog
)

# zlib for decompression
find_package(ZLIB)
if(ZLIB_FOUND)
    target_link_libraries(AtlasMP-RPFLib PRIVATE ZLIB::ZLIB)
    target_compile_definitions(AtlasMP-RPFLib PRIVATE ATLAS_HAVE_ZLIB)
else()
    message(WARNING "zlib not found — RPFLib decompression disabled")
endif()

set_target_properties(AtlasMP-RPFLib PROPERTIES
    CXX_STANDARD 17
)
