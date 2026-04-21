// runtimes/lua/src/LuaResource.cpp
#include "LuaRuntime.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace Atlas {

ScriptValue LuaResource::CallExport(const std::string& funcName,
                                     const ScriptArgs& args)
{
    if (!m_env || !m_env->L) return ScriptValue::Null();
    lua_State* L = m_env->L;

    lua_getglobal(L, funcName.c_str());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return ScriptValue::Null();
    }

    for (const auto& arg : args) {
        switch (arg.type) {
        case ScriptValueType::Boolean: lua_pushboolean(L, arg.boolVal);        break;
        case ScriptValueType::Integer: lua_pushinteger(L, arg.intVal);         break;
        case ScriptValueType::Float:   lua_pushnumber(L, arg.floatVal);        break;
        case ScriptValueType::String:  lua_pushstring(L, arg.strVal.c_str()); break;
        default:                       lua_pushnil(L);                         break;
        }
    }

    if (lua_pcall(L, (int)args.size(), 1, 0) != LUA_OK) {
        lua_pop(L, 1);
        return ScriptValue::Null();
    }

    ScriptValue result;
    if (lua_isboolean(L, -1))      result = ScriptValue::Bool(lua_toboolean(L, -1) != 0);
    else if (lua_isinteger(L, -1)) result = ScriptValue::Int(lua_tointeger(L, -1));
    else if (lua_isnumber(L, -1))  result = ScriptValue::Float(lua_tonumber(L, -1));
    else if (lua_isstring(L, -1))  result = ScriptValue::String(lua_tostring(L, -1));
    lua_pop(L, 1);
    return result;
}

} // namespace Atlas
