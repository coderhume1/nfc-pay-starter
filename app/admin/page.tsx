
import { prisma } from '@/lib/prisma';
import { cookies } from 'next/headers';

export default async function AdminPage() {
  const authed = cookies().get('admin_auth')?.value==='1';
  if(!authed){
    return (
      <section>
        <h1>Admin Login</h1>
        <form method="POST" action="/api/admin/login" style={{ display: 'grid', gap: 12, maxWidth: 360 }}>
          <label>Admin Key
            <input name="key" type="password" placeholder="Enter admin key" required style={{ width: '100%', padding: 8 }} />
          </label>
          <button type="submit" style={{ padding: '8px 12px' }}>Login</button>
        </form>
      </section>
    );
  }

  const sessions = await prisma.session.findMany({ orderBy: { createdAt: 'desc' } });

  return (
    <section>
      <div style={{ display: 'flex', gap: 12, alignItems: 'center' }}>
        <h1 style={{ marginRight: 'auto' }}>Admin Dashboard</h1>
        <form method="POST" action="/api/admin/logout">
          <button type="submit" style={{ padding: '6px 10px' }}>Logout</button>
        </form>
      </div>

      <h2>Create Test Session</h2>
      <form method="POST" action="/api/admin/create" style={{ display: 'flex', gap: 8, flexWrap: 'wrap', marginBottom: 24 }}>
        <input name="amount" type="number" min="0" placeholder="Amount (integer)" required style={{ padding: 8 }} />
        <input name="currency" type="text" defaultValue="USD" placeholder="Currency" required style={{ padding: 8 }} />
        <input name="terminalId" type="text" defaultValue="ADMIN_TEST" placeholder="Terminal ID" required style={{ padding: 8 }} />
        <button type="submit" style={{ padding: '8px 12px' }}>Create</button>
      </form>

      <h2>Sessions</h2>
      <div style={{ overflowX: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse' }}>
          <thead>
            <tr>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', padding: 6 }}>ID</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', padding: 6 }}>Terminal</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', padding: 6 }}>Amount</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', padding: 6 }}>Currency</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', padding: 6 }}>Status</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', padding: 6 }}>Created</th>
              <th style={{ textAlign: 'left', borderBottom: '1px solid #333', padding: 6 }}>Actions</th>
            </tr>
          </thead>
          <tbody>
            {sessions.map(s => (
              <tr key={s.id}>
                <td style={{ padding: 6, borderBottom: '1px solid #222' }}><code>{s.id}</code></td>
                <td style={{ padding: 6, borderBottom: '1px solid #222' }}>{s.terminalId}</td>
                <td style={{ padding: 6, borderBottom: '1px solid #222' }}>{s.amount}</td>
                <td style={{ padding: 6, borderBottom: '1px solid #222' }}>{s.currency}</td>
                <td style={{ padding: 6, borderBottom: '1px solid #222' }}>{s.status}</td>
                <td style={{ padding: 6, borderBottom: '1px solid #222' }}>{new Date(s.createdAt as unknown as string).toLocaleString()}</td>
                <td style={{ padding: 6, borderBottom: '1px solid #222' }}>
                  {s.status==='pending' ? (
                    <form method="POST" action="/api/admin/mark-paid" style={{ display: 'inline' }}>
                      <input type="hidden" name="sessionId" value={s.id} />
                      <button type="submit" style={{ padding: '6px 10px' }}>Mark Paid</button>
                    </form>
                  ) : <span>—</span>}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </section>
  );
}
