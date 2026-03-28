import React, { useState } from 'react';
import type { DeviceConfig, Orientation, ConsoleEntry, DevToolsTab } from '../types/device';

interface DevToolsProps {
  device: DeviceConfig;
  orientation: Orientation;
  consoleEntries: ConsoleEntry[];
  onClearConsole: () => void;
}

/** Lightweight DevTools panel with viewport info and console tabs */
export const DevTools: React.FC<DevToolsProps> = ({
  device,
  orientation,
  consoleEntries,
  onClearConsole,
}) => {
  const [isOpen, setIsOpen] = useState(false);
  const [activeTab, setActiveTab] = useState<DevToolsTab>('viewport');

  const tabs: { id: DevToolsTab; label: string }[] = [
    { id: 'viewport', label: 'Viewport' },
    { id: 'console', label: `Console${consoleEntries.length > 0 ? ` (${consoleEntries.length})` : ''}` },
    { id: 'network', label: 'Network' },
  ];

  const w = orientation === 'portrait' ? device.screenWidth : device.screenHeight;
  const h = orientation === 'portrait' ? device.screenHeight : device.screenWidth;

  return (
    <div
      style={{
        borderTop: '1px solid var(--border-default)',
        background: 'var(--bg-secondary)',
        flexShrink: 0,
      }}
    >
      {/* Header */}
      <button
        onClick={() => setIsOpen(!isOpen)}
        aria-label={`${isOpen ? 'Collapse' : 'Expand'} DevTools`}
        aria-expanded={isOpen}
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          width: '100%',
          padding: '8px 16px',
          fontSize: '12px',
          fontWeight: 600,
          color: 'var(--text-secondary)',
          background: 'var(--bg-secondary)',
          borderBottom: isOpen ? '1px solid var(--border-muted)' : 'none',
        }}
      >
        <span>DevTools</span>
        <svg
          width="12"
          height="12"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          strokeWidth="2"
          style={{
            transform: isOpen ? 'rotate(180deg)' : 'none',
            transition: 'transform 0.2s ease',
          }}
        >
          <polyline points="6 9 12 15 18 9" />
        </svg>
      </button>

      {isOpen && (
        <div className="panel-expand" style={{ overflow: 'hidden' }}>
          {/* Tabs */}
          <div
            style={{
              display: 'flex',
              borderBottom: '1px solid var(--border-muted)',
              padding: '0 12px',
            }}
          >
            {tabs.map((tab) => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                style={{
                  padding: '8px 12px',
                  fontSize: '11px',
                  fontWeight: 500,
                  color: activeTab === tab.id ? 'var(--accent)' : 'var(--text-muted)',
                  borderBottom: activeTab === tab.id ? '2px solid var(--accent)' : '2px solid transparent',
                  transition: 'color 0.15s, border-color 0.15s',
                }}
              >
                {tab.label}
              </button>
            ))}
          </div>

          {/* Tab content */}
          <div style={{ padding: '12px 16px', maxHeight: '200px', overflowY: 'auto' }}>
            {activeTab === 'viewport' && (
              <div style={{ fontFamily: 'var(--font-mono)', fontSize: '12px', lineHeight: '1.8' }}>
                <InfoRow label="Device" value={device.name} />
                <InfoRow label="Viewport" value={`${w} × ${h}`} />
                <InfoRow label="DPR" value={String(device.devicePixelRatio)} />
                <InfoRow label="Orientation" value={orientation} />
                <InfoRow label="OS" value={`${device.os === 'ios' ? 'iOS' : 'Android'} (${device.brand})`} />
                <InfoRow
                  label="User-Agent"
                  value={
                    device.os === 'ios'
                      ? `Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 Mobile/15E148 Safari/604.1`
                      : `Mozilla/5.0 (Linux; Android 14; ${device.name}) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36`
                  }
                />
              </div>
            )}

            {activeTab === 'console' && (
              <div>
                <div style={{ display: 'flex', justifyContent: 'flex-end', marginBottom: '8px' }}>
                  <button
                    onClick={onClearConsole}
                    style={{
                      fontSize: '11px',
                      color: 'var(--text-muted)',
                      padding: '2px 8px',
                      borderRadius: 'var(--radius-sm)',
                      border: '1px solid var(--border-default)',
                    }}
                  >
                    Clear
                  </button>
                </div>
                {consoleEntries.length === 0 ? (
                  <div style={{ color: 'var(--text-muted)', fontSize: '12px', textAlign: 'center', padding: '16px' }}>
                    No console output yet
                  </div>
                ) : (
                  consoleEntries.map((entry, i) => (
                    <div
                      key={i}
                      style={{
                        padding: '4px 8px',
                        fontFamily: 'var(--font-mono)',
                        fontSize: '11px',
                        borderBottom: '1px solid var(--border-muted)',
                        color:
                          entry.level === 'error' ? 'var(--danger)' :
                          entry.level === 'warn' ? 'var(--warning)' :
                          'var(--text-primary)',
                        background:
                          entry.level === 'error' ? 'rgba(248, 81, 73, 0.1)' :
                          entry.level === 'warn' ? 'rgba(210, 153, 34, 0.1)' :
                          'transparent',
                      }}
                    >
                      <span style={{ opacity: 0.5 }}>
                        {new Date(entry.timestamp).toLocaleTimeString()}
                      </span>{' '}
                      [{entry.level}] {entry.message}
                    </div>
                  ))
                )}
              </div>
            )}

            {activeTab === 'network' && (
              <div style={{ color: 'var(--text-muted)', fontSize: '12px', textAlign: 'center', padding: '16px' }}>
                Network logging requires same-origin iframe access.
                <br />
                Use browser DevTools for full network inspection.
              </div>
            )}
          </div>
        </div>
      )}
    </div>
  );
};

const InfoRow: React.FC<{ label: string; value: string }> = ({ label, value }) => (
  <div style={{ display: 'flex', gap: '12px' }}>
    <span style={{ color: 'var(--text-muted)', minWidth: '80px' }}>{label}:</span>
    <span style={{ color: 'var(--text-primary)', wordBreak: 'break-all' }}>{value}</span>
  </div>
);
