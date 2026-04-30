//Author: Krish Shah
//Reviewed by: Sai Raparla

interface MessageLogProps {
  logs: { payload: string; timestamp: string }[];
  onDownload?: () => void;
  onClear?: () => void;
  onChooseLiveFile?: () => void;
  onStopLiveFile?: () => void;
  liveLogFileName?: string | null;
}

export function MessageLog({
  logs,
  onDownload,
  onClear,
  onChooseLiveFile,
  onStopLiveFile,
  liveLogFileName,
}: MessageLogProps) {
  return (
    <div className="bg-white rounded-xl shadow-lg p-6 min-w-0 flex flex-col dark:bg-slate-800 dark:shadow-slate-900/50">
      <div className="flex items-center justify-between gap-2 mb-2 flex-wrap">
        <h2 className="text-lg font-semibold text-gray-700 dark:text-gray-200">Log</h2>
        <div className="flex gap-2 flex-wrap">
          {onChooseLiveFile && !liveLogFileName && (
            <button
              type="button"
              onClick={onChooseLiveFile}
              className="text-xs px-2 py-1 rounded-md bg-blue-100 text-blue-700 hover:bg-blue-200 dark:bg-blue-900/40 dark:text-blue-300 dark:hover:bg-blue-900/60"
              title="Pick a local file (e.g. log.txt) and have every command appended live"
            >
              Set live log file
            </button>
          )}
          {onStopLiveFile && liveLogFileName && (
            <button
              type="button"
              onClick={onStopLiveFile}
              className="text-xs px-2 py-1 rounded-md bg-amber-100 text-amber-700 hover:bg-amber-200 dark:bg-amber-900/40 dark:text-amber-300 dark:hover:bg-amber-900/60"
              title={`Stop appending to ${liveLogFileName}`}
            >
              Stop live log
            </button>
          )}
          {onDownload && (
            <button
              type="button"
              onClick={onDownload}
              className="text-xs px-2 py-1 rounded-md bg-gray-100 text-gray-700 hover:bg-gray-200 dark:bg-slate-700 dark:text-gray-200 dark:hover:bg-slate-600"
            >
              Download log.txt
            </button>
          )}
          {onClear && (
            <button
              type="button"
              onClick={onClear}
              className="text-xs px-2 py-1 rounded-md bg-gray-100 text-gray-700 hover:bg-gray-200 dark:bg-slate-700 dark:text-gray-200 dark:hover:bg-slate-600"
            >
              Clear
            </button>
          )}
        </div>
      </div>
      {liveLogFileName && (
        <p className="text-xs text-green-600 dark:text-green-400 mb-1">
          ● Streaming to {liveLogFileName}
        </p>
      )}
      {logs.length === 0 ? (
        <p className="text-xs text-gray-400 dark:text-gray-500">No messages</p>
      ) : (
        <div className="space-y-1 overflow-y-auto flex-1">
          {logs.map((log, idx) => (
            <div key={idx} className="flex gap-2 text-xs items-start">
              <span className="text-gray-400 shrink-0 dark:text-gray-500">{log.timestamp}</span>
              <code className="text-gray-700 font-mono break-all flex-1 min-w-0 dark:text-gray-200">{log.payload}</code>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
