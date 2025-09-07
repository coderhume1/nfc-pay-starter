
import { NextRequest, NextResponse } from 'next/server'; import { prisma } from '@/lib/prisma';
export async function POST(req: NextRequest){
  const ct = req.headers.get('content-type')||''; let sessionId = '';
  if(ct.includes('application/json')){ const b = await req.json().catch(()=>({})); sessionId = String((b as any).sessionId||''); }
  else { const f = await req.formData().catch(()=>null); sessionId = f? String(f.get('sessionId')||'') : ''; }
  if(!sessionId) return NextResponse.json({error:'sessionId required'},{status:400});
  const up = await prisma.session.update({where:{id:sessionId},data:{status:'paid'}}).catch(()=>null);
  if(!up) return NextResponse.json({error:'Session not found'},{status:404});
  const ref = req.headers.get('referer'); if(ref && !ct.includes('application/json')) return NextResponse.redirect(ref);
  return NextResponse.json({ok:true,status:'paid'});
}
