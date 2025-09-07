
CREATE TABLE IF NOT EXISTS "Device" (
  "deviceId" text PRIMARY KEY,
  "storeCode" text NOT NULL DEFAULT '',
  "terminalId" text NOT NULL,
  "amount" integer NOT NULL,
  "currency" text NOT NULL,
  "status" text NOT NULL DEFAULT 'active',
  "createdAt" timestamptz NOT NULL DEFAULT now(),
  "updatedAt" timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS "Device_store_idx" ON "Device" ("storeCode");

DROP TRIGGER IF EXISTS trg_device_updated_at ON "Device";
CREATE TRIGGER trg_device_updated_at
BEFORE UPDATE ON "Device"
FOR EACH ROW EXECUTE FUNCTION set_updated_at();
