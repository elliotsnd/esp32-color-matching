import React, { useState, useEffect } from 'react';
import Button from './ui/Button';
import Card from './ui/Card';

interface TCS3430CalibrationPanelProps {
  showToast: (message: string, type: 'success' | 'error', customDuration?: number) => void;
}

interface TCS3430CalibrationStatus {
  success: boolean;
  initialized: boolean;
  calibrationValid: boolean;
  dualModeEnabled: boolean;
  lowIRValid: boolean;
  highIRValid: boolean;
  numReferences: number;
  currentState: number;
  lastError: number;
  qualityScore?: number;
  meanDeltaE?: number;
  maxDeltaE?: number;
  pointsUnder2?: number;
  pointsUnder5?: number;
  irThresholdLow: number;
  irThresholdHigh: number;
  lastAutoZero: number;
  autoZeroAge: number;
}

interface SensorDiagnostics {
  success: boolean;
  initialized: boolean;
  calibrationValid: boolean;
  currentReading: {
    r: number;
    g: number;
    b: number;
    ir: number;
    valid: boolean;
    saturated: boolean;
    timestamp: number;
  };
  sensorConfig: {
    atime: number;
    again: number;
    wtime: number;
    autoZeroEnabled: boolean;
    autoZeroFrequency: number;
  };
  calibration: {
    dualModeEnabled: boolean;
    lowIRValid: boolean;
    highIRValid: boolean;
    irThresholdLow: number;
    irThresholdHigh: number;
    lowIR?: {
      kX: number;
      kY: number;
      kZ: number;
      source: string;
      timestamp: number;
      qualityScore: number;
    };
    highIR?: {
      kX: number;
      kY: number;
      kZ: number;
      source: string;
      timestamp: number;
      qualityScore: number;
    };
  };
  lastAutoZero: number;
  currentState: number;
  lastError: number;
  numReferences: number;
  calibrationStats?: {
    meanDeltaE: number;
    stdDeltaE: number;
    maxDeltaE: number;
    pointsUnder2: number;
    pointsUnder5: number;
    qualityScore: number;
  };
}

const TCS3430CalibrationPanel: React.FC<TCS3430CalibrationPanelProps> = ({ showToast }) => {
  const [status, setStatus] = useState<TCS3430CalibrationStatus | null>(null);
  const [diagnostics, setDiagnostics] = useState<SensorDiagnostics | null>(null);
  const [isLoading, setIsLoading] = useState(false);
  const [showDiagnostics, setShowDiagnostics] = useState(false);

  useEffect(() => {
    loadStatus();
  }, []);

  const loadStatus = async () => {
    try {
      const response = await fetch('/tcs3430-calibration/status');
      if (response.ok) {
        const statusData = await response.json();
        setStatus(statusData);
      } else {
        console.error('Failed to load TCS3430 calibration status');
      }
    } catch (error) {
      console.error('Failed to load TCS3430 calibration status:', error);
      showToast('Failed to load calibration status', 'error');
    }
  };

  const loadDiagnostics = async () => {
    try {
      const response = await fetch('/tcs3430-calibration/diagnostics');
      if (response.ok) {
        const diagnosticsData = await response.json();
        setDiagnostics(diagnosticsData);
      } else {
        console.error('Failed to load TCS3430 diagnostics');
      }
    } catch (error) {
      console.error('Failed to load TCS3430 diagnostics:', error);
      showToast('Failed to load diagnostics', 'error');
    }
  };

  const performAutoZero = async () => {
    setIsLoading(true);
    try {
      const response = await fetch('/tcs3430-calibration/auto-zero', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
      });
      
      const result = await response.json();
      if (result.success) {
        showToast('Auto-zero calibration completed successfully', 'success');
        await loadStatus();
      } else {
        showToast(`Auto-zero failed: ${result.error}`, 'error');
      }
    } catch (error) {
      console.error('Failed to perform auto-zero:', error);
      showToast('Failed to perform auto-zero calibration', 'error');
    } finally {
      setIsLoading(false);
    }
  };

  const exportCalibrationData = async () => {
    setIsLoading(true);
    try {
      const response = await fetch('/tcs3430-calibration/export-data');
      const result = await response.json();
      
      if (result.success) {
        showToast(`Calibration data exported to ${result.filename}`, 'success');
      } else {
        showToast(`Export failed: ${result.error}`, 'error');
      }
    } catch (error) {
      console.error('Failed to export calibration data:', error);
      showToast('Failed to export calibration data', 'error');
    } finally {
      setIsLoading(false);
    }
  };

  const getQualityColor = (score: number) => {
    if (score >= 90) return 'text-green-400';
    if (score >= 70) return 'text-yellow-400';
    if (score >= 50) return 'text-orange-400';
    return 'text-red-400';
  };

  const getQualityLabel = (score: number) => {
    if (score >= 90) return 'Excellent';
    if (score >= 70) return 'Good';
    if (score >= 50) return 'Fair';
    return 'Poor';
  };

  const formatTimestamp = (timestamp: number) => {
    if (timestamp === 0) return 'Never';
    const age = Date.now() - timestamp;
    const minutes = Math.floor(age / 60000);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);
    
    if (days > 0) return `${days}d ago`;
    if (hours > 0) return `${hours}h ago`;
    if (minutes > 0) return `${minutes}m ago`;
    return 'Just now';
  };

  return (
    <Card className="bg-slate-800 border-slate-600">
      <div className="space-y-6">
        <div className="flex items-center justify-between">
          <h3 className="text-lg font-semibold text-slate-100">Advanced TCS3430 Calibration</h3>
          {status?.calibrationValid && (
            <div className="flex items-center space-x-2">
              <div className="w-2 h-2 bg-green-400 rounded-full"></div>
              <span className="text-sm text-green-400">Active</span>
            </div>
          )}
        </div>

        {/* Current Status */}
        {status && (
          <div className="bg-slate-700 p-3 rounded-lg border border-slate-600">
            <div className="grid grid-cols-2 gap-4 text-sm">
              <div>
                <span className="text-slate-400">Status:</span>
                <span className={`ml-2 ${status.calibrationValid ? 'text-green-400' : 'text-slate-300'}`}>
                  {status.calibrationValid ? 'Calibrated' : 'Not Calibrated'}
                </span>
              </div>
              <div>
                <span className="text-slate-400">Mode:</span>
                <span className="ml-2 text-slate-300">
                  {status.dualModeEnabled ? 'Dual-Matrix' : 'Single-Matrix'}
                </span>
              </div>
              <div>
                <span className="text-slate-400">Low-IR Matrix:</span>
                <span className={`ml-2 ${status.lowIRValid ? 'text-green-400' : 'text-red-400'}`}>
                  {status.lowIRValid ? 'Valid' : 'Invalid'}
                </span>
              </div>
              <div>
                <span className="text-slate-400">High-IR Matrix:</span>
                <span className={`ml-2 ${status.highIRValid ? 'text-green-400' : 'text-red-400'}`}>
                  {status.highIRValid ? 'Valid' : 'Invalid'}
                </span>
              </div>
              {status.calibrationValid && status.qualityScore !== undefined && (
                <>
                  <div>
                    <span className="text-slate-400">Quality:</span>
                    <span className={`ml-2 ${getQualityColor(status.qualityScore)}`}>
                      {getQualityLabel(status.qualityScore)} ({status.qualityScore.toFixed(1)}%)
                    </span>
                  </div>
                  <div>
                    <span className="text-slate-400">Avg ΔE:</span>
                    <span className="ml-2 text-slate-300">{status.meanDeltaE?.toFixed(2)}</span>
                  </div>
                </>
              )}
              <div>
                <span className="text-slate-400">Last Auto-Zero:</span>
                <span className="ml-2 text-slate-300">{formatTimestamp(status.lastAutoZero)}</span>
              </div>
              <div>
                <span className="text-slate-400">IR Thresholds:</span>
                <span className="ml-2 text-slate-300">
                  {status.irThresholdLow.toFixed(2)} - {status.irThresholdHigh.toFixed(2)}
                </span>
              </div>
            </div>
          </div>
        )}

        {/* Control Buttons */}
        <div className="space-y-3">
          <div className="flex space-x-3">
            <Button
              onClick={performAutoZero}
              variant="primary"
              isLoading={isLoading}
              className="flex-1"
            >
              Perform Auto-Zero
            </Button>
            
            <Button
              onClick={exportCalibrationData}
              variant="secondary"
              isLoading={isLoading}
            >
              Export Data
            </Button>
          </div>

          <div className="flex space-x-3">
            <Button
              onClick={() => {
                setShowDiagnostics(!showDiagnostics);
                if (!showDiagnostics) loadDiagnostics();
              }}
              variant="secondary"
              className="flex-1"
            >
              {showDiagnostics ? 'Hide' : 'Show'} Diagnostics
            </Button>
            
            <Button
              onClick={loadStatus}
              variant="secondary"
            >
              Refresh
            </Button>
          </div>
        </div>

        {/* Diagnostics Panel */}
        {showDiagnostics && diagnostics && (
          <div className="bg-slate-700 p-3 rounded-lg border border-slate-600 space-y-4">
            <h4 className="text-slate-300 font-medium">Sensor Diagnostics</h4>
            
            {/* Current Reading */}
            <div className="bg-slate-800 p-3 rounded border border-slate-600">
              <h5 className="text-slate-400 text-sm font-medium mb-2">Current Reading</h5>
              <div className="grid grid-cols-2 gap-2 text-xs">
                <div>R: {diagnostics.currentReading.r}</div>
                <div>G: {diagnostics.currentReading.g}</div>
                <div>B: {diagnostics.currentReading.b}</div>
                <div>IR: {diagnostics.currentReading.ir}</div>
                <div className={`col-span-2 ${diagnostics.currentReading.saturated ? 'text-red-400' : 'text-green-400'}`}>
                  {diagnostics.currentReading.saturated ? 'SATURATED' : 'Normal'}
                </div>
              </div>
            </div>

            {/* Sensor Configuration */}
            <div className="bg-slate-800 p-3 rounded border border-slate-600">
              <h5 className="text-slate-400 text-sm font-medium mb-2">Sensor Configuration</h5>
              <div className="grid grid-cols-2 gap-2 text-xs">
                <div>ATIME: {diagnostics.sensorConfig.atime}</div>
                <div>AGAIN: {diagnostics.sensorConfig.again}x</div>
                <div>WTIME: {diagnostics.sensorConfig.wtime}</div>
                <div>Auto-Zero: {diagnostics.sensorConfig.autoZeroEnabled ? 'ON' : 'OFF'}</div>
                <div className="col-span-2">Auto-Zero Freq: {diagnostics.sensorConfig.autoZeroFrequency}</div>
              </div>
            </div>

            {/* Calibration Stats */}
            {diagnostics.calibrationStats && (
              <div className="bg-slate-800 p-3 rounded border border-slate-600">
                <h5 className="text-slate-400 text-sm font-medium mb-2">Calibration Statistics</h5>
                <div className="grid grid-cols-2 gap-2 text-xs">
                  <div>Mean ΔE: {diagnostics.calibrationStats.meanDeltaE.toFixed(2)}</div>
                  <div>Max ΔE: {diagnostics.calibrationStats.maxDeltaE.toFixed(2)}</div>
                  <div>Points &lt; 2: {diagnostics.calibrationStats.pointsUnder2}</div>
                  <div>Points &lt; 5: {diagnostics.calibrationStats.pointsUnder5}</div>
                  <div className="col-span-2">
                    Quality: <span className={getQualityColor(diagnostics.calibrationStats.qualityScore)}>
                      {diagnostics.calibrationStats.qualityScore.toFixed(1)}%
                    </span>
                  </div>
                </div>
              </div>
            )}
          </div>
        )}

        {/* Information Panel */}
        <div className="bg-blue-900/30 p-3 rounded-lg border border-blue-600">
          <p className="text-sm text-blue-300 mb-2">
            Advanced TCS3430 calibration provides professional-grade colorimetric accuracy using dual-matrix IR compensation.
          </p>
          <p className="text-xs text-slate-400">
            Features include auto-zero calibration, IR-based matrix blending, and comprehensive sensor diagnostics.
          </p>
        </div>
      </div>
    </Card>
  );
};

export default TCS3430CalibrationPanel;
