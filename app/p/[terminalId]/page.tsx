
import { prisma } from '@/lib/prisma';

export default async function TerminalPage({ params }: { params: { terminalId: string } }) {
  const { terminalId } = params;
  const session = await prisma.session.findFirst({
    where: { terminalId, status: 'pending' },
    orderBy: { createdAt: 'desc' },
  });

  return (
    <section>
      <h1>Checkout</h1>
      <p>Terminal: <b>{terminalId}</b></p>
      {!session ? (
        <div style={{ padding: 12, border: '1px solid #333', borderRadius: 8 }}>
          <p>No active session found. Please try again.</p>
        </div>
      ) : (
        <div style={{ padding: 12, border: '1px solid #333', borderRadius: 8 }}>
          <p><b>Amount:</b> {session.currency} {session.amount}</p>
          <p><b>Session ID:</b> <code>{session.id}</code></p>
          <form method="POST" action="/api/sandbox/pay" style={{ marginTop: 12 }}>
            <input type="hidden" name="sessionId" value={session.id} />
            <button type="submit" style={{ padding: '10px 14px' }}>Approve (Sandbox)</button>
          </form>
        </div>
      )}
    </section>
  );
}
