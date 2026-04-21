// dashboard/src/pages/Resources.tsx
import React, { useState } from 'react';

type ResourceStatus = 'running' | 'stopped' | 'error';

interface Resource {
  name: string;
  version: string;
  author: string;
  language: 'lua' | 'js' | 'csharp';
  status: ResourceStatus;
  uptime: string;
}

const MOCK_RESOURCES: Resource[] = [
  { name: 'chat',       version: '1.0.0', author: 'AtlasMP', language: 'lua',    status: 'running', uptime: '2h 14m' },
  { name: 'freeroam',   version: '1.0.0', author: 'AtlasMP', language: 'js',     status: 'running', uptime: '2h 14m' },
  { name: 'deathmatch', version: '0.9.0', author: 'Suhail',  language: 'csharp', status: 'stopped', uptime: '-'      },
  { name: 'anticheat',  version: '1.2.0', author: 'AtlasMP', language: 'lua',    status: 'error',   uptime: '0m'     },
];

const LANG_COLORS: Record<string, string> = {
  lua:    '#6366f1',
  js:     '#fbbf24',
  csharp: '#34d399',
};

const STATUS_COLORS: Record<ResourceStatus, string> = {
  running: '#34d399',
  stopped: '#4a5568',
  error:   '#f87171',
};

export default function Resources() {
  const [resources, setResources] = useState<Resource[]>(MOCK_RESOURCES);

  const toggle = (name: string) => {
    setResources(rs => rs.map(r =>
      r.name === name
        ? { ...r, status: r.status === 'running' ? 'stopped' : 'running' }
        : r
    ));
  };

  const restart = (name: string) => {
    setResources(rs => rs.map(r =>
      r.name === name ? { ...r, status: 'running', uptime: '0m' } : r
    ));
  };

  return (
    <div>
      <h1 style={styles.title}>Resources</h1>
      <div style={styles.grid}>
        {resources.map(res => (
          <div key={res.name} style={styles.card}>
            <div style={styles.cardTop}>
              <div>
                <span style={styles.resName}>{res.name}</span>
                <span style={{ ...styles.langBadge, background: LANG_COLORS[res.language] + '22', color: LANG_COLORS[res.language] }}>
                  {res.language}
                </span>
              </div>
              <div style={{ ...styles.dot, background: STATUS_COLORS[res.status] }} title={res.status} />
            </div>
            <div style={styles.meta}>
              <span style={styles.metaItem}>v{res.version}</span>
              <span style={styles.metaDot}>·</span>
              <span style={styles.metaItem}>{res.author}</span>
              <span style={styles.metaDot}>·</span>
              <span style={styles.metaItem}>{res.uptime}</span>
            </div>
            <div style={styles.status}>
              Status: <span style={{ color: STATUS_COLORS[res.status], fontWeight: 600 }}>{res.status}</span>
            </div>
            <div style={styles.actions}>
              <button style={res.status === 'running' ? styles.btnStop : styles.btnStart}
                onClick={() => toggle(res.name)}>
                {res.status === 'running' ? 'Stop' : 'Start'}
              </button>
              <button style={styles.btnRestart} onClick={() => restart(res.name)}>
                Restart
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

const styles: Record<string, React.CSSProperties> = {
  title:      { fontSize: '22px', fontWeight: 700, color: '#e2e8f0', marginBottom: '24px' },
  grid:       { display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(280px, 1fr))', gap: '16px' },
  card:       { background: '#131720', border: '1px solid #1e2535', borderRadius: '8px', padding: '18px' },
  cardTop:    { display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: '10px' },
  resName:    { fontWeight: 700, color: '#e2e8f0', marginRight: '8px' },
  langBadge:  { fontSize: '10px', padding: '2px 7px', borderRadius: '10px', fontWeight: 600, letterSpacing: '0.5px' },
  dot:        { width: '8px', height: '8px', borderRadius: '50%' },
  meta:       { display: 'flex', gap: '4px', marginBottom: '10px' },
  metaItem:   { fontSize: '11px', color: '#4a5568' },
  metaDot:    { fontSize: '11px', color: '#2d3748' },
  status:     { fontSize: '12px', color: '#718096', marginBottom: '14px' },
  actions:    { display: 'flex', gap: '8px' },
  btnStart:   { flex: 1, background: '#052e16', color: '#34d399', border: 'none', borderRadius: '5px', padding: '6px', fontSize: '12px', cursor: 'pointer', fontWeight: 600 },
  btnStop:    { flex: 1, background: '#2d1515', color: '#f87171', border: 'none', borderRadius: '5px', padding: '6px', fontSize: '12px', cursor: 'pointer', fontWeight: 600 },
  btnRestart: { background: '#1e2535', color: '#a78bfa', border: 'none', borderRadius: '5px', padding: '6px 12px', fontSize: '12px', cursor: 'pointer' },
};
