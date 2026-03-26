//Author: Krish Shah
//Reviewed by: Sai Raparla
export type Direction = 'forward' | 'back' | 'left' | 'right';

export interface DirectionMsg {
  direction: Direction;
}

// Map keyboard keys to directions (WASD, case-insensitive)
export const keyToDirection: Record<string, Direction> = {
  w: 'forward', W: 'forward',
  s: 'back', S: 'back',
  a: 'left', A: 'left',
  d: 'right', D: 'right',
};

// Check if a key is a direction key
export function isDirectionKey(key: string): key is keyof typeof keyToDirection {
  return key in keyToDirection;
}

// Build a direction message payload (legacy)
export function buildDirectionMsg(direction: Direction): DirectionMsg {
  return { direction };
}

// Map a set of directions to Control (C) axes (supports simultaneous directions)
export function directionsToControlAxes(directions: Set<Direction>): {
  forward: 0 | 1;
  backward: 0 | 1;
  left: 0 | 1;
  right: 0 | 1;
} {
  return {
    forward:  directions.has('forward') ? 1 : 0,
    backward: directions.has('back')    ? 1 : 0,
    left:     directions.has('left')    ? 1 : 0,
    right:    directions.has('right')   ? 1 : 0,
  } as { forward: 0 | 1; backward: 0 | 1; left: 0 | 1; right: 0 | 1 };
}

// Format direction for display
export function formatDirection(directions: Set<Direction>): string {
  if (directions.size === 0) return 'Idle';
  return `Direction: ${[...directions].join(' + ')}`;
}

