import { NextRequest, NextResponse } from "next/server";
import { resolveForDevice } from "@/lib/config";

export async function GET(req: NextRequest) {
  const api = req.headers.get("x-api-key") || "";
  if ((process.env.API_KEY || "") !== api) return NextResponse.json({ error: "Unauthorized" }, { status: 401 });

  const { searchParams } = new URL(req.url);
  const deviceId = searchParams.get("deviceId") || req.headers.get("x-device-id") || "";
  const conf = resolveForDevice(deviceId || undefined);

  const origin = process.env.PUBLIC_BASE_URL || new URL(req.url).origin;
  const checkoutUrl = `${origin}/p/${conf.terminalId}`;

  return NextResponse.json({ deviceId, ...conf, checkoutUrl });
}
