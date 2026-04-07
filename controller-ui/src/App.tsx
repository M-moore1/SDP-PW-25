//Author: Krish Shah
//Reviewed by: Sai Raparla
import { useEffect, useState, useCallback, useRef } from 'react';
import { useWebSocket } from './hooks/useWebSocket';
import { useHoldRepeater } from './hooks/useHoldRepeater';
import { DirectionPad } from './components/DirectionPad';
import { ArmPad } from './components/ArmPad';
import { CommandPanel } from './components/CommandPanel';
import { MessageLog } from './components/MessageLog';
import {
  Direction,
  formatDirection,
  isDirectionKey,
  keyToDirection,
  directionsToControlAxes,
} from './utils/direction';
import {
  ArmAction,
  isArmKey,
  keyToArmAction,
  armActionsToAxes,
} from './utils/armDirection';
import { buildControlMsg, buildArmControlMsg } from './utils/commands';
import { useThemePreference } from './hooks/useThemePreference';
import { ThemeToggle } from './components/ThemeToggle';

interface MessageLogEntry {
  payload: string;
  timestamp: string;
}

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080';
const ENCRYPTION_KEY = 'a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456'; // 64 hex chars (32 bytes)

function App() {
  const { isDark, toggleTheme } = useThemePreference();
  const [activeDirections, setActiveDirections] = useState<Set<Direction>>(new Set());
  const [activeArmActions, setActiveArmActions] = useState<Set<ArmAction>>(new Set());
  const [messageLog, setMessageLog] = useState<MessageLogEntry[]>([]);
  const [repeatRate, setRepeatRate] = useState(50);
  const [controlSpeed, setControlSpeed] = useState(50);
  const [armSpeed, setArmSpeed] = useState(50);
  const [encryptionEnabled, setEncryptionEnabled] = useState(false);

  const pressedKeysRef = useRef<Set<string>>(new Set());
  const armPressedKeysRef = useRef<Set<string>>(new Set());

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

  const sendActiveArmActions = useCallback((actions: Set<ArmAction>) => {
    const axes = armActionsToAxes(actions);
    const msg = buildArmControlMsg(
      axes.U, axes.D, axes.L, axes.R, axes.In, axes.O, armSpeed
    );
    sendMessage(msg);
  }, [sendMessage, armSpeed]);

  useHoldRepeater(
    activeDirections.size > 0,
    () => sendActiveDirections(activeDirections),
    repeatRate
  );

  useHoldRepeater(
    activeArmActions.size > 0,
    () => sendActiveArmActions(activeArmActions),
    repeatRate
  );

  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    if (pressedKeysRef.current.has(e.key) || armPressedKeysRef.current.has(e.key)) return;

    if (isDirectionKey(e.key)) {
      e.preventDefault();
      pressedKeysRef.current.add(e.key);
      setActiveDirections(prev => new Set([...prev, keyToDirection[e.key]]));
    } else if (isArmKey(e.key)) {
      e.preventDefault();
      armPressedKeysRef.current.add(e.key);
      setActiveArmActions(prev => new Set([...prev, keyToArmAction[e.key]]));
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
    } else if (isArmKey(e.key)) {
      e.preventDefault();
      armPressedKeysRef.current.delete(e.key);
      setActiveArmActions(prev => {
        const next = new Set(prev);
        next.delete(keyToArmAction[e.key]);
        return next;
      });
    }
  }, []);

  const handleBlur = useCallback(() => {
    setActiveDirections(new Set());
    pressedKeysRef.current.clear();
    setActiveArmActions(new Set());
    armPressedKeysRef.current.clear();
  }, []);

  useEffect(() => {
    const preventedKeys = new Set(['w', 'a', 's', 'd', ' ', 'arrowup', 'arrowdown', 'arrowleft', 'arrowright']);
    const preventScroll = (e: KeyboardEvent) => {
      if (preventedKeys.has(e.key.toLowerCase())) {
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

  const handleArmActionStart = useCallback((action: ArmAction) => {
    setActiveArmActions(prev => new Set([...prev, action]));
  }, []);

  const handleArmActionStop = useCallback((action: ArmAction) => {
    setActiveArmActions(prev => {
      const next = new Set(prev);
      next.delete(action);
      return next;
    });
  }, []);

  const handleArmReset = useCallback(() => {
    const msg = buildArmControlMsg(0, 0, 0, 0, 0, 0, armSpeed, 1);
    sendMessage(msg);
  }, [sendMessage, armSpeed]);

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
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-8 dark:from-slate-900 dark:to-slate-800">
      <ThemeToggle isDark={isDark} onToggle={toggleTheme} />
      <div className="max-w-5xl mx-auto">
        <header className="text-center mb-6">
          <h1 className="text-4xl font-bold text-gray-800 mb-2 dark:text-gray-100">Robot Controller</h1>
          <p className="text-gray-600 dark:text-gray-300">
            WASD to drive &middot; Arrow keys / Shift / Enter to control arm
          </p>
        </header>

        {/* Control pads — equal height */}
        <div className="grid gap-6 md:grid-cols-2 items-stretch">
          <DirectionPad
            onDirectionStart={handleDirectionStart}
            onDirectionStop={handleDirectionStop}
            activeDirections={activeDirections}
            controlSpeed={controlSpeed}
            onControlSpeedChange={setControlSpeed}
            encryptionEnabled={encryptionEnabled}
            onEncryptionChange={handleEncryptionChange}
            hasEncryptionKey={Boolean(ENCRYPTION_KEY)}
          />
          <ArmPad
            onActionStart={handleArmActionStart}
            onActionStop={handleArmActionStop}
            activeArmActions={activeArmActions}
            armSpeed={armSpeed}
            onArmSpeedChange={setArmSpeed}
            onReset={handleArmReset}
          />
        </div>

        {/* Command panel + message log */}
        <div className="grid gap-6 md:grid-cols-2 items-stretch mt-6">
          <CommandPanel
            onConnect={handleConnect}
            status={status}
            activeDirection={formatDirection(activeDirections)}
            lastError={lastError}
            wsUrl={WS_URL}
            repeatRate={repeatRate}
            onRepeatRateChange={setRepeatRate}
          />
          <MessageLog logs={messageLog} />
        </div>
      </div>

    </div>
  );
}

export default App;
