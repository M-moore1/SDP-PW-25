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

const PORT = Number(process.env.PORT || 8080);
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

// Map {direction:"w/a/s/d"} into your C-bridge Control message schema.
function directionToC(dir, maybeSpeed) {
  const d = String(dir || "").toLowerCase();

  const speed = ALLOW_SPEED_OVERRIDE
    ? clampInt(maybeSpeed, 0, 100, DEFAULT_SPEED)
    : DEFAULT_SPEED;

  return {
    type: "C",
    forward: d === "w" ? 1 : 0,
    backward: d === "s" ? 1 : 0,
    left: d === "a" ? 1 : 0,
    right: d === "d" ? 1 : 0,
    speed,
    priority_level: DEFAULT_PRIORITY,
  };
}

function isBridgeType(t) {
  return t === "C" || t === "P" || t === "S" || t === "Q";
}

wss.on("connection", (ws, req) => {
  const clientIp = req.socket.remoteAddress;
  console.log(`✅ WS client connected from ${clientIp} (total: ${wss.clients.size})`);

  ws.send(JSON.stringify({ type: "hello", ts: Date.now() }));

  ws.on("message", (raw) => {
    let data;
    try {
      data = JSON.parse(raw.toString());
    } catch {
      ws.send(JSON.stringify({ type: "ERR", msg: "bad json", ts: Date.now() }));
      return;
    }

    // Heartbeat
    if (data.type === "ping") {
      ws.send(JSON.stringify({ type: "pong", ts: Date.now() }));
      return;
    }

    // If UI sends direction key, translate to Control (C)
    if (data.direction) {
      const cmd = directionToC(data.direction, data.speed);

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

    // If UI sends a direct bridge-compatible message, forward it as-is
    if (isBridgeType(data.type)) {
      // Optional: light validation / clamping for common fields
      if (data.type === "C") {
        data.forward = clampInt(data.forward, 0, 1, 0);
        data.backward = clampInt(data.backward, 0, 1, 0);
        data.left = clampInt(data.left, 0, 1, 0);
        data.right = clampInt(data.right, 0, 1, 0);
        data.speed = clampInt(data.speed, 0, 100, DEFAULT_SPEED);
        data.priority_level = clampInt(data.priority_level, 0, 3, DEFAULT_PRIORITY);
      }
      console.log("WS->UDS sending:", data);
      const ok = udsSendJson(data);
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
