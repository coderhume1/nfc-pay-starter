import { cookies } from 'next/headers'; export function isAdminAuthed(){return cookies().get('admin_auth')?.value==='1'}
