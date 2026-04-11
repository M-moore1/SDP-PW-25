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
  directionsToControlAxes,
} from './utils/direction';
import { buildControlMsg } from './utils/commands';

interface MessageLogEntry {
  payload: string;
  timestamp: string;
}

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:3001';
const ENCRYPTION_KEY = 'a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456'; // 64 hex chars (32 bytes)

function App() {
  const [activeDirections, setActiveDirections] = useState<Set<Direction>>(new Set());
  const [messageLog, setMessageLog] = useState<MessageLogEntry[]>([]);
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

  const sendActiveDirections = useCallback((directions: Set<Direction>) => {
    const axes = directionsToControlAxes(directions);
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
    activeDirections.size > 0,
    () => sendActiveDirections(activeDirections),
    repeatRate
  );

  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    if (pressedKeysRef.current.has(e.key)) return;
    if (isDirectionKey(e.key)) {
      e.preventDefault();
      pressedKeysRef.current.add(e.key);
      setActiveDirections(prev => new Set([...prev, keyToDirection[e.key]]));
    }
  }, []);

  const handleKeyUp = useCallback((e: KeyboardEvent) => {
    if (isDirectionKey(e.key)) {
      e.preventDefault();
      pressedKeysRef.current.delete(e.key);
      setActiveDirections(prev => {
        const next = new Set(prev);
        next.delete(keyToDirection[e.key]);
        return next;
      });
    }
  }, []);

  const handleBlur = useCallback(() => {
    setActiveDirections(new Set());
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
    setActiveDirections(prev => new Set([...prev, direction]));
  }, []);

  const handleDirectionStop = useCallback((direction: Direction) => {
    setActiveDirections(prev => {
      const next = new Set(prev);
      next.delete(direction);
      return next;
    });
  }, []);

  const handleConnect = useCallback(() => {
    sendMessage({
      T: 'S',
      instruction: 0b0010,
      Authorization_Code: 0x03ff,
      instruction_specific: 0x00,
      PL: 1,
      ID: 28,
    });
  }, [sendMessage]);

  const handleEncryptionChange = useCallback(
    (enabled: boolean) => {
      const msg = {
        T: 'S',
        instruction: 0b0011,
        Authorization_Code: 0x03ff,
        instruction_specific: enabled ? 1 : 0,
        PL: 1,
        ID: 28,
      };
      logMessage(`Encryption ${enabled ? 'enabled' : 'disabled'} (${JSON.stringify(msg)})`);
      sendMessage(msg);
      setEncryptionEnabled(enabled);
    },
    [sendMessage, logMessage]
  );

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
              activeDirections={activeDirections}
            />
            <StatusIndicator
              status={status}
              activeDirection={formatDirection(activeDirections)}
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
              onEncryptionChange={handleEncryptionChange}
              hasEncryptionKey={Boolean(ENCRYPTION_KEY)}
              onConnect={handleConnect}
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
