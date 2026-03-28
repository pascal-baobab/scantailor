/** Brand identifier for device manufacturers */
export type Brand = 'apple' | 'samsung';

/** Operating system identifier */
export type OS = 'ios' | 'android';

/** Position of the punch-hole camera on Samsung devices */
export type PunchHolePosition = 'center' | 'left' | null;

/** Orientation of the device viewport */
export type Orientation = 'portrait' | 'landscape';

/** Network throttle preset names */
export type NetworkPreset = 'none' | 'fast4g' | 'slow3g' | '2g' | 'offline';

/** Zoom level as a percentage */
export type ZoomLevel = number;

/** Theme mode for the simulator UI */
export type ThemeMode = 'dark' | 'light';

/** Complete device configuration for rendering a device frame */
export interface DeviceConfig {
  /** Unique identifier for the device */
  id: string;
  /** Display name of the device */
  name: string;
  /** Manufacturer brand */
  brand: Brand;
  /** Operating system */
  os: OS;
  /** Logical viewport width in CSS pixels */
  screenWidth: number;
  /** Logical viewport height in CSS pixels */
  screenHeight: number;
  /** Device pixel ratio */
  devicePixelRatio: number;
  /** Screen corner border-radius in pixels */
  screenRadius: number;
  /** Whether the device has an iPhone-style notch */
  hasNotch: boolean;
  /** Whether the device has an iPhone Dynamic Island */
  hasDynamicIsland: boolean;
  /** Whether the device has a Samsung punch-hole camera */
  hasPunchHole: boolean;
  /** Position of the punch-hole camera */
  punchHolePosition: PunchHolePosition;
  /** Height of the status bar in pixels */
  statusBarHeight: number;
  /** Height of the Android navigation bar in pixels (0 for iOS) */
  navigationBarHeight: number;
  /** Height of the iOS home indicator in pixels (0 for Android) */
  homeIndicatorHeight: number;
  /** Default chassis/frame color */
  frameColor: string;
  /** Release year of the device */
  year: number;
}

/** Network throttle configuration */
export interface NetworkThrottleConfig {
  /** Display name */
  name: string;
  /** Preset identifier */
  preset: NetworkPreset;
  /** Simulated latency in milliseconds */
  latencyMs: number;
  /** Download speed in Mbps */
  downloadMbps: number;
  /** Upload speed in Mbps */
  uploadMbps: number;
}

/** Console log entry captured from the iframe */
export interface ConsoleEntry {
  /** Log level */
  level: 'log' | 'warn' | 'error' | 'info';
  /** Timestamp of the log entry */
  timestamp: number;
  /** Log message content */
  message: string;
}

/** DevTools active tab */
export type DevToolsTab = 'viewport' | 'console' | 'network';

/** Side-by-side device slot */
export interface DeviceSlot {
  /** Unique slot identifier */
  id: string;
  /** Selected device config */
  device: DeviceConfig;
  /** Orientation for this slot */
  orientation: Orientation;
}
