//Author: Krish Shah
//Reviewed by: Sai Raparla
import { useEffect, useRef } from 'react';

/**
 * Hook that runs a callback immediately when started, then repeatedly at a given interval
 * until stopped. Prevents duplicate intervals for the same action.
 */
export function useHoldRepeater(
  isActive: boolean,
  callback: () => void,
  intervalMs: number = 1000
) {
  const intervalRef = useRef<number | null>(null);
  const hasRunInitial = useRef(false);

  useEffect(() => {
    if (isActive) {
      // Run immediately on activation (only once)
      if (!hasRunInitial.current) {
        callback();
        hasRunInitial.current = true;
      }

      // Clear any existing interval to prevent duplicates
      if (intervalRef.current !== null) {
        clearInterval(intervalRef.current);
      }

      // Set up repeating interval
      intervalRef.current = window.setInterval(() => {
        callback();
      }, intervalMs);
    } else {
      // Clean up when deactivated
      if (intervalRef.current !== null) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
      hasRunInitial.current = false;
    }

    // Cleanup on unmount or when dependencies change
    return () => {
      if (intervalRef.current !== null) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    };
  }, [isActive, callback, intervalMs]);
}

