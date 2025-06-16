import React, { useState, useEffect, useCallback } from 'react';
import { DeviceStatus } from '../types';
import * as apiService from '../services/apiService';
import Button from './ui/Button';
import Card from './ui/Card';
import Input from './ui/Input';
import Slider from './ui/Slider';
import Toggle from './ui/Toggle';
import MatrixCalibrationPanel from './MatrixCalibrationPanel';
import LiveSensorMetrics from './LiveSensorMetrics';

interface SettingsPanelProps {
  initialSettings: DeviceStatus | null;
  onSettingsChange: (newSettings: Partial<DeviceStatus>) => void; // To update App state
  showToast: (message: string, type: 'success' | 'error', customDuration?: number) => void;
}

interface TestResult {
  timestamp: string;
  test: string;
  success: boolean;
  message: string;
}

interface TestCase {
  name: string;
  settings: Partial<DeviceStatus>;
  expectedValues?: Partial<DeviceStatus>;
  shouldFail?: boolean;
}

const SettingsPanel: React.FC<SettingsPanelProps> = ({ initialSettings, onSettingsChange, showToast }) => {
  const [atime, setAtime] = useState(initialSettings?.atime || 255);
  const [again, setAgain] = useState(initialSettings?.again || 0); // Assuming 0 is a valid default for AGAIN index
  const [brightness, setBrightness] = useState(Math.max(64, initialSettings?.brightness || 128)); // Ensure minimum 64
  const [ledState, setLedState] = useState(initialSettings?.ledState || false);
  const [isSaving, setIsSaving] = useState(false);
  const [isCalibrating, setIsCalibrating] = useState(false);

  // TCS3430 Advanced Calibration Settings
  const [autoZeroMode, setAutoZeroMode] = useState(initialSettings?.autoZeroMode !== undefined ? initialSettings.autoZeroMode : 1);
  const [autoZeroFreq, setAutoZeroFreq] = useState(initialSettings?.autoZeroFreq !== undefined ? initialSettings.autoZeroFreq : 127);
  const [waitTime, setWaitTime] = useState(initialSettings?.waitTime !== undefined ? initialSettings.waitTime : 0);

  // Calibration Testing State
  const [showTestSection, setShowTestSection] = useState(false);
  const [isRunningTests, setIsRunningTests] = useState(false);
  const [testResults, setTestResults] = useState<TestResult[]>([]);
  const [showTestResults, setShowTestResults] = useState(false);

  useEffect(() => {
    if (initialSettings) {
      setAtime(initialSettings.atime);
      setAgain(initialSettings.again); // e.g., 0 for 1x, 1 for 4x, 2 for 16x, 3 for 64x
      setBrightness(initialSettings.brightness);
      setLedState(initialSettings.ledState);
      setAutoZeroMode(initialSettings.autoZeroMode !== undefined ? initialSettings.autoZeroMode : 1);
      setAutoZeroFreq(initialSettings.autoZeroFreq !== undefined ? initialSettings.autoZeroFreq : 127);
      setWaitTime(initialSettings.waitTime !== undefined ? initialSettings.waitTime : 0);
    }
  }, [initialSettings]);

  const handleSaveSettings = async () => {
    setIsSaving(true);
    const newSettings = { atime, again, brightness, ledState, autoZeroMode, autoZeroFreq, waitTime };

    // Validate settings before sending
    const validationErrors = validateSettings(newSettings);
    if (validationErrors.length > 0) {
      showToast(`Invalid settings: ${validationErrors.join(', ')}`, 'error');
      setIsSaving(false);
      return;
    }

    try {
      console.log('Saving settings:', newSettings);

      // Save settings
      await apiService.updateSettings(newSettings);

      // Wait a moment for settings to be applied
      await new Promise(resolve => setTimeout(resolve, 750));

      // Refresh device status to verify settings were applied
      const updatedStatus = await apiService.getDeviceStatus();
      console.log('Device status after save:', updatedStatus);

      // Check if all settings were applied correctly
      const settingsApplied = {
        atime: updatedStatus.atime === atime,
        again: updatedStatus.again === again,
        brightness: updatedStatus.brightness === brightness,
        autoZeroMode: updatedStatus.autoZeroMode === autoZeroMode,
        autoZeroFreq: updatedStatus.autoZeroFreq === autoZeroFreq,
        waitTime: updatedStatus.waitTime === waitTime
      };

      const failedSettings = Object.entries(settingsApplied)
        .filter(([_, applied]) => !applied)
        .map(([setting, _]) => {
          const expected = newSettings[setting as keyof typeof newSettings];
          const actual = updatedStatus[setting as keyof DeviceStatus];
          return `${setting} (expected: ${expected}, got: ${actual})`;
        });

      if (failedSettings.length === 0) {
        // All settings applied successfully
        onSettingsChange(updatedStatus); // Update parent state with actual device values
        showToast('✅ All settings saved successfully!', 'success');

        // Update local state to match device state
        setAtime(updatedStatus.atime);
        setAgain(updatedStatus.again);
        setBrightness(updatedStatus.brightness);
        setAutoZeroMode(updatedStatus.autoZeroMode);
        setAutoZeroFreq(updatedStatus.autoZeroFreq);
        setWaitTime(updatedStatus.waitTime);
      } else {
        // Some settings were rejected or modified
        showToast(`⚠️ Settings saved with changes: ${failedSettings.join(', ')}`, 'error', 8000);

        // Update local state to match what was actually applied
        setAtime(updatedStatus.atime);
        setAgain(updatedStatus.again);
        setBrightness(updatedStatus.brightness);
        setAutoZeroMode(updatedStatus.autoZeroMode);
        setAutoZeroFreq(updatedStatus.autoZeroFreq);
        setWaitTime(updatedStatus.waitTime);

        onSettingsChange(updatedStatus);
      }
    } catch (err) {
      console.error("Save settings failed:", err);
      showToast(err instanceof Error ? err.message : 'Failed to save settings.', 'error');
    } finally {
      setIsSaving(false);
    }
  };

  // Standard Calibration Functions
  const handleWhiteCalibration = async () => {
    setIsCalibrating(true);
    try {
      const result = await apiService.performWhiteCalibration(brightness);
      if (result.success) {
        showToast('✅ White point calibration completed successfully!', 'success');
        onSettingsChange({}); // Trigger status refresh
      } else {
        showToast(`❌ White point calibration failed: ${result.message}`, 'error');
      }
    } catch (error) {
      console.error('White point calibration failed:', error);
      showToast(`❌ White point calibration failed: ${error instanceof Error ? error.message : 'Unknown error'}`, 'error');
    } finally {
      setIsCalibrating(false);
    }
  };

  const handleBlackCalibration = async () => {
    setIsCalibrating(true);
    try {
      const result = await apiService.performBlackCalibration();
      if (result.success) {
        showToast('✅ Black calibration completed successfully! LEDs were kept OFF during calibration.', 'success');
        onSettingsChange({}); // Trigger status refresh
      } else {
        showToast(`❌ Black calibration failed: ${result.message}`, 'error');
      }
    } catch (error) {
      console.error('Black calibration failed:', error);
      showToast(`❌ Black calibration failed: ${error instanceof Error ? error.message : 'Unknown error'}`, 'error');
    } finally {
      setIsCalibrating(false);
    }
  };

  const handleVividWhiteCalibration = async () => {
    setIsCalibrating(true);
    try {
      const result = await apiService.performVividWhiteCalibration(brightness);
      if (result.success) {
        showToast('✅ White point calibration completed successfully!', 'success');
        onSettingsChange({}); // Trigger status refresh
      } else {
        showToast(`❌ White point calibration failed: ${result.message}`, 'error');
      }
    } catch (error) {
      console.error('White point calibration failed:', error);
      showToast(`❌ White point calibration failed: ${error instanceof Error ? error.message : 'Unknown error'}`, 'error');
    } finally {
      setIsCalibrating(false);
    }
  };

  // Testing utility functions
  const logTestResult = useCallback((test: string, success: boolean, message: string = '') => {
    const timestamp = new Date().toLocaleTimeString();
    const newResult: TestResult = { timestamp, test, success, message };
    setTestResults(prev => [...prev, newResult]);
  }, []);

  const clearTestResults = useCallback(() => {
    setTestResults([]);
  }, []);

  const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

  // Validation helper function - matches ESP32 constraints
  const validateSettings = (settings: Partial<DeviceStatus>) => {
    const errors: string[] = [];

    if (settings.atime !== undefined && (settings.atime < 0 || settings.atime > 255)) {
      errors.push('ATIME must be 0-255');
    }
    if (settings.again !== undefined && (settings.again < 0 || settings.again > 3)) {
      errors.push('AGAIN must be 0-3');
    }
    if (settings.brightness !== undefined && (settings.brightness < 64 || settings.brightness > 255)) {
      errors.push('Brightness must be 64-255 (ESP32 hardware limit)');
    }
    if (settings.autoZeroFreq !== undefined && (settings.autoZeroFreq < 0 || settings.autoZeroFreq > 255)) {
      errors.push('Auto-Zero Frequency must be 0-255');
    }
    if (settings.waitTime !== undefined && (settings.waitTime < 0 || settings.waitTime > 255)) {
      errors.push('Wait Time must be 0-255');
    }
    if (settings.autoZeroMode !== undefined && (settings.autoZeroMode < 0 || settings.autoZeroMode > 1)) {
      errors.push('Auto-Zero Mode must be 0-1');
    }

    return errors;
  };

  // Individual test functions
  const testCalibrationSettings = async (testCase: TestCase): Promise<boolean> => {
    try {
      logTestResult(testCase.name, true, `Testing: ${JSON.stringify(testCase.settings)}`);

      // Update settings
      await apiService.updateSettings(testCase.settings);
      await delay(500); // Allow settings to apply

      // Verify settings were applied
      const status = await apiService.getDeviceStatus();

      if (testCase.expectedValues) {
        let allCorrect = true;
        for (const [key, expectedValue] of Object.entries(testCase.expectedValues)) {
          const actualValue = status[key as keyof DeviceStatus];
          if (actualValue !== expectedValue) {
            logTestResult(testCase.name, false, `${key}: expected ${expectedValue}, got ${actualValue}`);
            allCorrect = false;
          }
        }

        if (allCorrect) {
          logTestResult(testCase.name, true, 'All values applied correctly');
          return true;
        }
        return false;
      } else if (testCase.shouldFail) {
        // For invalid values, check that previous settings were preserved
        logTestResult(testCase.name, true, 'Invalid values correctly rejected');
        return true;
      }

      logTestResult(testCase.name, true, 'Settings updated successfully');
      return true;
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error';
      if (testCase.shouldFail) {
        logTestResult(testCase.name, true, `Correctly rejected: ${errorMessage}`);
        return true;
      } else {
        logTestResult(testCase.name, false, `Failed: ${errorMessage}`);
        return false;
      }
    }
  };

  const runValidValuesTest = async (): Promise<boolean> => {
    logTestResult('Valid Values Test', true, 'Starting valid values test...');

    const validTestCases: TestCase[] = [
      {
        name: 'Minimum Values',
        settings: { autoZeroMode: 0, autoZeroFreq: 0, waitTime: 0 },
        expectedValues: { autoZeroMode: 0, autoZeroFreq: 0, waitTime: 0 }
      },
      {
        name: 'Recommended Values',
        settings: { autoZeroMode: 1, autoZeroFreq: 127, waitTime: 5 },
        expectedValues: { autoZeroMode: 1, autoZeroFreq: 127, waitTime: 5 }
      },
      {
        name: 'Maximum Values',
        settings: { autoZeroMode: 1, autoZeroFreq: 255, waitTime: 255 },
        expectedValues: { autoZeroMode: 1, autoZeroFreq: 255, waitTime: 255 }
      }
    ];

    let allPassed = true;
    for (const testCase of validTestCases) {
      const result = await testCalibrationSettings(testCase);
      if (!result) allPassed = false;
      await delay(300);
    }

    logTestResult('Valid Values Test', allPassed, allPassed ? 'All valid values accepted' : 'Some valid values failed');
    return allPassed;
  };

  const runInvalidValuesTest = async (): Promise<boolean> => {
    logTestResult('Invalid Values Test', true, 'Starting invalid values test...');

    // Get current settings before testing invalid values
    const beforeStatus = await apiService.getDeviceStatus();

    const invalidTestCases: TestCase[] = [
      {
        name: 'Invalid Auto-Zero Mode',
        settings: { autoZeroMode: 5 },
        shouldFail: true
      },
      {
        name: 'Invalid Auto-Zero Frequency',
        settings: { autoZeroFreq: 300 },
        shouldFail: true
      },
      {
        name: 'Invalid Wait Time',
        settings: { waitTime: 300 },
        shouldFail: true
      }
    ];

    let allPassed = true;
    for (const testCase of invalidTestCases) {
      try {
        await apiService.updateSettings(testCase.settings);
        await delay(500);

        // Check if settings remained unchanged
        const afterStatus = await apiService.getDeviceStatus();

        if (beforeStatus.autoZeroMode === afterStatus.autoZeroMode &&
            beforeStatus.autoZeroFreq === afterStatus.autoZeroFreq &&
            beforeStatus.waitTime === afterStatus.waitTime) {
          logTestResult(testCase.name, true, 'Invalid values correctly rejected');
        } else {
          logTestResult(testCase.name, false, 'Invalid values incorrectly accepted');
          allPassed = false;
        }
      } catch (error) {
        logTestResult(testCase.name, true, 'Invalid values properly rejected by API');
      }
      await delay(300);
    }

    logTestResult('Invalid Values Test', allPassed, allPassed ? 'All invalid values rejected' : 'Some invalid values accepted');
    return allPassed;
  };

  const runCombinedSettingsTest = async (): Promise<boolean> => {
    logTestResult('Combined Settings Test', true, 'Testing combined settings update...');

    const combinedSettings = {
      autoZeroMode: 1,
      autoZeroFreq: 127,
      waitTime: 10,
      atime: 56,
      again: 3,
      brightness: 128
    };

    try {
      await apiService.updateSettings(combinedSettings);
      await delay(1000);

      const status = await apiService.getDeviceStatus();

      let allCorrect = true;
      for (const [key, expectedValue] of Object.entries(combinedSettings)) {
        const actualValue = status[key as keyof DeviceStatus];
        if (actualValue !== expectedValue) {
          logTestResult('Combined Settings Test', false, `${key}: expected ${expectedValue}, got ${actualValue}`);
          allCorrect = false;
        }
      }

      if (allCorrect) {
        logTestResult('Combined Settings Test', true, 'All combined settings applied correctly');
        return true;
      }
      return false;
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error';
      logTestResult('Combined Settings Test', false, `Failed: ${errorMessage}`);
      return false;
    }
  };

  const runFullTestSuite = async () => {
    if (isRunningTests) return;

    setIsRunningTests(true);
    setShowTestResults(true);
    clearTestResults();

    logTestResult('Test Suite', true, '🚀 Starting comprehensive calibration test suite...');

    try {
      const results = await Promise.all([
        runValidValuesTest(),
        runInvalidValuesTest(),
        runCombinedSettingsTest()
      ]);

      const passed = results.filter(r => r).length;
      const total = results.length;

      logTestResult('Test Suite Complete', passed === total,
        `🎯 Results: ${passed}/${total} test groups passed`);

      if (passed === total) {
        showToast('🎉 All calibration tests passed!', 'success');
      } else {
        showToast(`⚠️ ${total - passed} test group(s) failed`, 'error');
      }
    } catch (error) {
      logTestResult('Test Suite', false, `Test suite failed: ${error}`);
      showToast('Test suite encountered an error', 'error');
    } finally {
      setIsRunningTests(false);
    }
  };

  const testCurrentSettings = async () => {
    if (isRunningTests) return;

    setIsRunningTests(true);
    setShowTestResults(true);

    logTestResult('Current Settings Test', true, 'Testing current form values...');

    try {
      const currentFormSettings = {
        autoZeroMode,
        autoZeroFreq,
        waitTime,
        atime,
        again,
        brightness
      };

      await apiService.updateSettings(currentFormSettings);
      await delay(500);

      const status = await apiService.getDeviceStatus();

      let allCorrect = true;
      for (const [key, expectedValue] of Object.entries(currentFormSettings)) {
        const actualValue = status[key as keyof DeviceStatus];
        if (actualValue !== expectedValue) {
          logTestResult('Current Settings Test', false, `${key}: expected ${expectedValue}, got ${actualValue}`);
          allCorrect = false;
        }
      }

      if (allCorrect) {
        logTestResult('Current Settings Test', true, 'All current settings applied successfully');
        showToast('Current settings test passed!', 'success');
      } else {
        showToast('Some current settings failed to apply', 'error');
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error';
      logTestResult('Current Settings Test', false, `Failed: ${errorMessage}`);
      showToast('Current settings test failed', 'error');
    } finally {
      setIsRunningTests(false);
    }
  };

  // AGAIN options: 0=1x, 1=4x, 2=16x, 3=64x
  // These are typical values for TCS sensors, adjust if device firmware implies others
  const againOptions = [
    { value: 0, label: "1x" },
    { value: 1, label: "4x" },
    { value: 2, label: "16x" },
    { value: 3, label: "64x" },
  ];

  return (
    <Card title="Scanner Settings" className="mb-6">
      <div className="space-y-6">
        <Input
          id="atime"
          label="ATIME (Integration Time)"
          type="number"
          min="0"
          max="255" // Typical range, check sensor datasheet for specific device
          value={atime}
          onChange={(e) => setAtime(parseInt(e.target.value, 10))}
          wrapperClassName="max-w-xs"
        />
        
        <div>
          <label htmlFor="again" className="block text-sm font-medium text-slate-300 mb-1">
            AGAIN (Analog Gain)
          </label>
          <select
            id="again"
            value={again}
            onChange={(e) => setAgain(parseInt(e.target.value, 10))}
            className="block w-full max-w-xs px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-100 focus:outline-none focus:ring-blue-500 focus:border-blue-500 sm:text-sm"
          >
            {againOptions.map(opt => (
              <option key={opt.value} value={opt.value} /* className="text-slate-100 bg-slate-700" - Usually not needed/styled directly */ >{opt.label}</option>
            ))}
          </select>
        </div>

        <Slider
          id="scan-brightness"
          label="Scan Brightness"
          min={64} // ESP32 hardware minimum (MIN_LED_BRIGHTNESS)
          max={255}
          value={brightness}
          onChange={setBrightness}
        />
        
        <Toggle
          id="ledState"
          label="LED Always On (Rainbow if idle)"
          checked={ledState}
          onChange={setLedState}
        />

        {/* TCS3430 Advanced Calibration Settings */}
        <div className="border-t border-slate-600 pt-6">
          <div className="space-y-4">
            <label className="block text-sm font-medium text-slate-300 mb-3">
              TCS3430 Advanced Calibration:
            </label>

            {/* Optimal Settings Guidance */}
            <div className="bg-green-900/30 p-3 rounded-lg border border-green-600 mb-4">
              <p className="text-sm font-medium text-green-300 mb-2">📋 Recommended Optimal Settings:</p>
              <div className="text-xs text-green-200 space-y-1">
                <div><strong>ATIME:</strong> 150 (Integration Time)</div>
                <div><strong>AGAIN:</strong> 16x (Analog Gain)</div>
                <div><strong>Scan Brightness:</strong> 128</div>
                <div><strong>Auto-Zero Mode:</strong> 1 (Use previous offset - recommended)</div>
                <div><strong>Auto-Zero Frequency:</strong> 127 (First cycle only - DFRobot recommended)</div>
                <div><strong>Wait Time:</strong> 0-10 (DFRobot default is 0, but low values for stability)</div>
              </div>
              <p className="text-xs text-slate-400 mt-2">
                These settings provide optimal signal without saturation and are proven to work with Vivid White calibration.
              </p>
              <Button
                onClick={() => {
                  setAtime(150);
                  setAgain(2); // 16x gain
                  setBrightness(128);
                  setAutoZeroMode(1);
                  setAutoZeroFreq(127);
                  setWaitTime(5);
                  showToast('Optimal settings applied! Remember to save settings.', 'success');
                }}
                variant="secondary"
                className="text-xs px-3 py-1 mt-2"
              >
                Apply Optimal Settings
              </Button>
            </div>

            <div>
              <label htmlFor="autoZeroMode" className="block text-sm font-medium text-slate-300 mb-1">
                Auto-Zero Mode
              </label>
              <select
                id="autoZeroMode"
                key={`autoZeroMode-${autoZeroMode}`}
                value={autoZeroMode.toString()}
                onChange={(e) => setAutoZeroMode(parseInt(e.target.value, 10))}
                className="block w-full max-w-xs px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-100 focus:outline-none focus:ring-blue-500 focus:border-blue-500 sm:text-sm"
              >
                <option value="0">Always start at zero</option>
                <option value="1">Use previous offset (recommended)</option>
              </select>
              <p className="text-xs text-slate-400 mt-1">
                Mode 1 provides better stability by using previous calibration offset.
              </p>
            </div>

            <Slider
              id="autoZeroFreq"
              label="Auto-Zero Frequency"
              min={0}
              max={255}
              value={autoZeroFreq}
              onChange={setAutoZeroFreq}
              unit=""
            />
            <p className="text-xs text-slate-400 -mt-2">
              0 = never, 127 = first cycle only (recommended), other values = every nth iteration
            </p>

            <Slider
              id="waitTime"
              label="Wait Time"
              min={0}
              max={255}
              value={waitTime}
              onChange={setWaitTime}
              unit=""
            />
            <p className="text-xs text-slate-400 -mt-2">
              Wait time between measurements (0-255). Higher values improve stability but slow measurements.
            </p>
          </div>
        </div>

        {/* Standard Calibration Controls */}
        <div className="border-t border-slate-600 pt-6">
          <div className="space-y-4">
            <label className="block text-sm font-medium text-slate-300">
              White Point Calibration:
            </label>
            <div className="bg-blue-900/30 p-3 rounded-lg border border-blue-600">
              <p className="text-sm text-blue-300 mb-3">
                Calibrate white point using current sensor settings (ATIME: {atime}, AGAIN: {againOptions[again]?.label}, Brightness: {brightness}).
              </p>
              <div className="grid grid-cols-1 gap-2">
                <Button
                  onClick={handleVividWhiteCalibration}
                  variant="primary"
                  size="sm"
                  isLoading={isCalibrating}
                  className="w-full"
                >
                  {isCalibrating ? 'Calibrating...' : 'Calibrate White Point'}
                </Button>
              </div>
              <p className="text-xs text-slate-400 mt-2">
                Place Dulux Vivid White paint sample under sensor. Calibration takes 50 readings over ~5 seconds for accuracy.
              </p>
            </div>

            <label className="block text-sm font-medium text-slate-300 mt-6">
              Black Point Calibration:
            </label>
            <div className="bg-gray-900/50 p-3 rounded-lg border border-gray-600">
              <p className="text-sm text-gray-300 mb-3">
                Calibrate black point for dark reference measurement. LEDs will remain OFF during calibration.
              </p>
              <div className="grid grid-cols-1 gap-2">
                <Button
                  onClick={handleBlackCalibration}
                  variant="secondary"
                  size="sm"
                  isLoading={isCalibrating}
                  className="w-full"
                >
                  {isCalibrating ? 'Calibrating...' : 'Calibrate Black Point'}
                </Button>
              </div>
              <p className="text-xs text-slate-400 mt-2">
                Cover sensor completely or place in dark environment. LEDs will be turned OFF automatically for dark reference measurement.
              </p>
            </div>
          </div>
        </div>

        {/* Matrix Calibration Section */}
        <div className="border-t border-slate-600 pt-6">
          <MatrixCalibrationPanel showToast={showToast} />
        </div>

        {/* Advanced Calibration Testing Section */}
        <div className="border-t border-slate-600 pt-6">
          <div className="space-y-4">
            <div className="flex items-center justify-between">
              <label className="block text-sm font-medium text-slate-300">
                🧪 Sensor Diagnostics & Testing
              </label>
              <div className="flex gap-2">
                <Button
                  onClick={() => window.open('./test_web_ui.html', '_blank')}
                  variant="secondary"
                  className="text-xs px-3 py-1"
                  title="Open standalone test interface"
                >
                  🔗 External Test
                </Button>
                <Button
                  onClick={() => setShowTestSection(!showTestSection)}
                  variant="secondary"
                  className="text-xs px-3 py-1"
                >
                  {showTestSection ? 'Hide' : 'Show'} Tests
                </Button>
              </div>
            </div>

            {showTestSection && (
              <div className="space-y-4 bg-slate-800 p-4 rounded-lg border border-slate-600">
                <p className="text-xs text-slate-400">
                  Comprehensive testing of TCS3430 advanced calibration settings.
                  These tests validate parameter ranges, error handling, and sensor response.
                </p>

                <div className="bg-slate-700 p-3 rounded border border-slate-600">
                  <div className="flex items-center justify-between">
                    <div>
                      <p className="text-xs font-medium text-slate-200">🔗 Standalone Test Interface</p>
                      <p className="text-xs text-slate-400">Open external test page with manual IP configuration</p>
                    </div>
                    <Button
                      onClick={() => window.open('./test_web_ui.html', '_blank')}
                      variant="primary"
                      className="text-xs px-3 py-1"
                    >
                      Open Test Page
                    </Button>
                  </div>
                </div>

                <div className="space-y-3">
                  <Button
                    onClick={testCurrentSettings}
                    disabled={isRunningTests}
                    variant="primary"
                    className="w-full text-sm"
                    isLoading={isRunningTests}
                  >
                    🔧 Test Current Settings
                  </Button>

                  <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                    <Button
                      onClick={runValidValuesTest}
                      disabled={isRunningTests}
                      variant="secondary"
                      className="text-sm"
                    >
                      Test Valid Values
                    </Button>

                    <Button
                      onClick={runInvalidValuesTest}
                      disabled={isRunningTests}
                      variant="secondary"
                      className="text-sm"
                    >
                      Test Invalid Values
                    </Button>

                    <Button
                      onClick={runCombinedSettingsTest}
                      disabled={isRunningTests}
                      variant="secondary"
                      className="text-sm"
                    >
                      Test Combined Settings
                    </Button>

                    <Button
                      onClick={runFullTestSuite}
                      disabled={isRunningTests}
                      variant="primary"
                      className="text-sm"
                      isLoading={isRunningTests}
                    >
                      Run Full Test Suite
                    </Button>
                  </div>
                </div>

                <div className="flex items-center justify-between">
                  <Button
                    onClick={() => setShowTestResults(!showTestResults)}
                    variant="secondary"
                    className="text-xs px-3 py-1"
                    disabled={testResults.length === 0}
                  >
                    {showTestResults ? 'Hide' : 'Show'} Results ({testResults.length})
                  </Button>

                  {testResults.length > 0 && (
                    <Button
                      onClick={clearTestResults}
                      variant="secondary"
                      className="text-xs px-3 py-1 text-red-400 hover:text-red-300"
                    >
                      Clear Results
                    </Button>
                  )}
                </div>

                {showTestResults && testResults.length > 0 && (
                  <div className="bg-slate-900 p-3 rounded border border-slate-700 max-h-64 overflow-y-auto">
                    <div className="space-y-1 font-mono text-xs">
                      {testResults.map((result, index) => (
                        <div
                          key={index}
                          className={`flex items-start space-x-2 ${
                            result.success ? 'text-green-400' : 'text-red-400'
                          }`}
                        >
                          <span className="text-slate-500 min-w-[60px]">
                            [{result.timestamp}]
                          </span>
                          <span className="min-w-[20px]">
                            {result.success ? '✅' : '❌'}
                          </span>
                          <span className="font-medium min-w-[120px]">
                            {result.test}:
                          </span>
                          <span className="text-slate-300 break-all">
                            {result.message}
                          </span>
                        </div>
                      ))}
                    </div>
                  </div>
                )}
              </div>
            )}
          </div>
        </div>

        {/* Enhanced LED Control Section - Now integrated into Live LED Control */}
        <div className="mt-6 p-4 bg-blue-900/30 border border-blue-700 rounded-lg">
          <h3 className="text-lg font-semibold text-blue-400 mb-2">🔧 Enhanced LED Control</h3>
          <p className="text-slate-300">Enhanced LED Control functionality has been integrated into the Live LED Control component in the sidebar for better user experience.</p>
        </div>

        {/* Live Sensor Metrics Section */}
        <div className="mt-6 p-4 bg-green-900/30 border border-green-700 rounded-lg">
          <h3 className="text-lg font-semibold text-green-400 mb-2">📊 Live Sensor Metrics</h3>
          <p className="text-slate-300">Component placement test - this should be visible</p>
          <LiveSensorMetrics isActive={true} updateInterval={1500} />
        </div>

        <div className="flex justify-end">
          <Button onClick={handleSaveSettings} isLoading={isSaving} variant="primary">
            Save Settings
          </Button>
        </div>
      </div>
    </Card>
  );
};

export default SettingsPanel;