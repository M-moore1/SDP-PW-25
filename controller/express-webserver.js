// Author: Krish Shah
// Express + WebSocket server for the controller frontend

import express from 'express';
import { createServer } from 'http';
import { WebSocketServer } from 'ws';

const PORT = process.env.PORT || 8080;

// ── Express setup ──────────────────────────────────────────────────────────
const app = express();
app.use(express.json());

// Health-check endpoint (handy for monitoring / load-balancers)
app.get('/health', (_req, res) => {
  res.json({ status: 'ok', uptime: process.uptime(), clients: connectedClients() });
});

// ── HTTP server ────────────────────────────────────────────────────────────
const server = createServer(app);

// ── WebSocket server (shares the same HTTP server) ─────────────────────────
const wss = new WebSocketServer({ server });

/** Return current number of connected WebSocket clients */
function connectedClients() {
  return wss.clients.size;
}

wss.on('connection', (ws, req) => {
  const clientIp = req.socket.remoteAddress;
  console.log(`✅ Client connected from ${clientIp}  (total: ${connectedClients()})`);

  ws.on('message', (raw) => {
    try {
      const data = JSON.parse(raw.toString());

      // ── Heartbeat ──────────────────────────────────────────────────────
      if (data.type === 'ping') {
        ws.send(JSON.stringify({ type: 'pong' }));
        console.log('Heartbeat ping/pong');
        return;
      }

      // ── Direction command ──────────────────────────────────────────────
      if (data.direction) {
        const direction = data.direction;
        console.log(
          `Direction: ${direction.toUpperCase().padEnd(8)} [${new Date().toLocaleTimeString()}]`
        );

        // Acknowledge back to the client
        ws.send(
          JSON.stringify({
            type: 'ack',
            received: direction,
            timestamp: Date.now(),
          })
        );
        return;
      }

      // ── Unknown structured message ─────────────────────────────────────
      console.log('Unknown message:', data);
    } catch {
      console.log('Raw (non-JSON) message:', raw.toString());
    }
  });

  ws.on('close', (code, reason) => {
    console.log(
      `❌ Client disconnected (code: ${code}, reason: ${reason || 'none'})  ` +
        `(remaining: ${connectedClients()})`
    );
  });

  ws.on('error', (err) => {
    console.error('⚠️  WebSocket error:', err.message);
  });
});

wss.on('error', (err) => {
  console.error('⚠️  Server error:', err.message);
});

// ── Handle HTTP-level errors (e.g. ECONNRESET during upgrade) ──────────────
server.on('error', (err) => {
  console.error('⚠️  HTTP server error:', err.message);
});

// Catch connection resets that happen before the WS handshake completes
server.on('clientError', (err, socket) => {
  if (err.code === 'ECONNRESET') {
    console.warn('⚠️  Client reset connection before handshake completed');
  } else {
    console.error('⚠️  Client error:', err.message);
  }
  if (!socket.destroyed) {
    socket.destroy();
  }
});

// ── Start listening ────────────────────────────────────────────────────────
server.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Express + WebSocket server running on http://0.0.0.0:${PORT}`);
  console.log(`   WebSocket endpoint: ws://localhost:${PORT}`);
  console.log('Waiting for client connections...\n');
});

// ── Graceful shutdown ──────────────────────────────────────────────────────
process.on('SIGINT', () => {
  console.log('\n\n🛑 Shutting down server...');

  // Close all WebSocket connections
  for (const client of wss.clients) {
    client.close();
  }

  wss.close(() => {
    server.close(() => {
      console.log('👋 Server closed');
      process.exit(0);
    });
  });
});
