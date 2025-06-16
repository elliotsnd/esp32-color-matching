# ESP32 Color Matcher Calibration Results - Baseline Settings

## Calibration Session Summary
**Date:** 2025-01-14  
**Methodology:** Systematic Vivid White Calibration  
**Target:** RGB (255, 255, 255) for Dulux Vivid White (W01A1, LRV 90.00)

## Baseline Settings Applied
```
ATIME (Integration Time): 150
AGAIN (Analog Gain): 16x (index 2)
Scan Brightness: 128
LED Brightness for Calibration: 128
Auto-Zero Mode: Use previous offset (recommended)
Auto-Zero Frequency: 76
Wait Time: 194
```

## Calibration Process Completed
✅ White reference scan completed successfully  
✅ Black reference scan completed successfully  
✅ Black calibration data: X:1488 Y:1484 Z:647 IR:99  
✅ Calibration data saved to NVS storage  
✅ Device status shows "Calibrated: Yes"

## Test Results - Three Consecutive Scans

### Scan 1
- **RGB:** (255, 255, 134)
- **XYZ:** (40663, 41225, 16960) IR: 3265

### Scan 2  
- **RGB:** (254, 255, 131)
- **XYZ:** (35426, 36639, 14634) IR: 2656

### Scan 3
- **RGB:** (254, 255, 131)  
- **XYZ:** (36711, 37818, 15105) IR: 2751

## Analysis

### ✅ Successful Aspects
1. **Excellent Consistency:** Results highly repeatable (±1-3 RGB units)
2. **Optimal XYZ Range:** All values in target 30,000-50,000 range
3. **R & G Channels:** Nearly perfect at 254-255
4. **Calibration Persistence:** Data properly stored in NVS
5. **Baseline Settings:** Successfully applied and maintained

### ❌ Critical Issue
- **Blue Channel Deficiency:** Consistently reads ~131-134 instead of required 255
- **Deviation:** ~120-124 units below target (47-49% of required value)
- **Status:** CALIBRATION INCOMPLETE - does not meet strict RGB (255,255,255) requirement

## Calibration Status
**RESULT:** Partial Success - Requires Blue Channel Adjustment  
**NEXT STEPS:** Iterative calibration adjustment needed to correct blue channel factor

## Technical Notes
- XYZ values indicate no saturation issues
- Ambient light properly controlled during scans
- Two-point calibration (white + black) successfully implemented
- Calibration algorithm functioning but blue channel factor insufficient

## Settings for Future Reference
These baseline settings provide a solid foundation for calibration:
- Sensor stability: Excellent
- Reading consistency: Excellent  
- XYZ range optimization: Excellent
- Blue channel calibration: Requires adjustment

**Recommendation:** Use these settings as starting point for iterative blue channel calibration adjustment.
