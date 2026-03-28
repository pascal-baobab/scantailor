import React from 'react';
import type { DeviceConfig } from '../types/device';

interface StatusBarProps {
  device: DeviceConfig;
}

/** Renders a realistic iOS or Android status bar overlay */
export const StatusBar: React.FC<StatusBarProps> = ({ device }) => {
  const time = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: false });

  if (device.os === 'ios') {
    return (
      <div
        style={{
          position: 'absolute',
          top: 0,
          left: 0,
          right: 0,
          height: device.statusBarHeight,
          display: 'flex',
          alignItems: 'flex-end',
          justifyContent: 'space-between',
          padding: '0 24px 4px',
          zIndex: 60,
          pointerEvents: 'none',
          fontFamily: '-apple-system, BlinkMacSystemFont, "SF Pro Text", sans-serif',
          fontSize: '14px',
          fontWeight: 600,
          color: '#fff',
        }}
      >
        <span>{time}</span>
        <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
          <svg width="16" height="12" viewBox="0 0 16 12" fill="currentColor">
            <rect x="0" y="7" width="3" height="5" rx="0.5" opacity="0.4" />
            <rect x="4.5" y="5" width="3" height="7" rx="0.5" opacity="0.6" />
            <rect x="9" y="3" width="3" height="9" rx="0.5" opacity="0.8" />
            <rect x="13.5" y="0" width="3" height="12" rx="0.5" />
          </svg>
          <svg width="16" height="12" viewBox="0 0 16 12" fill="currentColor">
            <path d="M8 3.5C9.8 3.5 11.4 4.2 12.6 5.4L14 4C12.4 2.4 10.3 1.5 8 1.5S3.6 2.4 2 4L3.4 5.4C4.6 4.2 6.2 3.5 8 3.5Z" opacity="0.5" />
            <path d="M8 6.5C9.1 6.5 10.1 6.9 10.8 7.6L12.2 6.2C11.1 5.1 9.6 4.5 8 4.5S4.9 5.1 3.8 6.2L5.2 7.6C5.9 6.9 6.9 6.5 8 6.5Z" opacity="0.75" />
            <circle cx="8" cy="10" r="1.5" />
          </svg>
          <svg width="26" height="12" viewBox="0 0 26 12" fill="currentColor">
            <rect x="0" y="1" width="22" height="10" rx="2" stroke="currentColor" strokeWidth="1" fill="none" opacity="0.4" />
            <rect x="1.5" y="2.5" width="16" height="7" rx="1" />
            <rect x="23" y="4" width="2" height="4" rx="0.5" opacity="0.4" />
          </svg>
        </div>
      </div>
    );
  }

  return (
    <div
      style={{
        position: 'absolute',
        top: 0,
        left: 0,
        right: 0,
        height: device.statusBarHeight,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        padding: '0 16px',
        zIndex: 60,
        pointerEvents: 'none',
        fontFamily: '"Roboto", "Google Sans", sans-serif',
        fontSize: '12px',
        fontWeight: 500,
        color: '#fff',
      }}
    >
      <span>{time}</span>
      <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
        <svg width="14" height="12" viewBox="0 0 14 12" fill="currentColor">
          <rect x="0" y="7" width="2.5" height="5" rx="0.5" opacity="0.4" />
          <rect x="3.5" y="5" width="2.5" height="7" rx="0.5" opacity="0.6" />
          <rect x="7" y="3" width="2.5" height="9" rx="0.5" opacity="0.8" />
          <rect x="10.5" y="0" width="2.5" height="12" rx="0.5" />
        </svg>
        <svg width="14" height="12" viewBox="0 0 14 12" fill="currentColor">
          <path d="M7 3C8.5 3 9.8 3.6 10.8 4.5L12 3.3C10.6 1.9 8.9 1.2 7 1.2S3.4 1.9 2 3.3L3.2 4.5C4.2 3.6 5.5 3 7 3Z" opacity="0.5" />
          <path d="M7 6C7.8 6 8.6 6.3 9.2 6.9L10.4 5.7C9.5 4.8 8.3 4.3 7 4.3S4.5 4.8 3.6 5.7L4.8 6.9C5.4 6.3 6.2 6 7 6Z" opacity="0.75" />
          <circle cx="7" cy="9.5" r="1.3" />
        </svg>
        <div style={{ display: 'flex', alignItems: 'center', gap: '2px' }}>
          <svg width="22" height="12" viewBox="0 0 22 12" fill="currentColor">
            <rect x="0" y="1" width="18" height="10" rx="2" stroke="currentColor" strokeWidth="1" fill="none" opacity="0.4" />
            <rect x="1.5" y="2.5" width="13" height="7" rx="1" />
            <rect x="19" y="3.5" width="2" height="5" rx="0.5" opacity="0.4" />
          </svg>
          <span style={{ fontSize: '10px', opacity: 0.8 }}>85%</span>
        </div>
      </div>
    </div>
  );
};
