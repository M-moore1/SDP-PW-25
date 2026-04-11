//Author: Krish Shah
//Reviewed by: Sai Raparla

/** Control (C) - 0b00001, 50ms rate */
export interface ControlMsg {
  T: 'C';
  F: 0 | 1;
  B: 0 | 1;
  L: 0 | 1;
  R: 0 | 1;
  S: number;
  PL: 1;
  ID: 1;
}

/** Pose (P) - 0b00010, Event Driven */
export interface PoseMsg {
  T: 'P';
  action: string[];
  PL: 1;
  ID: number;
}

/** System (S) - 0b00011, Event Driven */
export interface SystemMsg {
  T: 'S';
  instruction: number;
  Authorization_Code: number;
  instruction_specific: number;
  PL: number;
  ID: number;
}

/** Query (Q) - 0b00100, Event Driven */
export interface QueryMsg {
  T: 'Q';
  request_item: string;
  Report: 'On' | 'Off';
  PL: number;
  ID: number;
}

/** Arm Control (A) - 0b00101, 50ms rate */
export interface ArmControlMsg {
  T: 'A';
  U: 0 | 1;
  D: 0 | 1;
  L: 0 | 1;
  R: 0 | 1;
  In: 0 | 1;
  O: 0 | 1;
  S: number;
  Re: 0 | 1;
  PL: 1;
  ID: 1;
}

export type CommandMsg = ControlMsg | ArmControlMsg | PoseMsg | SystemMsg | QueryMsg;

export function buildArmControlMsg(
  U: 0 | 1,
  D: 0 | 1,
  L: 0 | 1,
  R: 0 | 1,
  In: 0 | 1,
  O: 0 | 1,
  S: number,
  Re: 0 | 1 = 0
): ArmControlMsg {
  return {
    T: 'A',
    U,
    D,
    L,
    R,
    In,
    O,
    S: Math.min(100, Math.max(0, S)),
    Re,
    PL: 1,
    ID: 1,
  };
}

export function buildControlMsg(
  F: 0 | 1,
  B: 0 | 1,
  L: 0 | 1,
  R: 0 | 1,
  S: number
): ControlMsg {
  return {
    T: 'C',
    F,
    B,
    L,
    R,
    S: Math.min(100, Math.max(0, S)),
    PL: 1,
    ID: 1,
  };
}
