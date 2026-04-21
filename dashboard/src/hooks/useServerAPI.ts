// dashboard/src/hooks/useServerAPI.ts
// Hook for communicating with the AtlasMP server REST API
// Server API runs on port 7789

import { useState, useCallback } from 'react';
import axios from 'axios';

const API_BASE = import.meta.env.VITE_API_URL || 'http://localhost:7789/api/v1';

// Create axios instance with auth
const api = axios.create({
  baseURL: API_BASE,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Inject API password from localStorage
api.interceptors.request.use(cfg => {
  const pw = localStorage.getItem('atlas_dashboard_pw') || '';
  if (pw) cfg.headers['X-Dashboard-Password'] = pw;
  return cfg;
});

// ─── Types ────────────────────────────────────────────────────────────────────

export interface ServerInfo {
  name: string;
  version: string;
  playerCount: number;
  maxPlayers: number;
  uptime: number;
  tickRate: number;
  resources: ResourceInfo[];
}

export interface ResourceInfo {
  name: string;
  version: string;
  author: string;
  language: string;
  status: 'running' | 'stopped' | 'error';
}

export interface PlayerInfo {
  id: number;
  name: string;
  ping: number;
  health: number;
  position: { x: number; y: number; z: number };
  joinTime: number;
}

export interface BanEntry {
  id: number;
  playerName: string;
  reason: string;
  bannedBy: string;
  date: string;
  expires: string;
  ip: string;
}

// ─── API methods ──────────────────────────────────────────────────────────────

export const ServerAPI = {
  // Server info
  getInfo: () => api.get<ServerInfo>('/info'),

  // Players
  getPlayers: () => api.get<PlayerInfo[]>('/players'),
  kickPlayer:  (id: number, reason: string) =>
    api.post(`/players/${id}/kick`, { reason }),
  banPlayer:   (id: number, reason: string, duration?: string) =>
    api.post(`/players/${id}/ban`, { reason, duration }),

  // Resources
  getResources:    () => api.get<ResourceInfo[]>('/resources'),
  startResource:   (name: string) => api.post(`/resources/${name}/start`),
  stopResource:    (name: string) => api.post(`/resources/${name}/stop`),
  restartResource: (name: string) => api.post(`/resources/${name}/restart`),

  // Bans
  getBans:   () => api.get<BanEntry[]>('/bans'),
  addBan:    (data: Partial<BanEntry>) => api.post('/bans', data),
  removeBan: (id: number) => api.delete(`/bans/${id}`),

  // Console
  sendCommand: (cmd: string) => api.post('/console', { command: cmd }),
};

// ─── useServerAPI hook ────────────────────────────────────────────────────────

export function useAPI<T>(fn: () => Promise<{ data: T }>) {
  const [data,    setData]    = useState<T | null>(null);
  const [loading, setLoading] = useState(false);
  const [error,   setError]   = useState<string | null>(null);

  const fetch = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await fn();
      setData(res.data);
    } catch (e: any) {
      setError(e?.response?.data?.message || e?.message || 'Unknown error');
    } finally {
      setLoading(false);
    }
  }, [fn]);

  return { data, loading, error, fetch };
}
