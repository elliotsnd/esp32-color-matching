// ESP32 Color Matcher Constants

// Device Configuration
export const DEVICE_BASE_URL = 'http://192.168.0.152';

// API Paths
export const API_PATHS = {
  SETTINGS: '/settings',
  SETTINGS_PAGE: '/settings-page',
  STATUS: '/status',
  SCAN: '/scan',
  SAMPLES: '/samples',
  CALIBRATE: '/calibrate',
  BRIGHTNESS: '/brightness',
} as const;

// UI Constants
export const COLORS = {
  PRIMARY: 'blue',
  SECONDARY: 'slate',
  SUCCESS: 'green',
  ERROR: 'red',
  WARNING: 'amber',
} as const;

// Settings Ranges - Match ESP32 validation constraints
export const SETTINGS_RANGES = {
  ATIME: { min: 0, max: 255 },
  AGAIN: { min: 0, max: 3 },
  BRIGHTNESS: { min: 64, max: 255 }, // ESP32 MIN_LED_BRIGHTNESS = 64
  AUTO_ZERO_FREQ: { min: 0, max: 255 },
  WAIT_TIME: { min: 0, max: 255 },
} as const;

// Default Values - Optimized for stability and consistency
export const DEFAULTS = {
  ATIME: 150,        // Optimal integration time for stability
  AGAIN: 1,          // 16x gain - optimal for most conditions
  BRIGHTNESS: 128,   // Fixed brightness for consistency
  AUTO_ZERO_MODE: 1, // Use previous offset (recommended for stability)
  AUTO_ZERO_FREQ: 127, // Auto-zero frequency for stability
  WAIT_TIME: 50,     // Wait time for measurement stability
} as const;
