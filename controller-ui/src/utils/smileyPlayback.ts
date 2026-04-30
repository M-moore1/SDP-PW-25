import smileyRaw from '../data/smiley_v1.txt?raw';
import type { ArmControlMsg } from './commands';

const TIMESTAMP_PREFIX = /^\[[^\]]*\]\s*/;

export function parseSmileyCommands(raw: string): ArmControlMsg[] {
  const commands: ArmControlMsg[] = [];
  for (const line of raw.split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed) continue;
    const jsonPart = trimmed.replace(TIMESTAMP_PREFIX, '');
    try {
      const parsed = JSON.parse(jsonPart) as ArmControlMsg;
      if (parsed && parsed.T === 'A') {
        commands.push(parsed);
      }
    } catch {
      // skip malformed lines
    }
  }
  return commands;
}

export const SMILEY_COMMANDS: ArmControlMsg[] = parseSmileyCommands(smileyRaw);

export const SMILEY_STEP_DELAY_MS = 150;
