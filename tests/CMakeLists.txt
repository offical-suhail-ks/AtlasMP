include(GoogleTest)

add_executable(AtlasMP-Tests
    server/TestNetworking.cpp
    server/TestResourceLoader.cpp
    server/TestPlayerManager.cpp
    server/TestAntiCheat.cpp
    client/TestPacketCodec.cpp
    client/TestMath.cpp
    integration/TestHandshake.cpp
    ../server/src/core/PlayerManager.cpp
    ../server/src/resources/ResourceLoader.cpp
)

target_include_directories(AtlasMP-Tests PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/include
    ${CMAKE_SOURCE_DIR}/shared/src
    ${CMAKE_SOURCE_DIR}/server/include
    ${CMAKE_SOURCE_DIR}/server/src
    ${CMAKE_SOURCE_DIR}/server/src/core
    ${CMAKE_SOURCE_DIR}/server/src/network
    ${CMAKE_SOURCE_DIR}/server/src/resources
    ${CMAKE_SOURCE_DIR}/server/src/sync
    ${CMAKE_SOURCE_DIR}/server/src/scripting
    ${CMAKE_SOURCE_DIR}/server/src/database
    ${CMAKE_SOURCE_DIR}/client/src
    ${CMAKE_SOURCE_DIR}/sdk/include
)

target_link_libraries(AtlasMP-Tests PRIVATE
    atlas_shared
    GTest::gtest
    GTest::gtest_main
    spdlog::spdlog
    tomlplusplus::tomlplusplus
)

gtest_discover_tests(AtlasMP-Tests)
