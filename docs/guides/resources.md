# Writing AtlasMP Resources

Resources are the building blocks of your server. Every script, gamemode, and feature is a resource.

---

## What is a Resource?

A resource is a **folder** with:
1. A `resource.toml` manifest
2. One or more script files (`.lua`, `.js`, or `.cs`)
3. Optional client-side scripts and files to stream

```
my-resource/
├── resource.toml      ← Required
├── server.lua         ← Server-side script
├── client.lua         ← Client-side script (optional)
└── ui/
    └── index.html     ← NUI file (optional)
```

---

## resource.toml Reference

```toml
[resource]
name        = "my-resource"        # Must match folder name
version     = "1.0.0"
author      = "YourName"
description = "What this does"

[scripts]
server = ["server.lua"]            # Server-side scripts (run on server)
client = ["client.lua"]            # Client-side scripts (run on each player's PC)
shared = ["shared/utils.lua"]      # Shared scripts (run on both)

[files]
stream = ["ui/index.html", "ui/style.css"]  # Files streamed to clients

[dependencies]
requires = ["chat", "utils"]       # Resources that must start first
```

---

## Lua Resource

### server.lua

```lua
-- Called when resource starts
atlas.on("resourceStart", function()
    atlas.log("My resource started!")
end)

-- React to player joining
atlas.on("playerJoin", function(playerId)
    local name = atlas.getPlayerName(playerId)

    -- Give player welcome money
    atlas.setPlayerData(playerId, "money", 500)

    -- Spawn them
    atlas.spawnPlayer(playerId, -269.0, -955.0, 31.2)

    -- Welcome message
    atlas.broadcast(name .. " joined! Welcome!")
end)

-- Handle a client event
atlas.on("requestMoney", function(playerId, amount)
    local current = atlas.getPlayerData(playerId, "money") or 0
    atlas.setPlayerData(playerId, "money", current + amount)
    atlas.triggerClientEvent(playerId, "updateHUD", { money = current + amount })
end)
```

### client.lua

```lua
-- Called on client when resource starts
atlas.on("resourceStart", function()
    -- Nothing to do
end)

-- Called when local player spawns
atlas.on("playerSpawn", function()
    -- Restore HUD
    native.CALL(0xEF9A5B6E), 1) -- DISPLAY_HUD
end)

-- Called when server sends us an event
atlas.on("updateHUD", function(data)
    -- Update the NUI overlay
    atlas.sendNUIMessage({
        action = "setMoney",
        value  = data.money
    })
end)

-- NUI message from HTML back to Lua
atlas.on("nuiCallback", function(data)
    if data.action == "buyItem" then
        -- Tell server
        atlas.triggerServerEvent("buyItem", data.itemId)
    end
end)
```

---

## JavaScript Resource

### server.js

```js
atlas.on('resourceStart', () => {
    atlas.log('Resource started (JS)');
});

atlas.on('playerJoin', async (playerId) => {
    const name = atlas.getPlayerName(playerId);
    atlas.broadcast(`${name} joined!`);

    await atlas.wait(1000);
    atlas.spawnPlayer(playerId, -269.0, -955.0, 31.2);
});

atlas.on('playerDeath', async (playerId, killerId) => {
    await atlas.wait(5000);
    atlas.spawnPlayer(playerId, -269.0, -955.0, 31.2);
});
```

---

## C# Resource

### MyResource.cs

```csharp
using AtlasMP.SDK;

public class MyResource : AtlasResource
{
    public override void OnStart()
    {
        Atlas.Log("Resource started!");

        Atlas.On<int>("playerJoin", async (playerId) =>
        {
            string name = Atlas.GetPlayerName(playerId);
            Atlas.Broadcast($"{name} joined!");

            await Task.Delay(1000);
            Atlas.SpawnPlayer(playerId, -269.0f, -955.0f, 31.2f);
        });
    }

    public override void OnStop()
    {
        Atlas.Log("Resource stopped.");
    }
}
```

### MyResource.csproj

```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net8.0</TargetFramework>
    <OutputType>Library</OutputType>
  </PropertyGroup>
  <ItemGroup>
    <Reference Include="AtlasMP.SDK">
      <HintPath>$(ATLAS_SDK_PATH)\AtlasMP.SDK.dll</HintPath>
    </Reference>
  </ItemGroup>
</Project>
```

---

## NUI (HTML Overlay)

Resources can render HTML/CSS/JS overlays in-game using Chromium (CEF).

### client.lua

```lua
-- Create a browser window
local browser = atlas.createBrowser("ui/index.html", 1920, 1080)

-- Show/hide
atlas.setBrowserVisible(browser, true)

-- Send data from Lua to HTML
atlas.on("updateHUD", function(data)
    atlas.sendNUIMessage({ action = "setHealth", value = data.health })
end)

-- Receive from HTML
atlas.on("nuiCallback", function(data)
    if data.action == "openMenu" then
        -- player opened a menu from the HTML
    end
end)
```

### ui/index.html

```html
<!DOCTYPE html>
<html>
<head>
    <style>
        body { background: transparent; color: white; font-family: sans-serif; }
        #hud { position: fixed; top: 20px; left: 20px; }
    </style>
</head>
<body>
    <div id="hud">
        <span>Health: <strong id="health">100</strong></span>
    </div>

    <script>
        // Receive from Lua
        window.addEventListener('message', (event) => {
            const data = event.data;
            if (data.action === 'setHealth') {
                document.getElementById('health').textContent = data.value;
            }
        });

        // Send to Lua
        function sendToLua(data) {
            fetch('https://atlas/nuiCallback', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data)
            });
        }
    </script>
</body>
</html>
```

---

## Installing Resources

Place your resource folder in the server's `resources/` directory:

```
server/
└── resources/
    ├── chat/
    │   ├── resource.toml
    │   └── server.lua
    └── my-gamemode/
        ├── resource.toml
        ├── server.lua
        └── client.lua
```

Then add to `server.toml`:

```toml
[resources]
autostart = ["chat", "my-gamemode"]
```

Or start manually from the server console:
```
> start my-gamemode
> stop my-gamemode
> restart my-gamemode
```

---

## Best Practices

- **Keep server and client logic separate** — never send sensitive data to clients
- **Validate all client events server-side** — clients can send fake events
- **Use `atlas.setPlayerData` for session state** — don't use globals
- **Handle `resourceStop`** — clean up timers, vehicles, and peds you spawned
- **Limit event frequency** — don't trigger events every tick from client
