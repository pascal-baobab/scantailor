import React, { useState } from 'react';

interface ScreenshotButtonProps {
  onCaptureScreen: () => void;
  onCaptureMockup: () => void;
}

/** Screenshot capture button with dropdown for screen-only vs full mockup */
export const ScreenshotButton: React.FC<ScreenshotButtonProps> = ({
  onCaptureScreen,
  onCaptureMockup,
}) => {
  const [showMenu, setShowMenu] = useState(false);

  return (
    <div style={{ position: 'relative' }}>
      <button
        onClick={() => setShowMenu(!showMenu)}
        className="btn-feedback"
        aria-label="Take screenshot"
        title="Screenshot"
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
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="var(--text-secondary)" strokeWidth="2">
          <path d="M23 19a2 2 0 01-2 2H3a2 2 0 01-2-2V8a2 2 0 012-2h4l2-3h6l2 3h4a2 2 0 012 2z" />
          <circle cx="12" cy="13" r="4" />
        </svg>
      </button>

      {showMenu && (
        <>
          <div
            style={{ position: 'fixed', inset: 0, zIndex: 999 }}
            onClick={() => setShowMenu(false)}
          />
          <div
            className="fade-in"
            style={{
              position: 'absolute',
              top: '100%',
              right: 0,
              marginTop: '4px',
              background: 'var(--bg-elevated)',
              border: '1px solid var(--border-default)',
              borderRadius: 'var(--radius-md)',
              boxShadow: 'var(--shadow-md)',
              padding: '4px',
              zIndex: 1000,
              minWidth: '160px',
            }}
          >
            <button
              onClick={() => { onCaptureScreen(); setShowMenu(false); }}
              style={{
                display: 'block',
                width: '100%',
                padding: '8px 12px',
                fontSize: '12px',
                textAlign: 'left',
                borderRadius: 'var(--radius-sm)',
                color: 'var(--text-primary)',
              }}
              onMouseEnter={(e) => { e.currentTarget.style.background = 'var(--bg-tertiary)'; }}
              onMouseLeave={(e) => { e.currentTarget.style.background = 'transparent'; }}
            >
              Screen only
            </button>
            <button
              onClick={() => { onCaptureMockup(); setShowMenu(false); }}
              style={{
                display: 'block',
                width: '100%',
                padding: '8px 12px',
                fontSize: '12px',
                textAlign: 'left',
                borderRadius: 'var(--radius-sm)',
                color: 'var(--text-primary)',
              }}
              onMouseEnter={(e) => { e.currentTarget.style.background = 'var(--bg-tertiary)'; }}
              onMouseLeave={(e) => { e.currentTarget.style.background = 'transparent'; }}
            >
              Full device mockup
            </button>
          </div>
        </>
      )}
    </div>
  );
};
