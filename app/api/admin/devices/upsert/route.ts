
import { NextRequest, NextResponse } from 'next/server'; import { prisma } from '@/lib/prisma'; import { assignNextTerminalCode } from '@/lib/config';
export async function POST(req: NextRequest){
  if(req.cookies.get('admin_auth')?.value!=='1') return NextResponse.json({error:'Unauthorized'},{status:401});
  const f = await req.formData();
  const deviceId = String(f.get('deviceId')||'').trim();
  const storeCode = String(f.get('storeCode')||'').trim() || process.env.DEFAULT_STORE_CODE || 'STORE01';
  let terminalId = String(f.get('terminalId')||'').trim();
  const amount = parseInt(String(f.get('amount')||'0'),10);
  const currency = String(f.get('currency')||'USD').trim();
  if(!deviceId || Number.isNaN(amount) || !currency) return NextResponse.json({error:'Invalid fields'},{status:400});
  if(!terminalId) terminalId = await assignNextTerminalCode(storeCode);
  await prisma.device.upsert({ where:{ deviceId }, create:{ deviceId, storeCode, terminalId, amount, currency }, update:{ storeCode, terminalId, amount, currency } });
  return NextResponse.redirect(new URL('/admin/devices', req.url));
}
