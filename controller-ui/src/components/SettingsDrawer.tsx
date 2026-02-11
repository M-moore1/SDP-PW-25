//Author: Krish Shah
//Reviewed by: Sai Raparla
import { useState } from 'react';

interface SettingsDrawerProps {
  repeatRate: number;
  onRepeatRateChange: (rate: number) => void;
}

/**
 * Settings drawer for adjusting the repeat rate
 */
export function SettingsDrawer({ repeatRate, onRepeatRateChange }: SettingsDrawerProps) {
  const [isOpen, setIsOpen] = useState(false);

  const presets = [
    { label: 'Fast (250ms)', value: 250 },
    { label: 'Normal (1000ms)', value: 1000 },
    { label: 'Slow (2000ms)', value: 2000 },
  ];

  return (
    <>
      {/* Settings Button */}
      <button
        onClick={() => setIsOpen(!isOpen)}
        className="fixed bottom-4 right-4 bg-gray-800 text-white p-3 rounded-full shadow-lg hover:bg-gray-700 transition-colors z-50"
        aria-label="Settings"
      >
        <svg
          className="w-6 h-6"
          fill="none"
          stroke="currentColor"
          viewBox="0 0 24 24"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth={2}
            d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z"
          />
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth={2}
            d="M15 12a3 3 0 11-6 0 3 3 0 016 0z"
          />
        </svg>
      </button>

      {/* Drawer Overlay */}
      {isOpen && (
        <div
          className="fixed inset-0 bg-black bg-opacity-50 z-40"
          onClick={() => setIsOpen(false)}
        />
      )}

      {/* Drawer Panel */}
      <div
        className={`fixed right-0 top-0 h-full w-80 bg-white shadow-2xl z-50 transform transition-transform duration-300 ease-in-out ${
          isOpen ? 'translate-x-0' : 'translate-x-full'
        }`}
      >
        <div className="p-6">
          {/* Header */}
          <div className="flex items-center justify-between mb-6">
            <h2 className="text-2xl font-bold text-gray-800">Settings</h2>
            <button
              onClick={() => setIsOpen(false)}
              className="text-gray-500 hover:text-gray-700"
              aria-label="Close"
            >
              <svg
                className="w-6 h-6"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M6 18L18 6M6 6l12 12"
                />
              </svg>
            </button>
          </div>

          {/* Repeat Rate Setting */}
          <div className="mb-6">
            <h3 className="text-lg font-semibold text-gray-700 mb-3">
              Repeat Rate
            </h3>
            <p className="text-sm text-gray-600 mb-4">
              How often to send commands while holding a key/button
            </p>

            {/* Current Value Display */}
            <div className="bg-blue-50 border border-blue-200 rounded-lg p-3 mb-4">
              <p className="text-sm text-gray-600">Current Rate</p>
              <p className="text-2xl font-bold text-blue-600">
                {repeatRate}ms
              </p>
            </div>

            {/* Slider */}
            <div className="mb-4">
              <input
                type="range"
                min="250"
                max="2000"
                step="50"
                value={repeatRate}
                onChange={(e) => onRepeatRateChange(Number(e.target.value))}
                className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer accent-blue-600"
              />
              <div className="flex justify-between text-xs text-gray-500 mt-1">
                <span>250ms</span>
                <span>2000ms</span>
              </div>
            </div>

            {/* Preset Buttons */}
            <div className="space-y-2">
              {presets.map((preset) => (
                <button
                  key={preset.value}
                  onClick={() => onRepeatRateChange(preset.value)}
                  className={`w-full py-2 px-4 rounded-lg font-medium transition-colors ${
                    repeatRate === preset.value
                      ? 'bg-blue-600 text-white'
                      : 'bg-gray-100 text-gray-700 hover:bg-gray-200'
                  }`}
                >
                  {preset.label}
                </button>
              ))}
            </div>
          </div>

          {/* Info */}
          <div className="bg-gray-50 rounded-lg p-4 text-sm text-gray-600">
            <p className="font-semibold mb-2">💡 Tips:</p>
            <ul className="space-y-1 list-disc list-inside">
              <li>Lower values = faster repeat</li>
              <li>Higher values = more precise control</li>
              <li>Changes apply immediately</li>
            </ul>
          </div>
        </div>
      </div>
    </>
  );
}

