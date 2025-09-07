
export default function Home(){
  return (<section><h1>Welcome 👋</h1>
    <ol><li>ESP32 → <code>POST /api/v1/sessions</code></li>
    <li>NFC opens <code>/p/&lt;terminalId&gt;</code></li>
    <li>Approve (sandbox)</li>
    <li>ESP32 polls <code>GET /api/v1/sessions/:id</code></li></ol>
    <p>Use <a href="/admin">Admin</a> to create/test sessions.</p>
  </section>);
}
