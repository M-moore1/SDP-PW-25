//Author: Krish Shah
//Reviewed by: Sai Raparla

type ConnectionStatus = 'connected' | 'connecting' | 'reconnecting' | 'disconnected';

interface CommandPanelProps {
  onConnect?: () => void;
  status: ConnectionStatus;
  activeDirection: string;
  lastError: string | null;
  wsUrl: string;
  repeatRate: number;
  onRepeatRateChange: (rate: number) => void;
}

const STATUS_STYLE: Record<ConnectionStatus, string> = {
  connected:    'text-green-600 bg-green-100',
  connecting:   'text-yellow-600 bg-yellow-100',
  reconnecting: 'text-orange-600 bg-orange-100',
  disconnected: 'text-red-600 bg-red-100',
};

const STATUS_TEXT: Record<ConnectionStatus, string> = {
  connected:    '● Connected',
  connecting:   '⟳ Connecting…',
  reconnecting: '⟳ Reconnecting…',
  disconnected: '○ Disconnected',
};

const REPEAT_RATE_OPTIONS = [
  { label: '50ms', value: 50 },
  { label: '100ms', value: 100 },
  { label: '150ms', value: 150 },
];

export function CommandPanel({
  onConnect,
  status,
  activeDirection,
  lastError,
  wsUrl,
  repeatRate,
  onRepeatRateChange,
}: CommandPanelProps) {
  return (
    <div className="bg-white rounded-xl shadow-lg p-6">
      <h2 className="text-lg font-semibold text-gray-700">Command Panel</h2>

      {/* Connection status */}
      <div className="border border-gray-200 rounded-lg p-4 mt-4 space-y-2">
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-semibold text-gray-600">Connection Status</h3>
          <span className={`inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium ${STATUS_STYLE[status]}`}>
            {STATUS_TEXT[status]}
          </span>
        </div>
        <p className="text-xs text-gray-500">Active: {activeDirection}</p>
        <p className="text-xs text-gray-500 truncate font-mono" title={wsUrl}>
          {wsUrl}
        </p>
        {lastError && (
          <p className="text-xs text-red-600 truncate" title={lastError}>
            {lastError}
          </p>
        )}
      </div>

      {/* Connect button */}
      <div className="border border-gray-200 rounded-lg p-4 mt-4">
        <button
          type="button"
          onClick={onConnect}
          className="w-full py-2.5 px-4 bg-blue-600 hover:bg-blue-700 text-white font-medium rounded-lg transition-colors focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 disabled:opacity-50 disabled:cursor-not-allowed"
        >
          Connect to ESP32
        </button>
      </div>

      {/* Repeat rate */}
      <div className="border border-gray-200 rounded-lg p-4 mt-4">
        <h3 className="text-sm font-semibold text-gray-600 mb-2">Repeat Rate</h3>
        <div className="flex gap-2">
          {REPEAT_RATE_OPTIONS.map((opt) => (
            <button
              key={opt.value}
              type="button"
              onClick={() => onRepeatRateChange(opt.value)}
              className={`flex-1 py-2 px-3 rounded-lg text-sm font-medium transition-colors ${
                repeatRate === opt.value
                  ? 'bg-blue-600 text-white'
                  : 'bg-gray-100 text-gray-700 hover:bg-gray-200'
              }`}
            >
              {opt.label}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}
