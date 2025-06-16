# Calibration Error Duration Enhancement

## Overview
Extended the display duration for calibration saturation error messages from 3 seconds to 13 seconds (10 additional seconds) to give users more time to read and understand the important calibration guidance.

## Changes Made

### 1. App.tsx - Main Toast System
- **Modified `showToast` function** to accept optional `customDuration` parameter
- **Added automatic detection** of calibration saturation errors by checking message content
- **Extended duration logic**: Calibration saturation errors now display for 13 seconds instead of 3 seconds
- **Added new CSS animation** `animate-fadeInOutBackLong` for 13-second duration
- **Dynamic animation selection** based on error message content

### 2. Component Interface Updates
Updated all components that use `showToast` to support the new optional parameter:

- **CalibrationWizard.tsx**: Enhanced error handling for calibration actions
- **SettingsPanel.tsx**: Updated interface to support custom duration
- **ScanControl.tsx**: Updated interface to support custom duration  
- **SampleList.tsx**: Updated interface to support custom duration
- **LiveLedControl.tsx**: Updated interface to support custom duration

### 3. CSS Animations
- **New animation**: `fadeInOutBackLong` with 13-second duration
- **Optimized timing**: 5% fade-in, 90% visible, 5% fade-out for better UX
- **Backward compatibility**: Original 3-second animation preserved for normal messages

## Technical Implementation

### Error Detection Logic
```typescript
const isCalibrationSaturationError = message.includes('Calibration values too high (saturated)');
const duration = customDuration || (isCalibrationSaturationError ? 13000 : 3000);
```

### Animation Selection
```typescript
className={`... ${
  toast.message.includes('Calibration values too high (saturated)') 
    ? 'animate-fadeInOutBackLong' 
    : 'animate-fadeInOutBack'
} ...`}
```

### CSS Animation Definitions
```css
@keyframes fadeInOutBackLong {
  0% { opacity: 0; transform: translateY(-20px); }
  5%, 95% { opacity: 1; transform: translateY(0); }
  100% { opacity: 0; transform: translateY(-20px); }
}
.animate-fadeInOutBackLong {
  animation: fadeInOutBackLong 13s ease-in-out forwards;
}
```

## Error Message Targeted
The enhancement specifically targets this error message:
```
"API Error: 400 Bad Request - Calibration values too high (saturated). Reduce brightness or move away from light source."
```

## Benefits
1. **Better User Experience**: Users have more time to read important calibration guidance
2. **Reduced Confusion**: Extended visibility helps users understand what action to take
3. **Improved Calibration Success**: Users are more likely to see and follow the guidance
4. **Backward Compatibility**: Normal error messages still display for 3 seconds
5. **Automatic Detection**: No manual configuration required - works automatically

## Testing
- Created `test-calibration-error.html` to demonstrate the functionality
- Verified build process completes successfully
- Confirmed all component interfaces are properly updated
- Tested both normal (3s) and calibration saturation (13s) error durations

## Usage
The enhancement works automatically - no code changes needed in components that call `showToast`. The system automatically detects calibration saturation errors and extends their display duration.

For manual control, components can still pass a custom duration:
```typescript
showToast('Custom message', 'error', 10000); // 10 seconds
```
