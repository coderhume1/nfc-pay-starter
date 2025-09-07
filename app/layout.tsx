
import React from 'react';
export const metadata = { title:'NFC Pay Starter', description:'Netlify + Next.js + Neon + Prisma' };
export default function RootLayout({ children }:{children:React.ReactNode}) {
  const link = (href:string, text:string) => <a href={href} style={{color:'#a8c8ff'}}>{text}</a>;
  return (<html lang="en"><body style={{fontFamily:'system-ui, sans-serif',margin:0,padding:16,background:'#0b0f19',color:'#e6e9ef'}}>
    <header style={{display:'flex',justifyContent:'space-between',alignItems:'center',marginBottom:16}}>
      <h2 style={{margin:0}}>NFC Pay Starter</h2>
      <nav style={{display:'flex',gap:12}}>{link('/', 'Home')}{link('/admin', 'Admin')}{link('/admin/devices','Devices')}</nav>
    </header>
    <main style={{maxWidth:1100,margin:'0 auto'}}>{children}</main>
    <footer style={{marginTop:48,opacity:.6}}>© Demo</footer>
  </body></html>);
}
