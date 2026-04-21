// tests/server/TestPlayerManager.cpp
#include <gtest/gtest.h>
#include "../../server/include/PlayerManager.h"
using namespace Atlas;

TEST(PlayerManager, AddPlayer) {
    PlayerManager mgr;
    PlayerId id = mgr.AddPlayer("Suhail", "127.0.0.1");
    EXPECT_NE(id, INVALID_PLAYER);
    EXPECT_EQ(mgr.GetPlayerCount(), 1u);
}

TEST(PlayerManager, GetPlayer) {
    PlayerManager mgr;
    PlayerId id = mgr.AddPlayer("Suhail", "127.0.0.1");
    auto* p = mgr.GetPlayer(id);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name, "Suhail");
    EXPECT_EQ(p->ip,   "127.0.0.1");
    EXPECT_EQ(p->id,   id);
}

TEST(PlayerManager, GetNonExistentPlayer) {
    PlayerManager mgr;
    EXPECT_EQ(mgr.GetPlayer(999), nullptr);
}

TEST(PlayerManager, RemovePlayer) {
    PlayerManager mgr;
    PlayerId id = mgr.AddPlayer("Jaseel", "10.0.0.1");
    EXPECT_EQ(mgr.GetPlayerCount(), 1u);
    mgr.RemovePlayer(id);
    EXPECT_EQ(mgr.GetPlayerCount(), 0u);
    EXPECT_EQ(mgr.GetPlayer(id), nullptr);
}

TEST(PlayerManager, MultiplePlayersGetUniqueIds) {
    PlayerManager mgr;
    PlayerId a = mgr.AddPlayer("Alpha", "1.1.1.1");
    PlayerId b = mgr.AddPlayer("Beta",  "2.2.2.2");
    PlayerId c = mgr.AddPlayer("Gamma", "3.3.3.3");
    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
    EXPECT_NE(a, c);
    EXPECT_EQ(mgr.GetPlayerCount(), 3u);
}

TEST(PlayerManager, PlayerDataStore) {
    PlayerManager mgr;
    PlayerId id = mgr.AddPlayer("Suhail", "127.0.0.1");
    auto* p = mgr.GetPlayer(id);
    ASSERT_NE(p, nullptr);

    p->SetData("money", std::any(5000));
    auto val = p->GetData("money");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(std::any_cast<int>(val), 5000);
}

TEST(PlayerManager, GetAllPlayerIds) {
    PlayerManager mgr;
    mgr.AddPlayer("A", "1.0.0.0");
    mgr.AddPlayer("B", "2.0.0.0");
    auto ids = mgr.GetAllPlayerIds();
    EXPECT_EQ(ids.size(), 2u);
}

TEST(PlayerManager, HasPlayer) {
    PlayerManager mgr;
    PlayerId id = mgr.AddPlayer("X", "0.0.0.0");
    EXPECT_TRUE(mgr.HasPlayer(id));
    EXPECT_FALSE(mgr.HasPlayer(id + 100));
}
