// dashboard/src/App.tsx
import React from 'react';
import { BrowserRouter, Routes, Route, NavLink } from 'react-router-dom';
import Overview from './pages/Overview';
import Players from './pages/Players';
import Resources from './pages/Resources';
import Console from './pages/Console';
import Bans from './pages/Bans';

const NAV = [
  { to: '/',          label: '📊 Overview'  },
  { to: '/players',   label: '👤 Players'   },
  { to: '/resources', label: '📦 Resources' },
  { to: '/console',   label: '💻 Console'   },
  { to: '/bans',      label: '🚫 Bans'      },
];

export default function App() {
  return (
    <BrowserRouter>
      <div style={styles.layout}>
        {/* Sidebar */}
        <nav style={styles.sidebar}>
          <div style={styles.logo}>
            <span style={styles.logoIcon}>🌍</span>
            <span style={styles.logoText}>AtlasMP</span>
          </div>
          <div style={styles.logoSub}>Server Dashboard</div>
          <ul style={styles.navList}>
            {NAV.map(({ to, label }) => (
              <li key={to}>
                <NavLink
                  to={to}
                  end={to === '/'}
                  style={({ isActive }) => ({
                    ...styles.navItem,
                    ...(isActive ? styles.navItemActive : {}),
                  })}
                >
                  {label}
                </NavLink>
              </li>
            ))}
          </ul>
          <div style={styles.sidebarFooter}>
            <span style={styles.version}>v0.1.0-pre-alpha</span>
          </div>
        </nav>

        {/* Main content */}
        <main style={styles.main}>
          <Routes>
            <Route path="/"          element={<Overview />} />
            <Route path="/players"   element={<Players />} />
            <Route path="/resources" element={<Resources />} />
            <Route path="/console"   element={<Console />} />
            <Route path="/bans"      element={<Bans />} />
          </Routes>
        </main>
      </div>
    </BrowserRouter>
  );
}

const styles: Record<string, React.CSSProperties> = {
  layout: {
    display: 'flex',
    minHeight: '100vh',
    backgroundColor: '#0d0f14',
    color: '#e2e8f0',
    fontFamily: "'JetBrains Mono', 'Fira Code', monospace",
  },
  sidebar: {
    width: '220px',
    flexShrink: 0,
    background: 'linear-gradient(180deg, #131720 0%, #0d0f14 100%)',
    borderRight: '1px solid #1e2535',
    display: 'flex',
    flexDirection: 'column',
    padding: '24px 0',
  },
  logo: {
    display: 'flex',
    alignItems: 'center',
    gap: '10px',
    padding: '0 20px 4px',
  },
  logoIcon: { fontSize: '24px' },
  logoText: {
    fontSize: '20px',
    fontWeight: 700,
    color: '#60a5fa',
    letterSpacing: '2px',
  },
  logoSub: {
    fontSize: '10px',
    color: '#4a5568',
    padding: '0 20px 24px',
    letterSpacing: '1px',
    textTransform: 'uppercase',
  },
  navList: {
    listStyle: 'none',
    padding: 0,
    margin: 0,
    flex: 1,
  },
  navItem: {
    display: 'block',
    padding: '10px 20px',
    color: '#718096',
    textDecoration: 'none',
    fontSize: '13px',
    letterSpacing: '0.5px',
    transition: 'all 0.15s',
    borderLeft: '3px solid transparent',
  },
  navItemActive: {
    color: '#60a5fa',
    background: 'rgba(96,165,250,0.08)',
    borderLeft: '3px solid #60a5fa',
  },
  sidebarFooter: {
    padding: '16px 20px 0',
    borderTop: '1px solid #1e2535',
  },
  version: {
    fontSize: '11px',
    color: '#2d3748',
  },
  main: {
    flex: 1,
    padding: '28px 32px',
    overflowY: 'auto',
  },
};
