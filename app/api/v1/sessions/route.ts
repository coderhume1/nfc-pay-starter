import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { resolveForDevice, defaults } from "@/lib/config";

export async function POST(req: NextRequest){
  if ((process.env.API_KEY||"") !== (req.headers.get("x-api-key")||""))
    return NextResponse.json({error:"Unauthorized"},{status:401});

  const body = await req.json().catch(()=>null) as any;
  if(!body) return NextResponse.json({error:"Invalid JSON"},{status:400});

  // terminalId may come from body OR from the device’s mapping
  let terminalId: string = String(body.terminalId || "");
  const deviceId = req.headers.get("x-device-id") || "";

  const conf = resolveForDevice(deviceId || undefined);
  if(!terminalId) terminalId = conf.terminalId || defaults().terminalId;

  const amount   = Number.isFinite(Number(body.amount)) ? Number(body.amount) : conf.amount;
  const currency = typeof body.currency === "string" && body.currency ? body.currency : conf.currency;

  if(!terminalId || !Number.isFinite(amount) || !currency)
    return NextResponse.json({error:"Missing/invalid fields after resolution"},{status:400});

  const session = await prisma.session.create({ data:{ terminalId, amount, currency, status:"pending" }});
  return NextResponse.json(session,{status:201});
}
