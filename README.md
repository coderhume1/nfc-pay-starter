
# NFC Pay Starter (Netlify + Next.js + Neon + Prisma)

End-to-end scaffold for an ESP32 NFC demo.

## Stack
- Next.js (App Router, API routes)
- Netlify (free HTTPS subdomain)
- Neon Postgres (free tier) via Prisma
- Admin UI (/admin)
- Checkout page (/p/{terminalId})

## Quick start
```bash
cp .env.example .env   # set DATABASE_URL, API_KEY, ADMIN_KEY
npm install
npx prisma migrate dev --name init
npm run dev
```

## Deploy to Netlify
- Build: `npm run build`
- Publish dir: `.next`
- Env vars: `DATABASE_URL` (Neon with sslmode=require), `API_KEY`, `ADMIN_KEY`
- Plugin `@netlify/plugin-nextjs` already configured in `netlify.toml`

## API
- POST /api/v1/sessions  (X-API-Key) → { id, status, ... }
- GET  /api/v1/sessions/{id}  (X-API-Key) → { id, status, ... }
- POST /api/sandbox/pay  (sessionId) → marks paid

## Admin
- /admin → login with ADMIN_KEY cookie; list, create, mark paid

## ESP32
- Create session with HTTPS POST (include X-API-Key)
- Poll GET status until `paid`
- NFC tag URL: `https://<yoursite>.netlify.app/p/<terminalId>`

Path alias fix: use `@/lib/prisma` (see tsconfig `paths`).
