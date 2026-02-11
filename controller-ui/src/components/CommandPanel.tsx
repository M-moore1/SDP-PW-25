//Author: Krish Shah
//Reviewed by: Sai Raparla

interface CommandPanelProps {
  controlSpeed: number;
  onControlSpeedChange: (speed: number) => void;
  encryptionEnabled: boolean;
  onEncryptionChange: (enabled: boolean) => void;
  hasEncryptionKey: boolean;
}

export function CommandPanel({
  controlSpeed,
  onControlSpeedChange,
  encryptionEnabled,
  onEncryptionChange,
  hasEncryptionKey,
}: CommandPanelProps) {
  return (
    <div className="bg-white rounded-xl shadow-lg p-6">
      <h2 className="text-lg font-semibold text-gray-700">Command Panel</h2>

      {/* Encryption toggle */}
      <div className="border border-gray-200 rounded-lg p-4 mt-4">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-sm font-semibold text-gray-600">Encryption</h3>
            <p className="text-xs text-gray-500 mt-0.5">
              {hasEncryptionKey ? 'AES-256-GCM via WASM' : 'Set VITE_ENCRYPTION_KEY in .env'}
            </p>
          </div>
          <button
            type="button"
            role="switch"
            aria-checked={encryptionEnabled}
            disabled={!hasEncryptionKey}
            onClick={() => hasEncryptionKey && onEncryptionChange(!encryptionEnabled)}
            className={`relative inline-flex h-6 w-11 shrink-0 cursor-pointer rounded-full border-2 border-transparent transition-colors focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 disabled:cursor-not-allowed disabled:opacity-50 ${
              encryptionEnabled ? 'bg-blue-600' : 'bg-gray-200'
            } ${!hasEncryptionKey ? 'opacity-50' : ''}`}
          >
            <span
              className={`pointer-events-none inline-block h-5 w-5 transform rounded-full bg-white shadow ring-0 transition ${
                encryptionEnabled ? 'translate-x-5' : 'translate-x-1'
              }`}
            />
          </button>
        </div>
        <p className="text-xs text-gray-500 mt-2">
          {encryptionEnabled ? 'Encrypted' : 'Unencrypted'}
        </p>
      </div>

      {/* Control (C) - speed slider */}
      <div className="border border-gray-200 rounded-lg p-4 mt-4">
        <h3 className="text-sm font-semibold text-gray-600 mb-2 flex items-center gap-2">
          <span className="bg-gray-800 text-white text-xs px-1.5 py-0.5 rounded">C</span>
          Speed Control
        </h3>
        <div>
          <label className="text-xs text-gray-500 block mb-1">Speed (0–100)</label>
          <input
            type="range"
            min="0"
            max="100"
            value={controlSpeed}
            onChange={(e) => onControlSpeedChange(Number(e.target.value))}
            className="w-full h-2 bg-gray-200 rounded-lg accent-blue-600"
          />
          <input
            type="number"
            min={0}
            max={100}
            value={controlSpeed}
            onChange={(e) => {
              const v = Number(e.target.value);
              if (!Number.isNaN(v)) {
                onControlSpeedChange(Math.min(100, Math.max(0, Math.round(v))));
              }
            }}
            className="mt-2 w-full text-sm font-mono border border-gray-200 rounded px-2 py-1.5"
          />
        </div>
      </div>
    </div>
  );
}
