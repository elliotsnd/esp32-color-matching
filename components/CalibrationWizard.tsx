import React, { useState, useEffect, useCallback } from 'react';
import { CalibrationState, CalibrationStatusResponse, CalibrationDataPoint } from '../types';
import * as apiService from '../services/apiService';
import Button from './ui/Button';
import Modal from './Modal';
import Input from './ui/Input';
import LoadingSpinner from './LoadingSpinner';

interface CalibrationWizardProps {
  isOpen: boolean;
  onClose: () => void;
  onCalibrationComplete: () => void; // To refresh device status
  showToast: (message: string, type: 'success' | 'error', customDuration?: number) => void;
  initialScanBrightness: number;
}

const CalibrationWizard: React.FC<CalibrationWizardProps> = ({ 
    isOpen, onClose, onCalibrationComplete, showToast, initialScanBrightness 
}) => {
  const [calibrationBrightness, setCalibrationBrightness] = useState(initialScanBrightness);
  const [status, setStatus] = useState<CalibrationStatusResponse | null>(null);
  const [isLoading, setIsLoading] = useState(false); // For individual step loading
  const [isPolling, setIsPolling] = useState(false); // For status polling

  const [whiteData, setWhiteData] = useState<CalibrationDataPoint | null>(null);
  const [blackData, setBlackData] = useState<CalibrationDataPoint | null>(null);

  const fetchStatus = useCallback(async () => {
    if (!isOpen) return; // Don't poll if modal is closed
    setIsPolling(true);
    try {
      const newStatus = await apiService.getCalibrationStatus();
      setStatus(newStatus);
      if (newStatus.whiteData) setWhiteData(newStatus.whiteData as CalibrationDataPoint); // Type assertion
      if (newStatus.blackData) setBlackData(newStatus.blackData as CalibrationDataPoint); // Type assertion

      if (newStatus.state === CalibrationState.CAL_COMPLETE || newStatus.state === CalibrationState.CAL_ERROR || newStatus.state === CalibrationState.CAL_IDLE) {
        // Stop polling if process is finished or idle
        return false; 
      }
      return true; // Continue polling
    } catch (err) {
      console.error("Fetch calibration status failed:", err);
      showToast(err instanceof Error ? err.message : 'Failed to get calibration status.', 'error');
      setStatus(prev => ({ ...prev, state: CalibrationState.CAL_ERROR, message: 'Status fetch failed' } as CalibrationStatusResponse));
      return false; // Stop polling on error
    } finally {
      setIsPolling(false);
    }
  }, [isOpen, showToast]);

  useEffect(() => {
    let intervalId: number | undefined; 
    if (isOpen && status?.inProgress && 
        (status.state === CalibrationState.CAL_WHITE_COUNTDOWN || 
         status.state === CalibrationState.CAL_BLACK_COUNTDOWN ||
         status.state === CalibrationState.CAL_SAVING)) {
      
      intervalId = setInterval(async () => {
        const shouldContinue = await fetchStatus();
        if (!shouldContinue && intervalId) clearInterval(intervalId);
      }, 1000); 

    } else if (isOpen && status?.inProgress) {
         intervalId = setInterval(async () => {
            const shouldContinue = await fetchStatus();
            if (!shouldContinue && intervalId) clearInterval(intervalId);
        }, 3000); 
    }
    
    return () => {
      if (intervalId) clearInterval(intervalId);
    };
  }, [isOpen, status?.inProgress, status?.state, fetchStatus]);
  
  useEffect(() => {
    if (isOpen) {
      fetchStatus();
      if (!status?.inProgress) {
          setWhiteData(null);
          setBlackData(null);
      }
    }
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isOpen]); 

  const handleAction = async (action: () => Promise<any>, successMessage: string) => {
    setIsLoading(true);
    try {
      await action();
      showToast(successMessage, 'success');
      await fetchStatus(); 
    } catch (err) {
      console.error("Calibration action failed:", err);
      const errorMessage = err instanceof Error ? err.message : 'Calibration action failed.';

      // Check if this is a calibration saturation error and extend duration
      const isCalibrationSaturationError = errorMessage.includes('Calibration values too high (saturated)');
      showToast(errorMessage, 'error', isCalibrationSaturationError ? 13000 : undefined);

      await fetchStatus();
    } finally {
      setIsLoading(false);
    }
  };

  const handleStart = () => handleAction(() => apiService.startCalibration(calibrationBrightness), "Calibration sequence started.");
  const handleWhiteScan = () => handleAction(apiService.performWhiteCalibration, "White reference scan initiated.");
  const handleBlackScan = () => handleAction(apiService.performBlackCalibration, "Black reference scan initiated.");
  const handleSave = async () => {
    await handleAction(apiService.saveCalibration, "Calibration data saved.");
    onCalibrationComplete(); 
  };
  const handleCancel = async () => {
    await handleAction(apiService.cancelCalibration, "Calibration cancelled.");
    onClose();
  };
  
  const renderContent = () => {
    if (!status) return <LoadingSpinner message="Loading calibration status..." />;

    const currentState = status.state;
    const message = status.message;

    return (
      <div className="space-y-4 text-center">
        <p className="text-sm text-slate-400 italic">{message || `Current state: ${CalibrationState[currentState]}`}</p>
        {status.countdown !== undefined && status.countdown > 0 && (
          <p className="text-2xl font-bold text-blue-400">Countdown: {status.countdown}</p>
        )}

        {(currentState === CalibrationState.CAL_IDLE || !status.inProgress) && (
          <>
            <Input
              id="calibBrightness"
              label="LED Brightness for Calibration"
              type="number"
              min="10" max="255"
              value={calibrationBrightness}
              onChange={(e) => setCalibrationBrightness(parseInt(e.target.value, 10))}
              wrapperClassName="max-w-xs mx-auto text-left"
            />
            <Button onClick={handleStart} isLoading={isLoading} variant="primary" size="lg">
              Start Calibration
            </Button>
          </>
        )}

        {currentState === CalibrationState.CAL_WHITE_COUNTDOWN && (
          <p className="font-semibold text-slate-200">Prepare white reference. Scanning will begin shortly.</p>
        )}
        {currentState === CalibrationState.CAL_WHITE_SCANNING && !isLoading && (
          <>
            <p className="font-semibold text-slate-200">Place white reference under sensor.</p>
            <Button onClick={handleWhiteScan} isLoading={isLoading} variant="primary">Scan White Reference</Button>
          </>
        )}
        {currentState === CalibrationState.CAL_WHITE_COMPLETE && (
          <>
            <p className="text-green-400 font-semibold">White calibration scan complete!</p>
            {whiteData && <p className="text-xs text-slate-300">X:{whiteData.x} Y:{whiteData.y} Z:{whiteData.z} IR:{whiteData.ir}</p>}
            <p className="font-semibold mt-4 text-slate-200">Prepare black reference.</p>
          </>
        )}
        
         {currentState === CalibrationState.CAL_BLACK_PROMPT && !isLoading &&(
           <>
            <p className="font-semibold text-slate-200">Place black reference under sensor.</p>
            <Button onClick={handleBlackScan} isLoading={isLoading} variant="primary">Proceed to Black Scan</Button>
           </>
        )}
        {currentState === CalibrationState.CAL_BLACK_COUNTDOWN && (
          <p className="font-semibold text-slate-200">Prepare black reference. Scanning will begin shortly.</p>
        )}
        {currentState === CalibrationState.CAL_BLACK_SCANNING && !isLoading && (
          <>
            <p className="font-semibold text-slate-200">Place black reference under sensor.</p>
            <Button onClick={handleBlackScan} isLoading={isLoading} variant="primary">Scan Black Reference</Button>
          </>
        )}
         {currentState === CalibrationState.CAL_BLACK_COMPLETE && (
          <>
            <p className="text-green-400 font-semibold">Black calibration scan complete!</p>
            {blackData && <p className="text-xs text-slate-300">X:{blackData.x} Y:{blackData.y} Z:{blackData.z} IR:{blackData.ir}</p>}
          </>
        )}

        {(currentState === CalibrationState.CAL_BLACK_COMPLETE || (currentState === CalibrationState.CAL_WHITE_COMPLETE && !blackData)) && whiteData && !isLoading &&(
            <Button onClick={handleSave} isLoading={isLoading} variant="success">Save Calibration Data</Button>
        )}
        {currentState === CalibrationState.CAL_SAVING && (
          <p className="font-semibold text-slate-200">Saving calibration data...</p>
        )}
        {currentState === CalibrationState.CAL_COMPLETE && (
          <p className="text-green-400 font-bold text-lg">Calibration Completed and Saved Successfully!</p>
        )}
        {currentState === CalibrationState.CAL_ERROR && (
          <p className="text-red-400 font-bold text-lg">Calibration Error: {message}</p>
        )}
        
        {isLoading && <LoadingSpinner />}

        {status.inProgress && currentState !== CalibrationState.CAL_IDLE && (
          <Button onClick={handleCancel} isLoading={isLoading} variant="danger" className="mt-6">
            Cancel Calibration
          </Button>
        )}
      </div>
    );
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title="Device Calibration Wizard" size="lg">
      {renderContent()}
    </Modal>
  );
};

export default CalibrationWizard;