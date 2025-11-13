# Quick Setup Guide

## Prerequisites

- Node.js (v16 or higher)
- npm or yarn

## Installation Steps

### 1. Install Frontend Dependencies

```bash
cd /Users/krishshah/Desktop/Code/SDP/WebApp/controller
npm install
```

### 2. Configure WebSocket URL

The default WebSocket URL is `ws://localhost:8080`. If you need to change it:

Create a `.env` file in the controller directory:
```bash
# Copy the example file
cp .env.example .env

# Edit .env to set your WebSocket URL
# VITE_WS_URL=ws://your-server-address:port
```

### 3. Start the Development Server

```bash
npm run dev
```

The app will open automatically at `http://localhost:3000`

## Testing with the WebSocket Server

To test the controller, you can use the included test server:

### 1. Install Server Dependencies

```bash
# In the controller directory
npm install ws
```

### 2. Run the Test Server

In a separate terminal:

```bash
node test-server.js
```

You should see:
```
🚀 WebSocket server running on ws://localhost:8080
Waiting for client connections...
```

### 3. Use the Controller

Now you can:
- Press and hold arrow keys to send direction commands
- Use the on-screen buttons for touch control
- Watch the server terminal to see received messages
- Check the message log in the web UI

## Building for Production

```bash
npm run build
```

The production build will be in the `dist/` directory.

To preview the production build:
```bash
npm run preview
```

## Troubleshooting

### WebSocket won't connect

1. Make sure the server is running on the correct port
2. Check that your `.env` has the correct `VITE_WS_URL`
3. Look for browser console errors (F12)
4. Verify firewall settings aren't blocking WebSocket connections

### Keys not responding

1. Make sure the browser window has focus
2. Check browser console for errors
3. Try clicking on the page first to ensure focus
4. Try the on-screen buttons as an alternative

### Build errors

1. Delete `node_modules` and reinstall:
   ```bash
   rm -rf node_modules
   npm install
   ```

2. Clear the cache:
   ```bash
   rm -rf node_modules/.vite
   ```

## Project Structure

```
controller/
├── src/
│   ├── hooks/
│   │   ├── useWebSocket.ts      # WebSocket connection management
│   │   └── useHoldRepeater.ts   # Hold-to-repeat logic
│   ├── components/
│   │   ├── DirectionPad.tsx     # On-screen controls
│   │   └── SettingsDrawer.tsx   # Settings panel
│   ├── utils/
│   │   └── direction.ts         # Direction types and helpers
│   ├── App.tsx                  # Main application
│   ├── main.tsx                 # Entry point
│   └── index.css                # Tailwind styles
├── test-server.js               # WebSocket test server
├── package.json                 # Dependencies
├── vite.config.ts              # Vite configuration
├── tailwind.config.js          # Tailwind configuration
└── tsconfig.json               # TypeScript configuration
```

## Features Overview

### Keyboard Controls
- **↑** (Arrow Up) → `{"direction":"forward"}`
- **↓** (Arrow Down) → `{"direction":"back"}`
- **←** (Arrow Left) → `{"direction":"left"}`
- **→** (Arrow Right) → `{"direction":"right"}`

### On-Screen Controls
Four arrow buttons that work with mouse and touch input.

### Auto-Repeat
- Sends command immediately on press/click
- Repeats at the configured rate while held
- Default: every 1000ms (1 second)
- Adjustable: 250ms to 2000ms via settings

### Auto-Stop
- Stops sending when key/button is released
- Stops when window loses focus
- Stops when switching tabs
- Does not auto-resume (must press again)

### WebSocket Features
- **Auto-reconnect** with exponential backoff (1s → 30s max)
- **Message queuing** when disconnected
- **Heartbeat ping** every 25s to keep connection alive
- **Connection status** display (Connecting/Connected/Reconnecting/Disconnected)

### Settings
- Click the ⚙️ button (bottom right) to open settings
- Adjust repeat rate with slider or presets
- Changes apply immediately

### Message Log
- Shows last 10 messages sent
- Displays timestamp for each message
- Scrollable list

## Next Steps

1. Connect to your robot/device's WebSocket server
2. Customize the UI styling in `src/App.tsx` and `src/components/`
3. Extend message protocol in `src/utils/direction.ts`
4. Add authentication if needed in `src/hooks/useWebSocket.ts`
5. Deploy to production server

Enjoy controlling your device! 🎮

