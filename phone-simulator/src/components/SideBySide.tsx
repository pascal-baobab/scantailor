import React, { useRef, useCallback, useState } from 'react';
import type { DeviceConfig, NetworkPreset, ConsoleEntry, DeviceSlot } from '../types/device';
import { allDevices, defaultDevice } from '../devices';
import { DeviceFrame } from './DeviceFrame';

interface SideBySideProps {
  url: string;
  networkPreset: NetworkPreset;
  onConsoleEntry: (entry: ConsoleEntry) => void;
}

/** Multi-device comparison view with 2-3 devices side by side */
export const SideBySide: React.FC<SideBySideProps> = ({ url, networkPreset, onConsoleEntry }) => {
  const [slots, setSlots] = useState<DeviceSlot[]>([
    { id: '1', device: allDevices.find((d) => d.id === 'iphone-15-pro-max') ?? defaultDevice, orientation: 'portrait' },
    { id: '2', device: allDevices.find((d) => d.id === 'galaxy-s24-ultra') ?? allDevices[6], orientation: 'portrait' },
  ]);
  const refsMap = useRef<Record<string, HTMLDivElement | null>>({});

  const updateSlotDevice = useCallback((slotId: string, device: DeviceConfig) => {
    setSlots((prev) => prev.map((s) => (s.id === slotId ? { ...s, device } : s)));
  }, []);

  const addSlot = useCallback(() => {
    if (slots.length >= 3) return;
    const nextId = String(slots.length + 1);
    setSlots((prev) => [
      ...prev,
      { id: nextId, device: allDevices[0], orientation: 'portrait' },
    ]);
  }, [slots.length]);

  const removeSlot = useCallback((slotId: string) => {
    if (slots.length <= 2) return;
    setSlots((prev) => prev.filter((s) => s.id !== slotId));
  }, [slots.length]);

  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'column',
        height: '100%',
        width: '100%',
      }}
    >
      {/* Controls */}
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          gap: '8px',
          padding: '8px 16px',
          borderBottom: '1px solid var(--border-default)',
          background: 'var(--bg-secondary)',
          flexWrap: 'wrap',
        }}
      >
        {slots.map((slot) => (
          <div key={slot.id} style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
            <select
              value={slot.device.id}
              onChange={(e) => {
                const d = allDevices.find((dev) => dev.id === e.target.value);
                if (d) updateSlotDevice(slot.id, d);
              }}
              aria-label={`Device for slot ${slot.id}`}
              style={{
                padding: '4px 8px',
                fontSize: '11px',
                background: 'var(--bg-tertiary)',
                border: '1px solid var(--border-default)',
                borderRadius: 'var(--radius-sm)',
                color: 'var(--text-primary)',
                outline: 'none',
              }}
            >
              {allDevices.map((d) => (
                <option key={d.id} value={d.id}>
                  {d.name}
                </option>
              ))}
            </select>
            {slots.length > 2 && (
              <button
                onClick={() => removeSlot(slot.id)}
                className="btn-feedback"
                aria-label="Remove device"
                style={{
                  fontSize: '14px',
                  color: 'var(--text-muted)',
                  width: '20px',
                  height: '20px',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                ×
              </button>
            )}
          </div>
        ))}
        {slots.length < 3 && (
          <button
            onClick={addSlot}
            className="btn-feedback"
            aria-label="Add device"
            style={{
              padding: '4px 10px',
              fontSize: '11px',
              borderRadius: 'var(--radius-sm)',
              background: 'var(--accent-muted)',
              border: '1px solid var(--accent)',
              color: 'var(--accent)',
            }}
          >
            + Add device
          </button>
        )}
      </div>

      {/* Devices */}
      <div
        style={{
          flex: 1,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          gap: '32px',
          padding: '24px',
          overflowX: 'auto',
        }}
      >
        {slots.map((slot) => (
          <div
            key={slot.id}
            ref={(el) => { refsMap.current[slot.id] = el; }}
            style={{
              transform: `scale(${slots.length === 3 ? 0.5 : 0.6})`,
              transformOrigin: 'center center',
              flexShrink: 0,
            }}
          >
            <div style={{ textAlign: 'center', marginBottom: '12px' }}>
              <span
                style={{
                  fontSize: '12px',
                  fontWeight: 600,
                  color: 'var(--text-secondary)',
                  background: 'var(--bg-tertiary)',
                  padding: '4px 12px',
                  borderRadius: 'var(--radius-sm)',
                }}
              >
                {slot.device.name} — {slot.device.screenWidth}×{slot.device.screenHeight}
              </span>
            </div>
            <DeviceFrame
              device={slot.device}
              orientation={slot.orientation}
              url={url}
              networkPreset={networkPreset}
              isTransitioning={false}
              isRotating={false}
              onConsoleEntry={onConsoleEntry}
            />
          </div>
        ))}
      </div>
    </div>
  );
};
