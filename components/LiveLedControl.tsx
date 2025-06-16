
import React, { useState, useEffect, useRef } from 'react';
import * as apiService from '../services/apiService';
import Button from './ui/Button';
import Card from './ui/Card';
import Slider from './ui/Slider';
import ColorSwatch from './ColorSwatch';
import Input from './ui/Input';
import { DEVICE_BASE_URL, API_PATHS } from '../constants';

interface LiveLedControlProps {
  showToast: (message: string, type: 'success' | 'error', customDuration?: number) => void;
  initialLedState?: boolean;
}

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

interface LEDSettings {
  enhancedLEDMode: boolean;
  manualLEDIntensity: number;
}

const LiveLedControl: React.FC<LiveLedControlProps> = ({ showToast, initialLedState }) => {
  // Basic LED control state
  const [r, setR] = useState(255);
  const [g, setG] = useState(255);
  const [b, setB] = useState(255);
  const [brightness, setBrightness] = useState(128);
  const [keepOn, setKeepOn] = useState(initialLedState || false);
  const [isLoading, setIsLoading] = useState(false);

  // Enhanced LED control state
  const [enhancedMode, setEnhancedMode] = useState(false);
  const [optimizationSensitivity, setOptimizationSensitivity] = useState(128);
  const [firmwareUpdateNeeded, setFirmwareUpdateNeeded] = useState(false);

  // Live metrics state
  const [metrics, setMetrics] = useState<SensorMetrics | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [metricsError, setMetricsError] = useState<string | null>(null);
  const intervalRef = useRef<NodeJS.Timeout | null>(null);

  // Automatic brightness optimization state
  const [optimizedBrightness, setOptimizedBrightness] = useState(128);
  const [smoothedBrightness, setSmoothedBrightness] = useState(128);
  const [lastOptimizationTime, setLastOptimizationTime] = useState(0);
  const [optimizationHistory, setOptimizationHistory] = useState<number[]>([]);

  // Load enhanced LED settings and start live metrics on component mount
  useEffect(() => {
    loadEnhancedSettings();
    if (keepOn) {
      startLiveMetrics();
    }
    return () => stopLiveMetrics();
  }, []);

  // Start/stop live metrics based on LED state
  useEffect(() => {
    if (keepOn && enhancedMode) {
      startLiveMetrics();
    } else {
      stopLiveMetrics();
    }
  }, [keepOn, enhancedMode]);

  const loadEnhancedSettings = async () => {
    try {
      const response = await fetch(`${DEVICE_BASE_URL}${API_PATHS.SETTINGS}`, {
        method: 'GET',
        headers: { 'Accept': 'application/json' },
      });

      if (response.ok) {
        const contentType = response.headers.get('content-type');
        if (contentType && contentType.includes('application/json')) {
          const data = await response.json();
          setEnhancedMode(data.enhancedLEDMode ?? false);
          setOptimizationSensitivity(data.manualLEDIntensity ?? 128);
        } else {
          console.warn('Settings endpoint returned HTML instead of JSON - using defaults');
          setFirmwareUpdateNeeded(true);
        }
      }
    } catch (error) {
      console.error('Failed to load enhanced LED settings:', error);
      setFirmwareUpdateNeeded(true);
    }
  };

  const saveEnhancedSettings = async () => {
    try {
      const settings: LEDSettings = {
        enhancedLEDMode: enhancedMode,
        manualLEDIntensity: optimizationSensitivity
      };

      const response = await fetch(`${DEVICE_BASE_URL}${API_PATHS.SETTINGS}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(settings),
      });

      if (response.ok) {
        showToast('Enhanced LED settings saved successfully!', 'success');
      } else {
        const errorText = await response.text();
        showToast(`Failed to save settings: ${errorText}`, 'error');
      }
    } catch (error) {
      showToast(`Network error: ${error}`, 'error');
    }
  };

  const startLiveMetrics = () => {
    stopLiveMetrics(); // Clear any existing interval
    fetchMetrics(); // Initial fetch
    intervalRef.current = setInterval(fetchMetrics, 2000); // Update every 2 seconds
  };

  const stopLiveMetrics = () => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
  };

  const fetchMetrics = async () => {
    try {
      const response = await fetch(`${DEVICE_BASE_URL}/live-metrics`, {
        method: 'GET',
        headers: { 'Accept': 'application/json' },
      });

      if (response.ok) {
        const contentType = response.headers.get('content-type');
        if (contentType && contentType.includes('application/json')) {
          const data = await response.json();
          setMetrics(data);
          setIsConnected(true);
          setMetricsError(null);

          // Perform automatic brightness optimization if in enhanced mode
          if (enhancedMode && keepOn && data.sensorReadings) {
            performAutomaticOptimization(data);
          }
        } else {
          throw new Error('Live metrics endpoint not available - firmware update needed');
        }
      } else {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
    } catch (err) {
      setIsConnected(false);
      setMetricsError(err instanceof Error ? err.message : 'Unknown error');
    }
  };

  const performAutomaticOptimization = async (metricsData: SensorMetrics) => {
    const now = Date.now();

    // Adaptive throttling: faster adjustments when far from target, slower when close
    const { x, y, z, ir } = metricsData.sensorReadings;
    const controlVariable = Math.max(x, y, z);
    const targetMin = metricsData.metrics?.targetMin || 5000;
    const targetMax = metricsData.metrics?.targetMax || 60000;

    // Calculate how far we are from optimal range (0 = in range, 1 = very far)
    let distanceFromOptimal = 0;
    if (controlVariable < targetMin) {
      distanceFromOptimal = Math.min((targetMin - controlVariable) / targetMin, 1);
    } else if (controlVariable > targetMax) {
      distanceFromOptimal = Math.min((controlVariable - targetMax) / targetMax, 1);
    }

    // Adaptive throttling: 1-5 seconds based on distance from optimal
    const adaptiveThrottle = distanceFromOptimal > 0.5 ? 1000 : // Far from target: 1 second
                            distanceFromOptimal > 0.2 ? 2000 : // Moderate distance: 2 seconds
                            distanceFromOptimal > 0 ? 3000 :   // Close but not optimal: 3 seconds
                            5000; // In optimal range: 5 seconds (prevent oscillation)

    if (now - lastOptimizationTime < adaptiveThrottle) {
      return;
    }

    // Current brightness from LED status or fallback to local state
    const currentBrightness = metricsData.ledStatus?.currentBrightness || optimizedBrightness;

    // Calculate IR contamination ratio
    const irRatio = controlVariable > 0 ? ir / controlVariable : 0;
    const irContaminated = irRatio > 0.15; // 15% threshold for IR contamination

    // Determine if adjustment is needed with hysteresis (dead zone)
    let newBrightness = currentBrightness;
    let adjustmentReason = '';

    // Sensitivity factor: higher sensitivity = more aggressive adjustments
    // Scale from 0-255 to 0.5-2.0 multiplier
    const sensitivityFactor = 0.5 + (optimizationSensitivity / 255) * 1.5;

    // Dynamic maxStep based on distance from target
    const baseMaxStep = 40;
    const maxStep = Math.round(baseMaxStep * (1 + distanceFromOptimal)); // 40-80 range

    // Hysteresis zones to prevent oscillation
    const hysteresisZone = Math.round((targetMax - targetMin) * 0.05); // 5% of range
    const stableMin = targetMin + hysteresisZone;
    const stableMax = targetMax - hysteresisZone;

    if (controlVariable < stableMin) {
      // Too dim - increase brightness using improved proportional scaling
      const distance = stableMin - controlVariable;
      const rawFactor = Math.min(Math.max(distance / targetMin, 0), 1); // Clamp to 0-1

      // Enhanced scaling with urgency factor
      const urgencyFactor = controlVariable < targetMin * 0.5 ? 1.5 : 1.0; // More aggressive if very low
      const scaledAdjustment = Math.round(rawFactor * maxStep * sensitivityFactor * urgencyFactor);
      const adjustment = Math.max(2, Math.min(maxStep, scaledAdjustment)); // Minimum 2 for meaningful change

      newBrightness = Math.min(255, currentBrightness + adjustment);
      adjustmentReason = `Low signal (${controlVariable} < ${stableMin}), distance: ${distance}, urgency: ${urgencyFactor.toFixed(1)}x, adj: +${adjustment}`;

    } else if (controlVariable > stableMax) {
      // Too bright - reduce brightness using improved proportional scaling
      const excess = controlVariable - stableMax;
      const rawFactor = Math.min(excess / targetMax, 1.0); // How far above target (0-1)

      // Enhanced scaling with saturation protection
      const saturationFactor = controlVariable > targetMax * 1.5 ? 1.5 : 1.0; // More aggressive if very high
      const scaledAdjustment = Math.round(rawFactor * maxStep * sensitivityFactor * saturationFactor);
      const adjustment = Math.max(2, Math.min(maxStep, scaledAdjustment)); // Minimum 2 for meaningful change

      newBrightness = Math.max(64, currentBrightness - adjustment); // ESP32 minimum is 64
      adjustmentReason = `High signal (${controlVariable} > ${stableMax}), excess: ${excess}, saturation: ${saturationFactor.toFixed(1)}x, adj: -${adjustment}`;

    } else {
      // In stable zone - no adjustment needed
      adjustmentReason = `Stable (${controlVariable} in range ${stableMin}-${stableMax})`;
    }

    // IR contamination compensation (fixed the undefined baseAdjustment bug)
    if (irContaminated && newBrightness !== currentBrightness) {
      const irAdjustment = Math.round(Math.abs(newBrightness - currentBrightness) * 0.3); // 30% of planned adjustment
      newBrightness = Math.max(64, newBrightness - irAdjustment);
      adjustmentReason += ` + IR compensation (${(irRatio * 100).toFixed(1)}%, -${irAdjustment})`;
    }

    // Improved EMA smoothing with adaptive alpha
    const baseAlpha = 0.4; // Increased from 0.3 for faster response
    const adaptiveAlpha = distanceFromOptimal > 0.3 ? baseAlpha * 1.5 : baseAlpha; // Faster when far from target
    const smoothedTarget = adaptiveAlpha * newBrightness + (1 - adaptiveAlpha) * smoothedBrightness;
    const finalBrightness = Math.round(smoothedTarget);

    // Only apply if there's a meaningful change (> 1 brightness level)
    if (Math.abs(finalBrightness - currentBrightness) > 1) {
      setOptimizedBrightness(finalBrightness);
      setSmoothedBrightness(smoothedTarget);
      setLastOptimizationTime(now);

      // Update optimization history for debugging
      setOptimizationHistory(prev => [...prev.slice(-9), finalBrightness]);

      // Apply the brightness change
      await applyOptimizedBrightness(finalBrightness, adjustmentReason);
    } else {
      // Update smoothed value even if not applying change
      setSmoothedBrightness(smoothedTarget);
    }
  };

  const applyOptimizedBrightness = async (newBrightness: number, reason: string) => {
    try {
      const payload = {
        brightness: newBrightness,
        r, g, b,
        keepOn: true,
        enhancedMode: true
      };

      const result = await apiService.setLedBrightness(payload);
      if (result.success) {
        console.log(`🔧 Auto-optimized brightness: ${newBrightness} (${reason})`);
      } else {
        console.warn('Failed to apply optimized brightness:', result.message);
      }
    } catch (error) {
      console.error('Error applying optimized brightness:', error);
    }
  };

  const handleSetLed = async (turnOff = false) => {
    setIsLoading(true);

    // When turning off LED, disable enhanced mode
    if (turnOff && enhancedMode) {
      setEnhancedMode(false);
      await saveEnhancedSettings();
    }

    const payload = turnOff
      ? { brightness: 0 }
      : {
          brightness: enhancedMode ? optimizedBrightness : brightness,
          r, g, b, keepOn,
          enhancedMode: enhancedMode && !turnOff
        };

    try {
      const result = await apiService.setLedBrightness(payload);
      if (result.success) {
        showToast(result.message || (turnOff ? 'LED turned off.' : 'LED updated.'), 'success');
        if (turnOff) {
          setKeepOn(false);
          // Reset optimization state when turning off
          setOptimizedBrightness(128);
          setSmoothedBrightness(128);
          setOptimizationHistory([]);
        } else {
          setKeepOn(true);
          // Initialize optimization state when turning on in enhanced mode
          if (enhancedMode) {
            setOptimizedBrightness(payload.brightness);
            setSmoothedBrightness(payload.brightness);
          }
        }
      } else {
        showToast(result.message || 'Failed to update LED.', 'error');
      }
    } catch (err) {
      console.error("Set LED failed:", err);
      showToast(err instanceof Error ? err.message : 'Failed to update LED.', 'error');
    } finally {
      setIsLoading(false);
    }
  };

  const handleToggleEnhancedMode = async () => {
    const newMode = !enhancedMode;
    setEnhancedMode(newMode);

    // If turning off enhanced mode while LED is on, disable it
    if (!newMode && keepOn) {
      setKeepOn(false);
      await handleSetLed(true); // Turn off LED
    }

    await saveEnhancedSettings();
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

  return (
    <Card title="Live LED Control" className="mb-6">
      <div className="space-y-4">
        {firmwareUpdateNeeded && (
          <div className="mb-4 p-3 rounded-md text-sm bg-yellow-900/30 text-yellow-400 border border-yellow-700">
            <strong>Firmware Update Required:</strong> Enhanced LED Control features require updated ESP32 firmware.
          </div>
        )}

        {/* Color Preview */}
        <div className="flex justify-center my-4">
          <ColorSwatch r={r} g={g} b={b} size="lg" />
        </div>

        {/* RGB Controls */}
        <div className="grid grid-cols-3 gap-2">
          <Input
            id="led-r"
            label="R"
            type="number"
            min="0"
            max="255"
            value={r}
            onChange={e => setR(Math.max(0, Math.min(255, parseInt(e.target.value) || 0)))}
          />
          <Input
            id="led-g"
            label="G"
            type="number"
            min="0"
            max="255"
            value={g}
            onChange={e => setG(Math.max(0, Math.min(255, parseInt(e.target.value) || 0)))}
          />
          <Input
            id="led-b"
            label="B"
            type="number"
            min="0"
            max="255"
            value={b}
            onChange={e => setB(Math.max(0, Math.min(255, parseInt(e.target.value) || 0)))}
          />
        </div>

        {/* Enhanced Mode Toggle */}
        <div className="flex items-center justify-between p-3 bg-slate-700/50 rounded-lg">
          <div>
            <label className="text-sm font-medium text-slate-300">
              Enhanced LED Control Mode
            </label>
            <p className="text-xs text-slate-500 mt-1">
              {enhancedMode
                ? 'Automatic brightness optimization for optimal sensor range (5,000-60,000)'
                : 'Manual LED intensity control'
              }
            </p>
          </div>
          <button
            onClick={handleToggleEnhancedMode}
            disabled={firmwareUpdateNeeded}
            className={`relative inline-flex h-6 w-11 items-center rounded-full transition-colors ${
              enhancedMode ? 'bg-blue-600' : 'bg-slate-600'
            } ${firmwareUpdateNeeded ? 'opacity-50 cursor-not-allowed' : ''}`}
          >
            <span
              className={`inline-block h-4 w-4 transform rounded-full bg-white transition-transform ${
                enhancedMode ? 'translate-x-6' : 'translate-x-1'
              }`}
            />
          </button>
        </div>

        {/* Brightness/Optimization Slider */}
        <Slider
          id="live-brightness"
          label={enhancedMode ? "Optimization Sensitivity" : "LED Brightness"}
          min={enhancedMode ? 50 : 0}
          max={255}
          value={enhancedMode ? optimizationSensitivity : brightness}
          onChange={enhancedMode ? setOptimizationSensitivity : setBrightness}
        />

        {enhancedMode && (
          <div className="space-y-3 -mt-2">
            <p className="text-xs text-slate-500">
              Sensitivity: {optimizationSensitivity}/255 - Higher values = more aggressive automatic adjustments
            </p>

            {/* Real-time LED Brightness Display */}
            <div className="bg-gradient-to-r from-amber-900/20 to-orange-900/20 border border-amber-700/50 rounded-lg p-3">
              <div className="flex items-center justify-between mb-2">
                <h5 className="text-sm font-medium text-amber-300">🔆 Real-time LED Brightness</h5>
                <div className="flex items-center space-x-2">
                  <div className={`w-2 h-2 rounded-full ${keepOn ? 'bg-green-400' : 'bg-red-400'}`}></div>
                  <span className="text-xs text-slate-400">{keepOn ? 'Active' : 'Inactive'}</span>
                </div>
              </div>

              {keepOn ? (
                <div className="space-y-3">
                  <div className="grid grid-cols-2 gap-3">
                    <div className="bg-slate-800/50 rounded p-2">
                      <div className="text-amber-300 text-xs font-medium">Current PWM</div>
                      <div className="text-amber-100 font-mono text-xl">{optimizedBrightness}</div>
                      <div className="text-amber-400 text-xs">{((optimizedBrightness / 255) * 100).toFixed(1)}% duty cycle</div>
                    </div>
                    <div className="bg-slate-800/50 rounded p-2">
                      <div className="text-amber-300 text-xs font-medium">Smoothed Target</div>
                      <div className="text-amber-100 font-mono text-xl">{Math.round(smoothedBrightness)}</div>
                      <div className="text-amber-400 text-xs">{((Math.round(smoothedBrightness) / 255) * 100).toFixed(1)}% target</div>
                    </div>
                  </div>

                  {optimizationHistory.length > 0 && (
                    <div className="bg-slate-800/50 rounded p-2">
                      <div className="text-amber-300 text-xs font-medium">PWM History (Recent Adjustments)</div>
                      <div className="text-amber-100 font-mono text-sm">
                        {optimizationHistory.slice(-5).join(' → ')}
                      </div>
                      <div className="text-amber-400 text-xs mt-1">
                        Range: {Math.min(...optimizationHistory.slice(-5))} - {Math.max(...optimizationHistory.slice(-5))}
                        ({Math.max(...optimizationHistory.slice(-5)) - Math.min(...optimizationHistory.slice(-5))} point spread)
                      </div>
                    </div>
                  )}
                </div>
              ) : (
                <div className="text-center py-4">
                  <div className="text-slate-400 text-sm">LED is OFF</div>
                  <div className="text-slate-500 text-xs">Click "Start Enhanced LED" to begin real-time monitoring</div>
                </div>
              )}
            </div>

            {/* Technical Details (when active) */}
            {keepOn && (
              <div className="text-xs text-slate-400 bg-slate-700/30 rounded p-2">
                <div className="grid grid-cols-2 gap-2">
                  <div className="flex justify-between">
                    <span>Auto Brightness:</span>
                    <span className="font-mono text-slate-200">{optimizedBrightness}/255</span>
                  </div>
                  <div className="flex justify-between">
                    <span>EMA Smoothed:</span>
                    <span className="font-mono text-slate-200">{Math.round(smoothedBrightness)}/255</span>
                  </div>
                </div>
              </div>
            )}
          </div>
        )}

        {/* Live Sensor Feedback (Enhanced Mode Only) */}
        {enhancedMode && keepOn && (
          <div className="p-3 bg-slate-700/30 rounded-lg border border-slate-600">
            <div className="flex items-center justify-between mb-3">
              <h4 className="text-sm font-medium text-slate-300">Live Sensor Feedback</h4>
              <div className="flex items-center space-x-2">
                <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-400' : 'bg-red-400'}`}></div>
                <span className="text-xs text-slate-400">
                  {isConnected ? 'Connected' : 'Disconnected'}
                </span>
              </div>
            </div>

            {metricsError && (
              <div className="mb-3 p-2 rounded text-xs bg-red-900/30 text-red-400 border border-red-700">
                {metricsError}
              </div>
            )}

            {metrics && (
              <div className="space-y-3">
                {/* TCS3430 Raw Sensor Readings */}
                <div>
                  <h5 className="text-xs font-medium text-slate-300 mb-2">TCS3430 Raw Channel Values</h5>
                  <div className="grid grid-cols-3 gap-2 text-sm">
                    <div className="bg-red-900/20 border border-red-700/50 rounded p-2">
                      <div className="text-red-300 text-xs font-medium">R (Red)</div>
                      <div className="text-red-100 font-mono text-sm">{metrics.sensorReadings.x}</div>
                    </div>
                    <div className="bg-green-900/20 border border-green-700/50 rounded p-2">
                      <div className="text-green-300 text-xs font-medium">G (Green)</div>
                      <div className="text-green-100 font-mono text-sm">{metrics.sensorReadings.y}</div>
                    </div>
                    <div className="bg-blue-900/20 border border-blue-700/50 rounded p-2">
                      <div className="text-blue-300 text-xs font-medium">B (Blue)</div>
                      <div className="text-blue-100 font-mono text-sm">{metrics.sensorReadings.z}</div>
                    </div>
                    <div className="bg-slate-800/50 border border-slate-600 rounded p-2">
                      <div className="text-slate-300 text-xs font-medium">C (Clear)</div>
                      <div className="text-slate-100 font-mono text-sm">
                        {metrics.sensorReadings.x + metrics.sensorReadings.y + metrics.sensorReadings.z}
                      </div>
                    </div>
                    <div className="bg-purple-900/20 border border-purple-700/50 rounded p-2">
                      <div className="text-purple-300 text-xs font-medium">IR</div>
                      <div className="text-purple-100 font-mono text-sm">{metrics.sensorReadings.ir}</div>
                    </div>
                    <div className="bg-slate-800/50 border border-slate-600 rounded p-2">
                      <div className="text-slate-300 text-xs font-medium">Status</div>
                      <div className="text-slate-100 font-mono text-sm">0x{metrics.sensorReadings.status?.toString(16).toUpperCase().padStart(2, '0')}</div>
                    </div>
                  </div>
                </div>

                {/* Current Brightness and Control Variable */}
                <div className="grid grid-cols-2 gap-3 text-sm">
                  <div className="bg-slate-800/50 rounded p-2">
                    <span className="text-slate-400">Current Brightness:</span>
                    <span className="text-slate-200 ml-2 font-mono">{metrics.ledStatus.currentBrightness}</span>
                  </div>
                  <div className="bg-slate-800/50 rounded p-2">
                    <span className="text-slate-400">Control Variable:</span>
                    <div className="flex items-center space-x-1 mt-1">
                      <span className="text-slate-200 font-mono text-xs">{metrics.metrics.controlVariable}</span>
                      <span className={`px-1 py-0.5 rounded text-xs ${getStatusColor(metrics.statusIndicators.controlVariableStatus)}`}>
                        {metrics.statusIndicators.controlVariableStatus}
                      </span>
                    </div>
                  </div>
                </div>

                {/* Optimization Status */}
                <div className="text-xs">
                  <div className="flex justify-between items-center">
                    <span className="text-slate-400">Target Range:</span>
                    <span className="text-slate-300">{metrics.metrics.targetMin} - {metrics.metrics.targetMax}</span>
                  </div>
                  <div className="flex justify-between items-center mt-1">
                    <span className="text-slate-400">In Optimal Range:</span>
                    <span className={`px-2 py-0.5 rounded text-xs ${
                      metrics.metrics.inOptimalRange
                        ? 'text-green-400 bg-green-900/30'
                        : 'text-yellow-400 bg-yellow-900/30'
                    }`}>
                      {metrics.metrics.inOptimalRange ? 'Yes' : 'No'}
                    </span>
                  </div>
                  <div className="flex justify-between items-center mt-1">
                    <span className="text-slate-400">Auto-Optimization:</span>
                    <span className={`px-2 py-0.5 rounded text-xs ${
                      enhancedMode && keepOn
                        ? 'text-blue-400 bg-blue-900/30'
                        : 'text-slate-400 bg-slate-700/30'
                    }`}>
                      {enhancedMode && keepOn ? 'Active' : 'Inactive'}
                    </span>
                  </div>
                  {enhancedMode && keepOn && (
                    <div className="flex justify-between items-center mt-1">
                      <span className="text-slate-400">IR Ratio:</span>
                      <span className={`px-1 py-0.5 rounded text-xs ${
                        metrics.metrics.irRatio > 0.15
                          ? 'text-red-400 bg-red-900/30'
                          : 'text-green-400 bg-green-900/30'
                      }`}>
                        {(metrics.metrics.irRatio * 100).toFixed(1)}%
                      </span>
                    </div>
                  )}
                </div>

                {/* Status Indicators */}
                <div className="grid grid-cols-2 gap-2 text-xs">
                  <div className="flex justify-between">
                    <span className="text-slate-400">Saturation:</span>
                    <span className={`px-1 py-0.5 rounded ${getStatusColor(metrics.statusIndicators.saturationStatus)}`}>
                      {metrics.statusIndicators.saturationStatus}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">IR Status:</span>
                    <span className={`px-1 py-0.5 rounded ${getStatusColor(metrics.statusIndicators.irContaminationStatus)}`}>
                      {metrics.statusIndicators.irContaminationStatus}
                    </span>
                  </div>
                </div>
              </div>
            )}
          </div>
        )}

        {/* LED Control Buttons */}
        <div className="flex space-x-2 mt-4">
          <Button
            onClick={() => handleSetLed(false)}
            isLoading={isLoading}
            variant="primary"
            className="flex-1"
            disabled={enhancedMode && !keepOn} // In enhanced mode, must turn on LED first
          >
            {enhancedMode ? 'Start Enhanced LED' : 'Set LED'}
          </Button>
          <Button
            onClick={() => handleSetLed(true)}
            isLoading={isLoading}
            variant="secondary"
            className="flex-1"
          >
            Turn LED Off
          </Button>
        </div>

        {/* Enhanced Mode Features Info */}
        {enhancedMode && (
          <div className="p-3 bg-blue-900/20 rounded-lg border border-blue-700/50">
            <h4 className="text-sm font-medium text-blue-300 mb-2">Enhanced Mode Features</h4>
            <ul className="text-xs text-blue-200 space-y-1">
              <li>• Automatic brightness optimization using max(R,G,B) control variable</li>
              <li>• Target range: 5,000 - 60,000 for optimal sensor readings</li>
              <li>• IR contamination detection and compensation (threshold: 15%)</li>
              <li>• Exponential moving average smoothing to prevent flickering</li>
              <li>• Sensitivity-based adjustment scaling (0.5x - 2.0x multiplier)</li>
              <li>• 3-second throttling between automatic adjustments</li>
            </ul>
            {keepOn && (
              <div className="mt-3 pt-2 border-t border-blue-700/30">
                <p className="text-xs text-blue-300">
                  <strong>Status:</strong> Auto-optimization is {enhancedMode && keepOn ? 'ACTIVE' : 'INACTIVE'}
                </p>
                <p className="text-xs text-blue-400 mt-1">
                  Adjust sensitivity slider to control how aggressively the system optimizes brightness.
                </p>
              </div>
            )}
          </div>
        )}
      </div>
    </Card>
  );
};

export default LiveLedControl;
    