import React from 'react';
import { DeviceStatus } from '../types';
import Card from './ui/Card';
import ColorSwatch from './ColorSwatch';
import LoadingSpinner from './LoadingSpinner';

interface StatusDisplayProps {
  status: DeviceStatus | null;
  isLoading: boolean;
  error: string | null;
  onRefresh: () => void;
}

const StatusItem: React.FC<{ label: string; value: React.ReactNode }> = ({ label, value }) => (
  <div className="flex justify-between py-1 border-b border-slate-700 last:border-b-0">
    <span className="text-sm text-slate-400">{label}:</span>
    <span className="text-sm font-medium text-slate-100 text-right">{value}</span>
  </div>
);

const StatusDisplay: React.FC<StatusDisplayProps> = ({ status, isLoading, error, onRefresh }) => {
  if (isLoading && !status) return <Card title="Device Status"><LoadingSpinner message="Loading status..." /></Card>;
  if (error) return <Card title="Device Status"><p className="text-red-400">Error: {error}</p></Card>;
  if (!status) return <Card title="Device Status"><p className="text-slate-400">No status data available.</p></Card>;

  return (
    <Card title="Device Status" className="mb-6">
      <div className="grid grid-cols-1 md:grid-cols-2 gap-x-6 gap-y-2">
        <StatusItem label="Scanning" value={status.isScanning ? 'Yes' : 'No'} />
        <StatusItem label="LED On" value={status.ledState ? 'Yes' : 'No'} />
        <StatusItem label="Calibrated" value={status.isCalibrated ? 'Yes' : 'No'} />
        <StatusItem label="Sample Count" value={status.sampleCount} />
        <StatusItem label="ATIME" value={status.atime} />
        <StatusItem label="AGAIN" value={status.again} />
        <StatusItem label="Scan Brightness" value={status.brightness} />
        <StatusItem label="Ambient Lux" value={`${status.ambientLux.toFixed(2)} lx`} />
        {status.rssi && <StatusItem label="WiFi RSSI" value={`${status.rssi} dBm`} />}
        {status.esp32IP && <StatusItem label="Device IP" value={status.esp32IP} />}
        <div className="md:col-span-2 flex items-center space-x-2 py-1">
          <span className="text-sm text-slate-400">Current LED Color:</span>
          <ColorSwatch r={status.currentR} g={status.currentG} b={status.currentB} size="sm" />
        </div>
      </div>
       <div className="mt-4 flex justify-end">
        <button
          onClick={onRefresh}
          disabled={isLoading}
          className="text-sm text-blue-400 hover:text-blue-300 disabled:opacity-50"
        >
          {isLoading ? 'Refreshing...' : 'Refresh Status'}
        </button>
      </div>
    </Card>
  );
};

export default StatusDisplay;