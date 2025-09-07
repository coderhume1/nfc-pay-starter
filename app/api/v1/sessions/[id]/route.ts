
import { NextRequest, NextResponse } from 'next/server';
import { prisma } from '@/lib/prisma';

export async function GET(req: NextRequest, { params }: { params: { id: string } }) {
  const apiKey = req.headers.get('x-api-key') || '';
  const expected = process.env.API_KEY || '';
  if (!expected || apiKey !== expected) {
    return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
  }
  const id = params.id;
  const session = await prisma.session.findUnique({ where: { id } });
  if (!session) return NextResponse.json({ error: 'Not found' }, { status: 404 });
  return NextResponse.json(session);
}
