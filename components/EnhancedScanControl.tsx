import React, { useState } from 'react';
import Button from './ui/Button';
import Card from './ui/Card';
import LoadingSpinner from './LoadingSpinner';
import { DEVICE_BASE_URL } from '../constants';

interface EnhancedScanResult {
  success: boolean;
  r: number;
  g: number;
  b: number;
  x: number;
  y: number;
  z: number;
  ir1: number;
  ir2: number;
  timestamp: number;
  sensorConfig?: {
    atime: number;
    again: number;
    brightness: number;
    condition: number;
    isOptimal: boolean;
  };
  brightnessOptimization?: {
    controlVariable: number;
    targetMin: number;
    targetMax: number;
    inOptimalRange: boolean;
    optimizedBrightness: number;
  };
}

interface SensorDiagnostics {
  success: boolean;
  timestamp: number;
  sensor: {
    type: string;
    initialized: boolean;
  };
  currentReadings: {
    x: number;
    y: number;
    z: number;
    ir1: number;
    ir2: number;
    status: number;
    saturated: boolean;
  };
  dynamicSensor: {
    enabled: boolean;
    initialized: boolean;
    currentConfig?: {
      atime: number;
      again: number;
      brightness: number;
      condition: number;
      isOptimal: boolean;
      timestamp: number;
    };
    detectedCondition?: number;
    saturation?: boolean;
    signalAdequate?: boolean;
  };
}

const LIGHTING_CONDITIONS = [
  'Dark/Low-light',
  'Indoor',
  'Bright',
  'Very Bright'
];

const GAIN_VALUES = ['1x', '4x', '16x', '64x'];

export const EnhancedScanControl: React.FC = () => {
  const [isScanning, setIsScanning] = useState(false);
  const [scanResult, setScanResult] = useState<EnhancedScanResult | null>(null);
  const [diagnostics, setDiagnostics] = useState<SensorDiagnostics | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [showDiagnostics, setShowDiagnostics] = useState(false);

  const performEnhancedScan = async () => {
    setIsScanning(true);
    setError(null);
    
    try {
      const response = await fetch(`${DEVICE_BASE_URL}/enhanced-scan`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
      });
      
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      
      const result: EnhancedScanResult = await response.json();
      setScanResult(result);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Enhanced scan failed');
      console.error('Enhanced scan error:', err);
    } finally {
      setIsScanning(false);
    }
  };

  const fetchDiagnostics = async () => {
    try {
      const response = await fetch(`${DEVICE_BASE_URL}/sensor-diagnostics`);
      
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      
      const result: SensorDiagnostics = await response.json();
      setDiagnostics(result);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to fetch diagnostics');
      console.error('Diagnostics error:', err);
    }
  };

  const formatTimestamp = (timestamp: number) => {
    return new Date(timestamp).toLocaleTimeString();
  };

  return (
    <div className="space-y-6">
      <Card className="p-6">
        <h2 className="text-xl font-semibold text-slate-100 mb-4">
          Enhanced Color Scanning
        </h2>
        <p className="text-slate-300 text-sm mb-4">
          Uses dynamic sensor optimization for accurate color matching across all lighting conditions.
        </p>
        
        <div className="flex gap-3 mb-4">
          <Button
            onClick={performEnhancedScan}
            disabled={isScanning}
            className="flex-1"
          >
            {isScanning ? (
              <>
                <LoadingSpinner size="sm" />
                <span className="ml-2">Scanning...</span>
              </>
            ) : (
              'Enhanced Scan'
            )}
          </Button>
          
          <Button
            onClick={fetchDiagnostics}
            variant="secondary"
            className="flex-1"
          >
            Sensor Diagnostics
          </Button>
        </div>

        {error && (
          <div className="bg-red-900/50 border border-red-500 rounded-lg p-3 mb-4">
            <p className="text-red-200 text-sm">{error}</p>
          </div>
        )}

        {scanResult && (
          <div className="bg-slate-800 rounded-lg p-4 mb-4">
            <h3 className="text-lg font-medium text-slate-100 mb-3">Scan Results</h3>
            
            <div className="grid grid-cols-2 gap-4 mb-4">
              <div>
                <h4 className="text-sm font-medium text-slate-300 mb-2">Color Values</h4>
                <div className="space-y-1 text-sm">
                  <div className="flex justify-between">
                    <span className="text-slate-400">RGB:</span>
                    <span className="text-slate-200">
                      ({scanResult.r}, {scanResult.g}, {scanResult.b})
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">XYZ:</span>
                    <span className="text-slate-200">
                      ({scanResult.x}, {scanResult.y}, {scanResult.z})
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">IR:</span>
                    <span className="text-slate-200">
                      ({scanResult.ir1}, {scanResult.ir2})
                    </span>
                  </div>
                </div>
              </div>
              
              <div>
                <h4 className="text-sm font-medium text-slate-300 mb-2">Color Preview</h4>
                <div 
                  className="w-full h-16 rounded border border-slate-600"
                  style={{ 
                    backgroundColor: `rgb(${scanResult.r}, ${scanResult.g}, ${scanResult.b})` 
                  }}
                />
              </div>
            </div>

            {scanResult.sensorConfig && (
              <div className="border-t border-slate-600 pt-3">
                <h4 className="text-sm font-medium text-slate-300 mb-2">Sensor Configuration</h4>
                <div className="grid grid-cols-2 gap-2 text-sm">
                  <div className="flex justify-between">
                    <span className="text-slate-400">Integration Time:</span>
                    <span className="text-slate-200">{scanResult.sensorConfig.atime}ms</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Gain:</span>
                    <span className="text-slate-200">{GAIN_VALUES[scanResult.sensorConfig.again]}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Brightness:</span>
                    <span className="text-slate-200">{scanResult.sensorConfig.brightness}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Condition:</span>
                    <span className="text-slate-200">{LIGHTING_CONDITIONS[scanResult.sensorConfig.condition]}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Optimal:</span>
                    <span className={scanResult.sensorConfig.isOptimal ? "text-green-400" : "text-yellow-400"}>
                      {scanResult.sensorConfig.isOptimal ? "Yes" : "No"}
                    </span>
                  </div>
                </div>
              </div>
            )}

            {scanResult.brightnessOptimization && (
              <div className="border-t border-slate-600 pt-3 mt-3">
                <h4 className="text-sm font-medium text-slate-300 mb-2">Automatic Brightness Optimization</h4>
                <div className="grid grid-cols-2 gap-2 text-sm">
                  <div className="flex justify-between">
                    <span className="text-slate-400">Control Variable:</span>
                    <span className="text-slate-200">{scanResult.brightnessOptimization.controlVariable}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Target Range:</span>
                    <span className="text-slate-200">
                      {scanResult.brightnessOptimization.targetMin} - {scanResult.brightnessOptimization.targetMax}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">In Optimal Range:</span>
                    <span className={scanResult.brightnessOptimization.inOptimalRange ? "text-green-400" : "text-yellow-400"}>
                      {scanResult.brightnessOptimization.inOptimalRange ? "Yes" : "No"}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Optimized Brightness:</span>
                    <span className="text-slate-200">{scanResult.brightnessOptimization.optimizedBrightness}</span>
                  </div>
                </div>
              </div>
            )}

            <div className="text-xs text-slate-500 mt-3">
              Scanned at {formatTimestamp(scanResult.timestamp)}
            </div>
          </div>
        )}

        {diagnostics && (
          <div className="bg-slate-800 rounded-lg p-4">
            <div className="flex justify-between items-center mb-3">
              <h3 className="text-lg font-medium text-slate-100">Sensor Diagnostics</h3>
              <Button
                onClick={() => setShowDiagnostics(!showDiagnostics)}
                variant="secondary"
                size="sm"
              >
                {showDiagnostics ? 'Hide Details' : 'Show Details'}
              </Button>
            </div>
            
            <div className="grid grid-cols-2 gap-4 mb-3">
              <div>
                <h4 className="text-sm font-medium text-slate-300 mb-2">Current Readings</h4>
                <div className="space-y-1 text-sm">
                  <div className="flex justify-between">
                    <span className="text-slate-400">XYZ:</span>
                    <span className="text-slate-200">
                      ({diagnostics.currentReadings.x}, {diagnostics.currentReadings.y}, {diagnostics.currentReadings.z})
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Saturated:</span>
                    <span className={diagnostics.currentReadings.saturated ? "text-red-400" : "text-green-400"}>
                      {diagnostics.currentReadings.saturated ? "Yes" : "No"}
                    </span>
                  </div>
                </div>
              </div>
              
              <div>
                <h4 className="text-sm font-medium text-slate-300 mb-2">Dynamic Sensor</h4>
                <div className="space-y-1 text-sm">
                  <div className="flex justify-between">
                    <span className="text-slate-400">Enabled:</span>
                    <span className={diagnostics.dynamicSensor.enabled ? "text-green-400" : "text-red-400"}>
                      {diagnostics.dynamicSensor.enabled ? "Yes" : "No"}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Initialized:</span>
                    <span className={diagnostics.dynamicSensor.initialized ? "text-green-400" : "text-red-400"}>
                      {diagnostics.dynamicSensor.initialized ? "Yes" : "No"}
                    </span>
                  </div>
                  {diagnostics.dynamicSensor.detectedCondition !== undefined && (
                    <div className="flex justify-between">
                      <span className="text-slate-400">Condition:</span>
                      <span className="text-slate-200">
                        {LIGHTING_CONDITIONS[diagnostics.dynamicSensor.detectedCondition]}
                      </span>
                    </div>
                  )}
                </div>
              </div>
            </div>
            
            {showDiagnostics && (
              <div className="border-t border-slate-600 pt-3">
                <pre className="text-xs text-slate-300 bg-slate-900 p-3 rounded overflow-auto max-h-64">
                  {JSON.stringify(diagnostics, null, 2)}
                </pre>
              </div>
            )}
            
            <div className="text-xs text-slate-500 mt-3">
              Updated at {formatTimestamp(diagnostics.timestamp)}
            </div>
          </div>
        )}
      </Card>
    </div>
  );
};
