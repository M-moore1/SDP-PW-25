//Author: Krish Shah
//Reviewed by: Sai Raparla
import { useEffect, useState, useCallback, useRef } from 'react';
import { useWebSocket } from './hooks/useWebSocket';
import { useHoldRepeater } from './hooks/useHoldRepeater';
import { DirectionPad } from './components/DirectionPad';
import { SettingsDrawer } from './components/SettingsDrawer';
import { CommandPanel } from './components/CommandPanel';
import { StatusIndicator } from './components/StatusIndicator';
import { MessageLog } from './components/MessageLog';
import {
  Direction,
  formatDirection,
  isDirectionKey,
  keyToDirection,
  directionToControlAxes,
} from './utils/direction';
import { buildControlMsg } from './utils/commands';

interface MessageLogEntry {
  payload: string;
  timestamp: string;
}

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080';
const ENCRYPTION_KEY = import.meta.env.VITE_ENCRYPTION_KEY; // 64 hex chars (32 bytes)

function App() {
  const [activeDirection, setActiveDirection] = useState<Direction | null>(null);
  const [messageLog, setMessageLog] = useState<MessageLogEntry[]>([]);
  const [isKeyHeld, setIsKeyHeld] = useState(false);
  const [repeatRate, setRepeatRate] = useState(50);
  const [controlSpeed, setControlSpeed] = useState(50);
  const [encryptionEnabled, setEncryptionEnabled] = useState(false);

  const pressedKeysRef = useRef<Set<string>>(new Set());

  const logMessage = useCallback((payload: string) => {
    setMessageLog((prev) => [
      { payload, timestamp: new Date().toLocaleTimeString() },
      ...prev,
    ].slice(0, 10));
  }, []);

  const { status, sendMessage, lastError } = useWebSocket(WS_URL, {
    encryptionKey: encryptionEnabled && ENCRYPTION_KEY ? ENCRYPTION_KEY : undefined,
    onMessageSent: logMessage,
  });

  const sendDirection = useCallback((direction: Direction) => {
    const axes = directionToControlAxes(direction);
    const msg = buildControlMsg(
      axes.forward,
      axes.backward,
      axes.left,
      axes.right,
      controlSpeed
    );
    sendMessage(msg);
  }, [sendMessage, controlSpeed]);

  useHoldRepeater(
    isKeyHeld && activeDirection !== null,
    () => {
      if (activeDirection) sendDirection(activeDirection);
    },
    repeatRate
  );

  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    if (pressedKeysRef.current.has(e.key)) return;
    if (isDirectionKey(e.key)) {
      e.preventDefault();
      pressedKeysRef.current.add(e.key);
      setActiveDirection(keyToDirection[e.key]);
      setIsKeyHeld(true);
    }
  }, []);

  const handleKeyUp = useCallback((e: KeyboardEvent) => {
    if (isDirectionKey(e.key)) {
      e.preventDefault();
      pressedKeysRef.current.delete(e.key);
      const direction = keyToDirection[e.key];
      if (direction === activeDirection) {
        setActiveDirection(null);
        setIsKeyHeld(false);
      }
    }
  }, [activeDirection]);

  const handleBlur = useCallback(() => {
    setActiveDirection(null);
    setIsKeyHeld(false);
    pressedKeysRef.current.clear();
  }, []);

  useEffect(() => {
    const preventScroll = (e: KeyboardEvent) => {
      if (['w', 'a', 's', 'd', ' '].includes(e.key.toLowerCase())) {
        e.preventDefault();
      }
    };
    window.addEventListener('keydown', preventScroll, { capture: true });
    return () => window.removeEventListener('keydown', preventScroll, { capture: true });
  }, []);

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

  const handleDirectionStart = useCallback((direction: Direction) => {
    setActiveDirection(direction);
    setIsKeyHeld(true);
  }, []);

  const handleDirectionStop = useCallback(() => {
    setActiveDirection(null);
    setIsKeyHeld(false);
  }, []);

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-8">
      <div className="max-w-5xl mx-auto">
        <header className="text-center mb-6">
          <h1 className="text-4xl font-bold text-gray-800 mb-2">Robot Controller</h1>
          <p className="text-gray-600">
            Use WASD keys or on-screen buttons to control direction
          </p>
        </header>

        <div className="grid gap-6 md:grid-cols-2 items-start">
          {/* Left: Direction Pad + Status + Log underneath */}
          <div className="flex flex-col gap-4 min-h-0">
            <DirectionPad
              onDirectionStart={handleDirectionStart}
              onDirectionStop={handleDirectionStop}
              activeDirection={activeDirection}
            />
            <StatusIndicator
              status={status}
              activeDirection={formatDirection(activeDirection)}
              lastError={lastError}
              wsUrl={WS_URL}
            />
          </div>

          {/* Right: Command Panel + Log underneath */}
          <div className="flex flex-col gap-4">
            <CommandPanel
              controlSpeed={controlSpeed}
              onControlSpeedChange={setControlSpeed}
              encryptionEnabled={encryptionEnabled}
              onEncryptionChange={setEncryptionEnabled}
              hasEncryptionKey={Boolean(ENCRYPTION_KEY)}
            />
            <MessageLog logs={messageLog} />
          </div>
        </div>
      </div>

      <SettingsDrawer
        repeatRate={repeatRate}
        onRepeatRateChange={setRepeatRate}
      />
    </div>
  );
}

export default App;
