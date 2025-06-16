import React, { useState, useEffect } from 'react';
import { 
  ColorReference, 
  MatrixCalibrationStatus, 
  MatrixCalibrationStartResponse,
  MatrixCalibrationComputeResponse,
  MatrixCalibrationResultsResponse
} from '../types';
import * as apiService from '../services/apiService';
import Button from './ui/Button';
import Card from './ui/Card';

interface MatrixCalibrationPanelProps {
  showToast: (message: string, type: 'success' | 'error', customDuration?: number) => void;
}

enum CalibrationStep {
  IDLE = 'idle',
  MEASURING = 'measuring',
  COMPUTING = 'computing',
  RESULTS = 'results'
}

const MatrixCalibrationPanel: React.FC<MatrixCalibrationPanelProps> = ({ showToast }) => {
  const [status, setStatus] = useState<MatrixCalibrationStatus | null>(null);
  const [currentStep, setCurrentStep] = useState<CalibrationStep>(CalibrationStep.IDLE);
  const [colorReferences, setColorReferences] = useState<ColorReference[]>([]);
  const [currentColorIndex, setCurrentColorIndex] = useState(0);
  const [isLoading, setIsLoading] = useState(false);
  const [results, setResults] = useState<MatrixCalibrationResultsResponse | null>(null);
  const [computeResults, setComputeResults] = useState<MatrixCalibrationComputeResponse | null>(null);

  useEffect(() => {
    loadStatus();
  }, []);

  const loadStatus = async () => {
    try {
      const statusData = await apiService.getMatrixCalibrationStatus();
      setStatus(statusData);
      
      if (statusData.matrixValid) {
        const resultsData = await apiService.getMatrixCalibrationResults();
        setResults(resultsData);
      }
    } catch (error) {
      console.error('Failed to load matrix calibration status:', error);
      showToast('Failed to load calibration status', 'error');
    }
  };

  const startCalibration = async () => {
    setIsLoading(true);
    try {
      const response = await apiService.startMatrixCalibration();
      if (response.success) {
        setColorReferences(response.colorReferences);
        setCurrentColorIndex(0);
        setCurrentStep(CalibrationStep.MEASURING);
        showToast('Matrix calibration started', 'success');
      } else {
        showToast('Failed to start calibration', 'error');
      }
    } catch (error) {
      console.error('Failed to start calibration:', error);
      showToast('Failed to start calibration', 'error');
    } finally {
      setIsLoading(false);
    }
  };

  const measureCurrentColor = async () => {
    if (currentColorIndex >= colorReferences.length) return;
    
    const color = colorReferences[currentColorIndex];
    setIsLoading(true);
    
    try {
      const response = await apiService.measureMatrixCalibrationColor({
        colorName: color.name,
        r: color.r,
        g: color.g,
        b: color.b
      });
      
      if (response.success) {
        // Mark color as measured
        const updatedColors = [...colorReferences];
        updatedColors[currentColorIndex].measured = true;
        setColorReferences(updatedColors);
        
        showToast(`${color.name} measured successfully`, 'success');
        
        // Move to next color or finish measuring
        if (currentColorIndex < colorReferences.length - 1) {
          setCurrentColorIndex(currentColorIndex + 1);
        } else {
          setCurrentStep(CalibrationStep.COMPUTING);
        }
      } else {
        showToast(`Failed to measure ${color.name}: ${response.error}`, 'error');
      }
    } catch (error) {
      console.error('Failed to measure color:', error);
      showToast(`Failed to measure ${color.name}`, 'error');
    } finally {
      setIsLoading(false);
    }
  };

  const computeMatrix = async () => {
    setIsLoading(true);
    try {
      const response = await apiService.computeMatrixCalibration();
      if (response.success) {
        setComputeResults(response);
        setCurrentStep(CalibrationStep.RESULTS);
        showToast('Calibration matrix computed successfully', 'success');
        
        // Load detailed results
        const resultsData = await apiService.getMatrixCalibrationResults();
        setResults(resultsData);
      } else {
        showToast(`Failed to compute matrix: ${response.error}`, 'error');
      }
    } catch (error) {
      console.error('Failed to compute matrix:', error);
      showToast('Failed to compute calibration matrix', 'error');
    } finally {
      setIsLoading(false);
    }
  };

  const applyCalibration = async () => {
    setIsLoading(true);
    try {
      const response = await apiService.applyMatrixCalibration();
      if (response.success) {
        showToast('Matrix calibration applied successfully', 'success');
        setCurrentStep(CalibrationStep.IDLE);
        await loadStatus(); // Refresh status
      } else {
        showToast(`Failed to apply calibration: ${response.error}`, 'error');
      }
    } catch (error) {
      console.error('Failed to apply calibration:', error);
      showToast('Failed to apply calibration', 'error');
    } finally {
      setIsLoading(false);
    }
  };

  const clearCalibration = async () => {
    setIsLoading(true);
    try {
      const response = await apiService.clearMatrixCalibration();
      if (response.success) {
        showToast('Matrix calibration cleared', 'success');
        setCurrentStep(CalibrationStep.IDLE);
        setColorReferences([]);
        setResults(null);
        setComputeResults(null);
        await loadStatus();
      } else {
        showToast('Failed to clear calibration', 'error');
      }
    } catch (error) {
      console.error('Failed to clear calibration:', error);
      showToast('Failed to clear calibration', 'error');
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

  return (
    <Card className="bg-slate-800 border-slate-600">
      <div className="space-y-6">
        <div className="flex items-center justify-between">
          <h3 className="text-lg font-semibold text-slate-100">Matrix Calibration</h3>
          {status?.matrixValid && (
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
                <span className={`ml-2 ${status.matrixValid ? 'text-green-400' : 'text-slate-300'}`}>
                  {status.matrixValid ? 'Calibrated' : 'Not Calibrated'}
                </span>
              </div>
              <div>
                <span className="text-slate-400">Points:</span>
                <span className="ml-2 text-slate-300">{status.numPoints}</span>
              </div>
              {status.matrixValid && (
                <>
                  <div>
                    <span className="text-slate-400">Avg ΔE:</span>
                    <span className="ml-2 text-slate-300">{status.avgDeltaE?.toFixed(2)}</span>
                  </div>
                  <div>
                    <span className="text-slate-400">Quality:</span>
                    <span className={`ml-2 ${getQualityColor(status.qualityScore || 0)}`}>
                      {getQualityLabel(status.qualityScore || 0)}
                    </span>
                  </div>
                </>
              )}
            </div>
          </div>
        )}

        {/* Calibration Workflow */}
        {currentStep === CalibrationStep.IDLE && (
          <div className="space-y-4">
            <div className="bg-blue-900/30 p-3 rounded-lg border border-blue-600">
              <p className="text-sm text-blue-300 mb-3">
                Matrix calibration provides superior color accuracy across the full spectrum by using multiple color references.
              </p>
              <p className="text-xs text-slate-400">
                This process will guide you through measuring 7 standard colors to compute a transformation matrix.
              </p>
            </div>
            
            <div className="flex space-x-3">
              <Button
                onClick={startCalibration}
                variant="primary"
                isLoading={isLoading}
                className="flex-1"
              >
                Start Matrix Calibration
              </Button>
              
              {status?.matrixValid && (
                <Button
                  onClick={clearCalibration}
                  variant="secondary"
                  isLoading={isLoading}
                >
                  Clear
                </Button>
              )}
            </div>
          </div>
        )}

        {/* Measuring Colors */}
        {currentStep === CalibrationStep.MEASURING && (
          <div className="space-y-4">
            <div className="bg-amber-900/30 p-3 rounded-lg border border-amber-600">
              <h4 className="text-amber-300 font-medium mb-2">
                Step {currentColorIndex + 1} of {colorReferences.length}
              </h4>
              {currentColorIndex < colorReferences.length && (
                <div className="flex items-center space-x-3">
                  <div 
                    className="w-8 h-8 rounded border border-slate-500"
                    style={{ 
                      backgroundColor: `rgb(${colorReferences[currentColorIndex].r}, ${colorReferences[currentColorIndex].g}, ${colorReferences[currentColorIndex].b})` 
                    }}
                  ></div>
                  <div>
                    <p className="text-amber-300 font-medium">
                      {colorReferences[currentColorIndex].name}
                    </p>
                    <p className="text-xs text-slate-400">
                      Place sensor over {colorReferences[currentColorIndex].name.toLowerCase()} color patch
                    </p>
                  </div>
                </div>
              )}
            </div>

            {/* Progress */}
            <div className="space-y-2">
              <div className="flex justify-between text-sm">
                <span className="text-slate-400">Progress</span>
                <span className="text-slate-300">
                  {colorReferences.filter(c => c.measured).length} / {colorReferences.length}
                </span>
              </div>
              <div className="w-full bg-slate-700 rounded-full h-2">
                <div 
                  className="bg-blue-500 h-2 rounded-full transition-all duration-300"
                  style={{ width: `${(colorReferences.filter(c => c.measured).length / colorReferences.length) * 100}%` }}
                ></div>
              </div>
            </div>

            {/* Color Grid */}
            <div className="grid grid-cols-4 gap-2">
              {colorReferences.map((color, index) => (
                <div 
                  key={color.name}
                  className={`p-2 rounded border text-center text-xs ${
                    index === currentColorIndex 
                      ? 'border-amber-500 bg-amber-900/20' 
                      : color.measured 
                        ? 'border-green-500 bg-green-900/20' 
                        : 'border-slate-600 bg-slate-700'
                  }`}
                >
                  <div 
                    className="w-full h-6 rounded mb-1 border border-slate-500"
                    style={{ backgroundColor: `rgb(${color.r}, ${color.g}, ${color.b})` }}
                  ></div>
                  <div className={`${
                    index === currentColorIndex 
                      ? 'text-amber-300' 
                      : color.measured 
                        ? 'text-green-400' 
                        : 'text-slate-400'
                  }`}>
                    {color.name}
                  </div>
                  {color.measured && (
                    <div className="text-green-400 text-xs">✓</div>
                  )}
                </div>
              ))}
            </div>

            <div className="flex space-x-3">
              {currentColorIndex < colorReferences.length ? (
                <Button
                  onClick={measureCurrentColor}
                  variant="primary"
                  isLoading={isLoading}
                  className="flex-1"
                >
                  Measure {colorReferences[currentColorIndex]?.name}
                </Button>
              ) : (
                <Button
                  onClick={computeMatrix}
                  variant="primary"
                  isLoading={isLoading}
                  className="flex-1"
                >
                  Compute Matrix
                </Button>
              )}
              
              <Button
                onClick={() => setCurrentStep(CalibrationStep.IDLE)}
                variant="secondary"
              >
                Cancel
              </Button>
            </div>
          </div>
        )}

        {/* Computing Matrix */}
        {currentStep === CalibrationStep.COMPUTING && (
          <div className="space-y-4">
            <div className="bg-blue-900/30 p-3 rounded-lg border border-blue-600">
              <h4 className="text-blue-300 font-medium mb-2">Computing Calibration Matrix</h4>
              <p className="text-sm text-blue-300">
                All colors measured successfully. Ready to compute transformation matrix.
              </p>
            </div>

            <div className="flex space-x-3">
              <Button
                onClick={computeMatrix}
                variant="primary"
                isLoading={isLoading}
                className="flex-1"
              >
                Compute Matrix
              </Button>

              <Button
                onClick={() => setCurrentStep(CalibrationStep.IDLE)}
                variant="secondary"
              >
                Cancel
              </Button>
            </div>
          </div>
        )}

        {/* Results */}
        {currentStep === CalibrationStep.RESULTS && computeResults && (
          <div className="space-y-4">
            <div className="bg-green-900/30 p-3 rounded-lg border border-green-600">
              <h4 className="text-green-300 font-medium mb-2">Calibration Complete</h4>
              <div className="grid grid-cols-2 gap-4 text-sm">
                <div>
                  <span className="text-slate-400">Quality Score:</span>
                  <span className={`ml-2 font-medium ${getQualityColor(computeResults.qualityScore || 0)}`}>
                    {computeResults.qualityScore?.toFixed(1)}% ({computeResults.quality})
                  </span>
                </div>
                <div>
                  <span className="text-slate-400">Average ΔE:</span>
                  <span className="ml-2 text-slate-300">{computeResults.avgDeltaE?.toFixed(2)}</span>
                </div>
                <div>
                  <span className="text-slate-400">Points ΔE &lt; 2:</span>
                  <span className="ml-2 text-green-400">{computeResults.pointsUnder2}</span>
                </div>
                <div>
                  <span className="text-slate-400">Points ΔE &lt; 5:</span>
                  <span className="ml-2 text-yellow-400">{computeResults.pointsUnder5}</span>
                </div>
              </div>
            </div>

            {/* Detailed Results */}
            {results && results.calibrationPoints && (
              <div className="bg-slate-700 p-3 rounded-lg border border-slate-600">
                <h5 className="text-slate-300 font-medium mb-3">Calibration Points</h5>
                <div className="space-y-2 max-h-40 overflow-y-auto">
                  {results.calibrationPoints.map((point, index) => (
                    <div key={index} className="flex items-center justify-between text-sm">
                      <div className="flex items-center space-x-2">
                        <div
                          className="w-4 h-4 rounded border border-slate-500"
                          style={{ backgroundColor: `rgb(${point.refR}, ${point.refG}, ${point.refB})` }}
                        ></div>
                        <span className="text-slate-300">{point.name}</span>
                      </div>
                      <div className="flex items-center space-x-4">
                        <span className="text-slate-400 text-xs">
                          Sensor: {point.sensorR}, {point.sensorG}, {point.sensorB}
                        </span>
                        <span className={`font-medium ${
                          point.deltaE < 2 ? 'text-green-400' :
                          point.deltaE < 5 ? 'text-yellow-400' : 'text-red-400'
                        }`}>
                          ΔE {point.deltaE.toFixed(2)}
                        </span>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            )}

            <div className="flex space-x-3">
              <Button
                onClick={applyCalibration}
                variant="primary"
                isLoading={isLoading}
                className="flex-1"
              >
                Apply Calibration
              </Button>

              <Button
                onClick={() => setCurrentStep(CalibrationStep.IDLE)}
                variant="secondary"
              >
                Discard
              </Button>
            </div>
          </div>
        )}

        {/* Existing Calibration Results */}
        {currentStep === CalibrationStep.IDLE && status?.matrixValid && results && (
          <div className="space-y-4">
            <h4 className="text-slate-300 font-medium">Current Matrix Calibration</h4>

            <div className="bg-slate-700 p-3 rounded-lg border border-slate-600">
              <div className="grid grid-cols-2 gap-4 text-sm mb-3">
                <div>
                  <span className="text-slate-400">Quality Score:</span>
                  <span className={`ml-2 font-medium ${getQualityColor(results.qualityScore || 0)}`}>
                    {results.qualityScore?.toFixed(1)}%
                  </span>
                </div>
                <div>
                  <span className="text-slate-400">Average ΔE:</span>
                  <span className="ml-2 text-slate-300">{results.avgDeltaE?.toFixed(2)}</span>
                </div>
                <div>
                  <span className="text-slate-400">Excellent (ΔE &lt; 2):</span>
                  <span className="ml-2 text-green-400">{results.pointsUnder2}/{results.numPoints}</span>
                </div>
                <div>
                  <span className="text-slate-400">Acceptable (ΔE &lt; 5):</span>
                  <span className="ml-2 text-yellow-400">{results.pointsUnder5}/{results.numPoints}</span>
                </div>
              </div>

              {results.timestamp && (
                <div className="text-xs text-slate-400">
                  Calibrated: {new Date(results.timestamp).toLocaleString()}
                </div>
              )}
            </div>
          </div>
        )}
      </div>
    </Card>
  );
};

export default MatrixCalibrationPanel;
