import { useState, useCallback } from 'react';
import type { DeviceConfig } from '../types/device';
import { defaultDevice } from '../devices';

/** Hook for managing the currently selected device */
export function useDevice() {
  const [device, setDevice] = useState<DeviceConfig>(defaultDevice);
  const [isTransitioning, setIsTransitioning] = useState(false);

  const selectDevice = useCallback((newDevice: DeviceConfig) => {
    setIsTransitioning(true);
    setDevice(newDevice);
    setTimeout(() => setIsTransitioning(false), 400);
  }, []);

  return { device, selectDevice, isTransitioning };
}
