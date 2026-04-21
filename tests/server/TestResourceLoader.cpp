// tests/server/TestResourceLoader.cpp
#include <gtest/gtest.h>
#include <filesystem>
#include "../../server/src/resources/ResourceLoader.h"
using namespace Atlas;

TEST(ResourceLoader, InitiallyEmpty) {
    ResourceLoader loader;
    EXPECT_TRUE(loader.GetAll().empty());
}

TEST(ResourceLoader, GetNonExistentResource) {
    ResourceLoader loader;
    EXPECT_EQ(loader.GetResource("nonexistent"), nullptr);
}

TEST(ResourceLoader, StopNonRunningResource) {
    ResourceLoader loader;
    EXPECT_FALSE(loader.StopResource("nonexistent"));
}

TEST(ResourceLoader, StartNonExistentResource) {
    ResourceLoader loader;
    EXPECT_FALSE(loader.StartResource("doesnotexist"));
}

TEST(ResourceLoader, ScanNonExistentDir) {
    ResourceLoader loader;
    // Should not throw, just create the dir or warn
    EXPECT_NO_THROW(loader.ScanDirectory("test_resources_that_dont_exist"));
    EXPECT_TRUE(loader.GetAll().empty());

    // Cleanup
    std::filesystem::remove("test_resources_that_dont_exist");
}
