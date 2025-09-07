export type DeviceCfg = { deviceId?: string; terminalId?: string; amount?: number; currency?: string };

function num(n: any, d: number) { const x = Number(n); return Number.isFinite(x) ? x : d; }

export function defaults() {
  return {
    terminalId: process.env.DEFAULT_TERMINAL_ID || "TERM01",
    amount:     num(process.env.DEFAULT_AMOUNT,   0),
    currency:   process.env.DEFAULT_CURRENCY || "USD",
  };
}

export function resolveForDevice(deviceId?: string) {
  const base = defaults();
  const json = process.env.DEVICE_CONFIG; // e.g. [{"deviceId":"A1B2C3D4E5F6","terminalId":"TERM001","amount":1200,"currency":"USD"}]
  if (!json || !deviceId) return base;
  try {
    const arr = JSON.parse(json) as DeviceCfg[];
    const hit = arr.find(x => (x.deviceId||"").toLowerCase() === deviceId.toLowerCase());
    return { 
      terminalId: hit?.terminalId || base.terminalId,
      amount:     num(hit?.amount, base.amount),
      currency:   hit?.currency || base.currency
    };
  } catch { return base; }
}
