// Utilities for accumulating sent commands and persisting them to a local
// `log.txt` file from the browser.
//
// Two persistence paths are supported:
//   1. File System Access API (Chromium) — pick `log.txt` once, then every
//      command is appended to that handle live.
//   2. Anywhere else — accumulate in memory and download on demand.

const LOG_FILENAME = 'log.txt';

export interface LogEntry {
  timestamp: string; // ISO 8601
  payload: string;
}

export function formatLogLine(entry: LogEntry): string {
  return `[${entry.timestamp}] ${entry.payload}\n`;
}

export function formatLogFile(entries: LogEntry[]): string {
  return entries.map(formatLogLine).join('');
}

export function downloadLog(entries: LogEntry[], filename: string = LOG_FILENAME): void {
  const blob = new Blob([formatLogFile(entries)], { type: 'text/plain;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  // Defer revoke so the browser has time to start the download
  setTimeout(() => URL.revokeObjectURL(url), 0);
}

// ---- File System Access API live-streaming -----------------------------

// Minimal type shim: TS DOM lib doesn't yet ship `showSaveFilePicker`.
type FileSystemWritableFileStream = {
  write: (data: BufferSource | Blob | string) => Promise<void>;
  close: () => Promise<void>;
  seek: (position: number) => Promise<void>;
};

type FileSystemFileHandle = {
  createWritable: (opts?: { keepExistingData?: boolean }) => Promise<FileSystemWritableFileStream>;
  getFile: () => Promise<File>;
  name: string;
};

interface SaveFilePickerOptions {
  suggestedName?: string;
  types?: { description?: string; accept: Record<string, string[]> }[];
}

type ShowSaveFilePicker = (opts?: SaveFilePickerOptions) => Promise<FileSystemFileHandle>;

export function isFileSystemAccessSupported(): boolean {
  return typeof window !== 'undefined' && typeof (window as unknown as { showSaveFilePicker?: ShowSaveFilePicker }).showSaveFilePicker === 'function';
}

let fileHandle: FileSystemFileHandle | null = null;
let fileSize = 0;
let writeChain: Promise<void> = Promise.resolve();

export async function chooseLogFile(): Promise<boolean> {
  if (!isFileSystemAccessSupported()) return false;
  const picker = (window as unknown as { showSaveFilePicker: ShowSaveFilePicker }).showSaveFilePicker;
  try {
    const handle = await picker({
      suggestedName: LOG_FILENAME,
      types: [{ description: 'Log file', accept: { 'text/plain': ['.txt', '.log'] } }],
    });
    fileHandle = handle;
    const file = await handle.getFile();
    fileSize = file.size;
    return true;
  } catch {
    // User cancelled or permission denied
    return false;
  }
}

export function hasLiveLogFile(): boolean {
  return fileHandle !== null;
}

export function getLiveLogFileName(): string | null {
  return fileHandle?.name ?? null;
}

export function clearLiveLogFile(): void {
  fileHandle = null;
  fileSize = 0;
}

// Append a single line to the live file, if one was selected. Writes are
// serialised through `writeChain` so concurrent calls don't trample each
// other (each `createWritable` truncates by default, so we must seek to the
// end and write incrementally).
export function appendToLiveLog(entry: LogEntry): void {
  if (!fileHandle) return;
  const handle = fileHandle;
  const line = formatLogLine(entry);

  writeChain = writeChain
    .then(async () => {
      const writable = await handle.createWritable({ keepExistingData: true });
      try {
        await writable.seek(fileSize);
        await writable.write(line);
        fileSize += new Blob([line]).size;
      } finally {
        await writable.close();
      }
    })
    .catch((err) => {
      console.warn('Failed to append to live log file:', err);
      // If the handle becomes invalid (e.g. file deleted), drop it so the
      // user can re-pick.
      fileHandle = null;
      fileSize = 0;
    });
}
