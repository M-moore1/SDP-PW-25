# WebSocket Controller

A React + TypeScript single-page application that provides keyboard and touch-based directional controls via WebSocket.

## Features

- **Keyboard Controls**: Use arrow keys (↑ ↓ ← →) to send directional commands
- **Touch Controls**: On-screen button pad for mobile/touch devices
- **Hold-to-Repeat**: Commands send immediately, then every 1 second while held
- **Auto-Stop**: Automatically stops sending when key is released or window loses focus
- **Robust WebSocket**: Auto-reconnect with exponential backoff, message queuing, and heartbeat
- **Real-time Status**: View connection status and message log

## Tech Stack

- **React 18** with TypeScript
- **Vite** for fast development and building
- **Tailwind CSS** for styling
- **Custom Hooks** for WebSocket and hold-repeat logic

## Setup

1. **Install dependencies**:
   ```bash
   npm install
   ```

2. **Configure WebSocket URL** (optional):
   
   Copy `.env.example` to `.env` and update if needed:
   ```bash
   cp .env.example .env
   ```
   
   Default URL is `ws://localhost:8080`

3. **Run development server**:
   ```bash
   npm run dev
   ```
   
   The app will open at `http://localhost:3000`

4. **Build for production**:
   ```bash
   npm run build
   ```

## Usage

### Keyboard Controls
- **Arrow Up** (↑): Send `{"direction":"forward"}`
- **Arrow Down** (↓): Send `{"direction":"back"}`
- **Arrow Left** (←): Send `{"direction":"left"}`
- **Arrow Right** (→): Send `{"direction":"right"}`

Hold any arrow key to send the command immediately and then repeat every 1 second.

### On-Screen Controls
Click/tap and hold any of the four directional buttons for the same behavior as keyboard controls.

### Auto-Stop Behavior
- Commands stop when you release the key/button
- Switching tabs or losing window focus stops all active commands
- Returning to the tab doesn't auto-resume (must press again)

## WebSocket Protocol

### Outgoing Messages
```json
{
  "direction": "forward" | "back" | "left" | "right"
}
```

### Heartbeat
The client sends a ping every 25 seconds to keep the connection alive:
```json
{
  "type": "ping"
}
```

## Architecture

### Hooks
- **`useWebSocket`**: Manages WebSocket connection with auto-reconnect, message queue, and heartbeat
- **`useHoldRepeater`**: Implements hold-to-repeat behavior for keys and buttons

### Components
- **`DirectionPad`**: Touch-friendly on-screen button controls
- **`App`**: Main application with keyboard listeners and UI

### Utils
- **`direction.ts`**: Type definitions and direction mapping utilities

## Key Implementation Details

- **No Duplicate Intervals**: Prevents OS key-repeat from creating multiple send intervals
- **Message Queuing**: Messages queue if WebSocket isn't connected and send when it opens
- **Exponential Backoff**: Reconnects with increasing delays (1s → 30s max)
- **Visibility Handling**: Pauses heartbeat when tab is hidden, reconnects when visible
- **Memory Safety**: All intervals and listeners properly cleaned up on unmount

## Testing the WebSocket Connection

You can test with a simple WebSocket echo server:

```javascript
// test-server.js
const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', (ws) => {
  console.log('Client connected');
  
  ws.on('message', (message) => {
    console.log('Received:', message.toString());
    ws.send(JSON.stringify({ type: 'pong' }));
  });
  
  ws.on('close', () => console.log('Client disconnected'));
});

console.log('WebSocket server running on ws://localhost:8080');
```

Run with: `node test-server.js`

## License

MIT
