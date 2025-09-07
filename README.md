
# NFC Pay Starter (Fixed)
- Includes `.env` with test keys and Node 18 pin.
- Import `.env` into Netlify → edit only `DATABASE_URL` to Neon pooled URI.

Local:
```
cp .env .env.local  # optional
npm install
npx prisma migrate dev --name init
npm run dev
```

Netlify:
- Import `.env` (Project configuration → Environment variables → Import from a .env file)
- Build: npm run build
- Publish: .next
