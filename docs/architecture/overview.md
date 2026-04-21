# AtlasMP — Architecture Overview

## How It All Fits Together

AtlasMP has five main components that talk to each other:

```
  [Launcher] ──injects──► [Client DLL inside GTA5.exe]
                                    │
                         TCP/UDP (ENet) over network
                                    │
                          [Dedicated Server Binary]
                                    │
                        ┌───────────┼────────────┐
                   [Lua RT]    [JS RT]    [.NET RT]
                        └───────────┼────────────┘
                               [Resources]
```

---

## Component Deep Dives

### 1. Launcher (`tools/launcher/`)

The launcher is the player-facing entry point. It:
- Checks for GTA V installation (Steam, Epic, Rockstar Launcher paths)
- Downloads AtlasMP client files if missing or outdated
- Checks for updates against the AtlasMP update server
- Starts GTA V, then injects `AtlasMP-Client.dll` using `CreateRemoteThread` or a custom injector
- Shows a server browser UI so players can connect

**Key files:**
- `tools/launcher/src/Main.cpp` — entry point
- `tools/launcher/src/Injector.cpp` — DLL injection logic
- `tools/launcher/src/Updater.cpp` — download/update logic
- `tools/launcher/src/ServerBrowser.cpp` — server list UI

---

### 2. Client DLL (`client/`)

This is a Win64 DLL (`AtlasMP-Client.dll`) injected into the GTA V process. It:

1. **Waits** for the game to fully load
2. **Pattern-scans** `GTA5.exe` memory to find key RAGE engine functions
3. **Hooks** them using MinHook:
   - `scrThread::Tick()` → our main client tick every game frame
   - `IDXGISwapChain::Present()` → hook for NUI rendering overlay
   - Windows `WndProc` → capture input for NUI
4. **Connects** to the AtlasMP server over UDP (ENet)
5. **Reads** local player/vehicle state every tick → sends to server
6. **Receives** other players' state from server → applies to spawned NPC peds

#### RAGE Engine Hook Pattern

```
GTA5.exe memory layout:
  .text section
    └── scrEngine::RunScripts()
          └── scrThread::Tick(thread*)   ← we hook the vtable slot here
                └── [script code runs]
```

We hook `scrThread::Tick` to inject ourselves into the RAGE script loop. This gives us a safe, game-synchronized place to run our code every frame without threading issues.

**Pattern scan example (pseudocode):**
```cpp
// Find scrThread::Tick — pattern changes every GTA update!
// TODO: Update patterns for current GTA build
uintptr_t addr = PatternScanner::ScanIDA(
    "48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 55 41 56 41 57"
);
```

#### Native Invoker

GTA V script natives are called via a function pointer table:
```
NativeHash → (lookup in 0x100-bucket hash table) → NativeFunc*
NativeFunc*(NativeContext*)
```

We replicate this lookup to call any native by hash. This is the same mechanism ScriptHookV uses.

---

### 3. Server Binary (`server/`)

The server is a standalone C++ executable. It runs:

- **ENet UDP server** on the configured port
- **Tick loop** at 60Hz (configurable)
- **Resource loader** that scans `resources/` and starts configured resources
- **Sync manager** that holds server-authoritative entity state
- **Script host** that loads runtime DLLs and routes events

#### Server Tick Loop

```
Every 16.67ms (60Hz):
  1. Poll ENet — process all incoming packets
  2. Update entity states (apply received client deltas)
  3. Run script tick (timer callbacks, coroutines)
  4. Send state deltas to relevant clients
  5. Run anti-cheat validation
```

#### Entity Sync (Server-Authoritative)

The server owns all entity state. Clients send their **local** state and the server:
1. Validates it (anti-cheat checks)
2. Stores it as authoritative state
3. Broadcasts it to all other players in range

This means cheaters cannot affect other players by sending fake position data — the server validates everything.

---

### 4. Scripting Runtimes (`runtimes/`)

Each runtime is a separate DLL that implements `IScriptRuntime`. The server loads them dynamically at startup.

#### Runtime lifecycle
```
Server::Start()
  └── ScriptHost::LoadRuntimes()
        ├── LoadLibrary("runtimes/AtlasMP-Lua.dll")
        │     └── Atlas_CreateRuntime() → IScriptRuntime*
        ├── LoadLibrary("runtimes/AtlasMP-JS.dll")
        │     └── Atlas_CreateRuntime() → IScriptRuntime*
        └── LoadLibrary("runtimes/AtlasMP-DotNet.dll")
              └── Atlas_CreateRuntime() → IScriptRuntime*

ResourceLoader::StartResource("chat")
  └── detect language → "lua"
  └── luaRuntime->LoadResource("chat", "resources/chat/")
        └── new lua_State
        └── LuaBindings::Register(L)
        └── lua_dofile("server.lua")
```

#### Adding a new native function

To expose a new server function to scripts:
```cpp
// In ScriptHost.cpp
luaRuntime->RegisterNative("getPlayerPing", [](const ScriptArgs& args) -> ScriptValue {
    int playerId = args[0].intVal;
    int ping = playerManager->GetPlayer(playerId)->GetPing();
    return ScriptValue::Int(ping);
});
```

Then in Lua: `local ping = atlas.getPlayerPing(playerId)`

---

### 5. RPF Library (`tools/rpflib/`)

Rockstar Package Format v7 is a proprietary archive format GTA V uses for all assets. Our library lets AtlasMP:
- Read game RPF files to stream custom assets to clients
- Pack custom resources into RPF format for distribution

#### RPF7 Structure
```
RPF7Header (16 bytes)
  magic = 0x52504637
  entryCount
  namesLength
  encryption = OPEN | AES | NG

EntryTable (entryCount × 16 bytes)
  [DirectoryEntry or FileEntry per entry]

NamesSection (namesLength bytes)
  [null-terminated strings]

DataSection
  [file data, possibly compressed with zlib/oodle, possibly encrypted]
```

---

## Network Protocol

### Packet format

```
┌────────────────────────────────────────────────────────┐
│ PacketHeader (14 bytes)                                │
│   magic     [4]  = 0x41544C53 ('ATLS')                │
│   type      [2]  = PacketType enum                    │
│   size      [2]  = payload length                     │
│   sequence  [4]  = monotonic counter                  │
│   senderId  [2]  = player ID                          │
├────────────────────────────────────────────────────────┤
│ Payload (variable)                                     │
│   [packet-type-specific struct from Packets.h]        │
└────────────────────────────────────────────────────────┘
```

### Connection handshake

```
Client                              Server
  │                                   │
  │── HANDSHAKE_REQUEST ─────────────►│
  │   (protocol version, name, pw)    │
  │                                   │ validate password
  │                                   │ assign player ID
  │◄─ HANDSHAKE_RESPONSE ────────────│
  │   (accepted=true, playerId)       │
  │                                   │
  │◄─ SERVER_INFO ───────────────────│
  │◄─ PLAYER_LIST ───────────────────│
  │◄─ RESOURCE_START (foreach) ──────│
  │                                   │
  │   [connected, resources loading]  │
  │                                   │
  │══ PLAYER_STATE ═════════════════►│  (60Hz ongoing)
  │◄═ PLAYER_STATE (other players) ══│
```

---

## Anti-Cheat Architecture

AtlasMP uses **server-authority** as its primary anti-cheat mechanism:

1. **Position validation** — if a player moves faster than physically possible, reject the update
2. **Health validation** — players cannot gain health they weren't given by server script
3. **Weapon validation** — only allow weapon hashes that exist in the native DB
4. **Packet rate limiting** — kick players flooding the server with packets
5. **Sequence validation** — reject out-of-order or replayed packets

Planned advanced checks:
- Client integrity verification (hash known DLL regions)
- Encrypted client-server challenge-response
- Behavioral analysis (statistical anomaly detection)

---

## Adding a New Feature

Example: Adding a new `setPlayerWeather(playerId, weather)` server API.

1. **Add packet** in `shared/include/Packets.h`:
```cpp
struct SetWeatherPacket { uint8_t weatherType; };
// Add PACKET_SET_WEATHER = 0x0305 to PacketType enum
```

2. **Handle on server** in `server/src/scripting/ScriptHost.cpp`:
```cpp
luaRuntime->RegisterNative("setPlayerWeather", [&](const ScriptArgs& a) {
    int pid = a[0].intVal;
    int weather = a[1].intVal;
    SetWeatherPacket pkt{ (uint8_t)weather };
    network->SendTo(pid, PacketType::PACKET_SET_WEATHER, &pkt, sizeof(pkt));
    return ScriptValue::Null();
});
```

3. **Handle on client** in `client/src/network/PacketHandler.cpp`:
```cpp
case PacketType::PACKET_SET_WEATHER: {
    auto& pkt = *reinterpret_cast<const SetWeatherPacket*>(data);
    m_natives->Call<void>(Natives::SET_WEATHER_TYPE_NOW_PERSIST,
                          (int)pkt.weatherType);
    break;
}
```

4. **Expose in Lua**:
```lua
atlas.setPlayerWeather(playerId, 1) -- 1 = EXTRASUNNY
```
