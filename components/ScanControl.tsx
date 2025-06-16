import React, { useState } from 'react';
import { ScannedColorData } from '../types';
import * as apiService from '../services/apiService';
import Button from './ui/Button';
import Card from './ui/Card';
import ColorSwatch from './ColorSwatch';
import LoadingSpinner from './LoadingSpinner';

interface ScanControlProps {
  onSampleSaved: () => void; // Callback to refresh samples list
  showToast: (message: string, type: 'success' | 'error', customDuration?: number) => void;
}

const ScanControl: React.FC<ScanControlProps> = ({ onSampleSaved, showToast }) => {
  const [scannedColor, setScannedColor] = useState<ScannedColorData | null>(null);
  const [isScanning, setIsScanning] = useState(false);
  const [isSaving, setIsSaving] = useState(false);

  const handleScan = async () => {
    setIsScanning(true);
    setScannedColor(null);
    try {
      const data = await apiService.startScan();
      setScannedColor(data);
      showToast('Scan successful!', 'success');
    } catch (err) {
      console.error("Scan failed:", err);
      showToast(err instanceof Error ? err.message : 'Scan failed.', 'error');
    } finally {
      setIsScanning(false);
    }
  };

  const handleSaveSample = async () => {
    if (!scannedColor) {
      showToast('No color scanned to save.', 'error');
      return;
    }
    setIsSaving(true);
    try {
      await apiService.saveSample(scannedColor.r, scannedColor.g, scannedColor.b);
      showToast('Sample saved!', 'success');
      onSampleSaved(); // Trigger samples refresh
      setScannedColor(null); // Clear after saving
    } catch (err) {
      console.error("Save sample failed:", err);
      showToast(err instanceof Error ? err.message : 'Save sample failed.', 'error');
    } finally {
      setIsSaving(false);
    }
  };

  return (
    <Card title="Color Scanner" className="mb-6">
      <div className="flex flex-col items-center space-y-4">
        <Button onClick={handleScan} isLoading={isScanning} disabled={isScanning || isSaving} variant="primary" size="lg">
          Scan Color
        </Button>
        
        {isScanning && <LoadingSpinner message="Scanning..." />}

        {scannedColor && (
          <div className="mt-6 p-4 border border-slate-700 rounded-lg shadow-sm w-full max-w-md text-center">
            <h3 className="text-lg font-medium text-slate-200 mb-3">Scanned Color:</h3>
            <div className="flex justify-center mb-3">
                <ColorSwatch r={scannedColor.r} g={scannedColor.g} b={scannedColor.b} size="xl" />
            </div>
            <p className="font-mono text-slate-300">RGB: ({scannedColor.r}, {scannedColor.g}, {scannedColor.b})</p>
            <p className="font-mono text-xs text-slate-400">XYZ: ({scannedColor.x}, {scannedColor.y}, {scannedColor.z}) IR: {scannedColor.ir}</p>
            <Button 
              onClick={handleSaveSample} 
              isLoading={isSaving} 
              disabled={isScanning || isSaving} 
              variant="success" 
              className="mt-4"
            >
              Save Sample
            </Button>
          </div>
        )}
      </div>
    </Card>
  );
};

export default ScanControl;