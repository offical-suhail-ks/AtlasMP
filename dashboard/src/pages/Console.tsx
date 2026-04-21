// dashboard/src/pages/Console.tsx
import React, { useState, useRef, useEffect } from 'react';

interface LogLine {
  id: number;
  time: string;
  level: 'info' | 'warn' | 'error' | 'debug';
  message: string;
}

let _id = 0;
const mkLine = (level: LogLine['level'], message: string): LogLine => ({
  id: ++_id,
  time: new Date().toLocaleTimeString(),
  level,
  message,
});

const INITIAL_LOG: LogLine[] = [
  mkLine('info',  '[Server] AtlasMP Server v0.1.0 starting...'),
  mkLine('info',  '[Server] Loading config: server.toml'),
  mkLine('info',  '[Network] ENet server listening on 0.0.0.0:7788'),
  mkLine('info',  '[Resources] Scanning resources/ directory'),
  mkLine('info',  '[Resources] Found: chat, freeroam'),
  mkLine('info',  '[Lua] Starting resource: chat'),
  mkLine('debug', '[Chat] Chat system ready.'),
  mkLine('info',  '[JS] Starting resource: freeroam'),
  mkLine('debug', '[Freeroam] Freeroam resource ready.'),
  mkLine('info',  '[Server] Ready. Listening for players...'),
];

const LEVEL_COLORS: Record<string, string> = {
  info:  '#60a5fa',
  warn:  '#fbbf24',
  error: '#f87171',
  debug: '#4a5568',
};

export default function Console() {
  const [lines, setLines]     = useState<LogLine[]>(INITIAL_LOG);
  const [input, setInput]     = useState('');
  const [filter, setFilter]   = useState<string>('all');
  const [autoScroll, setAutoScroll] = useState(true);
  const bottomRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (autoScroll) bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [lines, autoScroll]);

  const sendCommand = () => {
    if (!input.trim()) return;
    setLines(l => [...l,
      mkLine('info', `> ${input}`),
      mkLine('info', `[Console] Command '${input}' acknowledged.`),
    ]);
    setInput('');
  };

  const visible = filter === 'all'
    ? lines
    : lines.filter(l => l.level === filter);

  return (
    <div style={styles.page}>
      <div style={styles.header}>
        <h1 style={styles.title}>Server Console</h1>
        <div style={styles.filters}>
          {['all','info','warn','error','debug'].map(f => (
            <button key={f} onClick={() => setFilter(f)}
              style={{ ...styles.filterBtn, ...(filter === f ? styles.filterActive : {}) }}>
              {f}
            </button>
          ))}
        </div>
        <label style={styles.autoScrollLabel}>
          <input type="checkbox" checked={autoScroll}
            onChange={e => setAutoScroll(e.target.checked)} style={{ marginRight: 6 }} />
          Auto-scroll
        </label>
        <button style={styles.clearBtn} onClick={() => setLines([])}>Clear</button>
      </div>

      <div style={styles.console}>
        {visible.map(line => (
          <div key={line.id} style={styles.logLine}>
            <span style={styles.time}>{line.time}</span>
            <span style={{ ...styles.level, color: LEVEL_COLORS[line.level] }}>
              [{line.level.toUpperCase()}]
            </span>
            <span style={styles.msg}>{line.message}</span>
          </div>
        ))}
        <div ref={bottomRef} />
      </div>

      <div style={styles.inputRow}>
        <span style={styles.prompt}>&gt;</span>
        <input
          style={styles.input}
          value={input}
          onChange={e => setInput(e.target.value)}
          onKeyDown={e => { if (e.key === 'Enter') sendCommand(); }}
          placeholder="Type a server command... (start <resource>, stop <resource>, players)"
          autoFocus
        />
        <button style={styles.send} onClick={sendCommand}>Send</button>
      </div>
    </div>
  );
}

const styles: Record<string, React.CSSProperties> = {
  page:          { display: 'flex', flexDirection: 'column', height: 'calc(100vh - 56px)' },
  header:        { display: 'flex', alignItems: 'center', gap: '12px', marginBottom: '16px', flexWrap: 'wrap' },
  title:         { fontSize: '22px', fontWeight: 700, color: '#e2e8f0', marginRight: 'auto' },
  filters:       { display: 'flex', gap: '4px' },
  filterBtn:     { background: '#1e2535', color: '#718096', border: 'none', borderRadius: '4px', padding: '5px 10px', fontSize: '11px', cursor: 'pointer' },
  filterActive:  { background: '#60a5fa22', color: '#60a5fa' },
  autoScrollLabel: { fontSize: '12px', color: '#4a5568', cursor: 'pointer' },
  clearBtn:      { background: '#1e2535', color: '#4a5568', border: 'none', borderRadius: '4px', padding: '5px 12px', fontSize: '11px', cursor: 'pointer' },
  console:       { flex: 1, background: '#0a0c11', border: '1px solid #1e2535', borderRadius: '8px', padding: '12px 16px', overflowY: 'auto', fontFamily: "'JetBrains Mono', monospace", fontSize: '12px' },
  logLine:       { display: 'flex', gap: '8px', lineHeight: '1.7', wordBreak: 'break-all' },
  time:          { color: '#2d3748', flexShrink: 0 },
  level:         { flexShrink: 0, fontWeight: 600 },
  msg:           { color: '#a0aec0' },
  inputRow:      { display: 'flex', alignItems: 'center', gap: '8px', marginTop: '12px' },
  prompt:        { color: '#60a5fa', fontWeight: 700, fontSize: '16px' },
  input:         { flex: 1, background: '#0a0c11', border: '1px solid #1e2535', borderRadius: '6px', padding: '10px 14px', color: '#e2e8f0', fontFamily: 'monospace', fontSize: '13px', outline: 'none' },
  send:          { background: '#1e3a5f', color: '#60a5fa', border: 'none', borderRadius: '6px', padding: '10px 18px', fontSize: '13px', cursor: 'pointer', fontWeight: 600 },
};
