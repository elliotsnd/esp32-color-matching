import React from 'react';

interface CardProps {
  title?: string;
  children: React.ReactNode;
  className?: string;
  titleClassName?: string;
}

const Card: React.FC<CardProps> = ({ title, children, className, titleClassName }) => {
  return (
    <div className={`bg-slate-800 shadow-md border border-slate-700 rounded-xl p-6 ${className || ''}`}>
      {title && (
        <h2 className={`text-xl font-semibold text-slate-200 mb-4 ${titleClassName || ''}`}>
          {title}
        </h2>
      )}
      {children}
    </div>
  );
};

export default Card;