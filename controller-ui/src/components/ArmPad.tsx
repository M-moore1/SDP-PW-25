import { ArmAction } from '../utils/armDirection';

interface ArmPadProps {
  onActionStart: (action: ArmAction) => void;
  onActionStop: (action: ArmAction) => void;
  activeArmActions: Set<ArmAction>;
  armSpeed: number;
  onArmSpeedChange: (speed: number) => void;
  onReset: () => void;
  onPlaySmiley: () => void;
  isPlayingSmiley: boolean;
}

export function ArmPad({
  onActionStart,
  onActionStop,
  activeArmActions,
  armSpeed,
  onArmSpeedChange,
  onReset,
  onPlaySmiley,
  isPlayingSmiley,
}: ArmPadProps) {
  const handlePointerDown = (action: ArmAction) => (e: React.PointerEvent) => {
    e.preventDefault();
    onActionStart(action);
  };

  const handlePointerUpOrLeave = (action: ArmAction) => (e: React.PointerEvent) => {
    e.preventDefault();
    onActionStop(action);
  };

  const getDirButtonClass = (action: ArmAction) => {
    const base =
      'w-16 h-16 text-xl font-bold rounded-lg transition-all duration-150 touch-none select-none focus:outline-none focus:ring-4 focus:ring-emerald-300 dark:focus:ring-emerald-500 dark:focus:ring-offset-2 dark:focus:ring-offset-slate-800';
    if (activeArmActions.has(action)) {
      return `${base} bg-emerald-600 text-white shadow-lg scale-95 dark:bg-emerald-500`;
    }
    return `${base} bg-gray-200 text-gray-700 hover:bg-gray-300 active:bg-emerald-500 active:text-white shadow-md dark:bg-slate-600 dark:text-gray-100 dark:hover:bg-slate-500 dark:active:bg-emerald-500`;
  };

  const getAuxButtonClass = (action: ArmAction) => {
    const base =
      'flex-1 h-14 text-sm font-bold rounded-lg transition-all duration-150 touch-none select-none focus:outline-none focus:ring-4 focus:ring-emerald-300 dark:focus:ring-emerald-500 dark:focus:ring-offset-2 dark:focus:ring-offset-slate-800';
    if (activeArmActions.has(action)) {
      return `${base} bg-emerald-600 text-white shadow-lg scale-95 dark:bg-emerald-500`;
    }
    return `${base} bg-gray-200 text-gray-700 hover:bg-gray-300 active:bg-emerald-500 active:text-white shadow-md dark:bg-slate-600 dark:text-gray-100 dark:hover:bg-slate-500 dark:active:bg-emerald-500`;
  };

  return (
    <div className="flex flex-col items-center gap-3 p-6 px-12 bg-white rounded-xl shadow-lg h-full dark:bg-slate-800 dark:shadow-slate-900/50">
      <h2 className="text-lg font-semibold text-gray-700 mb-1 dark:text-gray-200">Arm Controls</h2>

      {/* Arrow-cross layout */}
      <div className="flex flex-col items-center gap-2">
        <button
          onPointerDown={handlePointerDown('up')}
          onPointerUp={handlePointerUpOrLeave('up')}
          onPointerLeave={handlePointerUpOrLeave('up')}
          className={getDirButtonClass('up')}
          aria-label="Arm Up"
          type="button"
        >
          &#9650;
        </button>

        <div className="flex gap-2">
          <button
            onPointerDown={handlePointerDown('left')}
            onPointerUp={handlePointerUpOrLeave('left')}
            onPointerLeave={handlePointerUpOrLeave('left')}
            className={getDirButtonClass('left')}
            aria-label="Arm Left"
            type="button"
          >
            &#9664;
          </button>
          <button
            onPointerDown={handlePointerDown('right')}
            onPointerUp={handlePointerUpOrLeave('right')}
            onPointerLeave={handlePointerUpOrLeave('right')}
            className={getDirButtonClass('right')}
            aria-label="Arm Right"
            type="button"
          >
            &#9654;
          </button>
        </div>

        <button
          onPointerDown={handlePointerDown('down')}
          onPointerUp={handlePointerUpOrLeave('down')}
          onPointerLeave={handlePointerUpOrLeave('down')}
          className={getDirButtonClass('down')}
          aria-label="Arm Down"
          type="button"
        >
          &#9660;
        </button>
      </div>

      {/* In / Out buttons */}
      <div className="flex gap-2 w-full max-w-[10rem]">
        <button
          onPointerDown={handlePointerDown('in')}
          onPointerUp={handlePointerUpOrLeave('in')}
          onPointerLeave={handlePointerUpOrLeave('in')}
          className={getAuxButtonClass('in')}
          aria-label="Arm In"
          type="button"
        >
          In<span className="block text-[10px] font-normal opacity-70">Shift</span>
        </button>
        <button
          onPointerDown={handlePointerDown('out')}
          onPointerUp={handlePointerUpOrLeave('out')}
          onPointerLeave={handlePointerUpOrLeave('out')}
          className={getAuxButtonClass('out')}
          aria-label="Arm Out"
          type="button"
        >
          Out<span className="block text-[10px] font-normal opacity-70">Enter / Caps</span>
        </button>
      </div>

      {/* Speed slider */}
      <div className="w-full max-w-[12rem] mt-2">
        <label className="flex items-center justify-between text-sm text-gray-600 mb-1 dark:text-gray-300">
          <span>Arm Speed</span>
          <span className="font-mono font-semibold">{armSpeed}</span>
        </label>
        <input
          type="range"
          min={0}
          max={100}
          value={armSpeed}
          onChange={(e) => onArmSpeedChange(Number(e.target.value))}
          className="w-full accent-emerald-600"
        />
      </div>

      {/* Reset button */}
      <button
        onClick={onReset}
        type="button"
        className="mt-1 px-4 py-2 text-sm font-semibold rounded-lg bg-red-100 text-red-700 hover:bg-red-200 active:bg-red-300 transition-colors focus:outline-none focus:ring-4 focus:ring-red-300 dark:bg-red-950/50 dark:text-red-300 dark:hover:bg-red-900/50 dark:active:bg-red-900 dark:focus:ring-red-500 dark:focus:ring-offset-2 dark:focus:ring-offset-slate-800"
      >
        Reset Arm
      </button>

      {/* Smiley playback button */}
      <button
        onClick={onPlaySmiley}
        disabled={isPlayingSmiley}
        type="button"
        aria-label="Play smiley routine"
        className="mt-1 px-4 py-2 text-sm font-semibold rounded-lg bg-yellow-100 text-yellow-800 hover:bg-yellow-200 active:bg-yellow-300 disabled:opacity-60 disabled:cursor-not-allowed transition-colors focus:outline-none focus:ring-4 focus:ring-yellow-300 dark:bg-yellow-900/40 dark:text-yellow-200 dark:hover:bg-yellow-900/60 dark:focus:ring-yellow-500 dark:focus:ring-offset-2 dark:focus:ring-offset-slate-800"
      >
        {isPlayingSmiley ? 'Playing\u2026' : 'Smiley :)'}
      </button>

    </div>
  );
}
