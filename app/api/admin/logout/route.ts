
import { NextRequest, NextResponse } from 'next/server';
export async function POST(req: NextRequest) {
  const res = NextResponse.redirect(new URL('/admin', req.url));
  res.cookies.set('admin_auth', '', { httpOnly: true, secure: true, sameSite: 'lax', path: '/', maxAge: 0 });
  return res;
}
