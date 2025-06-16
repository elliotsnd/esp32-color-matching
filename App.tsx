import React, { useState, useEffect, useCallback } from 'react';
import { DeviceStatus, ColorSample } from './types';
import * as apiService from './services/apiService';
import StatusDisplay from './components/StatusDisplay';
import ScanControl from './components/ScanControl';
import { EnhancedScanControl } from './components/EnhancedScanControl';
import SampleList from './components/SampleList';
import SettingsPanel from './components/SettingsPanel';
import LiveLedControl from './components/LiveLedControl';

import Button from './components/ui/Button';
import Card from './components/ui/Card';
import { DEVICE_BASE_URL, API_PATHS } from './constants';

const App: React.FC = () => {
  const [status, setStatus] = useState<DeviceStatus | null>(null);
  const [samples, setSamples] = useState<ColorSample[]>([]);
  const [isLoadingStatus, setIsLoadingStatus] = useState(true);
  const [isLoadingSamples, setIsLoadingSamples] = useState(true);
  const [statusError, setStatusError] = useState<string | null>(null);
  const [samplesError, setSamplesError] = useState<string | null>(null);


  const [toast, setToast] = useState<{ message: string; type: 'success' | 'error'; id: number } | null>(null);

  // External link styling for Device Settings button
  const externalLinkClass = "flex items-center gap-1.5 px-3 py-2 sm:px-4 sm:py-2 rounded-md text-sm font-medium text-amber-700 bg-amber-100 hover:bg-amber-200 hover:shadow-md transition-all duration-150 ease-in-out focus:outline-none focus:ring-2 focus:ring-amber-400 border border-amber-300";

  const showToast = (message: string, type: 'success' | 'error', customDuration?: number) => {
    setToast({ message, type, id: Date.now() });

    // Check if this is a calibration saturation error and extend duration
    const isCalibrationSaturationError = message.includes('Calibration values too high (saturated)');
    const duration = customDuration || (isCalibrationSaturationError ? 13000 : 3000);

    setTimeout(() => setToast(null), duration);
  };

  const fetchStatus = useCallback(async () => {
    setIsLoadingStatus(true);
    setStatusError(null);
    try {
      const data = await apiService.getDeviceStatus();
      setStatus(data);
    } catch (err) {
      console.error("Fetch status failed:", err);
      setStatusError(err instanceof Error ? err.message : 'Failed to load device status.');
      showToast(err instanceof Error ? err.message : 'Failed to load device status.', 'error');
    } finally {
      setIsLoadingStatus(false);
    }
  }, []);

  const fetchSamples = useCallback(async () => {
    setIsLoadingSamples(true);
    setSamplesError(null);
    try {
      const data = await apiService.getSavedSamples();
      // Sort samples by timestamp, newest first
      const sortedSamples = data.samples.sort((a,b) => b.timestamp - a.timestamp);
      setSamples(sortedSamples);
    } catch (err) {
      console.error("Fetch samples failed:", err);
      setSamplesError(err instanceof Error ? err.message : 'Failed to load samples.');
      showToast(err instanceof Error ? err.message : 'Failed to load samples.', 'error');
    } finally {
      setIsLoadingSamples(false);
    }
  }, []);

  useEffect(() => {
    fetchStatus();
    fetchSamples();
    
    // Optional: Poll for status periodically
    const intervalId = setInterval(fetchStatus, 30000); // every 30 seconds
    return () => clearInterval(intervalId);
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []); // Run once on mount

  const handleSettingsChanged = (newSettings: Partial<DeviceStatus>) => {
    setStatus(prev => prev ? { ...prev, ...newSettings } : null);
    // Trust the partial update instead of immediately re-fetching to avoid race conditions
    // The periodic refresh (every 30 seconds) will sync any discrepancies
  };
  
  const handleSampleSaved = () => {
    fetchSamples(); // Refresh sample list
    fetchStatus(); // Refresh status (sampleCount might have changed)
  };

  const handleCalibrationComplete = () => {
    fetchStatus(); // Refresh device status, especially isCalibrated
  };



  return (
    <div className="min-h-screen bg-slate-900 p-4 md:p-8">
      {/* Toast Notification */}
      {toast && (
        <div className={`fixed top-5 right-5 p-4 rounded-md shadow-lg text-white ${
          toast.message.includes('Calibration values too high (saturated)')
            ? 'animate-fadeInOutBackLong'
            : 'animate-fadeInOutBack'
        } ${toast.type === 'success' ? 'bg-green-600' : 'bg-red-600'}`}
             style={{zIndex: 1000}} // Ensure toast is on top
        >
          {toast.message}
        </div>
      )}

      <header className="mb-8">
        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between">
          <div className="text-center sm:text-left">
            <h1 className="text-4xl font-bold text-slate-100">ESP32 Color Matcher</h1>
            <p className="text-slate-400">Web Interface</p>
          </div>
          <nav className="mt-4 sm:mt-0 flex justify-center sm:justify-end">
            <a
              href={`${DEVICE_BASE_URL}${API_PATHS.SETTINGS_PAGE}`}
              target="_blank"
              rel="noopener noreferrer"
              aria-label="Open device settings (external link)"
              className={externalLinkClass}
            >
              <span className="hidden sm:inline">Device Settings</span>
              <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-5 h-5">
                <path strokeLinecap="round" strokeLinejoin="round" d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.646.87.074.04.147.083.22.127.324.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.24-.438.613-.431.992a6.759 6.759 0 0 1 0 1.905c-.007.379.138.75.43.99l1.005.828c.424.35.534.954.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.57 6.57 0 0 1-.22.128c-.333.184-.582.496-.646.87l-.212 1.282c-.09.542-.56.94-1.11.94h-2.594c-.55 0-1.02-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.992a6.759 6.759 0 0 1 0-1.905c.007-.379-.138-.75-.43-.99l-1.004-.828a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.087.22-.128.332-.184.582-.496.644-.87l.214-1.282Z" />
                <path strokeLinecap="round" strokeLinejoin="round" d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z" />
              </svg>
            </a>
          </nav>
        </div>
      </header>

      <main className="max-w-6xl mx-auto grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left Column (or full width on smaller screens) */}
        <section className="lg:col-span-2 space-y-6">
          <StatusDisplay status={status} isLoading={isLoadingStatus} error={statusError} onRefresh={fetchStatus} />
          <ScanControl onSampleSaved={handleSampleSaved} showToast={showToast} />
          <EnhancedScanControl />
          <SampleList
            samples={samples}
            isLoading={isLoadingSamples}
            error={samplesError}
            onSampleDeleted={handleSampleSaved}
            showToast={showToast}
          />
        </section>

        {/* Right Column (Sidebar) */}
        <aside className="lg:col-span-1 space-y-6">

          {status && <LiveLedControl showToast={showToast} initialLedState={status?.ledState} />}
          <SettingsPanel initialSettings={status} onSettingsChange={handleSettingsChanged} showToast={showToast} />
        </aside>
      </main>




      
      <footer className="mt-12 text-center text-sm text-slate-400">
        <p>&copy; {new Date().getFullYear()} ESP32 Color Matcher Interface. For device firmware version X.Y.Z.</p>
      </footer>
       <style>{`
        @keyframes fadeInOutBack {
          0% { opacity: 0; transform: translateY(-20px); }
          10%, 90% { opacity: 1; transform: translateY(0); }
          100% { opacity: 0; transform: translateY(-20px); }
        }
        @keyframes fadeInOutBackLong {
          0% { opacity: 0; transform: translateY(-20px); }
          5%, 95% { opacity: 1; transform: translateY(0); }
          100% { opacity: 0; transform: translateY(-20px); }
        }
        .animate-fadeInOutBack {
          animation: fadeInOutBack 3s ease-in-out forwards;
        }
        .animate-fadeInOutBackLong {
          animation: fadeInOutBackLong 13s ease-in-out forwards;
        }
      `}</style>
    </div>
  );
};

export default App;