
import { NextRequest, NextResponse } from 'next/server';
import { prisma } from '@/lib/prisma';

export async function POST(req: NextRequest) {
  const cookie = req.cookies.get('admin_auth')?.value;
  if (cookie !== '1') return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });

  const form = await req.formData();
  const sessionId = String(form.get('sessionId') || '');
  if (!sessionId) return NextResponse.json({ error: 'sessionId required' }, { status: 400 });

  await prisma.session.update({ where: { id: sessionId }, data: { status: 'paid' } }).catch(() => null);
  return NextResponse.redirect(new URL('/admin', req.url));
}
