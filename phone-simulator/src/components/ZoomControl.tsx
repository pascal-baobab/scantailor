import React from 'react';

interface ZoomControlProps {
  zoom: number;
  presets: number[];
  onZoomChange: (level: number) => void;
  fitToWindow: boolean;
  onFitToggle: () => void;
}

/** Viewport zoom control with presets and fit-to-window option */
export const ZoomControl: React.FC<ZoomControlProps> = ({
  zoom,
  presets,
  onZoomChange,
  fitToWindow,
  onFitToggle,
}) => {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
      <button
        onClick={() => onZoomChange(zoom - 25)}
        className="btn-feedback"
        aria-label="Zoom out"
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          width: '28px',
          height: '28px',
          borderRadius: 'var(--radius-sm)',
          background: 'var(--bg-tertiary)',
          border: '1px solid var(--border-default)',
          fontSize: '14px',
          color: 'var(--text-secondary)',
        }}
      >
        −
      </button>

      <select
        value={zoom}
        onChange={(e) => onZoomChange(Number(e.target.value))}
        aria-label="Zoom level"
        style={{
          padding: '4px 6px',
          fontSize: '12px',
          background: 'var(--bg-tertiary)',
          border: '1px solid var(--border-default)',
          borderRadius: 'var(--radius-sm)',
          color: 'var(--text-primary)',
          outline: 'none',
          cursor: 'pointer',
        }}
      >
        {presets.map((p) => (
          <option key={p} value={p}>
            {p}%
          </option>
        ))}
        {!presets.includes(zoom) && (
          <option value={zoom}>{zoom}%</option>
        )}
      </select>

      <button
        onClick={() => onZoomChange(zoom + 25)}
        className="btn-feedback"
        aria-label="Zoom in"
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          width: '28px',
          height: '28px',
          borderRadius: 'var(--radius-sm)',
          background: 'var(--bg-tertiary)',
          border: '1px solid var(--border-default)',
          fontSize: '14px',
          color: 'var(--text-secondary)',
        }}
      >
        +
      </button>

      <button
        onClick={onFitToggle}
        className="btn-feedback"
        aria-label="Fit to window"
        title="Fit to window"
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          height: '28px',
          padding: '0 8px',
          borderRadius: 'var(--radius-sm)',
          background: fitToWindow ? 'var(--accent-muted)' : 'var(--bg-tertiary)',
          border: `1px solid ${fitToWindow ? 'var(--accent)' : 'var(--border-default)'}`,
          fontSize: '11px',
          color: fitToWindow ? 'var(--accent)' : 'var(--text-secondary)',
        }}
      >
        Fit
      </button>
    </div>
  );
};
