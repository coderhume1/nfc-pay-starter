# Quickstart — NFC Pay Starter (Minimal Setup)

This is the shortest path to get the project live on a **free HTTPS subdomain** and create a test payment flow.


## 0) What you need
- A **Netlify** account (free)  
- A **Neon** Postgres account (free)  
- Arduino IDE to flash the ESP32
- Library installation in Arduino IDE


## 1) Get the code
- **Option A**: Fork the repo on GitHub and connect it to Netlify (recommended)  
- **Option B**: Download ZIP and in Netlify choose **Deploy manually** (Upload your folder)


## 2) Create the database (Neon)
1. Create a Neon **Project**.
2. Copy the **pooled connection string** (contains `-pooler`) and append `?sslmode=require`  
   Example:  
   `postgresql://USER:PASSWORD@ep-xxxx-pooler.us-east-1.aws.neon.tech/neondb?sslmode=require`


## 3) Netlify setup
1. **New site** → **Import from Git** (or upload ZIP)  
2. **Build command**: `npm run build`  
   **Publish directory**: `.next`
3. Add **Environment variables** (Site settings → Environment variables):

```ini
# Required
DATABASE_URL=postgresql://USER:PASSWORD@ep-xxxx-pooler.us-east-1.aws.neon.tech/neondb?sslmode=require
API_KEY=esp32_test_api_key_123456
ADMIN_KEY=admin_demo_key_123456
NODE_VERSION=18
PUBLIC_BASE_URL=https://<your-site>.netlify.app

# Defaults (safe to keep)
DEFAULT_STORE_CODE=STORE01
DEFAULT_AMOUNT=0
DEFAULT_CURRENCY=USD
TERMINAL_PREFIX=
TERMINAL_PAD=4
```
4. **Deploy** (first time may take ~1–2 min). If you ever change `DATABASE_URL`, trigger **Clear cache and deploy**.


## 4) Smoke test
- Open the **Admin**: `https://<your-site>.netlify.app/admin` → log in with **ADMIN_KEY**.
- Open a checkout page: `https://<your-site>.netlify.app/p/ADMIN_TEST` → click **Approve (Sandbox)** → page shows **Paid**.

**API (optional quick curl):**
```bash
# Bootstrap (auto-enroll device)
curl -sS -H "X-API-Key: esp32_test_api_key_123456" \
     -H "X-Device-Id: A1B2C3D4E5F6" \
     https://<your-site>.netlify.app/api/v1/bootstrap

# Create session
curl -sS -X POST -H "Content-Type: application/json" \
     -H "X-API-Key: esp32_test_api_key_123456" \
     -H "X-Device-Id: A1B2C3D4E5F6" \
     -d '{}' \
     https://<your-site>.netlify.app/api/v1/sessions

# Mark paid (sandbox)
curl -sS -X POST -H "Content-Type: application/json" \
     -d '{"sessionId":"<SESSION_ID_FROM_PREV>"}' \
     https://<your-site>.netlify.app/api/sandbox/pay
```


## 5) ESP32
- Open the provided sketch `esp32_nfc_provisioner_softap.ino`
- Set at the top:
  ```cpp
  const char* BASE_URL = "https://<your-site>.netlify.app";
  const char* API_KEY  = "esp32_test_api_key_123456";
  ```
- Flash. If no Wi‑Fi saved, device starts **SoftAP**: `NFC-PAY-Setup-XXXX` (pass `nfcsetup`) → portal at `http://192.168.4.1` to enter Wi‑Fi.
- The device auto‑enrolls, creates a session, writes the NFC checkout URL, and shows **pending / paid / error** on LED & buzzer.


