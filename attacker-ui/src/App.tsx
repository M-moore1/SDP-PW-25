import { useCallback, useEffect, useRef, useState } from 'react';
import { useWebSocket } from './hooks/useWebSocket';
import { ConnectionStatusPanel } from './components/ConnectionStatus';
import { PacketLog } from './components/PacketLog';
import { AttackPanel } from './components/AttackPanel';
import { ReplayPanel } from './components/ReplayPanel';
import type { PacketEntry } from './components/PacketLog';

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080';

let packetCounter = 0;

function App() {
  const [packets, setPackets] = useState<PacketEntry[]>([]);
  const packetCountRef = useRef(0);

  const addPacket = useCallback((payload: string, direction: PacketEntry['direction']) => {
    packetCountRef.current += 1;
    const entry: PacketEntry = {
      id: packetCounter++,
      payload,
      timestamp: new Date().toLocaleTimeString(),
      direction,
    };
    setPackets((prev) => [entry, ...prev].slice(0, 100));
  }, []);

  const { status, sendRaw, lastError } = useWebSocket(WS_URL, {
    onMessage: (data) => addPacket(data, 'sniffed'),
  });

  const handleInject = useCallback((payload: string) => {
    sendRaw(payload);
    addPacket(payload, 'injected');
  }, [sendRaw, addPacket]);

  const handleReplay = useCallback((payload: string) => {
    sendRaw(payload);
    addPacket(payload, 'replayed');
  }, [sendRaw, addPacket]);

  const isConnected = status === 'connected';

  const KEY_PAYLOADS: Record<string, string> = {
    w: '{"type":"C","forward":1,"backward":0,"left":0,"right":0,"speed":50}',
    s: '{"type":"C","forward":0,"backward":1,"left":0,"right":0,"speed":50}',
    a: '{"type":"C","forward":0,"backward":0,"left":1,"right":0,"speed":50}',
    d: '{"type":"C","forward":0,"backward":0,"left":0,"right":1,"speed":50}',
  };

  const [activeKey, setActiveKey] = useState<string | null>(null);
  const pressedRef = useRef<Set<string>>(new Set());

  useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      const key = e.key.toLowerCase();
      if (!KEY_PAYLOADS[key] || pressedRef.current.has(key)) return;
      e.preventDefault();
      pressedRef.current.add(key);
      setActiveKey(key);
      if (isConnected) handleInject(KEY_PAYLOADS[key]);
    };
    const onKeyUp = (e: KeyboardEvent) => {
      const key = e.key.toLowerCase();
      pressedRef.current.delete(key);
      setActiveKey((prev) => (prev === key ? null : prev));
    };
    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('keyup', onKeyUp);
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
    };
  }, [isConnected, handleInject]);

  return (
    <div className="min-h-screen bg-black p-10">
      <div className="max-w-6xl mx-auto">

        <header className="text-center mb-10">
          <div className="inline-flex items-center gap-3 mb-2">
            <span className="w-3 h-3 rounded-full bg-red-600 animate-pulse" />
            <h1 className="text-5xl font-bold text-red-500 tracking-tight">
              Attacker Interface
            </h1>
            <span className="w-3 h-3 rounded-full bg-red-600 animate-pulse" />
          </div>
          <p className="text-gray-500 text-base">
            Security Research — SDP PW-25 &nbsp;|&nbsp; Sniff · Inject · Replay
          </p>
        </header>

        <div className="grid gap-8 md:grid-cols-2 items-start">
          {/* Left column */}
          <div className="flex flex-col gap-6">
            <ConnectionStatusPanel
              status={status}
              wsUrl={WS_URL}
              lastError={lastError}
            />
            <PacketLog
              packets={packets}
              onClear={() => setPackets([])}
            />
          </div>

          {/* Right column */}
          <div className="flex flex-col gap-6">
            <AttackPanel
              onInject={handleInject}
              isConnected={isConnected}
              activeKey={activeKey}
            />
            <ReplayPanel
              packets={packets}
              onReplay={handleReplay}
              isConnected={isConnected}
            />
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
