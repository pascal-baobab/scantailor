import React, { useState, useRef, useEffect, useCallback } from 'react';
import type { DeviceConfig, NetworkPreset, ConsoleEntry } from '../types/device';

interface ScreenContentProps {
  url: string;
  device: DeviceConfig;
  networkPreset: NetworkPreset;
  onConsoleEntry: (entry: ConsoleEntry) => void;
}

/** iframe wrapper that loads the target web app inside the device screen */
export const ScreenContent: React.FC<ScreenContentProps> = ({
  url,
  device,
  networkPreset,
  onConsoleEntry,
}) => {
  const [isLoading, setIsLoading] = useState(false);
  const iframeRef = useRef<HTMLIFrameElement>(null);

  const handleLoad = useCallback(() => {
    setIsLoading(false);
  }, []);

  useEffect(() => {
    if (url) {
      setIsLoading(true);
    }
  }, [url]);

  useEffect(() => {
    const handleMessage = (event: MessageEvent) => {
      if (event.data?.type === 'console' && event.data.level && event.data.message) {
        onConsoleEntry({
          level: event.data.level,
          timestamp: Date.now(),
          message: event.data.message,
        });
      }
    };
    window.addEventListener('message', handleMessage);
    return () => window.removeEventListener('message', handleMessage);
  }, [onConsoleEntry]);

  const isOffline = networkPreset === 'offline';

  return (
    <div
      style={{
        width: '100%',
        height: '100%',
        position: 'relative',
        background: '#000',
      }}
    >
      {isLoading && <div className="device-loading" />}
      {isOffline ? (
        <div
          style={{
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center',
            height: '100%',
            color: '#666',
            fontFamily: 'var(--font-sans)',
            fontSize: '14px',
            gap: '12px',
          }}
        >
          <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
            <path d="M1 1L23 23M16.72 11.06A10.94 10.94 0 0119.07 12.93M5 12.55A10.94 10.94 0 0112 9.27c1.15 0 2.26.18 3.3.52M10.71 5.05A16 16 0 0122.56 9M1.42 9A15.91 15.91 0 014.67 6.11M8.53 16.11A6 6 0 0116.76 15M12 20h.01" />
          </svg>
          <span>Offline Mode</span>
          <span style={{ fontSize: '12px', opacity: 0.6 }}>Network simulation active</span>
        </div>
      ) : url ? (
        <iframe
          ref={iframeRef}
          src={url}
          onLoad={handleLoad}
          title={`${device.name} viewport`}
          allow="accelerometer; camera; encrypted-media; geolocation; gyroscope; microphone"
          sandbox="allow-forms allow-modals allow-popups allow-presentation allow-same-origin allow-scripts"
          style={{
            width: device.screenWidth,
            height: device.screenHeight,
            border: 'none',
            background: '#fff',
            display: 'block',
          }}
        />
      ) : (
        <div
          style={{
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center',
            height: '100%',
            color: '#555',
            fontFamily: 'var(--font-sans)',
            fontSize: '14px',
            gap: '8px',
          }}
        >
          <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
            <circle cx="12" cy="12" r="10" />
            <path d="M2 12h20M12 2a15.3 15.3 0 014 10 15.3 15.3 0 01-4 10 15.3 15.3 0 01-4-10 15.3 15.3 0 014-10z" />
          </svg>
          <span>Enter a URL to begin</span>
        </div>
      )}
    </div>
  );
};
