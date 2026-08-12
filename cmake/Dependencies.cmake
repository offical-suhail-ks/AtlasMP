# cmake/Dependencies.cmake
# Fetch and configure all third-party dependencies

include(FetchContent)

# ─── ENet (UDP networking) ──────────────────────────────────────────────────
# http://enet.bespin.org/
FetchContent_Declare(enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        origin/master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(enet)

# ─── MinHook (x86/x64 inline hooking) ───────────────────────────────────────
# https://github.com/TsudaKageyu/minhook
FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(minhook)

# ─── LuaJIT (fast Lua 5.1 implementation) ───────────────────────────────────
# https://github.com/LuaJIT/LuaJIT
# NOTE: LuaJIT requires special build steps on Windows (build with MSVC or MinGW)
# For now, use a CMake-compatible fork
FetchContent_Declare(luajit
    GIT_REPOSITORY https://github.com/WohlSoft/LuaJIT.git
    GIT_TAG        v2.1
    GIT_SHALLOW    TRUE
)
# FetchContent_MakeAvailable(luajit)  # Uncomment when ready

# ─── toml++ (TOML config parser, header-only) ───────────────────────────────
# https://github.com/marzer/tomlplusplus
FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

# ─── spdlog (fast logging) ───────────────────────────────────────────────────
# https://github.com/gabime/spdlog
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

# ─── nlohmann/json (JSON for config/packets) ────────────────────────────────
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# ─── SQLite (embedded database) ─────────────────────────────────────────────
FetchContent_Declare(sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
)
FetchContent_MakeAvailable(sqlite3)

# ─── GoogleTest (unit testing) ───────────────────────────────────────────────
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)