
export default function Home(){
  return (<section><h1>Welcome 👋</h1>
    <p>Flow: Device → <code>/api/v1/bootstrap</code> (auto-enroll with per-store terminal sequence) → <code>/api/v1/sessions</code> → write <code>/p/&lt;terminalId&gt;</code> to NFC.</p>
  </section>);
}
