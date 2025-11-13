// Type definitions and direction utilities

export type Direction = 'forward' | 'back' | 'left' | 'right';

export interface DirectionMsg {
  direction: Direction;
}

// Map keyboard keys to directions
export const keyToDirection: Record<string, Direction> = {
  ArrowUp: 'forward',
  ArrowDown: 'back',
  ArrowLeft: 'left',
  ArrowRight: 'right',
};

// Check if a key is a direction key
export function isDirectionKey(key: string): key is keyof typeof keyToDirection {
  return key in keyToDirection;
}

// Build a direction message payload
export function buildDirectionMsg(direction: Direction): DirectionMsg {
  return { direction };
}

// Format direction for display
export function formatDirection(direction: Direction | null): string {
  if (!direction) return 'Idle';
  return `Direction: ${direction}`;
}

