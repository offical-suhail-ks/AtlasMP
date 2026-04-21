// runtimes/lua/src/LuaBindings.cpp
// Registers the atlas.* Lua API into a Lua state.
// This is what resource scripts call: atlas.on(), atlas.log(), atlas.getPlayers() etc.

#include "LuaBindings.h"
#include "LuaRuntime.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Atlas {

// ─── Internal event registry per Lua state ────────────────────────────────────
// We store event handlers as Lua functions in a table keyed by event name.
// Key: "__atlas_events" (Lua table in registry)

static const char* EVENTS_TABLE_KEY = "__atlas_events";

// ─── Helper: push a Vector3 table onto the Lua stack ─────────────────────────
static void PushVector3(lua_State* L, float x, float y, float z) {
    lua_newtable(L);
    lua_pushnumber(L, x); lua_setfield(L, -2, "x");
    lua_pushnumber(L, y); lua_setfield(L, -2, "y");
    lua_pushnumber(L, z); lua_setfield(L, -2, "z");
}

// ─── atlas.on(eventName, handler) ────────────────────────────────────────────
static int lua_atlas_on(lua_State* L) {
    const char* eventName = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // Get or create the events table in registry
    lua_getfield(L, LUA_REGISTRYINDEX, EVENTS_TABLE_KEY);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1); // dup
        lua_setfield(L, LUA_REGISTRYINDEX, EVENTS_TABLE_KEY);
    }

    // events[eventName] = events[eventName] or {}
    lua_getfield(L, -1, eventName);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, eventName);
    }

    // Append the handler function
    int len = (int)lua_rawlen(L, -1);
    lua_pushvalue(L, 2);          // push handler
    lua_rawseti(L, -2, len + 1);  // handlers[#handlers+1] = handler

    lua_pop(L, 2); // pop handler list + events table
    return 0;
}

// ─── atlas.emit(eventName, ...) ──────────────────────────────────────────────
static int lua_atlas_emit(lua_State* L) {
    const char* eventName = luaL_checkstring(L, 1);
    int nArgs = lua_gettop(L) - 1; // args after eventName

    lua_getfield(L, LUA_REGISTRYINDEX, EVENTS_TABLE_KEY);
    if (lua_isnil(L, -1)) { lua_pop(L, 1); return 0; }

    lua_getfield(L, -1, eventName);
    if (lua_isnil(L, -1)) { lua_pop(L, 2); return 0; }

    // Iterate handlers
    int len = (int)lua_rawlen(L, -1);
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i); // push handler
        // Push original args
        for (int j = 0; j < nArgs; j++) {
            lua_pushvalue(L, 2 + j);
        }
        lua_pcall(L, nArgs, 0, 0);
    }

    lua_pop(L, 2);
    return 0;
}

// ─── __atlas_dispatch_event (called from C++ side) ───────────────────────────
static int lua_atlas_dispatch_event(lua_State* L) {
    return lua_atlas_emit(L);
}

// ─── atlas.log(message) ──────────────────────────────────────────────────────
static int lua_atlas_log(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    // TODO: Route to server logger with resource name prefix
    printf("[Lua] %s\n", msg);
    return 0;
}

// ─── atlas.getPlayers() → table of player objects ────────────────────────────
static int lua_atlas_getPlayers(lua_State* L) {
    // TODO: Pull from PlayerManager via C++ upvalue
    lua_newtable(L);
    return 1;
}

// ─── atlas.getPlayerName(playerId) → string ──────────────────────────────────
static int lua_atlas_getPlayerName(lua_State* L) {
    // int playerId = (int)luaL_checkinteger(L, 1);
    // TODO: Look up player name from PlayerManager
    lua_pushstring(L, "Unknown");
    return 1;
}

// ─── atlas.kickPlayer(playerId, reason) ──────────────────────────────────────
static int lua_atlas_kickPlayer(lua_State* L) {
    int playerId = (int)luaL_checkinteger(L, 1);
    const char* reason = luaL_optstring(L, 2, "Kicked by server");
    // TODO: Call NetworkServer::KickPlayer(playerId, reason)
    (void)playerId; (void)reason;
    return 0;
}

// ─── atlas.broadcast(message) ────────────────────────────────────────────────
static int lua_atlas_broadcast(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    // TODO: Send PLAYER_CHAT packet to all players
    printf("[Broadcast] %s\n", msg);
    return 0;
}

// ─── atlas.setPlayerData(playerId, key, value) ────────────────────────────────
static int lua_atlas_setPlayerData(lua_State* L) {
    // int playerId = (int)luaL_checkinteger(L, 1);
    // const char* key = luaL_checkstring(L, 2);
    // value is at stack index 3
    // TODO: Store in PlayerManager KV store
    return 0;
}

// ─── atlas.getPlayerData(playerId, key) → value ──────────────────────────────
static int lua_atlas_getPlayerData(lua_State* L) {
    // int playerId = (int)luaL_checkinteger(L, 1);
    // const char* key = luaL_checkstring(L, 2);
    // TODO: Retrieve from PlayerManager KV store
    lua_pushnil(L);
    return 1;
}

// ─── atlas.spawnPlayer(playerId, x, y, z) ────────────────────────────────────
static int lua_atlas_spawnPlayer(lua_State* L) {
    int playerId = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    // TODO: Send PLAYER_SPAWN packet
    (void)playerId; (void)x; (void)y; (void)z;
    return 0;
}

// ─── atlas.triggerClientEvent(playerId, event, ...) ──────────────────────────
static int lua_atlas_triggerClientEvent(lua_State* L) {
    int playerId       = (int)luaL_checkinteger(L, 1);
    const char* event  = luaL_checkstring(L, 2);
    // Remaining args encoded as JSON and sent as SCRIPT_EVENT packet
    (void)playerId; (void)event;
    return 0;
}

// ─── atlas.triggerAllClientsEvent(event, ...) ────────────────────────────────
static int lua_atlas_triggerAllClientsEvent(lua_State* L) {
    const char* event = luaL_checkstring(L, 1);
    (void)event;
    return 0;
}

// ─── Vector3 constructor ──────────────────────────────────────────────────────
static int lua_Vector3(lua_State* L) {
    float x = (float)luaL_optnumber(L, 1, 0.0);
    float y = (float)luaL_optnumber(L, 2, 0.0);
    float z = (float)luaL_optnumber(L, 3, 0.0);
    PushVector3(L, x, y, z);
    return 1;
}

// ─── Registration ─────────────────────────────────────────────────────────────
static const luaL_Reg atlas_funcs[] = {
    { "on",                    lua_atlas_on                    },
    { "emit",                  lua_atlas_emit                  },
    { "log",                   lua_atlas_log                   },
    { "getPlayers",            lua_atlas_getPlayers            },
    { "getPlayerName",         lua_atlas_getPlayerName         },
    { "kickPlayer",            lua_atlas_kickPlayer            },
    { "broadcast",             lua_atlas_broadcast             },
    { "setPlayerData",         lua_atlas_setPlayerData         },
    { "getPlayerData",         lua_atlas_getPlayerData         },
    { "spawnPlayer",           lua_atlas_spawnPlayer           },
    { "triggerClientEvent",    lua_atlas_triggerClientEvent    },
    { "triggerAllClientsEvent",lua_atlas_triggerAllClientsEvent},
    { nullptr, nullptr }
};

void LuaBindings::Register(lua_State* L, const std::string& resourceName) {
    // Register atlas table
    luaL_newlib(L, atlas_funcs);
    lua_setglobal(L, "atlas");

    // Register Vector3 constructor
    lua_pushcfunction(L, lua_Vector3);
    lua_setglobal(L, "Vector3");

    // Register internal event dispatcher (called from C++)
    lua_pushcfunction(L, lua_atlas_dispatch_event);
    lua_setglobal(L, "__atlas_dispatch_event");

    // Resource name constant
    lua_pushstring(L, resourceName.c_str());
    lua_setglobal(L, "RESOURCE_NAME");

    // Compat aliases
    lua_getglobal(L, "atlas");
    lua_getfield(L, -1, "on");
    lua_setglobal(L, "AddEventHandler");  // FiveM-like alias

    lua_getfield(L, -1, "emit");
    lua_setglobal(L, "TriggerEvent");     // FiveM-like alias

    lua_pop(L, 1); // pop atlas table
}

} // namespace Atlas
