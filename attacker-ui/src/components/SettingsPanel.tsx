import { useEffect, useState } from 'react';

export interface Preset {
  label: string;
  key: string;
  payload: string;
}

interface SettingsPanelProps {
  presets: Preset[];
  onPresetsChange: (presets: Preset[]) => void;
}

function isValidHex(value: string): boolean {
  const clean = value.replace(/\s+/g, '');
  return clean.length > 0 && clean.length % 2 === 0 && /^[0-9a-fA-F]+$/.test(clean);
}

export function SettingsPanel({ presets, onPresetsChange }: SettingsPanelProps) {
  const [draft, setDraft] = useState<Preset[]>(presets);
  const [saved, setSaved] = useState(false);

  useEffect(() => {
    setDraft(presets);
  }, [presets]);

  const updateField = (idx: number, field: 'label' | 'payload', value: string) => {
    const next = draft.map((p, i) => (i === idx ? { ...p, [field]: value } : p));
    setDraft(next);
    setSaved(false);
  };

  const allValid = draft.every(
    (p) => (!p.label.trim() && !p.payload.trim()) || (p.label.trim() && isValidHex(p.payload)),
  );

  const handleSave = () => {
    if (!allValid) return;
    onPresetsChange(draft);
    setSaved(true);
    setTimeout(() => setSaved(false), 1500);
  };

  return (
    <div className="bg-gray-900 border border-red-800 rounded-xl shadow-lg shadow-red-950/50 p-6">
      <h2 className="text-sm font-semibold text-red-300 uppercase tracking-widest mb-4">
        Button Presets
      </h2>

      <div className="space-y-4">
        {draft.map((preset, idx) => {
          const payloadOk = isValidHex(preset.payload) || !preset.payload.trim();
          return (
            <div key={preset.key} className="space-y-2">
              <div className="flex items-center gap-2">
                <span className="w-8 h-8 flex items-center justify-center rounded bg-gray-800 border border-gray-700 text-white text-sm font-bold uppercase">
                  {preset.key}
                </span>
                <input
                  type="text"
                  value={preset.label}
                  onChange={(e) => updateField(idx, 'label', e.target.value)}
                  placeholder="Label"
                  className="flex-1 bg-gray-800 border border-gray-700 focus:border-red-600 text-white text-sm rounded-lg px-3 py-2 outline-none transition-colors placeholder-gray-600"
                />
              </div>
              <input
                type="text"
                value={preset.payload}
                onChange={(e) => updateField(idx, 'payload', e.target.value)}
                placeholder="hex bytes, e.g. 85 90 01 00 00 00 00 00"
                className={`w-full bg-gray-800 border text-gray-200 text-sm font-mono rounded-lg px-3 py-2 outline-none transition-colors placeholder-gray-600 ${
                  !payloadOk
                    ? 'border-yellow-500 focus:border-yellow-400'
                    : 'border-gray-700 focus:border-red-600'
                }`}
              />
              {!payloadOk && (
                <p className="text-yellow-400 text-xs">
                  Enter an even number of hex characters (0-9, A-F). Spaces are ignored.
                </p>
              )}
            </div>
          );
        })}
      </div>

      <div className="mt-6">
        <button
          onClick={handleSave}
          disabled={!allValid}
          className={`w-full py-3 text-base font-bold rounded-lg transition-all duration-200 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-gray-900 disabled:opacity-40 disabled:cursor-not-allowed ${
            saved
              ? 'bg-green-600 text-white ring-2 ring-green-400 scale-[1.02]'
              : 'bg-red-700 hover:bg-red-600 active:scale-95 text-white focus:ring-red-500'
          }`}
        >
          {saved ? 'Saved!' : 'Save Presets'}
        </button>
      </div>
    </div>
  );
}
