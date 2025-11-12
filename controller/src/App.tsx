import { useEffect, useState, useCallback, useRef } from 'react';
import { useWebSocket } from './hooks/useWebSocket';
import { useHoldRepeater } from './hooks/useHoldRepeater';
import { DirectionPad } from './components/DirectionPad';
import { SettingsDrawer } from './components/SettingsDrawer';
import {
  Direction,
  buildDirectionMsg,
  formatDirection,
  isDirectionKey,
  keyToDirection,
} from './utils/direction';

interface MessageLog {
  payload: string;
  timestamp: string;
}

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080';

function App() {
  const { status, sendMessage, lastError } = useWebSocket(WS_URL);
  const [activeDirection, setActiveDirection] = useState<Direction | null>(null);
  const [messageLog, setMessageLog] = useState<MessageLog[]>([]);
  const [isKeyHeld, setIsKeyHeld] = useState(false);
  const [repeatRate, setRepeatRate] = useState(1000); // Default 1000ms
  
  // Track which keys are currently pressed to prevent repeat keydown events
  const pressedKeysRef = useRef<Set<string>>(new Set());

  // Send direction message
  const sendDirection = useCallback((direction: Direction) => {
    const msg = buildDirectionMsg(direction);
    sendMessage(msg);
    
    // Log the message
    const log: MessageLog = {
      payload: JSON.stringify(msg),
      timestamp: new Date().toLocaleTimeString(),
    };
    
    setMessageLog((prev) => [log, ...prev].slice(0, 10)); // Keep last 10
  }, [sendMessage]);

  // Use hold repeater for keyboard with adjustable rate
  useHoldRepeater(
    isKeyHeld && activeDirection !== null,
    () => {
      if (activeDirection) {
        sendDirection(activeDirection);
      }
    },
    repeatRate
  );

  // Keyboard event handlers
  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    // Ignore if key already pressed (prevent OS key repeat)
    if (pressedKeysRef.current.has(e.key)) return;

    if (isDirectionKey(e.key)) {
      e.preventDefault();
      pressedKeysRef.current.add(e.key);
      
      const direction = keyToDirection[e.key];
      setActiveDirection(direction);
      setIsKeyHeld(true);
    }
  }, []);

  const handleKeyUp = useCallback((e: KeyboardEvent) => {
    if (isDirectionKey(e.key)) {
      e.preventDefault();
      pressedKeysRef.current.delete(e.key);
      
      // Only clear if this was the active key
      const direction = keyToDirection[e.key];
      if (direction === activeDirection) {
        setActiveDirection(null);
        setIsKeyHeld(false);
      }
    }
  }, [activeDirection]);

  // Handle window blur (stop sending when window loses focus)
  const handleBlur = useCallback(() => {
    setActiveDirection(null);
    setIsKeyHeld(false);
    pressedKeysRef.current.clear();
  }, []);

  // Prevent arrow key scrolling globally
  useEffect(() => {
    const preventArrowScroll = (e: KeyboardEvent) => {
      if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].includes(e.key)) {
        e.preventDefault();
      }
    };

    // Add listener with capture phase to prevent default before other handlers
    window.addEventListener('keydown', preventArrowScroll, { capture: true });

    return () => {
      window.removeEventListener('keydown', preventArrowScroll, { capture: true });
    };
  }, []);

  // Set up keyboard listeners and blur handler
  useEffect(() => {
    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    window.addEventListener('blur', handleBlur);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
      window.removeEventListener('blur', handleBlur);
    };
  }, [handleKeyDown, handleKeyUp, handleBlur]);

  // On-screen button handlers
  const handleDirectionStart = useCallback((direction: Direction) => {
    setActiveDirection(direction);
    setIsKeyHeld(true);
  }, []);

  const handleDirectionStop = useCallback(() => {
    setActiveDirection(null);
    setIsKeyHeld(false);
  }, []);

  // Connection status styling
  const getStatusColor = () => {
    switch (status) {
      case 'connected':
        return 'text-green-600 bg-green-100';
      case 'connecting':
        return 'text-yellow-600 bg-yellow-100';
      case 'reconnecting':
        return 'text-orange-600 bg-orange-100';
      case 'disconnected':
        return 'text-red-600 bg-red-100';
    }
  };

  const getStatusText = () => {
    switch (status) {
      case 'connected':
        return '● Connected';
      case 'connecting':
        return '⟳ Connecting…';
      case 'reconnecting':
        return '⟳ Reconnecting…';
      case 'disconnected':
        return '○ Disconnected';
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-8">
      <div className="max-w-4xl mx-auto">
        {/* Header */}
        <header className="text-center mb-8">
          <h1 className="text-4xl font-bold text-gray-800 mb-2">
            WebSocket Controller
          </h1>
          <p className="text-gray-600">
            Use arrow keys or on-screen buttons to control direction
          </p>
        </header>

        {/* Main content grid */}
        <div className="grid gap-6 md:grid-cols-2">
          {/* Status Panel */}
          <div className="bg-white rounded-xl shadow-lg p-6">
            <h2 className="text-xl font-semibold text-gray-700 mb-4">Status</h2>
            
            {/* Connection Status */}
            <div className="mb-4">
              <span className={`inline-flex items-center px-3 py-1 rounded-full text-sm font-medium ${getStatusColor()}`}>
                {getStatusText()}
              </span>
            </div>

            {/* Active Direction */}
            <div className="mb-4">
              <h3 className="text-sm font-medium text-gray-500 mb-1">Active Direction</h3>
              <p className="text-2xl font-bold text-gray-800">
                {formatDirection(activeDirection)}
              </p>
            </div>

            {/* WebSocket URL */}
            <div className="mb-4">
              <h3 className="text-sm font-medium text-gray-500 mb-1">WebSocket URL</h3>
              <p className="text-sm text-gray-700 font-mono bg-gray-100 px-2 py-1 rounded">
                {WS_URL}
              </p>
            </div>

            {/* Last Error */}
            {lastError && (
              <div className="bg-red-50 border border-red-200 rounded-lg p-3">
                <h3 className="text-sm font-medium text-red-700 mb-1">Error</h3>
                <p className="text-sm text-red-600">{lastError}</p>
              </div>
            )}
          </div>

          {/* Direction Pad */}
          <div className="flex items-center justify-center">
            <DirectionPad
              onDirectionStart={handleDirectionStart}
              onDirectionStop={handleDirectionStop}
              activeDirection={activeDirection}
            />
          </div>
        </div>

        {/* Message Log */}
        <div className="mt-6 bg-white rounded-xl shadow-lg p-6">
          <h2 className="text-xl font-semibold text-gray-700 mb-4">Message Log</h2>
          
          {messageLog.length === 0 ? (
            <p className="text-gray-400 text-center py-4">No messages sent yet</p>
          ) : (
            <div className="space-y-2 max-h-64 overflow-y-auto">
              {messageLog.map((log, idx) => (
                <div
                  key={idx}
                  className="flex items-center justify-between bg-gray-50 rounded-lg px-4 py-2 border border-gray-200"
                >
                  <code className="text-sm text-gray-700 font-mono">
                    {log.payload}
                  </code>
                  <span className="text-xs text-gray-500 ml-4">
                    {log.timestamp}
                  </span>
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Instructions */}
        <div className="mt-6 bg-blue-50 border border-blue-200 rounded-xl p-6">
          <h2 className="text-lg font-semibold text-blue-900 mb-2">How to Use</h2>
          <ul className="text-sm text-blue-800 space-y-1">
            <li>• <strong>Arrow Keys:</strong> Press and hold ↑ ↓ ← → to send direction commands</li>
            <li>• <strong>On-Screen Buttons:</strong> Click/tap and hold the arrow buttons</li>
            <li>• <strong>Auto-Repeat:</strong> Commands send immediately, then repeat at the set rate ({repeatRate}ms)</li>
            <li>• <strong>Auto-Stop:</strong> Releasing the key/button or switching tabs stops sending</li>
            <li>• <strong>Settings:</strong> Click the ⚙️ button to adjust repeat rate</li>
          </ul>
        </div>
      </div>

      {/* Settings Drawer */}
      <SettingsDrawer
        repeatRate={repeatRate}
        onRepeatRateChange={setRepeatRate}
      />
    </div>
  );
}

export default App;

