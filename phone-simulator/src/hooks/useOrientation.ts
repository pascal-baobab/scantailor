import { useState, useCallback } from 'react';
import type { Orientation } from '../types/device';

/** Hook for managing device orientation */
export function useOrientation() {
  const [orientation, setOrientation] = useState<Orientation>('portrait');
  const [isRotating, setIsRotating] = useState(false);

  const toggleOrientation = useCallback(() => {
    setIsRotating(true);
    setOrientation((prev) => (prev === 'portrait' ? 'landscape' : 'portrait'));
    setTimeout(() => setIsRotating(false), 500);
  }, []);

  return { orientation, toggleOrientation, isRotating };
}
