import React from 'react';
import type { NetworkPreset } from '../types/device';
import { networkPresets } from '../hooks/useNetworkThrottle';

interface NetworkThrottleProps {
  activePreset: NetworkPreset;
  onPresetChange: (preset: NetworkPreset) => void;
}

/** Network condition simulation dropdown */
export const NetworkThrottle: React.FC<NetworkThrottleProps> = ({
  activePreset,
  onPresetChange,
}) => {
  const isThrottled = activePreset !== 'none';

  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
      {isThrottled && (
        <div
          className="pulse"
          style={{
            width: '6px',
            height: '6px',
            borderRadius: '50%',
            background: activePreset === 'offline' ? 'var(--danger)' : 'var(--warning)',
            flexShrink: 0,
          }}
          aria-label={`Network: ${activePreset}`}
        />
      )}
      <select
        value={activePreset}
        onChange={(e) => onPresetChange(e.target.value as NetworkPreset)}
        aria-label="Network throttle"
        style={{
          padding: '4px 8px',
          fontSize: '11px',
          background: 'var(--bg-tertiary)',
          border: `1px solid ${isThrottled ? 'var(--warning)' : 'var(--border-default)'}`,
          borderRadius: 'var(--radius-sm)',
          color: isThrottled ? 'var(--warning)' : 'var(--text-secondary)',
          outline: 'none',
          cursor: 'pointer',
        }}
      >
        {networkPresets.map((p) => (
          <option key={p.preset} value={p.preset}>
            {p.name}
          </option>
        ))}
      </select>
    </div>
  );
};
