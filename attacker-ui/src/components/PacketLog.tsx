export interface PacketEntry {
  id: number;
  payload: string;
  timestamp: string;
  direction: 'sniffed' | 'injected' | 'replayed';
}

interface PacketLogProps {
  packets: PacketEntry[];
  onClear: () => void;
}

const DIRECTION_STYLES: Record<PacketEntry['direction'], string> = {
  sniffed:  'text-yellow-400 bg-yellow-900/30 border border-yellow-800',
  injected: 'text-red-400 bg-red-900/30 border border-red-800',
  replayed: 'text-orange-400 bg-orange-900/30 border border-orange-800',
};

export function PacketLog({ packets, onClear }: PacketLogProps) {
  return (
    <div className="bg-gray-900 border border-red-800 rounded-xl shadow-lg shadow-red-950/50 p-6 flex flex-col min-h-0">
      <div className="flex items-center justify-between mb-4">
        <h2 className="text-sm font-semibold text-red-300 uppercase tracking-widest">
          Packet Log
        </h2>
        <div className="flex items-center gap-3">
          <span className="text-sm text-white">{packets.length} captured</span>
          <button
            onClick={onClear}
            className="text-sm text-red-400 hover:text-red-200 transition-colors"
          >
            Clear
          </button>
        </div>
      </div>

      <div className="flex gap-2 mb-3">
        <span className="text-xs text-yellow-500 bg-yellow-900/30 border border-yellow-800 px-2 py-1 rounded">sniffed</span>
        <span className="text-xs text-red-400 bg-red-900/30 border border-red-800 px-2 py-1 rounded">injected</span>
        <span className="text-xs text-orange-400 bg-orange-900/30 border border-orange-800 px-2 py-1 rounded">replayed</span>
      </div>

      {packets.length === 0 ? (
        <p className="text-sm text-white italic">Awaiting packets…</p>
      ) : (
        <div className="space-y-1.5 max-h-72 overflow-y-auto">
          {packets.map((pkt) => (
            <div key={pkt.id} className="flex gap-2 items-start text-sm">
              <span className="text-white shrink-0 font-mono">{pkt.timestamp}</span>
              <span className={`shrink-0 px-1.5 py-0.5 rounded text-xs font-medium ${DIRECTION_STYLES[pkt.direction]}`}>
                {pkt.direction}
              </span>
              <code className="text-white font-mono break-all flex-1 min-w-0">{pkt.payload}</code>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
