# AtlasMP Scripting API Reference

> Available in: Lua | JavaScript | C#
> Context: Server-side unless marked `[CLIENT]`

---

## Events

### `atlas.on(eventName, handler)`
Subscribe to a named event.

```lua
atlas.on("playerJoin", function(playerId)
    atlas.log("Player " .. playerId .. " joined")
end)
```

### `atlas.emit(eventName, ...args)`
Trigger a local event (same resource, same side).

```lua
atlas.emit("myCustomEvent", 42, "hello")
```

### `atlas.triggerClientEvent(playerId, event, ...args)`
Send an event to a specific player's client scripts.

```lua
atlas.triggerClientEvent(playerId, "showHUD", true)
```

### `atlas.triggerAllClientsEvent(event, ...args)`
Send an event to all connected clients.

```lua
atlas.triggerAllClientsEvent("setWeather", "EXTRASUNNY")
```

---

## Players

### `atlas.getPlayers()` → `table`
Returns array of all connected player IDs.

```lua
local players = atlas.getPlayers()
for _, id in ipairs(players) do
    atlas.log(atlas.getPlayerName(id))
end
```

### `atlas.getPlayerName(playerId)` → `string`

### `atlas.getPlayerPing(playerId)` → `number`

### `atlas.getPlayerIdentifiers(playerId)` → `table`
Returns `{ ip = "...", steamId = "...", discordId = "..." }`

### `atlas.kickPlayer(playerId, reason)`
```lua
atlas.kickPlayer(playerId, "You were kicked.")
```

### `atlas.banPlayer(playerId, reason, duration)`
`duration` in seconds. 0 = permanent.

### `atlas.spawnPlayer(playerId, x, y, z [, heading])`
```lua
atlas.spawnPlayer(playerId, -269.0, -955.0, 31.2, 180.0)
```

### `atlas.setPlayerModel(playerId, modelHash)`
```lua
atlas.setPlayerModel(playerId, 0xA8683715) -- Michael
```

### `atlas.setPlayerData(playerId, key, value)`
### `atlas.getPlayerData(playerId, key)` → `any`

Per-player server-side key-value store. Cleared on disconnect.

```lua
atlas.setPlayerData(playerId, "money", 5000)
local money = atlas.getPlayerData(playerId, "money")
```

### `atlas.givePlayerWeapon(playerId, weaponHash, ammo)`
```lua
atlas.givePlayerWeapon(playerId, 0x1B06D571, 100) -- Pistol
```

---

## Vehicles

### `atlas.createVehicle(modelHash, x, y, z, heading)` → `vehicleId`
```lua
local vid = atlas.createVehicle(0x18D5FA52, -269.0, -955.0, 31.2, 0.0)
```

### `atlas.deleteVehicle(vehicleId)`

### `atlas.setVehicleData(vehicleId, key, value)`
### `atlas.getVehicleData(vehicleId, key)` → `any`

### `atlas.getVehicleDriver(vehicleId)` → `playerId | nil`

---

## World

### `atlas.setWeather(weather)` → broadcasts to all clients
```lua
atlas.setWeather("EXTRASUNNY")
atlas.setWeather("RAIN")
atlas.setWeather("FOGGY")
```

### `atlas.setTime(hour, minute)`
```lua
atlas.setTime(12, 0) -- Noon
```

### `atlas.setGravity(multiplier)`
```lua
atlas.setGravity(0.5) -- Half gravity
```

---

## Utilities

### `atlas.log(message)`
Logs to server console and log file.

### `atlas.logWarning(message)`
### `atlas.logError(message)`

### `atlas.wait(ms)` *(Lua coroutine / JS async)*
Non-blocking wait inside a thread.

```lua
-- Lua: must be inside a Citizen.CreateThread or coroutine
Citizen.CreateThread(function()
    atlas.wait(3000)
    atlas.log("3 seconds later")
end)
```

```js
// JS: use async/await
atlas.on('playerJoin', async (playerId) => {
    await atlas.wait(3000);
    atlas.log('3 seconds later');
});
```

### `atlas.broadcast(message)`
Send a chat message to all players.

### `atlas.sendChat(playerId, message)`
Send a chat message to one player.

---

## Built-in Event Reference

These events are fired by the AtlasMP core:

| Event | Args | Description |
|---|---|---|
| `resourceStart` | — | Resource just started |
| `resourceStop` | — | Resource about to stop |
| `playerJoin` | `playerId` | Player connected and authenticated |
| `playerLeave` | `playerId, reason` | Player disconnected |
| `playerChat` | `playerId, message` | Player sent a chat message |
| `playerCommand` | `playerId, command` | Player typed a `/command` |
| `playerDeath` | `playerId, killerId, weaponHash` | Player died |
| `playerSpawn` | `playerId` | Player spawned in world |
| `playerEnterVehicle` | `playerId, vehicleId, seat` | Player entered vehicle |
| `playerLeaveVehicle` | `playerId, vehicleId` | Player left vehicle |
| `vehicleDestroyed` | `vehicleId, destroyerId` | Vehicle exploded/destroyed |
| `serverTick` | `deltaTime` | Every server tick (60Hz) |

---

## Client-Side API `[CLIENT]`

Client scripts run inside GTA V and can call natives.

### `atlas.on(event, handler)` — same as server

### `native.CALL(hash, ...args)` → `any`
Call a GTA V native directly.
```lua
-- Get local player ped
local ped = native.CALL(0x43A66C31C68491C0, atlas.localPlayerId)
```

### `atlas.localPlayer` → `playerId`

### `atlas.sendNUIMessage(data)` `[CLIENT]`
Send a message to the NUI (HTML) overlay.
```lua
atlas.sendNUIMessage({ action = "updateHUD", health = 100 })
```

### Client Events

| Event | Description |
|---|---|
| `resourceStart` | Resource started on client |
| `playerSpawn` | Local player spawned |
| `nuiCallback` | Message received from NUI |
| `keyDown` | Keyboard input |

---

## Type Reference

### `Vector3`
```lua
local pos = Vector3(0.0, 0.0, 0.0)
pos.x, pos.y, pos.z
```

### Weapon Hashes
Common weapon hashes (from [alloc8or's nativedb](https://alloc8or.re/gta5/nativedb/)):
```
WEAPON_PISTOL        = 0x1B06D571
WEAPON_MICROSMG      = 0x13532244
WEAPON_ASSAULTRIFLE  = 0xBFEFFF6D
WEAPON_SNIPERRIFLE   = 0x05FC3C11
WEAPON_RPG           = 0xB1CA77B1
```

### Vehicle Model Hashes
Use the [GTA V vehicle hash list](https://wiki.gtanet.work/index.php/Vehicle_Models) or query via `atlas.getVehicleHashByName("adder")`.

---

## C# SDK Reference

```csharp
// Inherit from AtlasResource
public class MyResource : AtlasResource {
    public override void OnStart() { ... }
    public override void OnStop()  { ... }
}

// Static API (same as atlas.* in Lua/JS)
Atlas.On<int>("playerJoin", OnPlayerJoin);
Atlas.Log("message");
Atlas.Broadcast("hello all");
Atlas.SpawnPlayer(playerId, x, y, z);
Atlas.GetPlayerName(playerId);
Atlas.TriggerClientEvent(playerId, "event", args...);
Atlas.GetPlayers() → List<int>
```
