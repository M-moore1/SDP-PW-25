//Author: Sai Raparla 
//Reviewed by: Krish Shah
import { WebSocketServer } from 'ws';

const PORT = 8080;
const wss = new WebSocketServer({ port: PORT });

console.log(`🚀 WebSocket server running on ws://localhost:${PORT}`);
console.log('Waiting for client connections...\n');

wss.on('connection', (ws, req) => {
  const clientIp = req.socket.remoteAddress;
  console.log(`✅ Client connected from ${clientIp}`);

  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message.toString());

      if (data.type === 'ping') {
        ws.send(JSON.stringify({ type: 'pong' }));
        console.log('Heartbeat ping/pong');
      } else if (data.direction) {
        console.log(`Direction: ${data.direction.toUpperCase().padEnd(8)} [${new Date().toLocaleTimeString()}]`);
        ws.send(JSON.stringify({ type: 'ack', received: data.direction, timestamp: Date.now() }));
      } else {
        console.log('Message:', data);
      }
    } catch {
      console.log('Raw message:', message.toString());
    }
  });

  ws.on('close', (code, reason) => {
    console.log(`❌ Client disconnected (code: ${code}, reason: ${reason || 'none'})\n`);
  });

  ws.on('error', (err) => console.error('⚠️  WebSocket error:', err.message));
});

wss.on('error', (err) => console.error('⚠️  Server error:', err.message));

process.on('SIGINT', () => {
  console.log('\n\n🛑 Shutting down server...');
  wss.close(() => {
    console.log('👋 Server closed');
    process.exit(0);
  });
});
