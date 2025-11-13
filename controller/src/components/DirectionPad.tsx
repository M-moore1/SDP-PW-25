import { Direction } from '../utils/direction';

interface DirectionPadProps {
  onDirectionStart: (direction: Direction) => void;
  onDirectionStop: () => void;
  activeDirection: Direction | null;
}

/**
 * On-screen direction pad with four arrow buttons.
 * Supports mouse and touch events with hold-to-repeat behavior.
 */
export function DirectionPad({ onDirectionStart, onDirectionStop, activeDirection }: DirectionPadProps) {
  // Handle pointer down (mouse or touch)
  const handlePointerDown = (direction: Direction) => (e: React.PointerEvent) => {
    e.preventDefault();
    onDirectionStart(direction);
  };

  // Handle pointer up/leave
  const handlePointerUpOrLeave = (e: React.PointerEvent) => {
    e.preventDefault();
    onDirectionStop();
  };

  // Button styling with active state
  const getButtonClass = (direction: Direction) => {
    const baseClass = 'w-20 h-20 text-3xl font-bold rounded-lg transition-all duration-150 touch-none select-none focus:outline-none focus:ring-4 focus:ring-blue-300';
    const isActive = activeDirection === direction;
    
    if (isActive) {
      return `${baseClass} bg-blue-600 text-white shadow-lg scale-95`;
    }
    return `${baseClass} bg-gray-200 text-gray-700 hover:bg-gray-300 active:bg-blue-500 active:text-white shadow-md`;
  };

  return (
    <div className="flex flex-col items-center gap-2 p-4 bg-white rounded-xl shadow-lg">
      <h2 className="text-lg font-semibold text-gray-700 mb-2">On-Screen Controls</h2>
      
      {/* Grid layout for arrow buttons */}
      <div className="grid grid-cols-3 gap-2 place-items-center">
        {/* Top row - Forward */}
        <div className="col-start-2">
          <button
            onPointerDown={handlePointerDown('forward')}
            onPointerUp={handlePointerUpOrLeave}
            onPointerLeave={handlePointerUpOrLeave}
            className={getButtonClass('forward')}
            aria-label="Forward"
            type="button"
          >
            ↑
          </button>
        </div>

        {/* Middle row - Left and Right */}
        <button
          onPointerDown={handlePointerDown('left')}
          onPointerUp={handlePointerUpOrLeave}
          onPointerLeave={handlePointerUpOrLeave}
          className={getButtonClass('left')}
          aria-label="Left"
          type="button"
        >
          ←
        </button>
        
        <div /> {/* Empty space in center */}
        
        <button
          onPointerDown={handlePointerDown('right')}
          onPointerUp={handlePointerUpOrLeave}
          onPointerLeave={handlePointerUpOrLeave}
          className={getButtonClass('right')}
          aria-label="Right"
          type="button"
        >
          →
        </button>

        {/* Bottom row - Back */}
        <div className="col-start-2">
          <button
            onPointerDown={handlePointerDown('back')}
            onPointerUp={handlePointerUpOrLeave}
            onPointerLeave={handlePointerUpOrLeave}
            className={getButtonClass('back')}
            aria-label="Back"
            type="button"
          >
            ↓
          </button>
        </div>
      </div>

      <p className="text-sm text-gray-500 mt-2">
        Press and hold to repeat
      </p>
    </div>
  );
}

