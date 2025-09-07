
import { NextRequest, NextResponse } from 'next/server'; import { prisma } from '@/lib/prisma';
export async function POST(req: NextRequest){
  if(req.cookies.get('admin_auth')?.value!=='1') return NextResponse.json({error:'Unauthorized'},{status:401});
  const f = await req.formData(); const deviceId = String(f.get('deviceId')||'').trim();
  if(deviceId) await prisma.device.delete({ where:{ deviceId } }).catch(()=>null);
  return NextResponse.redirect(new URL('/admin/devices', req.url));
}
