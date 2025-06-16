import React, { useState } from 'react';
import { ColorSample } from '../types';
import Card from './ui/Card';
import ColorSwatch from './ColorSwatch';
import LoadingSpinner from './LoadingSpinner';
import * as apiService from '../services/apiService';

interface SampleListProps {
  samples: ColorSample[];
  isLoading: boolean;
  error: string | null;
  onSampleDeleted: () => void; // Callback to refresh samples list
  showToast: (message: string, type: 'success' | 'error', customDuration?: number) => void;
}

interface SampleItemProps {
  sample: ColorSample;
  index: number;
  onDelete: (index: number) => void;
}

const SampleItem: React.FC<SampleItemProps> = ({ sample, index, onDelete }) => {
  const [isDeleting, setIsDeleting] = useState(false);

  const handleDelete = async (e: React.MouseEvent) => {
    e.preventDefault();
    e.stopPropagation();

    if (window.confirm('Are you sure you want to delete this sample?')) {
      setIsDeleting(true);
      try {
        await onDelete(index);
      } finally {
        setIsDeleting(false);
      }
    }
  };

  return (
    <li className="bg-slate-700 p-4 rounded-lg shadow hover:shadow-md transition-shadow duration-150 relative group">
      <div className="flex items-start space-x-4">
        <ColorSwatch r={sample.r} g={sample.g} b={sample.b} size="lg" />
        <div className="flex-1">
          <h3 className="text-sm font-semibold text-slate-100">
            {sample.paintName && sample.paintName !== "Unknown" ? sample.paintName : `RGB: ${sample.r}, ${sample.g}, ${sample.b}`}
          </h3>
          {sample.paintCode && sample.paintCode !== "N/A" && (
            <p className="text-xs text-slate-300">Code: {sample.paintCode}</p>
          )}
          {(sample.paintName && sample.paintName !== "Unknown") && (
               <p className="text-xs text-slate-400 font-mono">RGB: {sample.r}, {sample.g}, {sample.b}</p>
          )}
          {sample.lrv > 0 && <p className="text-xs text-slate-400">LRV: {sample.lrv.toFixed(1)}</p>}
          <p className="text-xs text-slate-500 mt-1">
            Saved: {new Date(sample.timestamp).toLocaleString()}
          </p>
        </div>

        {/* Delete Button */}
        <button
          onClick={handleDelete}
          disabled={isDeleting}
          className="absolute top-2 right-2 w-6 h-6 bg-red-600 hover:bg-red-700 disabled:bg-red-800 disabled:opacity-50 text-white rounded-full flex items-center justify-center text-xs font-bold transition-colors duration-150"
          title="Delete sample"
        >
          {isDeleting ? (
            <div className="w-3 h-3 border border-white border-t-transparent rounded-full animate-spin"></div>
          ) : (
            '×'
          )}
        </button>
      </div>
    </li>
  );
};

const SampleList: React.FC<SampleListProps> = ({ samples, isLoading, error, onSampleDeleted, showToast }) => {
  const [isDeleting, setIsDeleting] = useState(false);
  const [showDeleteAllConfirm, setShowDeleteAllConfirm] = useState(false);

  const handleDeleteSample = async (index: number) => {
    try {
      const result = await apiService.deleteSample(index);
      if (result.success) {
        showToast('Sample deleted successfully!', 'success');
        onSampleDeleted(); // Refresh the samples list
      } else {
        showToast(result.message || 'Failed to delete sample', 'error');
      }
    } catch (err) {
      console.error("Delete sample failed:", err);
      showToast(err instanceof Error ? err.message : 'Failed to delete sample', 'error');
    }
  };

  const handleDeleteAll = async () => {
    setIsDeleting(true);
    try {
      const result = await apiService.clearAllSamples();
      if (result.success) {
        showToast('All samples deleted successfully!', 'success');
        onSampleDeleted(); // Refresh the samples list
      } else {
        showToast(result.message || 'Failed to delete all samples', 'error');
      }
    } catch (err) {
      console.error("Delete all samples failed:", err);
      showToast(err instanceof Error ? err.message : 'Failed to delete all samples', 'error');
    } finally {
      setIsDeleting(false);
      setShowDeleteAllConfirm(false);
    }
  };

  if (isLoading) return <Card title="Saved Samples"><LoadingSpinner message="Loading samples..." /></Card>;
  if (error) return <Card title="Saved Samples"><p className="text-red-400">Error: {error}</p></Card>;

  return (
    <Card title="Saved Samples" className="mb-6">
      {samples.length === 0 ? (
        <p className="text-slate-400">No samples saved yet.</p>
      ) : (
        <>
          {/* Delete All Button */}
          <div className="mb-4 flex justify-end">
            <button
              onClick={() => setShowDeleteAllConfirm(true)}
              disabled={isDeleting}
              className="px-3 py-1 bg-red-600 hover:bg-red-700 disabled:bg-red-800 disabled:opacity-50 text-white text-sm rounded transition-colors duration-150"
              title="Delete all samples"
            >
              {isDeleting ? (
                <div className="flex items-center">
                  <div className="w-3 h-3 border border-white border-t-transparent rounded-full animate-spin mr-2"></div>
                  Deleting...
                </div>
              ) : (
                'Delete All'
              )}
            </button>
          </div>

          {/* Samples List */}
          <ul className="space-y-3 max-h-96 overflow-y-auto pr-2">
            {samples.map((sample, index) => (
              // Assuming timestamp can be a unique enough key for this context
              // For production, a stable unique ID per sample would be better
              <SampleItem
                key={`${sample.timestamp}-${index}`}
                sample={sample}
                index={index}
                onDelete={handleDeleteSample}
              />
            ))}
          </ul>

          {/* Confirmation Dialog */}
          {showDeleteAllConfirm && (
            <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
              <div className="bg-slate-800 p-6 rounded-lg shadow-xl max-w-md w-full mx-4">
                <h3 className="text-lg font-semibold text-slate-100 mb-4">Confirm Delete All</h3>
                <p className="text-slate-300 mb-6">
                  Are you sure you want to delete all {samples.length} saved samples? This action cannot be undone.
                </p>
                <div className="flex justify-end space-x-3">
                  <button
                    onClick={() => setShowDeleteAllConfirm(false)}
                    disabled={isDeleting}
                    className="px-4 py-2 bg-slate-600 hover:bg-slate-700 disabled:opacity-50 text-white rounded transition-colors duration-150"
                  >
                    Cancel
                  </button>
                  <button
                    onClick={handleDeleteAll}
                    disabled={isDeleting}
                    className="px-4 py-2 bg-red-600 hover:bg-red-700 disabled:bg-red-800 disabled:opacity-50 text-white rounded transition-colors duration-150"
                  >
                    {isDeleting ? 'Deleting...' : 'Delete All'}
                  </button>
                </div>
              </div>
            </div>
          )}
        </>
      )}
    </Card>
  );
};

export default SampleList;