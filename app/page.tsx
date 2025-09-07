
export default function HomePage() {
  return (
    <section>
      <h1>Welcome 👋</h1>
      <p>This is a minimal full-stack demo for ESP32 NFC payments.</p>
      <ol>
        <li>ESP32 calls <code>POST /api/v1/sessions</code> to create a session.</li>
        <li>Customer taps NFC → opens <code>/p/&lt;terminalId&gt;</code> on this domain.</li>
        <li>Customer clicks Approve (sandbox) → session marked <code>paid</code>.</li>
        <li>ESP32 polls <code>GET /api/v1/sessions/:id</code> until <code>status=paid</code>.</li>
      </ol>
      <p>Use the <a href="/admin">Admin</a> page to inspect sessions or create test ones.</p>
    </section>
  );
}
