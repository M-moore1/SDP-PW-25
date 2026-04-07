//Author: Krish Shah
//Reviewed by: Sai Raparla
import { Direction } from '../utils/direction';

interface DirectionPadProps {
  onDirectionStart: (direction: Direction) => void;
  onDirectionStop: (direction: Direction) => void;
  activeDirections: Set<Direction>;
  controlSpeed: number;
  onControlSpeedChange: (speed: number) => void;
  encryptionEnabled: boolean;
  onEncryptionChange: (enabled: boolean) => void;
  hasEncryptionKey: boolean;
}

/**
 * On-screen direction pad with W/A/S/D buttons.
 * Supports mouse and touch events with hold-to-repeat behavior.
 */
export function DirectionPad({ onDirectionStart, onDirectionStop, activeDirections, controlSpeed, onControlSpeedChange, encryptionEnabled, onEncryptionChange, hasEncryptionKey }: DirectionPadProps) {
  // Handle pointer down (mouse or touch)
  const handlePointerDown = (direction: Direction) => (e: React.PointerEvent) => {
    e.preventDefault();
    onDirectionStart(direction);
  };

  // Handle pointer up/leave
  const handlePointerUpOrLeave = (direction: Direction) => (e: React.PointerEvent) => {
    e.preventDefault();
    onDirectionStop(direction);
  };

  // Button styling with active state
  const getButtonClass = (direction: Direction) => {
    const baseClass =
      'w-20 h-20 text-3xl font-bold rounded-lg transition-all duration-150 touch-none select-none focus:outline-none focus:ring-4 focus:ring-blue-300 dark:focus:ring-blue-500 dark:focus:ring-offset-2 dark:focus:ring-offset-slate-800';
    const isActive = activeDirections.has(direction);

    if (isActive) {
      return `${baseClass} bg-blue-600 text-white shadow-lg scale-95 dark:bg-blue-500`;
    }
    return `${baseClass} bg-gray-200 text-gray-700 hover:bg-gray-300 active:bg-blue-500 active:text-white shadow-md dark:bg-slate-600 dark:text-gray-100 dark:hover:bg-slate-500 dark:active:bg-blue-500`;
  };

  return (
    <div className="flex flex-col items-center gap-2 p-6 px-12 bg-white rounded-xl shadow-lg h-full dark:bg-slate-800 dark:shadow-slate-900/50">
      <h2 className="text-lg font-semibold text-gray-700 mb-2 dark:text-gray-200">Movement Controls</h2>
      
      {/* WASD layout */}
      <div className="flex flex-col items-center gap-2">
        {/* Top row - W */}
        <button
          onPointerDown={handlePointerDown('forward')}
          onPointerUp={handlePointerUpOrLeave('forward')}
          onPointerLeave={handlePointerUpOrLeave('forward')}
          className={getButtonClass('forward')}
          aria-label="Forward"
          type="button"
        >
          W
        </button>

        {/* Middle row - A and D */}
        <div className="flex gap-2">
          <button
            onPointerDown={handlePointerDown('left')}
            onPointerUp={handlePointerUpOrLeave('left')}
            onPointerLeave={handlePointerUpOrLeave('left')}
            className={getButtonClass('left')}
            aria-label="Left"
            type="button"
          >
            A
          </button>

          <button
            onPointerDown={handlePointerDown('right')}
            onPointerUp={handlePointerUpOrLeave('right')}
            onPointerLeave={handlePointerUpOrLeave('right')}
            className={getButtonClass('right')}
            aria-label="Right"
            type="button"
          >
            D
          </button>
        </div>

        {/* Bottom row - S */}
        <button
          onPointerDown={handlePointerDown('back')}
          onPointerUp={handlePointerUpOrLeave('back')}
          onPointerLeave={handlePointerUpOrLeave('back')}
          className={getButtonClass('back')}
          aria-label="Back"
          type="button"
        >
          S
        </button>
      </div>

      {/* Speed slider */}
      <div className="w-full max-w-[12rem] mt-2">
        <label className="flex items-center justify-between text-sm text-gray-600 mb-1 dark:text-gray-300">
          <span>Drive Speed</span>
          <span className="font-mono font-semibold">{controlSpeed}</span>
        </label>
        <input
          type="range"
          min={0}
          max={100}
          value={controlSpeed}
          onChange={(e) => onControlSpeedChange(Number(e.target.value))}
          className="w-full accent-blue-600"
        />
      </div>

      {/* Encryption toggle */}
      <div className="w-full max-w-[12rem] mt-2 flex items-center justify-between">
        <span className="text-sm text-gray-600 dark:text-gray-300">Encryption</span>
        <button
          type="button"
          role="switch"
          aria-checked={encryptionEnabled}
          disabled={!hasEncryptionKey}
          onClick={() => hasEncryptionKey && onEncryptionChange(!encryptionEnabled)}
          className={`relative inline-flex h-6 w-11 shrink-0 cursor-pointer rounded-full border-2 border-transparent transition-colors focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 dark:focus:ring-offset-slate-800 disabled:cursor-not-allowed disabled:opacity-50 ${
            encryptionEnabled ? 'bg-blue-600' : 'bg-gray-200 dark:bg-slate-600'
          }`}
        >
          <span
            className={`pointer-events-none inline-block h-5 w-5 transform rounded-full bg-white shadow ring-0 transition ${
              encryptionEnabled ? 'translate-x-5' : 'translate-x-1'
            }`}
          />
        </button>
      </div>
    </div>
  );
}

