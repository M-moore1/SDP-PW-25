//Author: Krish Shah
//Reviewed by: Sai Raparla

/** Control (C) - 0b00001, 50ms rate */
export interface ControlMsg {
  type: 'C';
  forward: 0 | 1;
  backward: 0 | 1;
  left: 0 | 1;
  right: 0 | 1;
  speed: number;
  priority_level: 1;
}

/** Pose (P) - 0b00010, Event Driven */
export interface PoseMsg {
  type: 'P';
  action: string[];
  priority_level: 1;
  id: number;
}

/** System (S) - 0b00011, Event Driven */
export interface SystemMsg {
  type: 'S';
  instruction: string[];
  Authurization_Code: number;
  Instruction_Specfic: string[];
  priority_level: number;
  id: number;
}

/** Query (Q) - 0b00100, Event Driven */
export interface QueryMsg {
  type: 'Q';
  request_item: string;
  Report: 'On' | 'Off';
  priority_level: number;
  id: number;
}

export type CommandMsg = ControlMsg | PoseMsg | SystemMsg | QueryMsg;

export function buildControlMsg(
  forward: 0 | 1,
  backward: 0 | 1,
  left: 0 | 1,
  right: 0 | 1,
  speed: number
): ControlMsg {
  return {
    type: 'C',
    forward,
    backward,
    left,
    right,
    speed: Math.min(100, Math.max(0, speed)),
    priority_level: 1,
  };
}
