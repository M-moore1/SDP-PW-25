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

// Map direction to Control (C) format for buildControlMsg
export function directionToControlAxes(direction: Direction): {
  forward: 0 | 1;
  backward: 0 | 1;
  left: 0 | 1;
  right: 0 | 1;
} {
  switch (direction) {
    case 'forward':
      return { forward: 1, backward: 0, left: 0, right: 0 };
    case 'back':
      return { forward: 0, backward: 1, left: 0, right: 0 };
    case 'left':
      return { forward: 0, backward: 0, left: 1, right: 0 };
    case 'right':
      return { forward: 0, backward: 0, left: 0, right: 1 };
  }
}

// Format direction for display
export function formatDirection(direction: Direction | null): string {
  if (!direction) return 'Idle';
  return `Direction: ${direction}`;
}

