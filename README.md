# NFC Pay Starter (Netlify + Next.js + Prisma + Neon + ESP32 PN532)

Turn any NFC tag into a “tap to pay” landing flow that starts on an ESP32 and ends on a free HTTPS subdomain.  
**Backend:** Next.js (App Router) on Netlify, Prisma ORM, Postgres on Neon.  
**Firmware:** ESP32 + PN532 (SPI) with **SoftAP Wi‑Fi provisioning + captive portal** and **factory reset**.  
**Admin UI:** Create/mark sessions, manage devices, per-store auto-incremented terminals.

---

## ✨ Features

- Free HTTPS subdomain (Netlify) – works with iPhone NFC.
- REST API + API Key auth:
  - `POST /api/v1/sessions` – create session
  - `GET /api/v1/sessions/{id}` – check status
  - `POST /api/sandbox/pay` – simulate provider webhook (mark paid)
  - `GET /api/v1/bootstrap` – device bootstrap & auto-enroll
- Postgres (Neon free tier) + Prisma migrations.
- **Devices auto-enroll** on first call with **per-store terminal numbers**, e.g. `STORE01-0001`, `STORE01-0002`, …
- Checkout page: `https://<site>/p/<terminalId>` shows session + “Approve (Sandbox)”.
- Admin UI on same domain:
  - Login with shared admin key
  - List/create/mark sessions
  - Devices CRUD (deviceId / storeCode / terminal / amount / currency)
- **ESP32 SoftAP provisioning**: starts a Wi‑Fi setup portal if no saved Wi‑Fi.
- **Factory reset**: clear Wi‑Fi via button (hold on boot) or serial command.
- ESP32 sample sketch writes checkout URL to NFC tag and **shows payment status** on LED & buzzer (pending/paid/error).
- OpenAPI spec + Postman collection included below.

---

## 🗂️ Repository structure

```
nfc-pay-netlify/
  app/
    api/
      admin/
        create/route.ts
        devices/delete/route.ts
        devices/upsert/route.ts
        login/route.ts
        logout/route.ts
      sandbox/
        pay/route.ts
      v1/
        bootstrap/route.ts
        sessions/[id]/route.ts
        sessions/route.ts
    admin/
      devices/page.tsx
      page.tsx
    p/[terminalId]/page.tsx
    layout.tsx
    page.tsx
  prisma/
    migrations/
      0001_init/...
      0002_device/...
      0003_terminal_number/...
      0004_store_sequences/...
    schema.prisma
  src/lib/
    config.ts
    prisma.ts
  scripts/
    build.sh
  netlify.toml
  package.json
  tsconfig.json
  next.config.mjs
  .env.example
  README.md  ← (this file)
```

---

## 🚀 Quick start (Production on Netlify)

1) **Create Neon DB**  
   - Create a Neon project (free tier) → get the **pooled** connection string (ends with `…-pooler…`).  
   - Make sure it includes `?sslmode=require`.

2) **Fork + Connect to Netlify**  
   - Fork this repo and import it into Netlify (New site from Git).

3) **Environment variables**  
   In Netlify → Site settings → **Environment variables** → add (or **Import from `.env`**):

```
DATABASE_URL=postgresql://USER:PASSWORD@ep-your-project-pooler.us-east-1.aws.neon.tech/neondb?sslmode=require
API_KEY=esp32_test_api_key_123456
ADMIN_KEY=admin_demo_key_123456
NODE_VERSION=18
PUBLIC_BASE_URL=https://<your-site>.netlify.app

# Defaults for device auto-enroll
DEFAULT_STORE_CODE=STORE01
DEFAULT_AMOUNT=0
DEFAULT_CURRENCY=USD

# Terminal format (STORE-CODE + prefix + zero-padded number)
TERMINAL_PREFIX=
TERMINAL_PAD=4
```

> **Zero-amount:** `DEFAULT_AMOUNT=0` makes all *new* devices/sessions default to 0.  
> Existing devices can be edited in **/admin/devices**, or bulk updated via SQL:  
> `UPDATE "Device" SET "amount" = 0;`

4) **Deploy**  
   - Trigger a deploy (use “Clear cache and deploy” the first time).  
   - The build script will run Prisma migrations automatically.

5) **Admin login**  
   - Go to `/admin` and log in with `ADMIN_KEY`.

6) **Devices**  
   - `/admin/devices` to view/add devices.  
   - Or let devices **auto-enroll** by calling `/api/v1/bootstrap` once; they’ll appear with status `new`.

---

## 📶 Wi‑Fi Provisioning (SoftAP + Captive Portal)

If the ESP32 does not have valid Wi‑Fi credentials or cannot connect, it starts a **SoftAP** and captive portal automatically.

- **AP SSID:** `NFC-PAY-Setup-XXXX` (last 4 hex of MAC)  
- **AP Password:** `nfcsetup` (WPA2; iOS requires passworded APs)  
- **Portal IP:** `http://192.168.4.1` (captive portal redirects most URLs)  

### Portal pages
- `/` – Web UI to select SSID and enter password (auto-populates with a live scan)
- `/scan.json` – List of nearby networks `{ networks: [{ ssid, rssi }] }`
- `/status` – Connection status (“Not connected”, “Trying…”, or current SSID + IP)
- `/save` – POST (form-url-encoded) `ssid=...&pass=...` → saves to NVS and attempts connection

### Provisioning flow
1. On boot the device tries stored Wi‑Fi for ~15s.  
2. If it fails, it starts AP + portal and waits for credentials.  
3. After saving, it connects in STA mode **without reboot**, shuts down the AP, and continues the normal flow (bootstrap → session → NFC write).

### Factory reset Wi‑Fi
- **Button:** hold **GPIO 34** **LOW** during boot for ~3 seconds to erase Wi‑Fi and reboot.  
  (Change pin in `#define BUTTON_PIN 34`.)  
- **Serial:** send `FACTORY` / `RESETWIFI` / `FORGET` to erase Wi‑Fi and reboot.

> Security note: The AP password is in the firmware (`AP_PASSWORD`). For production, consider a per-device AP password or provisioning via BLE with authenticated pairing. Also pin the backend TLS CA on the ESP32 (replace `setInsecure()`).

---

## 🔐 Auth model

- **Devices & ESP32 → API key**  
  Send header `X-API-Key: <API_KEY>` on all device API calls.
- **Admin UI → shared key**  
  On `/admin` login form, enter `ADMIN_KEY`.  
  A short-lived cookie is set for the session.

---

## 🧱 Database models

- `Session { id, terminalId, amount, currency, status, createdAt, updatedAt }`  
- `Device  { deviceId, storeCode, terminalId, amount, currency, status, ... }`  
- `TerminalSequence { storeCode PRIMARY KEY, last INT }` – **per-store counter**  
- `TerminalNumber { id SERIAL }` – (legacy/simple counter; not used when store sequencing is on)

---

## 🔢 Per-store terminal numbers

New/unknown devices calling `GET /api/v1/bootstrap` are auto-enrolled:

1. Determine **store code** from `X-Store-Code` header (or `?store=…`) or fallback to `DEFAULT_STORE_CODE`.
2. **Atomic** increment of `TerminalSequence.last` for that store.
3. Format terminal as:
   ```
   ${STORE}-${TERMINAL_PREFIX}${NNNN}
   ```
   Example: `STORE01-0007`, or if `TERMINAL_PREFIX=KIOSK`, `STORE01-KIOSK0007`.

You can also **manually set** a terminal in `/admin/devices`.  
Leaving TerminalId blank in the form **auto-assigns the next** number for that store.

---

## 🌐 Endpoints

All device endpoints require header `X-API-Key: <API_KEY>`.

### Bootstrap (auto-enroll)
`GET /api/v1/bootstrap?deviceId=<hex>[&store=STORE01]`

Headers: `X-API-Key`, `X-Device-Id` (optional if in query), `X-Store-Code` (optional)

**200**:
```json
{
  "deviceId": "A1B2C3D4E5F6",
  "storeCode": "STORE01",
  "terminalId": "STORE01-0007",
  "amount": 0,
  "currency": "USD",
  "checkoutUrl": "https://<site>/p/STORE01-0007",
  "autoEnrolled": true
}
```

### Create session
`POST /api/v1/sessions`

Headers: `X-API-Key`, `X-Device-Id`, optional `X-Store-Code`  
Body: `{}` (server fills from Device) or `{"terminalId": "STORE01-0007"}`

**201**:
```json
{
  "id": "e1c7f4dc-...-3e0d1f16",
  "terminalId": "STORE01-0007",
  "amount": 0,
  "currency": "USD",
  "status": "pending",
  "createdAt": "...",
  "updatedAt": "..."
}
```

### Check session
`GET /api/v1/sessions/{id}`

Headers: `X-API-Key`  
**200**: `{ ... "status": "pending" | "paid" }`

### Sandbox “provider webhook” (mark paid)
`POST /api/sandbox/pay`  
Form or JSON body: `{ "sessionId": "<uuid>" }`  
**200**: `{ "ok": true, "status": "paid" }` (or 302 redirect back to the page)

### Checkout page (customer)
`GET /p/{terminalId}` – shows the latest **pending** session for that terminal and an **Approve (Sandbox)** button.

---

## 📜 OpenAPI (save as `openapi.yaml`)

```yaml
openapi: 3.0.3
info:
  title: NFC Pay Starter API
  version: "1.0.0"
servers:
  - url: https://<your-site>.netlify.app
paths:
  /api/v1/bootstrap:
    get:
      summary: Device bootstrap (auto-enroll, per-store terminal)
      parameters:
        - name: deviceId
          in: query
          required: false
          schema: { type: string }
        - name: store
          in: query
          required: false
          schema: { type: string }
      security:
        - ApiKeyAuth: []
      responses:
        "200":
          description: OK
          content:
            application/json:
              schema:
                type: object
                properties:
                  deviceId: { type: string }
                  storeCode: { type: string }
                  terminalId: { type: string }
                  amount: { type: integer }
                  currency: { type: string }
                  checkoutUrl: { type: string, format: uri }
                  autoEnrolled: { type: boolean }
        "401": { description: Unauthorized }
  /api/v1/sessions:
    post:
      summary: Create a session for POS device
      security: [{ ApiKeyAuth: [] }]
      requestBody:
        required: false
        content:
          application/json:
            schema:
              type: object
              properties:
                terminalId: { type: string }
                amount: { type: integer }
                currency: { type: string }
      responses:
        "201":
          description: Created
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/Session'
        "401": { description: Unauthorized }
    get:
      summary: List sessions (admin/debug)
      security: [{ ApiKeyAuth: [] }]
      responses:
        "200":
          description: OK
          content:
            application/json:
              schema:
                type: array
                items: { $ref: '#/components/schemas/Session' }
  /api/v1/sessions/{id}:
    get:
      summary: Get session by id
      parameters:
        - name: id
          in: path
          required: true
          schema: { type: string }
      security: [{ ApiKeyAuth: [] }]
      responses:
        "200":
          description: OK
          content:
            application/json:
              schema: { $ref: '#/components/schemas/Session' }
        "404": { description: Not found }
        "401": { description: Unauthorized }
  /api/sandbox/pay:
    post:
      summary: Sandbox pay (mark paid)
      requestBody:
        required: true
        content:
          application/json:
            schema: { type: object, properties: { sessionId: { type: string } } }
          application/x-www-form-urlencoded:
            schema: { type: object, properties: { sessionId: { type: string } } }
      responses:
        "200": { description: OK }
components:
  securitySchemes:
    ApiKeyAuth:
      type: apiKey
      in: header
      name: X-API-Key
  schemas:
    Session:
      type: object
      properties:
        id: { type: string, format: uuid }
        terminalId: { type: string }
        amount: { type: integer }
        currency: { type: string }
        status: { type: string, enum: [pending, paid] }
        createdAt: { type: string, format: date-time }
        updatedAt: { type: string, format: date-time }
```

---

## 📬 Postman collection (save as `postman_collection.json`)

```json
{
  "info": {
    "name": "NFC Pay Starter",
    "_postman_id": "b9b5d93b-6b66-4b84-941a-1234567890ab",
    "schema": "https://schema.getpostman.com/json/collection/v2.1.0/collection.json"
  },
  "variable": [
    { "key": "baseUrl", "value": "https://<your-site>.netlify.app" },
    { "key": "apiKey", "value": "esp32_test_api_key_123456" },
    { "key": "deviceId", "value": "A1B2C3D4E5F6" },
    { "key": "storeCode", "value": "STORE01" },
    { "key": "sessionId", "value": "" }
  ],
  "item": [
    {
      "name": "Bootstrap",
      "request": {
        "method": "GET",
        "header": [
          { "key": "X-API-Key", "value": "{{apiKey}}" },
          { "key": "X-Device-Id", "value": "{{deviceId}}" },
          { "key": "X-Store-Code", "value": "{{storeCode}}" }
        ],
        "url": { "raw": "{{baseUrl}}/api/v1/bootstrap", "host": ["{{baseUrl}}"], "path": ["api","v1","bootstrap"] }
      }
    },
    {
      "name": "Create Session",
      "request": {
        "method": "POST",
        "header": [
          { "key": "X-API-Key", "value": "{{apiKey}}" },
          { "key": "X-Device-Id", "value": "{{deviceId}}" },
          { "key": "Content-Type", "value": "application/json" }
        ],
        "url": { "raw": "{{baseUrl}}/api/v1/sessions", "host": ["{{baseUrl}}"], "path": ["api","v1","sessions"] },
        "body": { "mode": "raw", "raw": "{}" }
      }
    },
    {
      "name": "Get Session",
      "request": {
        "method": "GET",
        "header": [ { "key": "X-API-Key", "value": "{{apiKey}}" } ],
        "url": { "raw": "{{baseUrl}}/api/v1/sessions/{{sessionId}}", "host": ["{{baseUrl}}"], "path": ["api","v1","sessions","{{sessionId}}"] }
      }
    },
    {
      "name": "Sandbox Pay",
      "request": {
        "method": "POST",
        "header": [ { "key": "Content-Type", "value": "application/json" } ],
        "url": { "raw": "{{baseUrl}}/api/sandbox/pay", "host": ["{{baseUrl}}"], "path": ["api","sandbox","pay"] },
        "body": { "mode": "raw", "raw": "{\n  \"sessionId\": \"{{sessionId}}\"\n}" }
      }
    }
  ]
}
```

---

## 🧪 ESP32 firmware (LED & buzzer status + SoftAP)

- Hardware: ESP32 + PN532 (SPI), NTAG213/215/216 tags.  
- Defaults:
  - PN532 pins: SCK=18, MOSI=23, MISO=19, SS=5, RST=27
  - LED pin: `GPIO 2`
  - Buzzer pin: `GPIO 15` (**active** buzzer; for passive, use `tone()`)
  - Factory button: `GPIO 34` → GND to erase Wi‑Fi on boot

**Flow on boot**
1) Try stored Wi‑Fi for ~15s → if fails, start **SoftAP portal** (see above).  
2) `GET /api/v1/bootstrap` with headers (`X-API-Key`, `X-Device-Id`, optional `X-Store-Code`).  
3) `POST /api/v1/sessions` – creates pending session.  
4) Write `checkoutUrl` to tag; verify; RF OFF.  
5) Poll `GET /api/v1/sessions/{id}` until `status:"paid"`.

**Indicators**
- Pending: LED slow blink; short chirp every 2s.  
- Paid: LED solid ON; 3 quick chirps once.  
- Error: fast blink + chirp every 500ms for 5s.

> If Arduino IDE uses the wrong `WiFiClientSecure`, remove any third-party folder at  
> `C:\Users\<you>\Documents\Arduino\libraries\WiFiClientSecure` so the ESP32 core’s version is used.

---

## ⚙️ Build & deploy details

- **Netlify runtime**: Next.js plugin + `NODE_VERSION=18`.  
- **Prisma migrations**: `scripts/build.sh` runs `prisma migrate deploy` (skips when `DATABASE_URL` is placeholder).  
- **Path aliases**: Use `@/lib/prisma` not `@/src/lib/prisma`. `tsconfig.json` sets `"paths": { "@/lib/*": ["src/lib/*"] }`.

---

## 🧯 Troubleshooting

**Can’t see the portal on iPhone**  
Ensure the AP has a password (WPA2). The default `AP_PASSWORD` is `nfcsetup`.

**Netlify build says TypeScript types missing**  
The repo includes `@types/react` & `@types/node`. If Netlify still complains, ensure `npm ci` ran.

**`DATABASE_URL not set or placeholder`**  
Set a real Neon pooled URL (`…-pooler…`) with `?sslmode=require`.

**`Module not found: '@/src/lib/prisma'`**  
Use `@/lib/prisma` or adjust `tsconfig.json` paths.

**Prisma major update banner**  
Informational. Update later with:
```
npm i --save-dev prisma@latest
npm i @prisma/client@latest
```

---

## 🔒 Security notes (before real payments)

- Give each device its **own secret** (replace global `API_KEY`) and verify HMAC in headers (`X-Signature`).  
- **Pin TLS CA** on ESP32 (replace `setInsecure()`).  
- Rate-limit `/bootstrap` and `/sessions`.  
- For a real PSP, **verify webhook signatures** and use **idempotency keys**.

---

## 📄 License

MIT (or your choice).
