
import { prisma } from '@/lib/prisma';
export default async function Page({params}:{params:{terminalId:string}}){
  const { terminalId } = params;
  const session = await prisma.session.findFirst({ where:{ terminalId, status:'pending' }, orderBy:{ createdAt:'desc' } });
  return (<section><h1>Checkout</h1><p>Terminal: <b>{terminalId}</b></p>
    {!session ? (<div style={{border:'1px solid #333',padding:12,borderRadius:8}}><p>No active session. Please try again.</p></div>)
    : (<div style={{border:'1px solid #333',padding:12,borderRadius:8}}>
        <p><b>Amount:</b> {session.currency} {session.amount}</p>
        <p><b>Session:</b> <code>{session.id}</code></p>
        <form method="POST" action="/api/sandbox/pay"><input type="hidden" name="sessionId" value={session.id}/>
          <button type="submit" style={{padding:'10px 14px'}}>Approve (Sandbox)</button></form></div>)}
  </section>);
}
