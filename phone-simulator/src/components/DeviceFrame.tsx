import { useRef, forwardRef, useImperativeHandle } from 'react';
import type { DeviceConfig, Orientation, NetworkPreset, ConsoleEntry } from '../types/device';
import { StatusBar } from './StatusBar';
import { NavigationBar } from './NavigationBar';
import { HomeIndicator } from './HomeIndicator';
import { ScreenContent } from './ScreenContent';

interface DeviceFrameProps {
  device: DeviceConfig;
  orientation: Orientation;
  url: string;
  networkPreset: NetworkPreset;
  isTransitioning: boolean;
  isRotating: boolean;
  onConsoleEntry: (entry: ConsoleEntry) => void;
}

export interface DeviceFrameHandle {
  frameElement: HTMLDivElement | null;
  screenElement: HTMLDivElement | null;
}

/** Core device frame component with photorealistic CSS chassis */
export const DeviceFrame = forwardRef<DeviceFrameHandle, DeviceFrameProps>(
  ({ device, orientation, url, networkPreset, isTransitioning, isRotating, onConsoleEntry }, ref) => {
    const frameRef = useRef<HTMLDivElement>(null);
    const screenRef = useRef<HTMLDivElement>(null);

    useImperativeHandle(ref, () => ({
      get frameElement() { return frameRef.current; },
      get screenElement() { return screenRef.current; },
    }));

    const isLandscape = orientation === 'landscape';
    const displayW = isLandscape ? device.screenHeight : device.screenWidth;
    const displayH = isLandscape ? device.screenWidth : device.screenHeight;

    const isIPhoneSE = device.id === 'iphone-se-3';
    const chassisPadding = isIPhoneSE ? '60px 16px' : '12px';
    const chassisRadius = isIPhoneSE
      ? '12px'
      : `${device.screenRadius + 12}px`;

    return (
      <div
        ref={frameRef}
        className={[
          'device-frame',
          'device-frame-glow',
          isTransitioning ? 'device-transitioning' : '',
          isRotating ? 'device-rotating' : '',
        ].filter(Boolean).join(' ')}
        style={{
          transform: isLandscape ? 'rotate(90deg)' : 'none',
          transition: 'transform 0.5s cubic-bezier(0.4, 0, 0.2, 1)',
        }}
      >
        {/* Chassis */}
        <div
          className="device-chassis"
          style={{
            padding: chassisPadding,
            borderRadius: chassisRadius,
            backgroundColor: device.frameColor,
          }}
        >
          {/* Side buttons */}
          <div className="device-button device-button--power" />
          <div className="device-button device-button--volume-up" />
          <div className="device-button device-button--volume-down" />
          {device.os === 'ios' && <div className="device-button device-button--silent" />}

          {/* iPhone SE specific elements */}
          {isIPhoneSE && (
            <>
              <div className="device-top-speaker" />
              <div className="device-home-button" />
            </>
          )}

          {/* Screen */}
          <div
            ref={screenRef}
            className="device-screen"
            style={{
              width: displayW,
              height: displayH,
              borderRadius: device.screenRadius,
            }}
          >
            {/* Notch */}
            {device.hasNotch && <div className="device-notch" />}

            {/* Dynamic Island */}
            {device.hasDynamicIsland && <div className="device-dynamic-island" />}

            {/* Punch-hole camera */}
            {device.hasPunchHole && (
              <div
                className={`device-punch-hole device-punch-hole--${device.punchHolePosition ?? 'center'}`}
              />
            )}

            {/* Status bar */}
            <StatusBar device={device} />

            {/* Screen content */}
            <ScreenContent
              url={url}
              device={device}
              networkPreset={networkPreset}
              onConsoleEntry={onConsoleEntry}
            />

            {/* Navigation bar (Android) */}
            {device.os === 'android' && (
              <NavigationBar height={device.navigationBarHeight} />
            )}

            {/* Home indicator (iOS) */}
            {device.os === 'ios' && (
              <HomeIndicator height={device.homeIndicatorHeight} />
            )}
          </div>
        </div>
      </div>
    );
  }
);

DeviceFrame.displayName = 'DeviceFrame';
