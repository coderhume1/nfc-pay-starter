
import { NextRequest, NextResponse } from 'next/server';
import { prisma } from '@/lib/prisma';

export async function POST(req: NextRequest) {
  const apiKey = req.headers.get('x-api-key') || '';
  const expected = process.env.API_KEY || '';
  if (!expected || apiKey !== expected) {
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }
  const body = await req.json().catch(() => null);
  if (!body) return NextResponse.json({ error: 'Invalid JSON' }, { status: 400 });

  const terminalId = String(body.terminalId || '');
  const amount = parseInt(String(body.amount ?? '0'), 10);
  const currency = String(body.currency || 'USD');
  if (!terminalId || Number.isNaN(amount) || !currency) {
    return NextResponse.json({ error: 'Missing or invalid fields' }, { status: 400 });
  }

  const session = await prisma.session.create({
    data: { terminalId, amount, currency, status: 'pending' },
  });
  return NextResponse.json(session, { status: 201 });
}

export async function GET(req: NextRequest) {
  const apiKey = req.headers.get('x-api-key') || '';
  const expected = process.env.API_KEY || '';
  if (!expected || apiKey !== expected) {
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }
  const sessions = await prisma.session.findMany({ orderBy: { createdAt: 'desc' } });
  return NextResponse.json(sessions);
}
