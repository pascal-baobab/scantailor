import { useState } from 'react';
import type { NetworkPreset, NetworkThrottleConfig } from '../types/device';

/** Available network throttle presets */
export const networkPresets: NetworkThrottleConfig[] = [
  { name: 'No throttle', preset: 'none', latencyMs: 0, downloadMbps: 0, uploadMbps: 0 },
  { name: 'Fast 4G', preset: 'fast4g', latencyMs: 50, downloadMbps: 10, uploadMbps: 5 },
  { name: 'Slow 3G', preset: 'slow3g', latencyMs: 300, downloadMbps: 0.5, uploadMbps: 0.25 },
  { name: '2G', preset: '2g', latencyMs: 800, downloadMbps: 0.1, uploadMbps: 0.05 },
  { name: 'Offline', preset: 'offline', latencyMs: 0, downloadMbps: 0, uploadMbps: 0 },
];

/** Hook for managing simulated network conditions */
export function useNetworkThrottle() {
  const [activePreset, setActivePreset] = useState<NetworkPreset>('none');

  const currentConfig = networkPresets.find((p) => p.preset === activePreset) ?? networkPresets[0];

  return { activePreset, setActivePreset, currentConfig, presets: networkPresets };
}
