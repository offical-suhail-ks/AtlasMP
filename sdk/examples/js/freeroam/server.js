// sdk/examples/js/freeroam/server.js
// AtlasMP Example Resource: Freeroam Server
// Shows: JavaScript async/await API, player management

console.log('[Freeroam] Starting freeroam resource...');

// ─── Spawn positions (Los Santos landmarks) ───────────────────────────────
const SPAWN_POINTS = [
    { x: -269.0, y: -955.0, z: 31.2,  name: "Legion Square" },
    { x:  215.0, y: -810.0, z: 30.7,  name: "Maze Bank Arena" },
    { x: -1037.0,y: -2735.0,z: 20.2,  name: "LSIA" },
    { x:  1689.0, y: 4929.0, z: 42.1,  name: "Sandy Shores" },
    { x: -3163.0, y: 1052.0, z: 20.9,  name: "Paleto Bay" },
];

function getRandomSpawn() {
    return SPAWN_POINTS[Math.floor(Math.random() * SPAWN_POINTS.length)];
}

// ─── Player join handler ──────────────────────────────────────────────────
atlas.on('playerJoin', (playerId) => {
    const name = atlas.getPlayerName(playerId);
    const spawn = getRandomSpawn();

    atlas.log(`[Freeroam] ${name} joined. Spawning at ${spawn.name}`);

    // Welcome notification
    atlas.triggerClientEvent(playerId, 'showNotification',
        `Welcome to AtlasMP Freeroam, ${name}!`);

    // Spawn after short delay
    setTimeout(() => {
        atlas.spawnPlayer(playerId, spawn.x, spawn.y, spawn.z);
    }, 1000);

    // Announce to all
    atlas.broadcast(`${name} has joined. Welcome!`);
});

// ─── Player death handler ─────────────────────────────────────────────────
atlas.on('playerDeath', (playerId, killerId, weaponHash) => {
    const name = atlas.getPlayerName(playerId);

    atlas.log(`[Freeroam] ${name} died`);

    // Respawn after 3 seconds
    setTimeout(() => {
        const spawn = getRandomSpawn();
        atlas.spawnPlayer(playerId, spawn.x, spawn.y, spawn.z);
        atlas.triggerClientEvent(playerId, 'showNotification',
            `Respawning at ${spawn.name}...`);
    }, 3000);
});

// ─── Player leave ─────────────────────────────────────────────────────────
atlas.on('playerLeave', (playerId) => {
    const name = atlas.getPlayerName(playerId);
    atlas.broadcast(`${name} has left the server.`);
});

// ─── Resource ready ───────────────────────────────────────────────────────
atlas.on('resourceStart', () => {
    atlas.log('[Freeroam] Freeroam resource ready.');
    atlas.log(`[Freeroam] ${SPAWN_POINTS.length} spawn points loaded.`);
});
