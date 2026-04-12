import { useState } from 'react';
import type { Preset } from './SettingsPanel';

interface AttackPanelProps {
  onInject: (payload: string) => void;
  isConnected: boolean;
  activeKey: string | null;
  presets: Preset[];
}

type InputMode = 'hex' | 'json';

function isValidHex(value: string): boolean {
  const clean = value.replace(/\s+/g, '');
  return clean.length > 0 && clean.length % 2 === 0 && /^[0-9a-fA-F]+$/.test(clean);
}

function isValidJson(value: string): boolean {
  try {
    JSON.parse(value);
    return true;
  } catch {
    return false;
  }
}

function stringToHex(text: string): string {
  return Array.from(text)
    .map((ch) => ch.charCodeAt(0).toString(16).padStart(2, '0'))
    .join(' ');
}

export function AttackPanel({ onInject, isConnected, activeKey, presets }: AttackPanelProps) {
  const [customPayload, setCustomPayload] = useState('');
  const [inputMode, setInputMode] = useState<InputMode>('hex');

  const payloadReady =
    inputMode === 'json'
      ? isValidJson(customPayload.trim())
      : isValidHex(customPayload);

  const hexOutput =
    inputMode === 'json' ? stringToHex(customPayload.trim()) : customPayload.trim();

  const handleInjectCustom = () => {
    if (payloadReady) {
      onInject(hexOutput);
      setCustomPayload('');
    }
  };

  return (
    <div className="bg-gray-900 border border-red-800 rounded-xl shadow-lg shadow-red-950/50 p-6">
      <h2 className="text-sm font-semibold text-red-300 uppercase tracking-widest mb-4">
        Command Injection
      </h2>

      <div className="grid grid-cols-3 grid-rows-2 gap-3 mb-5">
        {([
          { key: 'w', col: 2,     row: 1        },
          { key: 'a', col: 1,     row: 'span 2' },
          { key: 's', col: 2,     row: 2        },
          { key: 'd', col: 3,     row: 'span 2' },
        ] as const).map((cmd) => {
          const preset = presets.find(p => p.key === cmd.key);
          return (
            <button
              key={cmd.key}
              onClick={() => preset && onInject(preset.payload)}
              disabled={!isConnected || !preset}
              style={{ gridColumn: cmd.col, gridRow: cmd.row }}
              className={`flex items-center justify-center py-2.5 px-4 border text-base font-bold rounded-lg transition-colors disabled:opacity-40 disabled:cursor-not-allowed ${
                activeKey === cmd.key
                  ? 'bg-red-600 border-red-400 text-white shadow-lg shadow-red-900/50'
                  : 'bg-red-900/50 hover:bg-red-800 border-red-700 hover:border-red-500 text-white'
              }`}
            >
              {preset?.label ?? cmd.key.toUpperCase()}
            </button>
          );
        })}
      </div>

      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <label className="text-sm text-white">
            Custom Payload <span className="text-gray-500">({inputMode === 'hex' ? 'hex bytes' : 'JSON → hex'})</span>
          </label>
          <div className="flex rounded-md overflow-hidden border border-gray-700 text-xs">
            {(['hex', 'json'] as const).map((mode) => (
              <button
                key={mode}
                onClick={() => { setInputMode(mode); setCustomPayload(''); }}
                className={`px-3 py-1 font-medium transition-colors ${
                  inputMode === mode
                    ? 'bg-red-700 text-white'
                    : 'bg-gray-800 text-gray-400 hover:text-white'
                }`}
              >
                {mode.toUpperCase()}
              </button>
            ))}
          </div>
        </div>
        <textarea
          value={customPayload}
          onChange={(e) => setCustomPayload(e.target.value)}
          placeholder={inputMode === 'hex' ? '85 90 01 00 00 00 00 00' : '{"forward":1,"backward":0,"speed":50}'}
          rows={4}
          className={`w-full bg-gray-800 border text-gray-200 text-sm font-mono rounded-lg px-3 py-2 resize-none outline-none transition-colors placeholder-gray-600 ${
            inputMode === 'hex' && customPayload.trim() && !isValidHex(customPayload)
              ? 'border-yellow-500 focus:border-yellow-400'
              : 'border-gray-700 focus:border-red-600'
          }`}
        />
        {inputMode === 'hex' && customPayload.trim() && !isValidHex(customPayload) && (
          <p className="text-yellow-400 text-xs">
            Enter an even number of hex characters (0-9, A-F). Spaces are ignored.
          </p>
        )}
        {inputMode === 'json' && customPayload.trim() && !isValidJson(customPayload.trim()) && (
          <p className="text-yellow-400 text-xs">
            Invalid JSON. Make sure it's well-formed.
          </p>
        )}
        {inputMode === 'json' && customPayload.trim() && isValidJson(customPayload.trim()) && (
          <p className="text-gray-500 text-xs font-mono truncate">
            → {hexOutput}
          </p>
        )}
        <button
          onClick={handleInjectCustom}
          disabled={!isConnected || !payloadReady}
          className="w-full py-2.5 bg-red-700 hover:bg-red-600 text-white text-base font-semibold rounded-lg transition-colors disabled:opacity-40 disabled:cursor-not-allowed focus:outline-none focus:ring-2 focus:ring-red-500 focus:ring-offset-2 focus:ring-offset-gray-900"
        >
          Inject Packet
        </button>
      </div>
    </div>
  );
}
