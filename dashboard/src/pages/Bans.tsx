// dashboard/src/pages/Bans.tsx
import React, { useState } from 'react';

interface Ban {
  id: number;
  playerName: string;
  reason: string;
  bannedBy: string;
  date: string;
  expires: string;
  ip: string;
}

const MOCK_BANS: Ban[] = [
  { id: 1, playerName: 'SpeedHack99', reason: 'Speed hacking (AC)', bannedBy: 'AntiCheat', date: '2025-04-10', expires: 'Permanent', ip: '192.168.1.x' },
  { id: 2, playerName: 'Toxic_Player', reason: 'Harassment', bannedBy: 'Admin', date: '2025-04-12', expires: '2025-04-19', ip: '10.0.0.x' },
];

export default function Bans() {
  const [bans, setBans] = useState<Ban[]>(MOCK_BANS);
  const [form, setForm] = useState({ name: '', reason: '', duration: '' });

  const unban = (id: number) => setBans(bs => bs.filter(b => b.id !== id));

  const addBan = () => {
    if (!form.name || !form.reason) return;
    setBans(bs => [...bs, {
      id: Date.now(),
      playerName: form.name,
      reason: form.reason,
      bannedBy: 'Admin',
      date: new Date().toISOString().split('T')[0],
      expires: form.duration || 'Permanent',
      ip: 'N/A',
    }]);
    setForm({ name: '', reason: '', duration: '' });
  };

  return (
    <div>
      <h1 style={styles.title}>Ban List</h1>

      {/* Add ban form */}
      <div style={styles.addBox}>
        <div style={styles.addTitle}>Add Ban</div>
        <div style={styles.formRow}>
          <input style={styles.input} placeholder="Player name"
            value={form.name} onChange={e => setForm(f => ({ ...f, name: e.target.value }))} />
          <input style={styles.input} placeholder="Reason"
            value={form.reason} onChange={e => setForm(f => ({ ...f, reason: e.target.value }))} />
          <input style={styles.input} placeholder="Duration (e.g. 7d, permanent)"
            value={form.duration} onChange={e => setForm(f => ({ ...f, duration: e.target.value }))} />
          <button style={styles.addBtn} onClick={addBan}>Add Ban</button>
        </div>
      </div>

      <table style={styles.table}>
        <thead>
          <tr>
            {['Player', 'Reason', 'Banned By', 'Date', 'Expires', 'IP', ''].map(h => (
              <th key={h} style={styles.th}>{h}</th>
            ))}
          </tr>
        </thead>
        <tbody>
          {bans.map(b => (
            <tr key={b.id} style={styles.tr}>
              <td style={styles.td}><strong style={{ color: '#e2e8f0' }}>{b.playerName}</strong></td>
              <td style={styles.td}>{b.reason}</td>
              <td style={styles.td}><span style={styles.byBadge}>{b.bannedBy}</span></td>
              <td style={styles.td}>{b.date}</td>
              <td style={{ ...styles.td, color: b.expires === 'Permanent' ? '#f87171' : '#fbbf24' }}>
                {b.expires}
              </td>
              <td style={{ ...styles.td, fontFamily: 'monospace', fontSize: '11px', color: '#4a5568' }}>{b.ip}</td>
              <td style={styles.td}>
                <button style={styles.unbanBtn} onClick={() => unban(b.id)}>Unban</button>
              </td>
            </tr>
          ))}
          {bans.length === 0 && (
            <tr><td colSpan={7} style={styles.empty}>No bans on record.</td></tr>
          )}
        </tbody>
      </table>
    </div>
  );
}

const styles: Record<string, React.CSSProperties> = {
  title:    { fontSize: '22px', fontWeight: 700, color: '#e2e8f0', marginBottom: '24px' },
  addBox:   { background: '#131720', border: '1px solid #1e2535', borderRadius: '8px', padding: '18px', marginBottom: '24px' },
  addTitle: { fontSize: '13px', color: '#718096', marginBottom: '12px', textTransform: 'uppercase', letterSpacing: '1px' },
  formRow:  { display: 'flex', gap: '10px', flexWrap: 'wrap' },
  input:    { background: '#0a0c11', border: '1px solid #1e2535', borderRadius: '5px', padding: '8px 12px', color: '#e2e8f0', fontSize: '13px', outline: 'none', flex: '1 1 180px' },
  addBtn:   { background: '#7f1d1d', color: '#f87171', border: 'none', borderRadius: '5px', padding: '8px 18px', fontSize: '13px', cursor: 'pointer', fontWeight: 600 },
  table:    { width: '100%', borderCollapse: 'collapse' },
  th:       { textAlign: 'left', padding: '10px 12px', fontSize: '11px', color: '#4a5568', letterSpacing: '1px', textTransform: 'uppercase', borderBottom: '1px solid #1e2535' },
  tr:       { borderBottom: '1px solid #131720' },
  td:       { padding: '10px 12px', fontSize: '13px', color: '#718096' },
  byBadge:  { background: '#1e2535', borderRadius: '4px', padding: '2px 7px', fontSize: '11px', color: '#4a5568' },
  unbanBtn: { background: '#052e16', color: '#34d399', border: 'none', borderRadius: '4px', padding: '4px 12px', fontSize: '11px', cursor: 'pointer', fontWeight: 600 },
  empty:    { textAlign: 'center', padding: '32px', color: '#4a5568' },
};
