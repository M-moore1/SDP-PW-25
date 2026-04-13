import { useState } from 'react';
import type { PacketEntry } from './PacketLog';

interface ReplayPanelProps {
  packets: PacketEntry[];
  onReplay: (payload: string) => void;
  isConnected: boolean;
}

export function ReplayPanel({ packets, onReplay, isConnected }: ReplayPanelProps) {
  const [selectedId, setSelectedId] = useState<number | null>(null);
  const [replayCount, setReplayCount] = useState(1);

  const sniffed = packets.filter((p) => p.direction === 'sniffed');
  const selected = sniffed.find((p) => p.id === selectedId) ?? null;

  const handleReplay = () => {
    if (!selected) return;
    for (let i = 0; i < replayCount; i++) {
      onReplay(selected.payload);
    }
  };

  return (
    <div className="bg-gray-900 border border-red-800 rounded-xl shadow-lg shadow-red-950/50 p-6">
      <h2 className="text-sm font-semibold text-red-300 uppercase tracking-widest mb-4">
        Replay Attack
      </h2>

      {sniffed.length === 0 ? (
        <p className="text-sm text-white italic">No sniffed packets to replay yet.</p>
      ) : (
        <>
          <div className="space-y-1.5 max-h-48 overflow-y-auto mb-4">
            {sniffed.map((pkt) => (
              <button
                key={pkt.id}
                onClick={() => setSelectedId(pkt.id)}
                className={`w-full text-left px-3 py-2 rounded-lg text-sm font-mono transition-colors ${
                  selectedId === pkt.id
                    ? 'bg-red-800/60 border border-red-600 text-white'
                    : 'bg-gray-800 border border-gray-700 text-white hover:border-red-700 hover:text-white'
                }`}
              >
                <span className="text-white mr-2">{pkt.timestamp}</span>
                <span className="truncate">{pkt.payload.slice(0, 60)}{pkt.payload.length > 60 ? '…' : ''}</span>
              </button>
            ))}
          </div>

          <div className="flex items-center gap-3 mb-4">
            <label className="text-sm text-white shrink-0">Repeat</label>
            <input
              type="number"
              min={1}
              max={100}
              value={replayCount}
              onChange={(e) => setReplayCount(Math.max(1, Math.min(100, Number(e.target.value))))}
              className="w-24 bg-gray-800 border border-gray-700 focus:border-red-600 text-gray-200 text-sm rounded-lg px-3 py-1.5 outline-none transition-colors"
            />
            <span className="text-sm text-white">× times</span>
          </div>

          <button
            onClick={handleReplay}
            disabled={!isConnected || !selected}
            className="w-full py-2.5 bg-orange-800 hover:bg-orange-700 text-white text-base font-semibold rounded-lg transition-colors disabled:opacity-40 disabled:cursor-not-allowed focus:outline-none focus:ring-2 focus:ring-orange-600 focus:ring-offset-2 focus:ring-offset-gray-900"
          >
            Replay Selected Packet
          </button>
        </>
      )}
    </div>
  );
}
