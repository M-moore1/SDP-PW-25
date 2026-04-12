import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useWebSocket } from './hooks/useWebSocket';
import { ConnectionStatusPanel } from './components/ConnectionStatus';
import { PacketLog } from './components/PacketLog';
import { AttackPanel } from './components/AttackPanel';
import { ReplayPanel } from './components/ReplayPanel';
import { SettingsPanel } from './components/SettingsPanel';
import type { PacketEntry } from './components/PacketLog';
import type { Preset } from './components/SettingsPanel';

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080';
const SNIFFER_WS_URL = import.meta.env.VITE_SNIFFER_WS_URL || 'ws://localhost:8765';

const DEFAULT_PRESETS: Preset[] = [
  { label: '', key: 'w', payload: '' },
  { label: '', key: 'a', payload: '' },
  { label: '', key: 's', payload: '' },
  { label: '', key: 'd', payload: '' },
];

const STORAGE_KEY = 'attacker-presets-v2';

function loadPresets(): Preset[] {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return JSON.parse(raw);
  } catch { /* corrupt data, fall through */ }
  return DEFAULT_PRESETS;
}

let packetCounter = 0;

type RightTab = 'attack' | 'settings';

function App() {
  const [packets, setPackets] = useState<PacketEntry[]>([]);
  const packetCountRef = useRef(0);
  const [presets, setPresets] = useState<Preset[]>(loadPresets);
  const [rightTab, setRightTab] = useState<RightTab>('attack');

  const handlePresetsChange = useCallback((next: Preset[]) => {
    setPresets(next);
    localStorage.setItem(STORAGE_KEY, JSON.stringify(next));
  }, []);

  const keyPayloads = useMemo(() => {
    const map: Record<string, string> = {};
    for (const p of presets) map[p.key] = p.payload;
    return map;
  }, [presets]);

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
    onMessage: (data) => {
      try {
        const pkt = JSON.parse(data);
        if (pkt.type === 'ble_ack') {
          const prefix = pkt.status === 'ok' ? '[BLE OK]' : '[BLE ERR]';
          addPacket(`${prefix} ${pkt.msg}`, 'injected');
          return;
        }
      } catch {
        /* ignore non-JSON */
      }
    },
  });

  useWebSocket(SNIFFER_WS_URL, {
    onMessage: (data) => {
      try {
        const pkt = JSON.parse(data);
        if (pkt.type === 'sniffed_packet' && pkt.value) {
          addPacket(`[Frame ${pkt.frame}] ${pkt.info} | ${pkt.value}`, 'sniffed');
        }
      } catch {
        /* ignore non-JSON */
      }
    },
  });

  const handleInject = useCallback((payload: string) => {
    sendRaw(JSON.stringify({ type: 'ble_inject', data: payload }));
    addPacket(payload, 'injected');
  }, [sendRaw, addPacket]);

  const handleReplay = useCallback((payload: string) => {
    sendRaw(payload);
    addPacket(payload, 'replayed');
  }, [sendRaw, addPacket]);

  const isConnected = status === 'connected';

  const SEND_INTERVAL_MS = 150;

  const [activeKey, setActiveKey] = useState<string | null>(null);
  const pressedRef = useRef<Set<string>>(new Set());
  const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null);

  useEffect(() => {
    const startSending = (key: string) => {
      stopSending();
      const payload = keyPayloads[key];
      if (isConnected) handleInject(payload);
      intervalRef.current = setInterval(() => {
        if (isConnected) handleInject(payload);
      }, SEND_INTERVAL_MS);
    };

    const stopSending = () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    };

    const onKeyDown = (e: KeyboardEvent) => {
      const tag = (e.target as HTMLElement)?.tagName;
      if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT') return;
      const key = e.key.toLowerCase();
      if (!keyPayloads[key] || pressedRef.current.has(key)) return;
      e.preventDefault();
      pressedRef.current.add(key);
      setActiveKey(key);
      startSending(key);
    };

    const onKeyUp = (e: KeyboardEvent) => {
      const key = e.key.toLowerCase();
      if (!pressedRef.current.has(key)) return;
      pressedRef.current.delete(key);
      stopSending();
      setActiveKey((prev) => (prev === key ? null : prev));
    };

    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('keyup', onKeyUp);
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
      stopSending();
    };
  }, [isConnected, handleInject, keyPayloads]);

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
            <div className="flex rounded-lg overflow-hidden border border-gray-700">
              {([['attack', 'Attack'], ['settings', 'Settings']] as const).map(([id, label]) => (
                <button
                  key={id}
                  onClick={() => setRightTab(id)}
                  className={`flex-1 py-2 text-sm font-semibold tracking-wide transition-colors ${
                    rightTab === id
                      ? 'bg-red-700 text-white'
                      : 'bg-gray-900 text-gray-400 hover:text-white'
                  }`}
                >
                  {label}
                </button>
              ))}
            </div>

            {rightTab === 'attack' ? (
              <>
                <AttackPanel
                  onInject={handleInject}
                  isConnected={isConnected}
                  activeKey={activeKey}
                  presets={presets}
                />
                <ReplayPanel
                  packets={packets}
                  onReplay={handleReplay}
                  isConnected={isConnected}
                />
              </>
            ) : (
              <SettingsPanel
                presets={presets}
                onPresetsChange={handlePresetsChange}
              />
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
