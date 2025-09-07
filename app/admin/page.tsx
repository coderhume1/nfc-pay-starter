
import { prisma } from '@/lib/prisma'; import { cookies } from 'next/headers';
export default async function Admin(){
  const authed = cookies().get('admin_auth')?.value==='1';
  if(!authed){
    return (<section><h1>Admin Login</h1>
      <form method="POST" action="/api/admin/login" style={{display:'grid',gap:12,maxWidth:360}}>
        <label>Admin Key<input name="key" type="password" required style={{width:'100%',padding:8}}/></label>
        <button type="submit">Login</button>
      </form></section>);
  }
  const sessions = await prisma.session.findMany({orderBy:{createdAt:'desc'}, take: 50});
  return (<section>
    <div style={{display:'flex',gap:12,alignItems:'center'}}><h1 style={{marginRight:'auto'}}>Admin</h1>
      <a href="/admin/devices" style={{color:'#a8c8ff'}}>Devices</a>
      <form method="POST" action="/api/admin/logout"><button type="submit">Logout</button></form></div>
    <h2>Create Test Session</h2>
    <form method="POST" action="/api/admin/create" style={{display:'flex',gap:8,flexWrap:'wrap',marginBottom:24}}>
      <input name="amount" type="number" min="0" placeholder="Amount" required style={{padding:8}}/>
      <input name="currency" type="text" defaultValue="USD" required style={{padding:8}}/>
      <input name="terminalId" type="text" defaultValue="ADMIN_TEST" required style={{padding:8}}/>
      <button type="submit">Create</button>
    </form>
    <h2>Sessions</h2>
    <div style={{overflowX:'auto'}}><table style={{width:'100%',borderCollapse:'collapse'}}>
      <thead><tr><th>ID</th><th>Terminal</th><th>Amount</th><th>Currency</th><th>Status</th><th>Created</th><th>Actions</th></tr></thead>
      <tbody>{sessions.map(s=>(<tr key={s.id}>
        <td><code>{s.id}</code></td><td>{s.terminalId}</td><td>{s.amount}</td><td>{s.currency}</td><td>{s.status}</td>
        <td>{new Date(s.createdAt as unknown as string).toLocaleString()}</td>
        <td>{s.status==='pending'?(<form method="POST" action="/api/admin/mark-paid" style={{display:'inline'}}>
          <input type="hidden" name="sessionId" value={s.id}/><button type="submit">Mark Paid</button></form>):'—'}</td>
      </tr>))}</tbody></table></div>
  </section>);
}
