#pragma once
// runtimes/lua/src/LuaBindings.h

#include <string>
struct lua_State;

namespace Atlas {

class LuaBindings {
public:
    /// Register the full atlas.* API into a Lua state
    static void Register(lua_State* L, const std::string& resourceName);
};

} // namespace Atlas
