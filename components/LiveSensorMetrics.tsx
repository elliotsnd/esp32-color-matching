import React, { useState, useEffect, useRef } from 'react';
import { DEVICE_BASE_URL } from '../constants';

interface SensorMetrics {
  success: boolean;
  timestamp: number;
  sensorReadings: {
    x: number;
    y: number;
    z: number;
    ir: number;
    status: number;
  };
  metrics: {
    controlVariable: number;
    irRatio: number;
    saturated: boolean;
    inOptimalRange: boolean;
    targetMin: number;
    targetMax: number;
  };
  ledStatus: {
    currentBrightness: number;
    enhancedMode: boolean;
    manualIntensity: number;
    isScanning: boolean;
  };
  statusIndicators: {
    controlVariableStatus: 'optimal' | 'high' | 'low';
    saturationStatus: 'saturated' | 'normal';
    irContaminationStatus: 'contaminated' | 'clean';
    signalStatus: 'low' | 'adequate';
  };
  enhancedControl: {
    available: boolean;
    inOptimalRange: boolean;
  };
}

interface LiveSensorMetricsProps {
  isActive?: boolean;
  updateInterval?: number;
}

export default function LiveSensorMetrics({ 
  isActive = true, 
  updateInterval = 1500 
}: LiveSensorMetricsProps) {
  const [metrics, setMetrics] = useState<SensorMetrics | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const intervalRef = useRef<NodeJS.Timeout | null>(null);

  useEffect(() => {
    if (isActive) {
      startPolling();
    } else {
      stopPolling();
    }

    return () => stopPolling();
  }, [isActive, updateInterval]);

  const startPolling = () => {
    stopPolling(); // Clear any existing interval
    
    // Initial fetch
    fetchMetrics();
    
    // Set up polling
    intervalRef.current = setInterval(fetchMetrics, updateInterval);
  };

  const stopPolling = () => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
  };

  const fetchMetrics = async () => {
    try {
      const response = await fetch(`${DEVICE_BASE_URL}/live-metrics`, {
        method: 'GET',
        headers: {
          'Accept': 'application/json',
        },
      });

      if (response.ok) {
        const contentType = response.headers.get('content-type');
        if (contentType && contentType.includes('application/json')) {
          const data = await response.json();
          setMetrics(data);
          setIsConnected(true);
          setError(null);
        } else {
          throw new Error('Live metrics endpoint not available - firmware update needed');
        }
      } else {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
    } catch (err) {
      setIsConnected(false);
      setError(err instanceof Error ? err.message : 'Unknown error');
    }
  };

  const getStatusColor = (status: string): string => {
    switch (status) {
      case 'optimal':
      case 'normal':
      case 'clean':
      case 'adequate':
        return 'text-green-400 bg-green-900/30';
      case 'high':
      case 'low':
      case 'saturated':
      case 'contaminated':
        return 'text-red-400 bg-red-900/30';
      default:
        return 'text-yellow-400 bg-yellow-900/30';
    }
  };

  const formatValue = (value: number, decimals: number = 0): string => {
    return value.toFixed(decimals);
  };

  if (!isActive) {
    return (
      <div className="bg-slate-800 rounded-lg p-6 border border-slate-600">
        <h3 className="text-lg font-semibold text-slate-200 mb-4">Live Sensor Metrics</h3>
        <p className="text-slate-400 text-center">Monitoring disabled</p>
      </div>
    );
  }

  return (
    <div className="bg-slate-800 rounded-lg p-6 border border-slate-600">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-semibold text-slate-200">Live Sensor Metrics</h3>
        <div className="flex items-center space-x-2">
          <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-400' : 'bg-red-400'}`}></div>
          <span className="text-xs text-slate-400">
            {isConnected ? 'Connected' : 'Disconnected'}
          </span>
        </div>
      </div>

      {error && (
        <div className="mb-4 p-3 rounded-md text-sm bg-red-900/30 text-red-400 border border-red-700">
          Connection Error: {error}
        </div>
      )}

      {metrics ? (
        <div className="space-y-4">
          {/* Sensor Readings */}
          <div>
            <h4 className="text-sm font-medium text-slate-300 mb-2">Current Sensor Readings</h4>
            <div className="grid grid-cols-2 gap-3 text-sm">
              <div className="bg-slate-700/50 rounded p-2">
                <span className="text-slate-400">X (Red):</span>
                <span className="text-slate-200 ml-2 font-mono">{metrics.sensorReadings.x}</span>
              </div>
              <div className="bg-slate-700/50 rounded p-2">
                <span className="text-slate-400">Y (Green):</span>
                <span className="text-slate-200 ml-2 font-mono">{metrics.sensorReadings.y}</span>
              </div>
              <div className="bg-slate-700/50 rounded p-2">
                <span className="text-slate-400">Z (Blue):</span>
                <span className="text-slate-200 ml-2 font-mono">{metrics.sensorReadings.z}</span>
              </div>
              <div className="bg-slate-700/50 rounded p-2">
                <span className="text-slate-400">IR:</span>
                <span className="text-slate-200 ml-2 font-mono">{metrics.sensorReadings.ir}</span>
              </div>
            </div>
          </div>

          {/* Control Variable and Status */}
          <div>
            <h4 className="text-sm font-medium text-slate-300 mb-2">Control Metrics</h4>
            <div className="space-y-2">
              <div className="flex justify-between items-center">
                <span className="text-slate-400">Control Variable:</span>
                <div className="flex items-center space-x-2">
                  <span className="text-slate-200 font-mono">{metrics.metrics.controlVariable}</span>
                  <span className={`px-2 py-1 rounded text-xs ${getStatusColor(metrics.statusIndicators.controlVariableStatus)}`}>
                    {metrics.statusIndicators.controlVariableStatus}
                  </span>
                </div>
              </div>
              
              <div className="flex justify-between items-center">
                <span className="text-slate-400">Target Range:</span>
                <span className="text-slate-200 font-mono">
                  {metrics.metrics.targetMin} - {metrics.metrics.targetMax}
                </span>
              </div>

              <div className="flex justify-between items-center">
                <span className="text-slate-400">IR Ratio:</span>
                <div className="flex items-center space-x-2">
                  <span className="text-slate-200 font-mono">{formatValue(metrics.metrics.irRatio, 3)}</span>
                  <span className={`px-2 py-1 rounded text-xs ${getStatusColor(metrics.statusIndicators.irContaminationStatus)}`}>
                    {metrics.statusIndicators.irContaminationStatus}
                  </span>
                </div>
              </div>
            </div>
          </div>

          {/* LED Status */}
          <div>
            <h4 className="text-sm font-medium text-slate-300 mb-2">LED Status</h4>
            <div className="space-y-2">
              <div className="flex justify-between items-center">
                <span className="text-slate-400">Current Brightness:</span>
                <span className="text-slate-200 font-mono">{metrics.ledStatus.currentBrightness}</span>
              </div>
              
              <div className="flex justify-between items-center">
                <span className="text-slate-400">Enhanced Mode:</span>
                <span className={`px-2 py-1 rounded text-xs ${
                  metrics.ledStatus.enhancedMode 
                    ? 'text-green-400 bg-green-900/30' 
                    : 'text-yellow-400 bg-yellow-900/30'
                }`}>
                  {metrics.ledStatus.enhancedMode ? 'Enabled' : 'Manual'}
                </span>
              </div>

              {!metrics.ledStatus.enhancedMode && (
                <div className="flex justify-between items-center">
                  <span className="text-slate-400">Manual Intensity:</span>
                  <span className="text-slate-200 font-mono">{metrics.ledStatus.manualIntensity}</span>
                </div>
              )}

              <div className="flex justify-between items-center">
                <span className="text-slate-400">Scanning:</span>
                <span className={`px-2 py-1 rounded text-xs ${
                  metrics.ledStatus.isScanning 
                    ? 'text-blue-400 bg-blue-900/30' 
                    : 'text-slate-400 bg-slate-700/50'
                }`}>
                  {metrics.ledStatus.isScanning ? 'Active' : 'Idle'}
                </span>
              </div>
            </div>
          </div>

          {/* Status Summary */}
          <div>
            <h4 className="text-sm font-medium text-slate-300 mb-2">Status Summary</h4>
            <div className="grid grid-cols-2 gap-2 text-xs">
              <div className={`px-2 py-1 rounded ${getStatusColor(metrics.statusIndicators.saturationStatus)}`}>
                Saturation: {metrics.statusIndicators.saturationStatus}
              </div>
              <div className={`px-2 py-1 rounded ${getStatusColor(metrics.statusIndicators.signalStatus)}`}>
                Signal: {metrics.statusIndicators.signalStatus}
              </div>
            </div>
          </div>

          {/* Timestamp */}
          <div className="text-xs text-slate-500 text-center">
            Last updated: {new Date(metrics.timestamp).toLocaleTimeString()}
          </div>
        </div>
      ) : (
        <div className="text-center text-slate-400">
          <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-blue-400 mx-auto mb-2"></div>
          Loading sensor metrics...
        </div>
      )}
    </div>
  );
}
