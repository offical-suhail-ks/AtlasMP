// dashboard/src/pages/Overview.tsx
import React, { useEffect, useState } from 'react';
import { LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer, CartesianGrid } from 'recharts';

interface ServerStats {
  name: string;
  playerCount: number;
  maxPlayers: number;
  uptime: number;       // seconds
  tickRate: number;
  version: string;
  resources: number;
}

interface TickData { tick: number; players: number; cpu: number; }

const MOCK_STATS: ServerStats = {
  name: 'My AtlasMP Server',
  playerCount: 0,
  maxPlayers: 128,
  uptime: 0,
  tickRate: 60,
  version: '0.1.0',
  resources: 2,
};

function formatUptime(s: number) {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(sec).padStart(2,'0')}`;
}

export default function Overview() {
  const [stats, setStats]     = useState<ServerStats>(MOCK_STATS);
  const [history, setHistory] = useState<TickData[]>([]);
  const [tick, setTick]       = useState(0);

  // Simulate live data (replace with real WebSocket/polling to API)
  useEffect(() => {
    const id = setInterval(() => {
      setTick(t => t + 1);
      setStats(s => ({ ...s, uptime: s.uptime + 1 }));
      setHistory(h => {
        const next = [...h, {
          tick: h.length,
          players: Math.floor(Math.random() * 5),
          cpu: 20 + Math.random() * 15,
        }].slice(-60); // last 60 seconds
        return next;
      });
    }, 1000);
    return () => clearInterval(id);
  }, []);

  const statCards = [
    { label: 'Players',   value: `${stats.playerCount} / ${stats.maxPlayers}`, color: '#60a5fa' },
    { label: 'Uptime',    value: formatUptime(stats.uptime),                    color: '#34d399' },
    { label: 'Tick Rate', value: `${stats.tickRate} Hz`,                        color: '#a78bfa' },
    { label: 'Resources', value: `${stats.resources} loaded`,                   color: '#fb923c' },
  ];

  return (
    <div>
      <h1 style={styles.pageTitle}>Server Overview</h1>
      <p style={styles.serverName}>{stats.name}</p>

      {/* Stat cards */}
      <div style={styles.cards}>
        {statCards.map(card => (
          <div key={card.label} style={styles.card}>
            <div style={{ ...styles.cardLabel, color: card.color }}>{card.label}</div>
            <div style={styles.cardValue}>{card.value}</div>
          </div>
        ))}
      </div>

      {/* Player count chart */}
      <div style={styles.chartBox}>
        <div style={styles.chartTitle}>Players (last 60s)</div>
        <ResponsiveContainer width="100%" height={180}>
          <LineChart data={history}>
            <CartesianGrid strokeDasharray="3 3" stroke="#1e2535" />
            <XAxis dataKey="tick" hide />
            <YAxis domain={[0, stats.maxPlayers]} stroke="#4a5568" tick={{ fontSize: 11, fill: '#4a5568' }} />
            <Tooltip
              contentStyle={{ background: '#131720', border: '1px solid #1e2535', borderRadius: 6 }}
              labelStyle={{ color: '#4a5568' }}
              itemStyle={{ color: '#60a5fa' }}
            />
            <Line type="monotone" dataKey="players" stroke="#60a5fa" strokeWidth={2} dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>

      {/* CPU chart */}
      <div style={styles.chartBox}>
        <div style={styles.chartTitle}>CPU Usage %</div>
        <ResponsiveContainer width="100%" height={140}>
          <LineChart data={history}>
            <CartesianGrid strokeDasharray="3 3" stroke="#1e2535" />
            <XAxis dataKey="tick" hide />
            <YAxis domain={[0, 100]} stroke="#4a5568" tick={{ fontSize: 11, fill: '#4a5568' }} />
            <Tooltip
              contentStyle={{ background: '#131720', border: '1px solid #1e2535', borderRadius: 6 }}
              itemStyle={{ color: '#34d399' }}
            />
            <Line type="monotone" dataKey="cpu" stroke="#34d399" strokeWidth={2} dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

const styles: Record<string, React.CSSProperties> = {
  pageTitle:  { fontSize: '22px', fontWeight: 700, color: '#e2e8f0', marginBottom: '4px' },
  serverName: { fontSize: '13px', color: '#60a5fa', marginBottom: '28px', letterSpacing: '0.5px' },
  cards: {
    display: 'grid',
    gridTemplateColumns: 'repeat(4, 1fr)',
    gap: '16px',
    marginBottom: '28px',
  },
  card: {
    background: '#131720',
    border: '1px solid #1e2535',
    borderRadius: '8px',
    padding: '18px 20px',
  },
  cardLabel: { fontSize: '11px', letterSpacing: '1px', textTransform: 'uppercase', marginBottom: '8px' },
  cardValue: { fontSize: '24px', fontWeight: 700, color: '#e2e8f0' },
  chartBox: {
    background: '#131720',
    border: '1px solid #1e2535',
    borderRadius: '8px',
    padding: '20px',
    marginBottom: '20px',
  },
  chartTitle: { fontSize: '13px', color: '#718096', marginBottom: '16px', letterSpacing: '0.5px' },
};
