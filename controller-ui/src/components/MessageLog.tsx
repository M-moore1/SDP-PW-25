//Author: Krish Shah
//Reviewed by: Sai Raparla

interface MessageLogProps {
  logs: { payload: string; timestamp: string }[];
}

export function MessageLog({ logs }: MessageLogProps) {
  return (
    <div className="bg-white rounded-xl shadow-lg p-4 min-w-0">
      <h2 className="text-sm font-semibold text-gray-600 mb-2">Log</h2>
      {logs.length === 0 ? (
        <p className="text-xs text-gray-400">No messages</p>
      ) : (
        <div className="space-y-1 max-h-48 overflow-y-auto">
          {logs.map((log, idx) => (
            <div key={idx} className="flex gap-2 text-xs items-start">
              <span className="text-gray-400 shrink-0">{log.timestamp}</span>
              <code className="text-gray-700 font-mono break-all flex-1 min-w-0">{log.payload}</code>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
