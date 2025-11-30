# Author Krish Shah
# Reviewed by: Sai Raparla

# Architecture Overview

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      WebSocket Controller                    │
│                         (React + TS)                         │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─────────────┐         ┌──────────────┐                   │
│  │   App.tsx   │────────▶│  useWebSocket│                   │
│  │             │         │     Hook     │                   │
│  │  - State    │         │              │                   │
│  │  - Keyboard │         │ • Connect    │                   │
│  │  - Events   │         │ • Send       │◀─── WebSocket     │
│  └──────┬──────┘         │ • Reconnect  │     Connection    │
│         │                │ • Heartbeat  │                   │
│         │                │ • Queue      │                   │
│         │                └──────────────┘                   │
│         │                                                    │
│         ├───────────▶┌──────────────────┐                   │
│         │            │ useHoldRepeater  │                   │
│         │            │                  │                   │
│         │            │ • Immediate send │                   │
│         │            │ • Interval loop  │                   │
│         │            │ • Cleanup        │                   │
│         │            └──────────────────┘                   │
│         │                                                    │
│         └───────────▶┌──────────────────┐                   │
│                      │   DirectionPad   │                   │
│                      │                  │                   │
│                      │ • Pointer events │                   │
│                      │ • Touch support  │                   │
│                      │ • Visual states  │                   │
│                      └──────────────────┘                   │
│                                                               │
│                      ┌──────────────────┐                   │
│                      │ SettingsDrawer   │                   │
│                      │                  │                   │
│                      │ • Repeat rate    │                   │
│                      │ • Presets        │                   │
│                      │ • Slider         │                   │
│                      └──────────────────┘                   │
└─────────────────────────────────────────────────────────────┘
                               │
                               │ WebSocket
                               │ Protocol
                               ▼
                    ┌─────────────────────┐
                    │   WebSocket Server  │
                    │   (Your Backend)    │
                    │                     │
                    │ • Receives commands │
                    │ • Controls device   │
                    └─────────────────────┘
```

## Component Hierarchy

```
App.tsx
├── SettingsDrawer.tsx
│   └── (Floating panel with rate controls)
│
├── Status Panel
│   ├── Connection Status
│   ├── Active Direction
│   ├── WebSocket URL
│   └── Error Display
│
├── DirectionPad.tsx
│   ├── Forward Button (↑)
│   ├── Left Button (←)
│   ├── Right Button (→)
│   └── Back Button (↓)
│
├── Message Log
│   └── Last 10 messages with timestamps
│
└── Instructions Panel
```

## Data Flow

### 1. User Input (Keyboard)

```
User presses ↑
    │
    ├─▶ handleKeyDown (App.tsx)
    │   • Check if already pressed (prevent OS repeat)
    │   • Map key to direction
    │   • Set activeDirection state
    │   • Set isKeyHeld = true
    │
    └─▶ useHoldRepeater hook activates
        • Sends message immediately
        • Starts interval (every repeatRate ms)
        • Continues until isKeyHeld = false
```

### 2. User Input (On-Screen Button)

```
User presses button
    │
    ├─▶ onPointerDown (DirectionPad.tsx)
    │   • Prevent default
    │   • Call onDirectionStart(direction)
    │
    ├─▶ handleDirectionStart (App.tsx)
    │   • Set activeDirection state
    │   • Set isKeyHeld = true
    │
    └─▶ useHoldRepeater hook activates
        • Same as keyboard flow
```

### 3. Message Sending

```
useHoldRepeater triggers sendDirection
    │
    ├─▶ buildDirectionMsg(direction)
    │   • Creates { direction: "forward" }
    │
    ├─▶ sendMessage (useWebSocket)
    │   │
    │   ├─▶ WebSocket OPEN?
    │   │   ├─ Yes ─▶ ws.send(JSON.stringify(msg))
    │   │   └─ No ──▶ Add to messageQueue
    │   │
    │   └─▶ Update messageLog state
    │       • Add timestamp
    │       • Keep last 10 messages
    │
    └─▶ Display in UI
        • Message Log updates
        • Timestamp shown
```

### 4. WebSocket Connection Management

```
useWebSocket initializes
    │
    ├─▶ connect()
    │   • Create WebSocket instance
    │   • Set status: 'connecting'
    │
    ├─▶ ws.onopen
    │   • Set status: 'connected'
    │   • Start heartbeat (25s interval)
    │   • Flush message queue
    │
    ├─▶ ws.onclose
    │   • Set status: 'reconnecting'
    │   • Stop heartbeat
    │   • Schedule reconnect (exponential backoff)
    │   • Delay: 1s → 2s → 4s → ... → 30s max
    │
    ├─▶ ws.onerror
    │   • Set lastError state
    │   • Log to console
    │
    └─▶ Visibility change
        • Page hidden: Pause heartbeat
        • Page visible: Resume/reconnect
```

## State Management

### App.tsx State

```typescript
{
  // From useWebSocket hook
  status: ConnectionStatus,     // 'connecting' | 'connected' | 'reconnecting' | 'disconnected'
  lastError: string | null,
  
  // Local state
  activeDirection: Direction | null,  // Current direction being sent
  messageLog: MessageLog[],           // Last 10 messages
  isKeyHeld: boolean,                 // Whether key/button is held
  repeatRate: number,                 // Milliseconds between sends (250-2000)
  
  // Ref (not state)
  pressedKeysRef: Set<string>         // Track pressed keys (prevent duplicates)
}
```

### useWebSocket Internal State

```typescript
{
  wsRef: WebSocket | null,
  reconnectTimeoutRef: number | null,
  heartbeatIntervalRef: number | null,
  reconnectDelayRef: number,
  messageQueueRef: unknown[],
  shouldConnectRef: boolean
}
```

## Event Handling

### Keyboard Events

```typescript
// Global window listeners
window.addEventListener('keydown', handleKeyDown)
window.addEventListener('keyup', handleKeyUp)
window.addEventListener('blur', handleBlur)

// Cleanup on unmount
return () => {
  window.removeEventListener('keydown', handleKeyDown)
  window.removeEventListener('keyup', handleKeyUp)
  window.removeEventListener('blur', handleBlur)
}
```

### Pointer Events (Buttons)

```typescript
// DirectionPad button handlers
onPointerDown={handlePointerDown('forward')}
onPointerUp={handlePointerUpOrLeave}
onPointerLeave={handlePointerUpOrLeave}

// Supports both mouse and touch
// Prevents default to avoid conflicts
```

### Visibility Events

```typescript
// Handled in useWebSocket
document.addEventListener('visibilitychange', handleVisibilityChange)

// When hidden: Pause heartbeat
// When visible: Resume/reconnect
```

## Message Protocol

### Outgoing Messages

#### Direction Command
```json
{
  "direction": "forward" | "back" | "left" | "right"
}
```

#### Heartbeat Ping
```json
{
  "type": "ping"
}
```

### Expected Incoming Messages (Optional)

#### Heartbeat Pong
```json
{
  "type": "pong"
}
```

#### Acknowledgment (Optional)
```json
{
  "type": "ack",
  "received": "forward",
  "timestamp": 1234567890
}
```

## Robustness Features

### 1. Auto-Reconnect
- Exponential backoff: 1s, 2s, 4s, 8s, 16s, 30s (max)
- Continues indefinitely until connected
- Resets delay on successful connection

### 2. Message Queue
- Messages queued when WebSocket not open
- Flushed immediately when connection opens
- Prevents message loss during reconnects

### 3. Heartbeat
- Sends ping every 25 seconds
- Keeps connection alive
- Server can respond with pong (optional)
- Pauses when page hidden

### 4. No Duplicate Intervals
- Tracks pressed keys in Set
- Ignores OS key-repeat events
- Only one interval per direction

### 5. Auto-Stop on Blur
- Stops sending when window loses focus
- Stops when switching tabs
- Clears all pressed keys
- Must manually resume (won't auto-restart)

### 6. Memory Safety
- All intervals cleared on unmount
- All event listeners removed on unmount
- WebSocket closed on unmount
- Refs used for non-reactive values

## Performance Considerations

### Optimization Techniques

1. **useCallback**: Memoize event handlers to prevent recreating functions
2. **useRef**: Store non-reactive values (WebSocket, intervals, pressed keys)
3. **Minimal re-renders**: State updates only when necessary
4. **Event delegation**: Global keyboard listeners instead of per-component
5. **Touch-action CSS**: Prevent scrolling on button hold

### Bundle Size

- React + React-DOM: ~140KB (gzipped)
- Tailwind CSS: ~10KB (purged, gzipped)
- Custom code: ~15KB (gzipped)
- **Total**: ~165KB gzipped

## Testing Strategy

### Manual Testing Checklist

- [ ] Press and hold arrow key → sends repeatedly
- [ ] Release arrow key → stops sending
- [ ] Switch tabs → stops sending
- [ ] Return to tab → doesn't auto-resume
- [ ] Press multiple keys → only last one active
- [ ] On-screen buttons work with mouse
- [ ] On-screen buttons work with touch
- [ ] Settings drawer opens/closes
- [ ] Adjust repeat rate → changes apply immediately
- [ ] WebSocket disconnects → shows reconnecting status
- [ ] WebSocket reconnects → shows connected status
- [ ] Message log shows last 10 messages
- [ ] Timestamps are accurate
- [ ] No memory leaks after prolonged use

### Automated Testing (Future)

```typescript
// Example with Testing Library + fake timers
test('sends message immediately and repeats every 1s', () => {
  jest.useFakeTimers()
  const { user } = render(<App />)
  
  user.keyboard('[ArrowUp>]')  // Hold key
  expect(mockSend).toHaveBeenCalledTimes(1)
  
  jest.advanceTimersByTime(1000)
  expect(mockSend).toHaveBeenCalledTimes(2)
  
  jest.advanceTimersByTime(1000)
  expect(mockSend).toHaveBeenCalledTimes(3)
  
  user.keyboard('[/ArrowUp]')  // Release key
  jest.advanceTimersByTime(1000)
  expect(mockSend).toHaveBeenCalledTimes(3)  // No more sends
})
```

## Deployment

### Development
```bash
npm run dev
```
- HMR enabled
- Source maps
- Port: 3000

### Production Build
```bash
npm run build
```
- Minified
- Tree-shaken
- Optimized assets
- Output: `dist/`

### Preview Production
```bash
npm run preview
```
- Serves production build
- Port: 4173

## Extension Ideas

1. **Multiple simultaneous directions**: Send combinations like "forward+left"
2. **Speed control**: Add slider for speed parameter in message
3. **Recording/Playback**: Record a sequence and replay it
4. **Gamepad support**: Use Gamepad API for joystick control
5. **Voice control**: Add speech recognition for hands-free operation
6. **Virtual joystick**: Touch-based analog stick instead of D-pad
7. **Macro buttons**: Programmable buttons for complex command sequences
8. **Authentication**: Add JWT token support for secure connections
9. **Multiple devices**: Switch between controlling different devices
10. **Command history**: Save and replay previous command sequences

