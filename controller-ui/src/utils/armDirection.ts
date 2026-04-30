export type ArmAction = 'up' | 'down' | 'left' | 'right' | 'in' | 'out';

export const keyToArmAction: Record<string, ArmAction> = {
  ArrowUp: 'up',
  ArrowDown: 'down',
  ArrowLeft: 'left',
  ArrowRight: 'right',
  Shift: 'in',
  Enter: 'out',
  CapsLock: 'out',
};

export function isArmKey(key: string): key is keyof typeof keyToArmAction {
  return key in keyToArmAction;
}

export function armActionsToAxes(actions: Set<ArmAction>): {
  U: 0 | 1;
  D: 0 | 1;
  L: 0 | 1;
  R: 0 | 1;
  In: 0 | 1;
  O: 0 | 1;
} {
  return {
    U:  actions.has('up')    ? 1 : 0,
    D:  actions.has('down')  ? 1 : 0,
    L:  actions.has('left')  ? 1 : 0,
    R:  actions.has('right') ? 1 : 0,
    In: actions.has('in')    ? 1 : 0,
    O:  actions.has('out')   ? 1 : 0,
  } as { U: 0 | 1; D: 0 | 1; L: 0 | 1; R: 0 | 1; In: 0 | 1; O: 0 | 1 };
}

export function formatArmAction(actions: Set<ArmAction>): string {
  if (actions.size === 0) return 'Idle';
  return `Arm: ${[...actions].join(' + ')}`;
}
