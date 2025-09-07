
import { prisma } from '@/lib/prisma'; import { cookies } from 'next/headers';
export default async function Devices(){
  const authed = cookies().get('admin_auth')?.value==='1';
  if(!authed){ return (<section><h1>Devices</h1><p>Please <a href="/admin">login</a>.</p></section>); }
  const devices = await prisma.device.findMany({ orderBy:{ createdAt:'desc' }, take: 200 });
  return (<section>
    <h1>Devices</h1>
    <h2>Add / Update</h2>
    <form method="POST" action="/api/admin/devices/upsert" style={{display:'flex',gap:8,flexWrap:'wrap',marginBottom:24}}>
      <input name="deviceId" placeholder="DeviceId (hex MAC)" required style={{padding:8}}/>
      <input name="storeCode" placeholder="Store Code (e.g. STORE01)" style={{padding:8}}/>
      <input name="terminalId" placeholder="TerminalId (leave blank for auto)" style={{padding:8}}/>
      <input name="amount" type="number" placeholder="Amount (cents)" defaultValue="0" required style={{padding:8}}/>
      <input name="currency" placeholder="Currency" defaultValue="USD" required style={{padding:8}}/>
      <button type="submit">Save</button>
    </form>
    <h2>All</h2>
    <div style={{overflowX:'auto'}}><table style={{width:'100%',borderCollapse:'collapse'}}>
      <thead><tr><th>DeviceId</th><th>Store</th><th>Terminal</th><th>Amount</th><th>Currency</th><th>Status</th><th>Created</th><th>Actions</th></tr></thead>
      <tbody>{devices.map(d=>(<tr key={d.deviceId}>
        <td><code>{d.deviceId}</code></td><td>{d.storeCode}</td><td>{d.terminalId}</td><td>{d.amount}</td><td>{d.currency}</td><td>{d.status}</td>
        <td>{new Date(d.createdAt as unknown as string).toLocaleString()}</td>
        <td>
          <form method="POST" action="/api/admin/devices/delete" style={{display:'inline'}}>
            <input type="hidden" name="deviceId" value={d.deviceId}/><button type="submit">Delete</button>
          </form>
        </td>
      </tr>))}</tbody></table></div>
  </section>);
}
