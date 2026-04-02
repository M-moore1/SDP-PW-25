import { useState } from 'react';

interface AttackPanelProps {
  onInject: (payload: string) => void;
  isConnected: boolean;
  activeKey: string | null;
}

const PRESET_COMMANDS = [
  { label: 'Forward',  key: 'w', payload: '{"type":"C","forward":1,"backward":0,"left":0,"right":0,"speed":50}' },
  { label: 'Left',     key: 'a', payload: '{"type":"C","forward":0,"backward":0,"left":1,"right":0,"speed":50}' },
  { label: 'Backward', key: 's', payload: '{"type":"C","forward":0,"backward":1,"left":0,"right":0,"speed":50}' },
  { label: 'Right',    key: 'd', payload: '{"type":"C","forward":0,"backward":0,"left":0,"right":1,"speed":50}' },
];

export function AttackPanel({ onInject, isConnected, activeKey }: AttackPanelProps) {
  const [customPayload, setCustomPayload] = useState('');

  const handleInjectCustom = () => {
    const trimmed = customPayload.trim();
    if (trimmed) {
      onInject(trimmed);
      setCustomPayload('');
    }
  };

  return (
    <div className="bg-gray-900 border border-red-800 rounded-xl shadow-lg shadow-red-950/50 p-6">
      <h2 className="text-sm font-semibold text-red-300 uppercase tracking-widest mb-4">
        Command Injection
      </h2>

      <div className="grid grid-cols-3 grid-rows-2 gap-3 mb-5">
        {/* Forward — col 2, row 1 */}
        {[
          { label: 'Forward',  key: 'w', col: 2,     row: 1          },
          { label: 'Left',     key: 'a', col: 1,     row: 'span 2'   },
          { label: 'Backward', key: 's', col: 2,     row: 2          },
          { label: 'Right',    key: 'd', col: 3,     row: 'span 2'   },
        ].map((cmd) => (
          <button
            key={cmd.label}
            onClick={() => onInject(PRESET_COMMANDS.find(c => c.key === cmd.key)!.payload)}
            disabled={!isConnected}
            style={{ gridColumn: cmd.col, gridRow: cmd.row }}
            className={`flex items-center justify-center py-2.5 px-4 border text-base font-bold rounded-lg transition-colors disabled:opacity-40 disabled:cursor-not-allowed ${
              activeKey === cmd.key
                ? 'bg-red-600 border-red-400 text-white shadow-lg shadow-red-900/50'
                : 'bg-red-900/50 hover:bg-red-800 border-red-700 hover:border-red-500 text-white'
            }`}
          >
            {cmd.label}
          </button>
        ))}
      </div>

      <div className="space-y-3">
        <label className="block text-sm text-white">Custom Payload</label>
        <textarea
          value={customPayload}
          onChange={(e) => setCustomPayload(e.target.value)}
          placeholder='{"type":"C","forward":1,...}'
          rows={4}
          className="w-full bg-gray-800 border border-gray-700 focus:border-red-600 text-gray-200 text-sm font-mono rounded-lg px-3 py-2 resize-none outline-none transition-colors placeholder-gray-600"
        />
        <button
          onClick={handleInjectCustom}
          disabled={!isConnected || !customPayload.trim()}
          className="w-full py-2.5 bg-red-700 hover:bg-red-600 text-white text-base font-semibold rounded-lg transition-colors disabled:opacity-40 disabled:cursor-not-allowed focus:outline-none focus:ring-2 focus:ring-red-500 focus:ring-offset-2 focus:ring-offset-gray-900"
        >
          Inject Packet
        </button>
      </div>
    </div>
  );
}
