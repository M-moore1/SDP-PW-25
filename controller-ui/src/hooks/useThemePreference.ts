//Author: Krish Shah
//Reviewed by: Sai Raparla

import { useCallback, useEffect, useState, useSyncExternalStore } from 'react';

export const THEME_STORAGE_KEY = 'controller-ui-theme';

export type ThemeOverride = 'light' | 'dark' | null;

function readStoredOverride(): ThemeOverride {
  try {
    const v = localStorage.getItem(THEME_STORAGE_KEY);
    if (v === 'light' || v === 'dark') return v;
  } catch {
    /* ignore */
  }
  return null;
}

function getSystemDark(): boolean {
  return window.matchMedia('(prefers-color-scheme: dark)').matches;
}

function applyHtmlClass(isDark: boolean) {
  document.documentElement.classList.toggle('dark', isDark);
}

/** Subscribe to prefers-color-scheme changes (for useSyncExternalStore when following system). */
function subscribeSystemTheme(onStoreChange: () => void) {
  const mq = window.matchMedia('(prefers-color-scheme: dark)');
  mq.addEventListener('change', onStoreChange);
  return () => mq.removeEventListener('change', onStoreChange);
}

function getServerSnapshot(): boolean {
  return false;
}

/**
 * Theme: default follows OS; explicit light/dark persisted in localStorage.
 * Syncs `dark` class on document.documentElement.
 */
export function useThemePreference() {
  const systemDark = useSyncExternalStore(
    subscribeSystemTheme,
    getSystemDark,
    getServerSnapshot
  );

  const [override, setOverrideState] = useState<ThemeOverride>(() =>
    typeof window === 'undefined' ? null : readStoredOverride()
  );

  const isDark =
    override === 'dark' || (override === null && systemDark);

  useEffect(() => {
    applyHtmlClass(isDark);
  }, [isDark]);

  const setOverride = useCallback((next: ThemeOverride) => {
    setOverrideState(next);
    try {
      if (next === null) {
        localStorage.removeItem(THEME_STORAGE_KEY);
      } else {
        localStorage.setItem(THEME_STORAGE_KEY, next);
      }
    } catch {
      /* ignore */
    }
    const resolved = next === 'dark' || (next === null && getSystemDark());
    applyHtmlClass(resolved);
  }, []);

  const toggleTheme = useCallback(() => {
    setOverride(isDark ? 'light' : 'dark');
  }, [isDark, setOverride]);

  return { isDark, toggleTheme };
}
