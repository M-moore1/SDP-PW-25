// server-c.js
// -----------------------------------------------------------------------------
// WebSocket + HTTP server that bridges:
//   Browser/UI (WS JSON)  <->  Node.js  <->  Unix Domain Socket (len-prefixed JSON)  <->  gs_bridge.c
//
// Matches gs_bridge.c UDS framing exactly:
//   [4-byte big-endian length] + [JSON bytes]
//
// Also forwards robot reports (SR/HR/A/HPR, etc.) coming back from C to all WS clients.
// -----------------------------------------------------------------------------

import express from "express";
import { createServer } from "http";
import { WebSocketServer } from "ws";
import net from "net";

// ------------------------- Config -------------------------

const PORT = Number(process.env.PORT || 3001);
const BIND = process.env.BIND || "0.0.0.0";
const SOCKET_PATH = process.env.SOCKET_PATH || "/tmp/gs_bridge.sock";

// If your UI sends just "direction": "w/a/s/d", we map it to a Control (C) command:
const DEFAULT_SPEED = Number(process.env.DEFAULT_SPEED || 50);          // 0..100
const DEFAULT_PRIORITY = Number(process.env.DEFAULT_PRIORITY || 0);     // 0..3

// Optional: if UI sends "speed" in direction messages, allow it:
const ALLOW_SPEED_OVERRIDE = (process.env.ALLOW_SPEED_OVERRIDE || "1") !== "0";

// ------------------------- Express -------------------------

const app = express();
app.use(express.json({ limit: "1mb" }));

// Health-check endpoint
app.get("/health", (_req, res) => {
  res.json({
    status: "ok",
    uptime_s: process.uptime(),
    wsClients: wss.clients.size,
    uds: {
      path: SOCKET_PATH,
      connected: Boolean(cSocket && !cSocket.destroyed),
    },
  });
});

// ------------------------- HTTP + WS -------------------------

const server = createServer(app);
const wss = new WebSocketServer({ server });

// Broadcast helper
function wsBroadcast(obj) {
  const s = JSON.stringify(obj);
  for (const client of wss.clients) {
    if (client.readyState === 1) client.send(s);
  }
}

// ------------------------- UDS (Node <-> C) -------------------------

let cSocket = null;

// RX buffer for length-prefixed frames from C
let udsRxBuf = Buffer.alloc(0);

function udsSendJson(obj) {
  if (!cSocket || cSocket.destroyed) return false;

  const jsonBuf = Buffer.from(JSON.stringify(obj), "utf8");
  if (jsonBuf.length <= 0 || jsonBuf.length > 1024 * 1024) {
    console.warn("⚠️  Refusing to send UDS JSON of invalid size:", jsonBuf.length);
    return false;
  }

  const lenBuf = Buffer.alloc(4);
  lenBuf.writeUInt32BE(jsonBuf.length, 0);

  cSocket.write(Buffer.concat([lenBuf, jsonBuf]));
  return true;
}

function udsSendRaw(str) {
  if (!cSocket || cSocket.destroyed) return false;

  const buf = Buffer.from(String(str), "utf8");
  console.log("udsSendRaw: sending", buf.length, "bytes:", str.slice(0, 32), "...");  // add this

  if (buf.length <= 0 || buf.length > 1024 * 1024) {
    console.warn("⚠️  Refusing to send UDS raw of invalid size:", buf.length);
    return false;
  }

  const lenBuf = Buffer.alloc(4);
  lenBuf.writeUInt32BE(buf.length, 0);

  cSocket.write(Buffer.concat([lenBuf, buf]));
  return true;
}

function udsProcessIncoming() {
  // Need at least 4 bytes for length
  while (udsRxBuf.length >= 4) {
    const len = udsRxBuf.readUInt32BE(0);

    // Mirror gs_bridge.c sanity check
    if (len === 0 || len > 1024 * 1024) {
      console.error("⚠️  Bad UDS frame length from C:", len, "-> closing socket");
      cSocket.destroy();
      return;
    }

    if (udsRxBuf.length < 4 + len) return; // wait for full frame

    const payload = udsRxBuf.slice(4, 4 + len);
    udsRxBuf = udsRxBuf.slice(4 + len);

    try {
      const msg = JSON.parse(payload.toString("utf8"));

      // Forward to all WS clients
      wsBroadcast(msg);
    } catch (e) {
      console.warn("⚠️  Failed to parse JSON from C:", e?.message || e);
    }
  }
}

function connectToC() {
  cSocket = net.createConnection(SOCKET_PATH);

  cSocket.on("connect", () => {
    console.log("🧠 Connected to C bridge via UDS:", SOCKET_PATH);
    udsRxBuf = Buffer.alloc(0);

    // Optional: notify WS clients that backend is live
    wsBroadcast({ type: "INFO", msg: "Connected to C bridge", ts: Date.now() });
  });

  cSocket.on("data", (chunk) => {
    udsRxBuf = Buffer.concat([udsRxBuf, chunk]);
    udsProcessIncoming();
  });

  cSocket.on("error", (err) => {
    console.error("⚠️  UDS error:", err.message);
  });

  cSocket.on("close", () => {
    console.warn("⚠️  UDS closed — retrying in 1s");
    wsBroadcast({ type: "INFO", msg: "Disconnected from C bridge", ts: Date.now() });

    setTimeout(connectToC, 1000);
  });
}

connectToC();

// ------------------------- WS Handling -------------------------

function clampInt(v, lo, hi, fallback) {
  const n = Number(v);
  if (!Number.isFinite(n)) return fallback;
  const i = Math.trunc(n);
  return Math.max(lo, Math.min(hi, i));
}

// Map {direction:"w/a/s/d"} into concise C-bridge Control message schema.
function directionToC(dir, maybeSpeed, maybeId) {
  const d = String(dir || "").toLowerCase();

  const speed = ALLOW_SPEED_OVERRIDE
    ? clampInt(maybeSpeed, 0, 100, DEFAULT_SPEED)
    : DEFAULT_SPEED;

  return {
    T: "C",
    F: d === "w" ? 1 : 0,
    B: d === "s" ? 1 : 0,
    L: d === "a" ? 1 : 0,
    R: d === "d" ? 1 : 0,
    S: speed,
    PL: DEFAULT_PRIORITY,
    ID: clampInt(maybeId, 0, 255, 1),
  };
}

function isBridgeType(t) {
  return t === "C" || t === "P" || t === "S" || t === "Q" || t === "raw";
}

/** Normalize Control (C) to compact UDS form; accepts legacy { type, forward, ... } or { T, F, ... }. */
function compactControlMessage(data) {
  if (data.T !== "C" && data.type !== "C") return null;
  const compact = data.T === "C";
  return {
    T: "C",
    F: clampInt(compact ? data.F : data.forward, 0, 1, 0),
    B: clampInt(compact ? data.B : data.backward, 0, 1, 0),
    L: clampInt(compact ? data.L : data.left, 0, 1, 0),
    R: clampInt(compact ? data.R : data.right, 0, 1, 0),
    S: clampInt(compact ? data.S : data.speed, 0, 100, DEFAULT_SPEED),
    PL: clampInt(compact ? data.PL : data.priority_level, 0, 3, DEFAULT_PRIORITY),
    ID: clampInt(compact ? data.ID : (data.id ?? data.ID), 0, 255, 1),
  };
}

/** Build the object to send on UDS from a parsed WS JSON message. */
function wsPayloadToUds(data) {
  const c = compactControlMessage(data);
  if (c) return c;

  const t = data.T ?? data.type;
  if (!isBridgeType(t)) return null;

  // Legacy bridge messages: use T instead of type for the C side
  if (data.type !== undefined && data.T === undefined) {
    const { type, ...rest } = data;
    return { T: type, ...rest };
  }

  return data;
}

wss.on("connection", (ws, req) => {
  const clientIp = req.socket.remoteAddress;
  console.log(`✅ WS client connected from ${clientIp} (total: ${wss.clients.size})`);

  ws.send(JSON.stringify({ type: "hello", ts: Date.now() }));

  ws.on("message", (raw) => {
    const rawStr = raw.toString();
    let data;
    try {
      data = JSON.parse(rawStr);
    } catch {
      // Plain string (invalid JSON) - send to UDS as raw
      const ok = udsSendRaw(rawStr);
      ws.send(JSON.stringify({ type: ok ? "ack" : "ERR", msg: ok ? "sent" : "C bridge not connected", ts: Date.now() }));
      return;
    }

    // If parsed result is a string (e.g. JSON "wnsijcfwed"), send to UDS
    if (typeof data === "string") {
      const ok = udsSendRaw(data);
      ws.send(JSON.stringify({ type: ok ? "ack" : "ERR", msg: ok ? "sent" : "C bridge not connected", ts: Date.now() }));
      return;
    }

    // Heartbeat
    if (data.type === "ping") {
      ws.send(JSON.stringify({ type: "pong", ts: Date.now() }));
      return;
    }

    // If UI sends direction key, translate to Control (C)
    if (data.direction) {
      const cmd = directionToC(data.direction, data.speed, data.id);

      const ok = udsSendJson(cmd);
      ws.send(
        JSON.stringify({
          type: ok ? "ack" : "ERR",
          msg: ok ? "sent" : "C bridge not connected",
          sent: cmd,
          ts: Date.now(),
        })
      );
      return;
    }

    // Direct bridge messages: Control (C) as compact { T,F,B,L,R,S,PL,ID }; other types get T alias if legacy
    const udsPayload = wsPayloadToUds(data);
    if (udsPayload) {
      console.log("WS->UDS sending:", udsPayload);
      const ok = udsSendJson(udsPayload);
      ws.send(JSON.stringify({ type: ok ? "ack" : "ERR", msg: ok ? "sent" : "C bridge not connected", ts: Date.now() }));
      return;
    }

    ws.send(JSON.stringify({ type: "ERR", msg: "unknown message format", got: data, ts: Date.now() }));
  });

  ws.on("close", () => {
    console.log(`❌ WS client disconnected (remaining: ${wss.clients.size})`);
  });

  ws.on("error", (err) => {
    console.error("⚠️  WS error:", err.message);
  });
});

// ------------------------- Start server -------------------------

server.listen(PORT, BIND, () => {
  console.log(`🚀 HTTP server: http://${BIND}:${PORT}`);
  console.log(`🔌 WS endpoint: ws://${BIND}:${PORT}`);
  console.log(`🧠 UDS path:    ${SOCKET_PATH}`);
  console.log("Waiting for connections...\n");
});

// ------------------------- Graceful shutdown -------------------------

process.on("SIGINT", () => {
  console.log("\n🛑 Shutting down...");

  for (const client of wss.clients) {
    try { client.close(); } catch {}
  }

  if (cSocket && !cSocket.destroyed) {
    try { cSocket.end(); } catch {}
  }

  wss.close(() => {
    server.close(() => {
      console.log("👋 Server closed");
      process.exit(0);
    });
  });
});