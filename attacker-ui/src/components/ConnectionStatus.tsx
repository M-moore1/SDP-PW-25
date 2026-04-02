import type { ConnectionStatus } from '../hooks/useWebSocket';

interface ConnectionStatusProps {
  status: ConnectionStatus;
  wsUrl: string;
  lastError: string | null;
}

export function ConnectionStatusPanel({ status, wsUrl, lastError }: ConnectionStatusProps) {
  const getStatusStyle = () => {
    switch (status) {
      case 'connected':    return 'text-red-400 bg-red-900/40 border border-red-700';
      case 'connecting':   return 'text-yellow-400 bg-yellow-900/40 border border-yellow-700';
      case 'reconnecting': return 'text-orange-400 bg-orange-900/40 border border-orange-700';
      case 'disconnected': return 'text-red-400 bg-red-900/40 border border-red-700';
    }
  };

  const getStatusText = () => {
    switch (status) {
      case 'connected':    return '● Connected to Target';
      case 'connecting':   return '⟳ Connecting…';
      case 'reconnecting': return '⟳ Reconnecting…';
      case 'disconnected': return '○ No Connection';
    }
  };

  return (
    <div className="bg-gray-900 border border-red-800 rounded-xl shadow-lg shadow-red-950/50 p-6">
      <h2 className="text-sm font-semibold text-red-300 uppercase tracking-widest mb-4">
        Target Connection
      </h2>
      <div className="space-y-3">
        <span className={`inline-flex items-center px-3 py-1 rounded-full text-sm font-medium ${getStatusStyle()}`}>
          {getStatusText()}
        </span>
        <p className="text-sm text-white truncate font-mono" title={wsUrl}>
          {wsUrl}
        </p>
        {lastError && (
          <p className="text-sm text-red-300 truncate" title={lastError}>
            {lastError}
          </p>
        )}
      </div>
    </div>
  );
}
