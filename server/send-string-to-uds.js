#!/usr/bin/env node
// -----------------------------------------------------------------------------
// Sends a raw string to /tmp/gs_bridge.sock using the same framing as gs_bridge.c:
//   [4-byte big-endian length] + [payload bytes]
//
// Usage:
//   node send-string-to-uds.js wnsijcfwed
//   node send-string-to-uds.js "any string here"
//
// Or with SOCKET_PATH env:
//   SOCKET_PATH=/tmp/gs_bridge.sock node send-string-to-uds.js wnsijcfwed
// -----------------------------------------------------------------------------

import net from "net";

const SOCKET_PATH = process.env.SOCKET_PATH || "/tmp/gs_bridge.sock";

const input = process.argv.slice(2).join(" ") || "wnsijcfwed";

const payload = Buffer.from(input, "utf8");
if (payload.length === 0) {
  console.error("Empty string, nothing to send");
  process.exit(1);
}
if (payload.length > 1024 * 1024) {
  console.error("Payload too large (max 1MB)");
  process.exit(1);
}

const lenBuf = Buffer.alloc(4);
lenBuf.writeUInt32BE(payload.length, 0);

const socket = net.createConnection(SOCKET_PATH);

socket.on("connect", () => {
  socket.write(Buffer.concat([lenBuf, payload]));
  console.log(`Sent ${payload.length} bytes to ${SOCKET_PATH}: "${input}"`);
  socket.end();
});

socket.on("error", (err) => {
  console.error("Error:", err.message);
  process.exit(1);
});

socket.on("close", (hadError) => {
  process.exit(hadError ? 1 : 0);
});
