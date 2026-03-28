import React from 'react';
import type { DeviceConfig, Orientation, NetworkPreset } from '../types/device';
import { URLBar } from './URLBar';
import { OrientationToggle } from './OrientationToggle';
import { ZoomControl } from './ZoomControl';
import { ScreenshotButton } from './ScreenshotButton';
import { NetworkThrottle } from './NetworkThrottle';

interface ToolbarProps {
  device: DeviceConfig;
  url: string;
  onNavigate: (url: string) => void;
  onRefresh: () => void;
  orientation: Orientation;
  onToggleOrientation: () => void;
  zoom: number;
  zoomPresets: number[];
  onZoomChange: (level: number) => void;
  fitToWindow: boolean;
  onFitToggle: () => void;
  networkPreset: NetworkPreset;
  onNetworkChange: (preset: NetworkPreset) => void;
  onCaptureScreen: () => void;
  onCaptureMockup: () => void;
  sidebarCollapsed: boolean;
  onToggleSidebar: () => void;
  sideBySideMode: boolean;
  onToggleSideBySide: () => void;
  theme: 'dark' | 'light';
  onToggleTheme: () => void;
}

/** Top toolbar with all simulator controls */
export const Toolbar: React.FC<ToolbarProps> = ({
  url,
  onNavigate,
  onRefresh,
  orientation,
  onToggleOrientation,
  zoom,
  zoomPresets,
  onZoomChange,
  fitToWindow,
  onFitToggle,
  networkPreset,
  onNetworkChange,
  onCaptureScreen,
  onCaptureMockup,
  sidebarCollapsed,
  onToggleSidebar,
  sideBySideMode,
  onToggleSideBySide,
  theme,
  onToggleTheme,
}) => {
  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'center',
        gap: '12px',
        padding: '8px 12px',
        borderBottom: '1px solid var(--border-default)',
        background: 'var(--bg-secondary)',
        flexShrink: 0,
        minHeight: '48px',
        flexWrap: 'wrap',
      }}
    >
      {/* Sidebar toggle */}
      <button
        onClick={onToggleSidebar}
        className="btn-feedback"
        aria-label={sidebarCollapsed ? 'Show device sidebar' : 'Hide device sidebar'}
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          width: '32px',
          height: '32px',
          borderRadius: 'var(--radius-sm)',
          background: 'var(--bg-tertiary)',
          border: '1px solid var(--border-default)',
          flexShrink: 0,
        }}
      >
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="var(--text-secondary)" strokeWidth="2">
          <rect x="3" y="3" width="18" height="18" rx="2" />
          <line x1="9" y1="3" x2="9" y2="21" />
        </svg>
      </button>

      {/* URL Bar */}
      <URLBar url={url} onNavigate={onNavigate} onRefresh={onRefresh} />

      {/* Divider */}
      <div style={{ width: '1px', height: '24px', background: 'var(--border-default)', flexShrink: 0 }} />

      {/* Orientation */}
      <OrientationToggle orientation={orientation} onToggle={onToggleOrientation} />

      {/* Zoom */}
      <ZoomControl
        zoom={zoom}
        presets={zoomPresets}
        onZoomChange={onZoomChange}
        fitToWindow={fitToWindow}
        onFitToggle={onFitToggle}
      />

      {/* Divider */}
      <div style={{ width: '1px', height: '24px', background: 'var(--border-default)', flexShrink: 0 }} />

      {/* Network */}
      <NetworkThrottle activePreset={networkPreset} onPresetChange={onNetworkChange} />

      {/* Screenshot */}
      <ScreenshotButton onCaptureScreen={onCaptureScreen} onCaptureMockup={onCaptureMockup} />

      {/* Side by side toggle */}
      <button
        onClick={onToggleSideBySide}
        className="btn-feedback"
        aria-label="Toggle side by side view"
        title="Compare devices"
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          width: '32px',
          height: '32px',
          borderRadius: 'var(--radius-sm)',
          background: sideBySideMode ? 'var(--accent-muted)' : 'var(--bg-tertiary)',
          border: `1px solid ${sideBySideMode ? 'var(--accent)' : 'var(--border-default)'}`,
          flexShrink: 0,
        }}
      >
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke={sideBySideMode ? 'var(--accent)' : 'var(--text-secondary)'} strokeWidth="2">
          <rect x="2" y="3" width="8" height="18" rx="1" />
          <rect x="14" y="3" width="8" height="18" rx="1" />
        </svg>
      </button>

      {/* Theme toggle */}
      <button
        onClick={onToggleTheme}
        className="btn-feedback"
        aria-label={`Switch to ${theme === 'dark' ? 'light' : 'dark'} theme`}
        title={`${theme === 'dark' ? 'Light' : 'Dark'} mode`}
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          width: '32px',
          height: '32px',
          borderRadius: 'var(--radius-sm)',
          background: 'var(--bg-tertiary)',
          border: '1px solid var(--border-default)',
          flexShrink: 0,
        }}
      >
        {theme === 'dark' ? (
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="var(--text-secondary)" strokeWidth="2">
            <circle cx="12" cy="12" r="5" />
            <line x1="12" y1="1" x2="12" y2="3" /><line x1="12" y1="21" x2="12" y2="23" />
            <line x1="4.22" y1="4.22" x2="5.64" y2="5.64" /><line x1="18.36" y1="18.36" x2="19.78" y2="19.78" />
            <line x1="1" y1="12" x2="3" y2="12" /><line x1="21" y1="12" x2="23" y2="12" />
            <line x1="4.22" y1="19.78" x2="5.64" y2="18.36" /><line x1="18.36" y1="5.64" x2="19.78" y2="4.22" />
          </svg>
        ) : (
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="var(--text-secondary)" strokeWidth="2">
            <path d="M21 12.79A9 9 0 1111.21 3 7 7 0 0021 12.79z" />
          </svg>
        )}
      </button>
    </div>
  );
};
