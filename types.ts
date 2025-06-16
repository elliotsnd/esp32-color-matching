
export interface DeviceStatus {
  isScanning: boolean;
  ledState: boolean;
  isCalibrated: boolean;
  currentR: number;
  currentG: number;
  currentB: number;
  sampleCount: number;
  atime: number;
  again: number;
  brightness: number;
  ambientLux: number;
  // TCS3430 Advanced Calibration Settings
  autoZeroMode: number;
  autoZeroFreq: number;
  waitTime: number;
  esp32IP?: string;
  clientIP?: string;
  gateway?: string;
  subnet?: string;
  macAddress?: string;
  rssi?: number;
}

export interface ColorSample {
  r: number;
  g: number;
  b: number;
  timestamp: number;
  paintName: string;
  paintCode: string;
  lrv: number;
}

export interface ScannedColorData {
  r: number;
  g: number;
  b: number;
  x: number;
  y: number;
  z: number;
  ir: number;
}

export enum CalibrationState {
  CAL_IDLE = 0,
  CAL_WHITE_COUNTDOWN = 1,
  CAL_WHITE_SCANNING = 2,
  CAL_WHITE_COMPLETE = 3,
  CAL_BLACK_PROMPT = 4,
  CAL_BLACK_COUNTDOWN = 5,
  CAL_BLACK_SCANNING = 6,
  CAL_BLACK_COMPLETE = 7,
  CAL_SAVING = 8,
  CAL_COMPLETE = 9,
  CAL_ERROR = 10
}

export interface CalibrationStatusResponse {
  success: boolean;
  inProgress: boolean;
  state: CalibrationState;
  message: string;
  sessionId?: string;
  countdown?: number;
  whiteComplete?: boolean;
  whiteData?: { x: number; y: number; z: number; ir: number };
  blackComplete?: boolean;
  blackData?: { x: number; y: number; z: number; ir: number };
  complete?: boolean;
  error?: boolean; // If overall status is error
  errorMessage?: string; // from server if success:false
}

export interface CalibrationDataPoint {
  x: number;
  y: number;
  z: number;
  ir: number;
  brightness?: number; // Only for white cal
}

// Standard Calibration Types
export interface StandardCalibrationResponse {
  success: boolean;
  message: string;
}

// Matrix Calibration Types
export interface ColorReference {
  name: string;
  r: number;
  g: number;
  b: number;
  measured?: boolean;
}

export interface CalibrationPoint {
  name: string;
  refR: number;
  refG: number;
  refB: number;
  sensorR: number;
  sensorG: number;
  sensorB: number;
  deltaE: number;
}

export interface CalibrationMatrix {
  matrix: number[][];
  numPoints: number;
  avgDeltaE: number;
  maxDeltaE: number;
  timestamp: number;
  valid: boolean;
}

export interface CalibrationStats {
  meanDeltaE: number;
  stdDeltaE: number;
  maxDeltaE: number;
  pointsUnder2: number;
  pointsUnder5: number;
  totalPoints: number;
  qualityScore: number;
}

export interface MatrixCalibrationStatus {
  success: boolean;
  initialized: boolean;
  matrixValid: boolean;
  numPoints: number;
  avgDeltaE?: number;
  maxDeltaE?: number;
  timestamp?: number;
  numMatrixPoints?: number;
  qualityScore?: number;
  pointsUnder2?: number;
  pointsUnder5?: number;
}

export interface MatrixCalibrationStartResponse {
  success: boolean;
  message: string;
  colorsLoaded: number;
  totalColors: number;
  colorReferences: ColorReference[];
}

export interface MatrixCalibrationMeasureRequest {
  colorName: string;
  r: number;
  g: number;
  b: number;
}

export interface MatrixCalibrationMeasureResponse {
  success: boolean;
  message?: string;
  error?: string;
  colorName?: string;
  numPoints?: number;
  sensorR?: number;
  sensorG?: number;
  sensorB?: number;
  sensorC?: number;
}

export interface MatrixCalibrationComputeResponse {
  success: boolean;
  message?: string;
  error?: string;
  numPoints?: number;
  avgDeltaE?: number;
  maxDeltaE?: number;
  qualityScore?: number;
  pointsUnder2?: number;
  pointsUnder5?: number;
  timestamp?: number;
  quality?: string;
  matrix?: number[][];
}

export interface MatrixCalibrationResultsResponse {
  success: boolean;
  matrixValid: boolean;
  numPoints: number;
  avgDeltaE?: number;
  maxDeltaE?: number;
  qualityScore?: number;
  pointsUnder2?: number;
  pointsUnder5?: number;
  timestamp?: number;
  calibrationPoints?: CalibrationPoint[];
}

export interface MatrixCalibrationApplyResponse {
  success: boolean;
  message?: string;
  error?: string;
  avgDeltaE?: number;
  maxDeltaE?: number;
  numPoints?: number;
}
    