
import React from 'react';
import type { Metadata } from 'next';

export const metadata: Metadata = {
  title: 'NFC Pay Starter',
  description: 'Netlify + Next.js + Neon Postgres NFC payment demo',
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en">
      <body style={{ fontFamily: 'system-ui, sans-serif', margin: 0, padding: 16, background: '#0b0f19', color: '#e6e9ef' }}>
        <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 16 }}>
          <h2 style={{ margin: 0 }}>NFC Pay Starter</h2>
          <nav style={{ display: 'flex', gap: 12 }}>
            <a href="/" style={{ color: '#a8c8ff' }}>Home</a>
            <a href="/admin" style={{ color: '#a8c8ff' }}>Admin</a>
          </nav>
        </header>
        <main style={{ maxWidth: 920, margin: '0 auto' }}>{children}</main>
        <footer style={{ marginTop: 48, opacity: 0.6 }}>© Demo</footer>
      </body>
    </html>
  );
}
