//Author: Krish Shah
//Reviewed by: Sai Raparla

type ConnectionStatus = 'connected' | 'connecting' | 'reconnecting' | 'disconnected';

interface StatusIndicatorProps {
  status: ConnectionStatus;
  activeDirection: string;
  lastError: string | null;
  wsUrl: string;
}

export function StatusIndicator({ status, activeDirection, lastError, wsUrl }: StatusIndicatorProps) {
  const getStatusColor = () => {
    switch (status) {
      case 'connected':
        return 'text-green-600 bg-green-100';
      case 'connecting':
        return 'text-yellow-600 bg-yellow-100';
      case 'reconnecting':
        return 'text-orange-600 bg-orange-100';
      case 'disconnected':
        return 'text-red-600 bg-red-100';
    }
  };

  const getStatusText = () => {
    switch (status) {
      case 'connected':
        return '● Connected';
      case 'connecting':
        return '⟳ Connecting…';
      case 'reconnecting':
        return '⟳ Reconnecting…';
      case 'disconnected':
        return '○ Disconnected';
    }
  };

  return (
    <div className="bg-white rounded-xl shadow-lg p-4">
      <h2 className="text-sm font-semibold text-gray-600 mb-2">Server Connection Status</h2>
      <div className="space-y-2">
        <span className={`inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium ${getStatusColor()}`}>
          {getStatusText()}
        </span>
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
    </div>
  );
}
