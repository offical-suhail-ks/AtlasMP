// sdk/examples/csharp/Deathmatch/Server.cs
// AtlasMP Example Resource: Deathmatch Gamemode (C#)
// Demonstrates the .NET scripting API

using AtlasMP.SDK;
using AtlasMP.SDK.Events;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace AtlasMP.Resources.Deathmatch
{
    public class DeathmatchServer : AtlasResource
    {
        private readonly Dictionary<int, int> _kills  = new();
        private readonly Dictionary<int, int> _deaths = new();

        private static readonly (float X, float Y, float Z, string Name)[] SpawnPoints =
        {
            (-1028.6f, -2732.5f, 20.2f, "LSIA Terminal"),
            ( 216.0f,  -810.0f,  30.7f, "Stadium"),
            (-428.6f, -1550.0f,  20.3f, "Del Perro Pier"),
        };

        public override void OnStart()
        {
            Atlas.Log("[DM] Deathmatch resource started.");

            Atlas.On<int>("playerJoin",  OnPlayerJoin);
            Atlas.On<int>("playerLeave", OnPlayerLeave);
            Atlas.On<int, int, uint>("playerDeath", OnPlayerDeath);
            Atlas.On<int, string>("playerCommand", OnPlayerCommand);
        }

        public override void OnStop()
        {
            Atlas.Log("[DM] Deathmatch resource stopped.");
        }

        // ── Event handlers ────────────────────────────────────────────────

        private async void OnPlayerJoin(int playerId)
        {
            string name = Atlas.GetPlayerName(playerId);
            _kills[playerId]  = 0;
            _deaths[playerId] = 0;

            Atlas.Broadcast($"{name} joined the deathmatch!");
            Atlas.TriggerClientEvent(playerId, "showNotification",
                $"Welcome {name}! Type /score for scoreboard.");

            // Small delay then spawn
            await Task.Delay(1500);
            SpawnPlayer(playerId);
        }

        private void OnPlayerLeave(int playerId)
        {
            string name = Atlas.GetPlayerName(playerId);
            Atlas.Broadcast($"{name} left the match.");
            _kills.Remove(playerId);
            _deaths.Remove(playerId);
        }

        private async void OnPlayerDeath(int victimId, int killerId, uint weaponHash)
        {
            string victim = Atlas.GetPlayerName(victimId);
            _deaths[victimId] = _deaths.GetValueOrDefault(victimId) + 1;

            if (killerId > 0 && killerId != victimId)
            {
                string killer = Atlas.GetPlayerName(killerId);
                _kills[killerId] = _kills.GetValueOrDefault(killerId) + 1;

                Atlas.Broadcast($"{killer} killed {victim}  " +
                    $"[{_kills[killerId]}K/{_deaths.GetValueOrDefault(killerId)}D]");
            }
            else
            {
                Atlas.Broadcast($"{victim} died.");
            }

            // Respawn after 4 seconds
            await Task.Delay(4000);
            SpawnPlayer(victimId);
        }

        private void OnPlayerCommand(int playerId, string command)
        {
            if (command == "/score")
            {
                var players = Atlas.GetPlayers();
                string msg = "=== Scoreboard ===\n";

                foreach (int pid in players)
                {
                    string name = Atlas.GetPlayerName(pid);
                    int k = _kills.GetValueOrDefault(pid);
                    int d = _deaths.GetValueOrDefault(pid);
                    msg += $"{name}: {k}K / {d}D\n";
                }

                Atlas.TriggerClientEvent(playerId, "showChat", msg);
            }
        }

        // ── Helpers ───────────────────────────────────────────────────────

        private static readonly Random _rng = new Random();

        private void SpawnPlayer(int playerId)
        {
            var spawn = SpawnPoints[_rng.Next(SpawnPoints.Length)];
            Atlas.SpawnPlayer(playerId, spawn.X, spawn.Y, spawn.Z);
            Atlas.TriggerClientEvent(playerId, "showNotification",
                $"Spawning at {spawn.Name}...");
        }
    }
}
