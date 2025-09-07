
import { NextRequest, NextResponse } from 'next/server'; import { prisma } from '@/lib/prisma';
export async function POST(req: NextRequest){
  if(req.cookies.get('admin_auth')?.value!=='1') return NextResponse.json({error:'Unauthorized'},{status:401});
  const f = await req.formData(); const id = String(f.get('sessionId')||'');
  if(!id) return NextResponse.json({error:'sessionId required'},{status:400});
  await prisma.session.update({where:{id},data:{status:'paid'}}).catch(()=>null);
  return NextResponse.redirect(new URL('/admin', req.url));
}
