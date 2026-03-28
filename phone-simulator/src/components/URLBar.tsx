import React, { useState, useCallback } from 'react';

interface URLBarProps {
  url: string;
  onNavigate: (url: string) => void;
  onRefresh: () => void;
}

/** Address bar for entering URLs to load in the device viewport */
export const URLBar: React.FC<URLBarProps> = ({ url, onNavigate, onRefresh }) => {
  const [inputValue, setInputValue] = useState(url);

  const handleSubmit = useCallback(
    (e: React.FormEvent) => {
      e.preventDefault();
      let finalUrl = inputValue.trim();
      if (finalUrl && !finalUrl.startsWith('http://') && !finalUrl.startsWith('https://')) {
        finalUrl = 'https://' + finalUrl;
      }
      onNavigate(finalUrl);
    },
    [inputValue, onNavigate]
  );

  return (
    <form
      onSubmit={handleSubmit}
      style={{
        display: 'flex',
        alignItems: 'center',
        gap: '6px',
        flex: 1,
        minWidth: 0,
      }}
    >
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          flex: 1,
          background: 'var(--bg-tertiary)',
          border: '1px solid var(--border-default)',
          borderRadius: 'var(--radius-md)',
          padding: '0 10px',
          minWidth: 0,
        }}
      >
        <svg
          width="14"
          height="14"
          viewBox="0 0 24 24"
          fill="none"
          stroke="var(--text-muted)"
          strokeWidth="2"
          style={{ flexShrink: 0 }}
        >
          <circle cx="12" cy="12" r="10" />
          <path d="M2 12h20M12 2a15.3 15.3 0 014 10 15.3 15.3 0 01-4 10 15.3 15.3 0 01-4-10 15.3 15.3 0 014-10z" />
        </svg>
        <input
          type="text"
          value={inputValue}
          onChange={(e) => setInputValue(e.target.value)}
          placeholder="Enter URL (e.g., localhost:3000)"
          aria-label="URL address bar"
          style={{
            flex: 1,
            background: 'none',
            border: 'none',
            outline: 'none',
            padding: '8px',
            fontSize: '13px',
            color: 'var(--text-primary)',
            minWidth: 0,
          }}
        />
      </div>
      <button
        type="button"
        onClick={onRefresh}
        className="btn-feedback"
        aria-label="Refresh page"
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
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="var(--text-secondary)" strokeWidth="2">
          <path d="M23 4v6h-6M1 20v-6h6" />
          <path d="M3.51 9a9 9 0 0114.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0020.49 15" />
        </svg>
      </button>
    </form>
  );
};
