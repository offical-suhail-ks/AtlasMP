# cmake/CompilerFlags.cmake
# Compiler flags — MSVC 19.50 (VS2026) compatible

if(MSVC)
    # Force-include our compatibility header as the VERY FIRST thing
    # in every translation unit. This is the only reliable way to fix
    # MSVC 19.50's broken include ordering for strtol/strtod/<string>.
    set(COMPAT_HEADER "${CMAKE_SOURCE_DIR}/cmake/msvc_compat.h")
    # /FI requires the path without spaces — if your path has spaces,
    # use the short 8.3 path or move the project to a path without spaces.
    add_compile_options("/FI${COMPAT_HEADER}")

    add_compile_options(/W3 /MP /Zi /utf-8)
    add_compile_options(/wd4100 /wd4505 /wd4244 /wd4996 /wd4267 /wd4127 /wd4530 /wd4005 /wd4702)
    add_compile_options("$<$<CONFIG:Release>:/O2>")
    add_compile_options("$<$<CONFIG:Release>:/DNDEBUG>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options("$<$<CONFIG:Debug>:-g>")
endif()

add_compile_definitions(ATLAS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(ATLAS_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(ATLAS_VERSION_PATCH=${PROJECT_VERSION_PATCH})
add_compile_definitions(ATLAS_VERSION_STRING="${PROJECT_VERSION}")
add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(NOMINMAX)
add_compile_definitions(_USE_MATH_DEFINES)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)
add_compile_definitions("$<$<CONFIG:Debug>:ATLAS_DEBUG>")
add_compile_definitions("$<$<CONFIG:Release>:ATLAS_RELEASE>")
