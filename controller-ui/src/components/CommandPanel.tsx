//Author: Krish Shah
//Reviewed by: Sai Raparla

interface CommandPanelProps {
  controlSpeed: number;
  onControlSpeedChange: (speed: number) => void;
}

export function CommandPanel({ controlSpeed, onControlSpeedChange }: CommandPanelProps) {
  return (
    <div className="bg-white rounded-xl shadow-lg p-6">
      <h2 className="text-lg font-semibold text-gray-700">Command Panel</h2>

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
