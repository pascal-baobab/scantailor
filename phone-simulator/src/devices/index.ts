import type { DeviceConfig } from '../types/device';
import { appleDevices } from './apple';
import { samsungDevices } from './samsung';

/** Complete registry of all supported devices */
export const allDevices: DeviceConfig[] = [...appleDevices, ...samsungDevices];

/** Find a device by its unique ID */
export function getDeviceById(id: string): DeviceConfig | undefined {
  return allDevices.find((d) => d.id === id);
}

/** Get all devices for a specific brand */
export function getDevicesByBrand(brand: 'apple' | 'samsung'): DeviceConfig[] {
  return allDevices.filter((d) => d.brand === brand);
}

/** Default device to show on initial load */
export const defaultDevice: DeviceConfig =
  allDevices.find((d) => d.id === 'iphone-15-pro-max') ?? allDevices[0];
