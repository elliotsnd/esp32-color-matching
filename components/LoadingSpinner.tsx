import React from 'react';

interface LoadingSpinnerProps {
  size?: 'sm' | 'md' | 'lg';
  className?: string;
  message?: string;
}

const LoadingSpinner: React.FC<LoadingSpinnerProps> = ({ size = 'md', className, message }) => {
  const sizeClasses = {
    sm: 'w-6 h-6 border-2',
    md: 'w-8 h-8 border-4',
    lg: 'w-12 h-12 border-4',
  };

  return (
    <div className={`flex flex-col items-center justify-center ${className || ''}`}>
      <div 
        className={`spinner ${sizeClasses[size]} border-slate-600 border-t-blue-500 rounded-full animate-spin`}
      />
      {message && <p className="mt-2 text-sm text-slate-400">{message}</p>}
    </div>
  );
};
export default LoadingSpinner;