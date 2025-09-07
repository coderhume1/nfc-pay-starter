
# NFC Pay Starter — Per-store terminal sequences
- Unknown devices auto-enroll on `/api/v1/bootstrap` and receive a **store-scoped** terminal like `STORE01-0001`, `STORE01-0002`, etc.
- Admin UI at `/admin/devices` to manage `storeCode`, `terminalId`, amount, currency.
- `POST /api/v1/sessions` resolves config from the Device row.

## Headers the device should send
- `X-API-Key`: your API key
- `X-Device-Id`: device chip id (e.g., MAC in hex)
- `X-Store-Code`: optional, to allocate terminals within that store (defaults to DEFAULT_STORE_CODE)

## Env
DEFAULT_STORE_CODE=STORE01
TERMINAL_PREFIX=        # optional, prefix inserted before the number, after store code
TERMINAL_PAD=4          # digits of zero-padding

Result formats:
- With store code: `${STORE}-${TERMINAL_PREFIX}${NNNN}` (e.g. STORE01-0007, or STORE01-KIOSK0007)
- Without store code: `${TERMINAL_PREFIX}${NNNN}`
