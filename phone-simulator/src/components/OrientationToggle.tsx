import React from 'react';
import type { Orientation } from '../types/device';

interface OrientationToggleProps {
  orientation: Orientation;
  onToggle: () => void;
}

/** Toggle button for switching between portrait and landscape orientation */
export const OrientationToggle: React.FC<OrientationToggleProps> = ({ orientation, onToggle }) => {
  const isPortrait = orientation === 'portrait';

  return (
    <button
      onClick={onToggle}
      className="btn-feedback"
      aria-label={`Switch to ${isPortrait ? 'landscape' : 'portrait'} orientation`}
      title={`Switch to ${isPortrait ? 'landscape' : 'portrait'}`}
      style={{
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        width: '32px',
        height: '32px',
        borderRadius: 'var(--radius-sm)',
        background: 'var(--bg-tertiary)',
        border: '1px solid var(--border-default)',
      }}
    >
      <svg
        width="16"
        height="16"
        viewBox="0 0 24 24"
        fill="none"
        stroke="var(--text-secondary)"
        strokeWidth="2"
        style={{
          transform: isPortrait ? 'none' : 'rotate(90deg)',
          transition: 'transform 0.3s ease',
        }}
      >
        <rect x="5" y="2" width="14" height="20" rx="2" />
        <line x1="12" y1="18" x2="12" y2="18" strokeLinecap="round" strokeWidth="3" />
      </svg>
    </button>
  );
};
