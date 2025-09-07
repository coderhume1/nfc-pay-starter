
import { NextRequest, NextResponse } from 'next/server'; import { prisma } from '@/lib/prisma';
export async function POST(req: NextRequest){
  if(req.cookies.get('admin_auth')?.value!=='1') return NextResponse.json({error:'Unauthorized'},{status:401});
  const f = await req.formData(); const amount = parseInt(String(f.get('amount')||'0'),10);
  const currency = String(f.get('currency')||'USD'); const terminalId = String(f.get('terminalId')||'ADMIN_TEST');
  if(Number.isNaN(amount)) return NextResponse.json({error:'Invalid amount'},{status:400});
  await prisma.session.create({data:{terminalId,amount,currency,status:'pending'}});
  return NextResponse.redirect(new URL('/admin', req.url));
}
