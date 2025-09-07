
import { NextRequest, NextResponse } from 'next/server';
import { prisma } from '@/lib/prisma';

export async function POST(req: NextRequest) {
  const cookie = req.cookies.get('admin_auth')?.value;
  if (cookie !== '1') return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });

  const form = await req.formData();
  const amount = parseInt(String(form.get('amount') || '0'), 10);
  const currency = String(form.get('currency') || 'USD');
  const terminalId = String(form.get('terminalId') || 'ADMIN_TEST');

  if (Number.isNaN(amount) || !currency) {
    return NextResponse.json({ error: 'Invalid data' }, { status: 400 });
  }

  await prisma.session.create({ data: { terminalId, amount, currency, status: 'pending' } });
  return NextResponse.redirect(new URL('/admin', req.url));
}
