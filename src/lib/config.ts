
import { prisma } from './prisma';

export type DeviceConfig = { terminalId: string; amount: number; currency: string; storeCode?: string };

function num(n: any, d: number) { const x = Number(n); return Number.isFinite(x) ? x : d; }
const PREFIX = process.env.TERMINAL_PREFIX || '';
const PAD = Math.max(0, Number(process.env.TERMINAL_PAD || 4) || 4);

export function defaults(): DeviceConfig {
  return {
    terminalId: '', // generated per store when needed
    amount: num(process.env.DEFAULT_AMOUNT, 0),
    currency: process.env.DEFAULT_CURRENCY || 'USD',
    storeCode: process.env.DEFAULT_STORE_CODE || 'STORE01',
  };
}

function formatTerminal(storeCode: string|undefined|null, n: number): string {
  const s = (storeCode || '').trim();
  const numPart = String(n).padStart(PAD, '0');
  if (s) return `${s}-${PREFIX}${numPart}`;
  return `${PREFIX}${numPart}`;
}

export async function assignNextTerminalCode(storeCode?: string): Promise<string> {
  const sc = (storeCode || process.env.DEFAULT_STORE_CODE || '').trim();
  // Ensure row exists, then atomically increment
  await prisma.$executeRaw`INSERT INTO "TerminalSequence" ("storeCode","last","updatedAt") VALUES (${sc}, 0, NOW()) ON CONFLICT ("storeCode") DO NOTHING;`;
  const rows = await prisma.$queryRaw<{ last: number }[]>`UPDATE "TerminalSequence" SET "last" = "last" + 1, "updatedAt" = NOW() WHERE "storeCode" = ${sc} RETURNING "last";`;
  const next = rows[0]?.last ?? 1;
  return formatTerminal(sc, next);
}

export async function getOrCreateDevice(deviceId?: string, storeCodeFromReq?: string): Promise<DeviceConfig & { deviceId?: string, created?: boolean }> {
  const d = defaults();
  if (!deviceId) return { ...d };

  const existing = await prisma.device.findUnique({ where: { deviceId } }).catch(()=>null);
  if (existing) {
    return { deviceId, terminalId: existing.terminalId, amount: existing.amount, currency: existing.currency, storeCode: existing.storeCode };
  }

  const sc = (storeCodeFromReq || d.storeCode)!;
  const term = await assignNextTerminalCode(sc);
  await prisma.device.create({ data: { deviceId, storeCode: sc, terminalId: term, amount: d.amount, currency: d.currency, status: 'new' } });
  return { deviceId, storeCode: sc, terminalId: term, amount: d.amount, currency: d.currency, created: true };
}
