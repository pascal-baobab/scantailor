import React, { useState, useCallback, useRef } from 'react';
import type { ConsoleEntry, ThemeMode } from '../types/device';
import { useDevice } from '../hooks/useDevice';
import { useOrientation } from '../hooks/useOrientation';
import { useZoom } from '../hooks/useZoom';
import { useScreenshot } from '../hooks/useScreenshot';
import { useNetworkThrottle } from '../hooks/useNetworkThrottle';
import { Toolbar } from './Toolbar';
import { DeviceSelector } from './DeviceSelector';
import { DeviceFrame } from './DeviceFrame';
import type { DeviceFrameHandle } from './DeviceFrame';
import { DevTools } from './DevTools';
import { SideBySide } from './SideBySide';

/** Main simulator layout: toolbar + sidebar + viewport + devtools */
export const Simulator: React.FC = () => {
  const { device, selectDevice, isTransitioning } = useDevice();
  const { orientation, toggleOrientation, isRotating } = useOrientation();
  const { zoom, setZoomLevel, fitToWindow, toggleFitToWindow, containerRef, zoomPresets } = useZoom();
  const { capture } = useScreenshot();
  const { activePreset, setActivePreset } = useNetworkThrottle();

  const [url, setUrl] = useState('');
  const [refreshKey, setRefreshKey] = useState(0);
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false);
  const [sideBySideMode, setSideBySideMode] = useState(false);
  const [consoleEntries, setConsoleEntries] = useState<ConsoleEntry[]>([]);
  const [theme, setTheme] = useState<ThemeMode>('dark');

  const deviceFrameRef = useRef<DeviceFrameHandle>(null);

  const handleNavigate = useCallback((newUrl: string) => {
    setUrl(newUrl);
  }, []);

  const handleRefresh = useCallback(() => {
    setRefreshKey((k) => k + 1);
    setUrl((prev) => {
      const u = prev;
      setUrl('');
      requestAnimationFrame(() => setUrl(u));
      return prev;
    });
  }, []);

  const handleConsoleEntry = useCallback((entry: ConsoleEntry) => {
    setConsoleEntries((prev) => [...prev.slice(-99), entry]);
  }, []);

  const handleClearConsole = useCallback(() => {
    setConsoleEntries([]);
  }, []);

  const handleCaptureScreen = useCallback(() => {
    if (deviceFrameRef.current?.screenElement) {
      capture(deviceFrameRef.current.screenElement, device.name, false);
    }
  }, [capture, device.name]);

  const handleCaptureMockup = useCallback(() => {
    if (deviceFrameRef.current?.frameElement) {
      capture(deviceFrameRef.current.frameElement, device.name, true);
    }
  }, [capture, device.name]);

  const toggleTheme = useCallback(() => {
    setTheme((prev) => {
      const next = prev === 'dark' ? 'light' : 'dark';
      document.documentElement.setAttribute('data-theme', next);
      return next;
    });
  }, []);

  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'column',
        height: '100%',
        width: '100%',
        overflow: 'hidden',
      }}
    >
      {/* Toolbar */}
      <Toolbar
        device={device}
        url={url}
        onNavigate={handleNavigate}
        onRefresh={handleRefresh}
        orientation={orientation}
        onToggleOrientation={toggleOrientation}
        zoom={zoom}
        zoomPresets={zoomPresets}
        onZoomChange={setZoomLevel}
        fitToWindow={fitToWindow}
        onFitToggle={toggleFitToWindow}
        networkPreset={activePreset}
        onNetworkChange={setActivePreset}
        onCaptureScreen={handleCaptureScreen}
        onCaptureMockup={handleCaptureMockup}
        sidebarCollapsed={sidebarCollapsed}
        onToggleSidebar={() => setSidebarCollapsed(!sidebarCollapsed)}
        sideBySideMode={sideBySideMode}
        onToggleSideBySide={() => setSideBySideMode(!sideBySideMode)}
        theme={theme}
        onToggleTheme={toggleTheme}
      />

      {/* Main content area */}
      <div style={{ display: 'flex', flex: 1, overflow: 'hidden' }}>
        {/* Device Selector Sidebar */}
        <DeviceSelector
          currentDevice={device}
          onSelect={selectDevice}
          collapsed={sidebarCollapsed}
        />

        {/* Viewport area */}
        <div
          style={{
            flex: 1,
            display: 'flex',
            flexDirection: 'column',
            overflow: 'hidden',
          }}
        >
          {sideBySideMode ? (
            <SideBySide
              url={url}
              networkPreset={activePreset}
              onConsoleEntry={handleConsoleEntry}
            />
          ) : (
            <div
              ref={containerRef}
              style={{
                flex: 1,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                overflow: 'auto',
                padding: '24px',
              }}
            >
              <div
                key={refreshKey}
                style={{
                  transform: `scale(${zoom / 100})`,
                  transformOrigin: 'center center',
                  transition: 'transform 0.3s ease',
                }}
              >
                <DeviceFrame
                  ref={deviceFrameRef}
                  device={device}
                  orientation={orientation}
                  url={url}
                  networkPreset={activePreset}
                  isTransitioning={isTransitioning}
                  isRotating={isRotating}
                  onConsoleEntry={handleConsoleEntry}
                />
              </div>
            </div>
          )}

          {/* DevTools Panel */}
          <DevTools
            device={device}
            orientation={orientation}
            consoleEntries={consoleEntries}
            onClearConsole={handleClearConsole}
          />
        </div>
      </div>
    </div>
  );
};
