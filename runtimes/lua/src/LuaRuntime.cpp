// runtimes/lua/src/LuaRuntime.cpp
#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifdef _MSC_VER
#  include <corecrt_math.h>
#endif
#include <cmath>
#include <cstdlib>

#include "LuaRuntime.h"
#include "LuaBindings.h"
#include "../../../sdk/include/IScriptRuntime.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace Atlas {

LuaEnvironment::~LuaEnvironment() {
    if (L) { lua_close(L); L = nullptr; }
}

// ─── LuaResource::Execute ────────────────────────────────────────────────────
bool LuaResource::Execute(const std::string& filePath) {
    if (!m_env || !m_env->L) return false;
    int r = luaL_dofile(m_env->L, filePath.c_str());
    if (r != LUA_OK) {
        const char* err = lua_tostring(m_env->L, -1);
        fprintf(stderr, "[Lua:%s] Error loading %s: %s\n",
                m_name.c_str(), filePath.c_str(), err ? err : "unknown");
        lua_pop(m_env->L, 1);
        return false;
    }
    return true;
}

// ─── LuaResource::CallExport ─────────────────────────────────────────────────
ScriptValue LuaResource::CallExport(const std::string& funcName,
                                     const ScriptArgs& args)
{
    if (!m_env || !m_env->L) return ScriptValue::Null();
    lua_State* L = m_env->L;

    lua_getglobal(L, funcName.c_str());
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return ScriptValue::Null(); }

    for (const auto& a : args) {
        switch (a.type) {
        case ScriptValueType::Boolean: lua_pushboolean(L, a.boolVal); break;
        case ScriptValueType::Integer: lua_pushinteger(L, (lua_Integer)a.intVal); break;
        case ScriptValueType::Float:   lua_pushnumber(L, (lua_Number)a.floatVal); break;
        case ScriptValueType::String:  lua_pushstring(L, a.strVal.c_str()); break;
        default:                       lua_pushnil(L); break;
        }
    }

    if (lua_pcall(L, (int)args.size(), 1, 0) != LUA_OK) {
        lua_pop(L, 1);
        return ScriptValue::Null();
    }

    ScriptValue ret;
    if      (lua_isboolean(L, -1)) ret = ScriptValue::Bool(lua_toboolean(L,-1)!=0);
    else if (lua_isinteger(L, -1)) ret = ScriptValue::Int(lua_tointeger(L,-1));
    else if (lua_isnumber(L,-1))   ret = ScriptValue::Float(lua_tonumber(L,-1));
    else if (lua_isstring(L,-1))   ret = ScriptValue::String(lua_tostring(L,-1));
    lua_pop(L, 1);
    return ret;
}

// ─── LuaRuntime ──────────────────────────────────────────────────────────────
bool LuaRuntime::Initialize(uint32_t /*sdkVersion*/) {
    m_initialized = true;
    return true;
}

void LuaRuntime::Shutdown() {
    for (auto& env : m_environments) {
        if (env && env->L) {
            lua_getglobal(env->L, "__atlas_dispatch_event");
            if (lua_isfunction(env->L, -1)) {
                lua_pushstring(env->L, "resourceStop");
                lua_pcall(env->L, 1, 0, 0);
            } else lua_pop(env->L, 1);
        }
    }
    m_resources.clear();
    m_environments.clear();
    m_initialized = false;
}

void LuaRuntime::Tick() {
    // Drive per-resource timers via __atlas_tick if defined
    for (auto& res : m_resources) {
        if (!res->m_env || !res->m_env->L) continue;
        lua_getglobal(res->m_env->L, "__atlas_tick");
        if (lua_isfunction(res->m_env->L, -1)) {
            lua_pcall(res->m_env->L, 0, 0, 0);
        } else {
            lua_pop(res->m_env->L, 1);
        }
    }
}

IResource* LuaRuntime::LoadResource(const std::string& name,
                                     const std::string& basePath)
{
    auto env = std::make_unique<LuaEnvironment>();
    env->resourceName = name;
    env->basePath     = basePath;

    env->L = luaL_newstate();
    if (!env->L) return nullptr;

    // Open safe standard libs
    luaopen_base(env->L);
    luaopen_table(env->L);
    luaopen_string(env->L);
    luaopen_math(env->L);
    luaopen_os(env->L);    // limited to time functions in production
    luaopen_io(env->L);    // TODO: sandbox to resource dir

    // Remove dangerous globals
    const char* blocked[] = {"dofile","loadfile","load","require",nullptr};
    for (int i = 0; blocked[i]; i++) {
        lua_pushnil(env->L);
        lua_setglobal(env->L, blocked[i]);
    }

    LuaBindings::Register(env->L, name);

    lua_pushstring(env->L, name.c_str());
    lua_setglobal(env->L, "RESOURCE_NAME");
    lua_pushstring(env->L, basePath.c_str());
    lua_setglobal(env->L, "RESOURCE_PATH");

    env->running = true;
    LuaEnvironment* rawPtr = env.get();
    m_environments.push_back(std::move(env));

    auto res = std::make_unique<LuaResource>(name, basePath, rawPtr);

    // Auto-load server.lua if it exists
    std::string serverScript = basePath + "/server.lua";
    if (std::filesystem::exists(serverScript)) {
        res->Execute(serverScript);
    }

    IResource* ptr = res.get();
    m_resources.push_back(std::move(res));
    return ptr;
}

void LuaRuntime::UnloadResource(IResource* resource) {
    TriggerResourceEvent(resource, "resourceStop", {});
    auto* lr = static_cast<LuaResource*>(resource);
    // Remove environment
    m_environments.erase(
        std::remove_if(m_environments.begin(), m_environments.end(),
            [lr](const auto& e){ return e.get() == lr->m_env; }),
        m_environments.end());
    m_resources.erase(
        std::remove_if(m_resources.begin(), m_resources.end(),
            [resource](const auto& r){ return r.get() == resource; }),
        m_resources.end());
}

void LuaRuntime::TriggerEvent(const std::string& name, const ScriptArgs& args) {
    for (auto& res : m_resources)
        TriggerResourceEvent(res.get(), name, args);
}

void LuaRuntime::TriggerResourceEvent(IResource* resource,
                                       const std::string& name,
                                       const ScriptArgs& args)
{
    auto* lr = static_cast<LuaResource*>(resource);
    if (!lr || !lr->m_env || !lr->m_env->L) return;
    lua_State* L = lr->m_env->L;

    lua_getglobal(L, "__atlas_dispatch_event");
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return; }

    lua_pushstring(L, name.c_str());
    for (const auto& a : args) {
        switch (a.type) {
        case ScriptValueType::Boolean: lua_pushboolean(L, a.boolVal); break;
        case ScriptValueType::Integer: lua_pushinteger(L, (lua_Integer)a.intVal); break;
        case ScriptValueType::Float:   lua_pushnumber(L, (lua_Number)a.floatVal); break;
        case ScriptValueType::String:  lua_pushstring(L, a.strVal.c_str()); break;
        default:                       lua_pushnil(L); break;
        }
    }
    if (lua_pcall(L, 1 + (int)args.size(), 0, 0) != LUA_OK) {
        lua_pop(L, 1);
    }
}

void LuaRuntime::RegisterNative(const std::string& name, ScriptCallback cb) {
    m_natives[name] = std::move(cb);
}

extern "C" {
    __declspec(dllexport) IScriptRuntime* Atlas_CreateRuntime() { return new LuaRuntime(); }
    __declspec(dllexport) uint32_t Atlas_GetSDKVersion() { return ATLAS_SDK_VERSION; }
}

} // namespace Atlas
