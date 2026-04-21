// dashboard/src/pages/Players.tsx
import React, { useState } from 'react';

interface Player {
  id: number;
  name: string;
  ping: number;
  position: string;
  health: number;
  joinTime: string;
}

const MOCK_PLAYERS: Player[] = [
  { id: 1, name: 'Suhail',    ping: 22,  position: '-269.0, -955.0, 31.2', health: 200, joinTime: '5m ago' },
  { id: 2, name: 'Jaseel',    ping: 45,  position: '215.0, -810.0, 30.7',  health: 150, joinTime: '2m ago' },
  { id: 3, name: 'Player003', ping: 110, position: '-1037.0, -2735.0, 20', health: 75,  joinTime: '12m ago' },
];

export default function Players() {
  const [players] = useState<Player[]>(MOCK_PLAYERS);
  const [search, setSearch] = useState('');
  const [selected, setSelected] = useState<Player | null>(null);

  const filtered = players.filter(p =>
    p.name.toLowerCase().includes(search.toLowerCase())
  );

  return (
    <div>
      <h1 style={styles.title}>Players</h1>
      <div style={styles.toolbar}>
        <input
          style={styles.search}
          placeholder="Search by name..."
          value={search}
          onChange={e => setSearch(e.target.value)}
        />
        <span style={styles.count}>{players.length} / 128 online</span>
      </div>

      <table style={styles.table}>
        <thead>
          <tr>
            {['ID', 'Name', 'Ping', 'Health', 'Position', 'Joined', 'Actions'].map(h => (
              <th key={h} style={styles.th}>{h}</th>
            ))}
          </tr>
        </thead>
        <tbody>
          {filtered.map(p => (
            <tr key={p.id} style={styles.tr}>
              <td style={styles.td}><span style={styles.idBadge}>#{p.id}</span></td>
              <td style={styles.td}><span style={styles.name}>{p.name}</span></td>
              <td style={styles.td}>
                <span style={{ ...styles.ping, color: p.ping < 60 ? '#34d399' : p.ping < 120 ? '#fbbf24' : '#f87171' }}>
                  {p.ping}ms
                </span>
              </td>
              <td style={styles.td}>
                <div style={styles.healthBar}>
                  <div style={{ ...styles.healthFill, width: `${(p.health / 200) * 100}%`,
                    background: p.health > 100 ? '#34d399' : p.health > 50 ? '#fbbf24' : '#f87171' }} />
                </div>
                <span style={styles.healthNum}>{p.health}</span>
              </td>
              <td style={styles.td}><span style={styles.pos}>{p.position}</span></td>
              <td style={styles.td}>{p.joinTime}</td>
              <td style={styles.td}>
                <button style={styles.btnWarn}    onClick={() => setSelected(p)}>Kick</button>
                <button style={styles.btnDanger}  onClick={() => alert(`Ban ${p.name}?`)}>Ban</button>
                <button style={styles.btnNeutral} onClick={() => alert(`Spectate ${p.name}`)}>👁</button>
              </td>
            </tr>
          ))}
          {filtered.length === 0 && (
            <tr><td colSpan={7} style={styles.empty}>No players found.</td></tr>
          )}
        </tbody>
      </table>

      {selected && (
        <div style={styles.modal} onClick={() => setSelected(null)}>
          <div style={styles.modalBox} onClick={e => e.stopPropagation()}>
            <h3 style={styles.modalTitle}>Kick {selected.name}?</h3>
            <input style={styles.search} placeholder="Reason (optional)" />
            <div style={{ marginTop: 16, display: 'flex', gap: 8 }}>
              <button style={styles.btnDanger} onClick={() => { alert(`Kicked ${selected.name}`); setSelected(null); }}>
                Confirm Kick
              </button>
              <button style={styles.btnNeutral} onClick={() => setSelected(null)}>Cancel</button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

const styles: Record<string, React.CSSProperties> = {
  title:     { fontSize: '22px', fontWeight: 700, color: '#e2e8f0', marginBottom: '20px' },
  toolbar:   { display: 'flex', alignItems: 'center', gap: '16px', marginBottom: '20px' },
  search:    { background: '#131720', border: '1px solid #1e2535', borderRadius: '6px', padding: '8px 12px', color: '#e2e8f0', fontSize: '13px', outline: 'none', flex: 1 },
  count:     { fontSize: '12px', color: '#4a5568' },
  table:     { width: '100%', borderCollapse: 'collapse' },
  th:        { textAlign: 'left', padding: '10px 12px', fontSize: '11px', color: '#4a5568', letterSpacing: '1px', textTransform: 'uppercase', borderBottom: '1px solid #1e2535' },
  tr:        { borderBottom: '1px solid #131720' },
  td:        { padding: '10px 12px', fontSize: '13px', color: '#cbd5e0', verticalAlign: 'middle' },
  idBadge:   { background: '#1e2535', borderRadius: '4px', padding: '2px 6px', fontSize: '11px', color: '#4a5568' },
  name:      { color: '#e2e8f0', fontWeight: 600 },
  ping:      { fontWeight: 600 },
  healthBar: { display: 'inline-block', width: '60px', height: '6px', background: '#1e2535', borderRadius: '3px', marginRight: '6px', verticalAlign: 'middle', overflow: 'hidden' },
  healthFill:{ height: '100%', borderRadius: '3px', transition: 'width 0.3s' },
  healthNum: { fontSize: '11px', color: '#4a5568' },
  pos:       { fontSize: '11px', color: '#4a5568', fontFamily: 'monospace' },
  btnWarn:   { background: '#78350f', color: '#fbbf24', border: 'none', borderRadius: '4px', padding: '4px 10px', fontSize: '11px', cursor: 'pointer', marginRight: '4px' },
  btnDanger: { background: '#7f1d1d', color: '#f87171', border: 'none', borderRadius: '4px', padding: '4px 10px', fontSize: '11px', cursor: 'pointer', marginRight: '4px' },
  btnNeutral:{ background: '#1e2535', color: '#718096', border: 'none', borderRadius: '4px', padding: '4px 10px', fontSize: '11px', cursor: 'pointer' },
  empty:     { textAlign: 'center', padding: '32px', color: '#4a5568' },
  modal:     { position: 'fixed', inset: 0, background: 'rgba(0,0,0,0.7)', display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 100 },
  modalBox:  { background: '#131720', border: '1px solid #1e2535', borderRadius: '10px', padding: '28px', minWidth: '320px' },
  modalTitle:{ color: '#e2e8f0', marginBottom: '16px', fontSize: '16px' },
};
