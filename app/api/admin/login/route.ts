
import { NextRequest, NextResponse } from 'next/server';

export async function POST(req: NextRequest) {
  const form = await req.formData();
  const key = String(form.get('key') || '');
  const adminKey = process.env.ADMIN_KEY || '';

  if (adminKey && key === adminKey) {
    const res = NextResponse.redirect(new URL('/admin', req.url));
    res.cookies.set('admin_auth', '1', {
      httpOnly: true,
      secure: true,
      sameSite: 'lax',
      path: '/',
      maxAge: 60 * 60 * 8,
    });
    return res;
  }
  return NextResponse.json({ error: 'Invalid admin key' }, { status: 401 });
}
