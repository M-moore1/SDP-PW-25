//Author: Krish Shah
//Reviewed by: Sai Raparla

interface MessageLogProps {
  logs: { payload: string; timestamp: string }[];
}

export function MessageLog({ logs }: MessageLogProps) {
  return (
    <div className="bg-white rounded-xl shadow-lg p-6 min-w-0 flex flex-col">
      <h2 className="text-lg font-semibold text-gray-700">Log</h2>
      {logs.length === 0 ? (
        <p className="text-xs text-gray-400">No messages</p>
      ) : (
        <div className="space-y-1 overflow-y-auto flex-1">
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
