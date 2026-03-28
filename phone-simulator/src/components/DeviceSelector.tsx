import React, { useState, useMemo } from 'react';
import type { DeviceConfig } from '../types/device';
import { allDevices, getDevicesByBrand } from '../devices';

interface DeviceSelectorProps {
  currentDevice: DeviceConfig;
  onSelect: (device: DeviceConfig) => void;
  collapsed: boolean;
}

/** Device quick switcher sidebar with visual thumbnails grouped by brand */
export const DeviceSelector: React.FC<DeviceSelectorProps> = ({
  currentDevice,
  onSelect,
  collapsed,
}) => {
  const [search, setSearch] = useState('');
  const [brandFilter, setBrandFilter] = useState<'all' | 'apple' | 'samsung'>('all');

  const filteredDevices = useMemo(() => {
    const devices = brandFilter === 'all' ? allDevices : getDevicesByBrand(brandFilter);
    if (!search.trim()) return devices;
    const q = search.toLowerCase();
    return devices.filter((d) => d.name.toLowerCase().includes(q));
  }, [search, brandFilter]);

  const appleList = filteredDevices.filter((d) => d.brand === 'apple');
  const samsungList = filteredDevices.filter((d) => d.brand === 'samsung');

  if (collapsed) return null;

  return (
    <div
      className="slide-in-left"
      style={{
        width: '240px',
        height: '100%',
        background: 'var(--bg-secondary)',
        borderRight: '1px solid var(--border-default)',
        display: 'flex',
        flexDirection: 'column',
        overflow: 'hidden',
        flexShrink: 0,
      }}
    >
      {/* Search */}
      <div style={{ padding: '12px', borderBottom: '1px solid var(--border-muted)' }}>
        <input
          type="text"
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          placeholder="Search devices..."
          aria-label="Search devices"
          style={{
            width: '100%',
            padding: '6px 10px',
            background: 'var(--bg-tertiary)',
            border: '1px solid var(--border-default)',
            borderRadius: 'var(--radius-sm)',
            fontSize: '12px',
            outline: 'none',
            color: 'var(--text-primary)',
          }}
        />
        {/* Brand filter tabs */}
        <div style={{ display: 'flex', gap: '4px', marginTop: '8px' }}>
          {(['all', 'apple', 'samsung'] as const).map((b) => (
            <button
              key={b}
              onClick={() => setBrandFilter(b)}
              className="btn-feedback"
              style={{
                flex: 1,
                padding: '4px 0',
                fontSize: '11px',
                borderRadius: 'var(--radius-sm)',
                background: brandFilter === b ? 'var(--accent-muted)' : 'transparent',
                color: brandFilter === b ? 'var(--accent)' : 'var(--text-secondary)',
                border: `1px solid ${brandFilter === b ? 'var(--accent)' : 'var(--border-default)'}`,
                textTransform: 'capitalize',
              }}
            >
              {b === 'all' ? 'All' : b === 'apple' ? 'Apple' : 'Samsung'}
            </button>
          ))}
        </div>
      </div>

      {/* Device list */}
      <div style={{ flex: 1, overflowY: 'auto', padding: '8px' }}>
        {appleList.length > 0 && (
          <DeviceGroup
            label="Apple"
            devices={appleList}
            currentId={currentDevice.id}
            onSelect={onSelect}
          />
        )}
        {samsungList.length > 0 && (
          <DeviceGroup
            label="Samsung"
            devices={samsungList}
            currentId={currentDevice.id}
            onSelect={onSelect}
          />
        )}
        {filteredDevices.length === 0 && (
          <div style={{ padding: '16px', textAlign: 'center', color: 'var(--text-muted)', fontSize: '12px' }}>
            No devices found
          </div>
        )}
      </div>
    </div>
  );
};

/** Group of devices under a brand heading */
const DeviceGroup: React.FC<{
  label: string;
  devices: DeviceConfig[];
  currentId: string;
  onSelect: (d: DeviceConfig) => void;
}> = ({ label, devices, currentId, onSelect }) => (
  <div style={{ marginBottom: '12px' }}>
    <div
      style={{
        fontSize: '10px',
        fontWeight: 600,
        textTransform: 'uppercase',
        letterSpacing: '0.5px',
        color: 'var(--text-muted)',
        padding: '4px 8px',
      }}
    >
      {label}
    </div>
    {devices.map((d) => {
      const isActive = d.id === currentId;
      return (
        <button
          key={d.id}
          onClick={() => onSelect(d)}
          className="btn-feedback"
          aria-label={`Select ${d.name}`}
          aria-pressed={isActive}
          style={{
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
            width: '100%',
            padding: '8px',
            borderRadius: 'var(--radius-sm)',
            background: isActive ? 'var(--accent-muted)' : 'transparent',
            border: isActive ? '1px solid var(--accent)' : '1px solid transparent',
            textAlign: 'left',
            transition: 'background 0.15s, border-color 0.15s',
          }}
        >
          {/* Mini device thumbnail */}
          <div
            style={{
              width: '24px',
              height: '40px',
              borderRadius: d.screenRadius > 0 ? '6px' : '2px',
              background: '#222',
              border: `2px solid ${isActive ? 'var(--accent)' : '#444'}`,
              flexShrink: 0,
            }}
          />
          <div style={{ minWidth: 0 }}>
            <div
              style={{
                fontSize: '12px',
                fontWeight: 500,
                color: isActive ? 'var(--accent)' : 'var(--text-primary)',
                whiteSpace: 'nowrap',
                overflow: 'hidden',
                textOverflow: 'ellipsis',
              }}
            >
              {d.name}
            </div>
            <div style={{ fontSize: '10px', color: 'var(--text-muted)' }}>
              {d.screenWidth} × {d.screenHeight}
            </div>
          </div>
        </button>
      );
    })}
  </div>
);
