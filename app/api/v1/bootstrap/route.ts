
import { NextRequest, NextResponse } from "next/server";
import { getOrCreateDevice } from "@/lib/config";

export async function GET(req: NextRequest) {
  if ((process.env.API_KEY || "") !== (req.headers.get("x-api-key") || ""))
    return NextResponse.json({ error: "Unauthorized" }, { status: 401 });

  const { searchParams } = new URL(req.url);
  const deviceId = searchParams.get("deviceId") || req.headers.get("x-device-id") || "";
  const storeCode = searchParams.get("store") || req.headers.get("x-store-code") || undefined;

  const conf = await getOrCreateDevice(deviceId || undefined, storeCode || undefined);
  const origin = process.env.PUBLIC_BASE_URL || new URL(req.url).origin;
  const checkoutUrl = `${origin}/p/${conf.terminalId}`;

  return NextResponse.json({ deviceId, storeCode: conf.storeCode, terminalId: conf.terminalId, amount: conf.amount, currency: conf.currency, checkoutUrl, autoEnrolled: !!(conf as any).created });
}
