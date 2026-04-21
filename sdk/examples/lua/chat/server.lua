-- sdk/examples/lua/chat/server.lua
-- AtlasMP Example Resource: Basic Chat System
-- Shows: atlas.on(), atlas.broadcast(), atlas.getPlayerName()

print("[Chat] Chat resource starting...")

-- ─── Player join announcement ──────────────────────────────────────────────
atlas.on("playerJoin", function(playerId)
    local name = atlas.getPlayerName(playerId)
    atlas.broadcast("^2" .. name .. " ^7has joined the server.")
    atlas.log("Player joined: " .. name .. " (id=" .. playerId .. ")")
end)

-- ─── Player leave announcement ────────────────────────────────────────────
atlas.on("playerLeave", function(playerId, reason)
    local name = atlas.getPlayerName(playerId)
    atlas.broadcast("^1" .. name .. " ^7has left the server. (" .. reason .. ")")
end)

-- ─── Chat message handler ─────────────────────────────────────────────────
atlas.on("playerChat", function(playerId, message)
    local name = atlas.getPlayerName(playerId)

    -- Block empty messages
    if not message or #message == 0 then return end

    -- Truncate long messages
    if #message > 200 then
        message = string.sub(message, 1, 200) .. "..."
    end

    -- Command handling
    if string.sub(message, 1, 1) == "/" then
        atlas.emit("playerCommand", playerId, message)
        return
    end

    -- Format and broadcast
    local formatted = string.format("^7[^3%s^7] %s", name, message)
    atlas.broadcast(formatted)
    atlas.log("[CHAT] " .. name .. ": " .. message)
end)

-- ─── Command handler ──────────────────────────────────────────────────────
atlas.on("playerCommand", function(playerId, command)
    local parts = {}
    for part in string.gmatch(command, "%S+") do
        table.insert(parts, part)
    end

    local cmd = parts[1]

    if cmd == "/help" then
        -- Send help message to this player only
        atlas.triggerClientEvent(playerId, "showNotification",
            "Commands: /help, /players, /ping")

    elseif cmd == "/players" then
        local players = atlas.getPlayers()
        local count = #players
        atlas.triggerClientEvent(playerId, "showNotification",
            string.format("Players online: %d", count))

    elseif cmd == "/ping" then
        atlas.triggerClientEvent(playerId, "showNotification", "Pong!")

    else
        atlas.triggerClientEvent(playerId, "showNotification",
            "Unknown command: " .. cmd)
    end
end)

-- ─── Resource lifecycle ────────────────────────────────────────────────────
atlas.on("resourceStart", function()
    atlas.log("[Chat] Chat system ready.")
end)

atlas.on("resourceStop", function()
    atlas.log("[Chat] Chat system stopped.")
end)
