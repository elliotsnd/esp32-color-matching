
import {
  DeviceStatus,
  ColorSample,
  ScannedColorData,
  CalibrationStatusResponse,
  MatrixCalibrationStatus,
  MatrixCalibrationStartResponse,
  MatrixCalibrationMeasureRequest,
  MatrixCalibrationMeasureResponse,
  MatrixCalibrationComputeResponse,
  MatrixCalibrationResultsResponse,
  MatrixCalibrationApplyResponse
} from '../types';
import { DEVICE_BASE_URL } from '../constants';

const API_BASE_URL = DEVICE_BASE_URL; // Use ESP32 device IP address

async function handleResponse<T,>(response: Response): Promise<T> {
  // Check for "application/json" content type before parsing
  const contentType = response.headers.get("content-type");

  if (!response.ok) {
    let errorText: string;
    try {
      if (contentType && contentType.includes("application/json")) {
        const errorData = await response.json();
        errorText = errorData.error || errorData.message || JSON.stringify(errorData);
      } else {
        errorText = await response.text();
      }
    } catch {
      errorText = `${response.status} ${response.statusText}`;
    }
    throw new Error(`API Error: ${response.status} ${response.statusText} - ${errorText}`);
  }

  if (contentType && contentType.includes("application/json")) {
    return response.json() as Promise<T>;
  }
  // For text/plain responses, we might need to handle them differently or expect specific structures
  // For now, if not JSON, assume it's a simple text confirmation and wrap it if needed by the caller
  return response.text() as unknown as Promise<T>;
}


export async function getDeviceStatus(): Promise<DeviceStatus> {
  const response = await fetch(`${API_BASE_URL}/status`);
  return handleResponse<DeviceStatus>(response);
}

export async function startScan(): Promise<ScannedColorData> {
  const response = await fetch(`${API_BASE_URL}/scan`, { method: 'POST' });
  return handleResponse<ScannedColorData>(response);
}

export async function saveSample(r: number, g: number, b: number): Promise<string> {
  const response = await fetch(`${API_BASE_URL}/save`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ r, g, b }),
  });
  return handleResponse<string>(response); // Expects "Sample saved" or error
}

export async function getSavedSamples(): Promise<{ samples: ColorSample[] }> {
  const response = await fetch(`${API_BASE_URL}/samples`);
  return handleResponse<{ samples: ColorSample[] }>(response);
}

export async function deleteSample(index: number): Promise<{ success: boolean; message: string }> {
  const response = await fetch(`${API_BASE_URL}/delete`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ index }),
  });
  return handleResponse<{ success: boolean; message: string }>(response);
}

export async function clearAllSamples(): Promise<{ success: boolean; message: string }> {
  const response = await fetch(`${API_BASE_URL}/samples/clear`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ success: boolean; message: string }>(response);
}

export async function updateSettings(settings: Partial<DeviceStatus>): Promise<string> {
  const response = await fetch(`${API_BASE_URL}/settings`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(settings),
  });
  return handleResponse<string>(response); // Expects "Settings saved"
}

export async function setLedBrightness(payload: {
  brightness: number;
  r?: number;
  g?: number;
  b?: number;
  keepOn?: boolean;
  enhancedMode?: boolean;
}): Promise<{success: boolean, brightness: number, actualBrightness: number, ledState: boolean, message: string}> {
  const response = await fetch(`${API_BASE_URL}/brightness`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });
  return handleResponse<{success: boolean, brightness: number, actualBrightness: number, ledState: boolean, message: string}>(response);
}


// Calibration API
export async function startCalibration(brightness: number): Promise<{success: boolean, sessionId: string, message: string, countdown: number}> {
  const response = await fetch(`${API_BASE_URL}/calibrate/start`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ brightness }),
  });
  return handleResponse<{success: boolean, sessionId: string, message: string, countdown: number}>(response);
}

export async function getCalibrationStatus(): Promise<CalibrationStatusResponse> {
  const response = await fetch(`${API_BASE_URL}/calibrate/status`);
  return handleResponse<CalibrationStatusResponse>(response);
}



export async function saveCalibration(): Promise<{success: boolean, message: string, hasWhite: boolean, hasBlack: boolean}> {
  const response = await fetch(`${API_BASE_URL}/calibrate/save`, { method: 'POST' });
  return handleResponse<{success: boolean, message: string, hasWhite: boolean, hasBlack: boolean}>(response);
}

export async function cancelCalibration(): Promise<{success: boolean, message: string}> {
  const response = await fetch(`${API_BASE_URL}/calibrate/cancel`, { method: 'POST' });
  return handleResponse<{success: boolean, message: string}>(response);
}

// Standard Calibration API - Using actual ESP32 endpoint
export async function performWhiteCalibration(brightness: number = 128): Promise<{success: boolean, message: string}> {
  const response = await fetch(`${API_BASE_URL}/calibrate/standard/white`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ brightness }),
  });
  return handleResponse<{success: boolean, message: string}>(response);
}

// Black point calibration - measures dark reference with LEDs OFF
export async function performBlackCalibration(): Promise<{success: boolean, message: string}> {
  try {
    const response = await fetch(`${DEVICE_BASE_URL}/calibrate/standard/black`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const result = await response.json();
    return {
      success: result.success,
      message: result.message || 'Black calibration completed'
    };
  } catch (error) {
    console.error('Black calibration failed:', error);
    return {
      success: false,
      message: error instanceof Error ? error.message : 'Black calibration failed'
    };
  }
}

export async function performVividWhiteCalibration(brightness: number = 128): Promise<{success: boolean, message: string}> {
  // Use the same white calibration endpoint - it's designed for Vivid White
  return performWhiteCalibration(brightness);
}

// Matrix Calibration API
export async function getMatrixCalibrationStatus(): Promise<MatrixCalibrationStatus> {
  const response = await fetch(`${API_BASE_URL}/matrix-calibration/status`);
  return handleResponse<MatrixCalibrationStatus>(response);
}

export async function startMatrixCalibration(): Promise<MatrixCalibrationStartResponse> {
  const response = await fetch(`${API_BASE_URL}/matrix-calibration/start`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<MatrixCalibrationStartResponse>(response);
}

export async function measureMatrixCalibrationColor(request: MatrixCalibrationMeasureRequest): Promise<MatrixCalibrationMeasureResponse> {
  const response = await fetch(`${API_BASE_URL}/matrix-calibration/measure`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(request),
  });
  return handleResponse<MatrixCalibrationMeasureResponse>(response);
}

export async function computeMatrixCalibration(): Promise<MatrixCalibrationComputeResponse> {
  const response = await fetch(`${API_BASE_URL}/matrix-calibration/compute`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<MatrixCalibrationComputeResponse>(response);
}

export async function getMatrixCalibrationResults(): Promise<MatrixCalibrationResultsResponse> {
  const response = await fetch(`${API_BASE_URL}/matrix-calibration/results`);
  return handleResponse<MatrixCalibrationResultsResponse>(response);
}

export async function applyMatrixCalibration(): Promise<MatrixCalibrationApplyResponse> {
  const response = await fetch(`${API_BASE_URL}/matrix-calibration/apply`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<MatrixCalibrationApplyResponse>(response);
}

export async function clearMatrixCalibration(): Promise<{success: boolean, message: string}> {
  const response = await fetch(`${API_BASE_URL}/matrix-calibration/clear`, {
    method: 'DELETE',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{success: boolean, message: string}>(response);
}

// Removed old calibration API functions - using advanced calibration wizard only
    