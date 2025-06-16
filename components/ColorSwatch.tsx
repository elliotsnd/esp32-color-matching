import React from 'react';

interface ColorSwatchProps {
  r: number;
  g: number;
  b: number;
  size?: 'sm' | 'md' | 'lg' | 'xl';
  className?: string;
  showRgbText?: boolean;
}

const ColorSwatch: React.FC<ColorSwatchProps> = ({ r, g, b, size = 'md', className, showRgbText = false }) => {
  const sizeClasses = {
    sm: 'w-8 h-8',
    md: 'w-12 h-12',
    lg: 'w-20 h-20',
    xl: 'w-32 h-32',
  };

  const backgroundColor = `rgb(${r}, ${g}, ${b})`;
  const textColor = (r * 0.299 + g * 0.587 + b * 0.114) > 186 ? '#000000' : '#FFFFFF';


  return (
    <div className={`rounded-md shadow-sm border border-slate-600 flex items-center justify-center ${sizeClasses[size]} ${className || ''}`} style={{ backgroundColor }}>
       {showRgbText && (
         <span style={{ color: textColor }} className="text-xs font-mono">
           {r},{g},{b}
         </span>
       )}
    </div>
  );
};

export default ColorSwatch;