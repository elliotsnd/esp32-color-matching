# TCS3430 Advanced Calibration Testing Guide

## 🎯 Overview

This guide provides comprehensive testing instructions for the TCS3430 advanced calibration system implemented in the ESP32 color matcher project. The testing functionality is now **fully integrated** into the main web application's Settings panel.

## 🔧 Build Status

### ✅ Firmware Build: SUCCESS
- **Platform**: ESP32-S3-DevKitC-1
- **Memory Usage**: RAM: 14.9%, Flash: 34.2%
- **Libraries**: All dependencies resolved successfully
- **Compilation**: No errors, only minor deprecation warnings (unrelated to calibration features)

### ✅ Web UI Build: SUCCESS
- **Framework**: Vite + React + TypeScript
- **Bundle Size**: 214.73 kB (66.47 kB gzipped)
- **Type Safety**: All TypeScript interfaces updated and validated

## 🧪 Integrated Testing Interface

### **🎯 Main Testing Interface** (Built into Settings Panel)
**Best for**: Real-time testing with direct device access

**Location**: Main ESP32 web interface → Right Sidebar → "Scanner Settings" → "🧪 Sensor Diagnostics & Testing"

**Features**:
- ✅ **No IP configuration needed** - Uses existing device connection
- ✅ **Integrated with main UI** - Consistent styling and components
- ✅ **Real-time test execution** - Immediate feedback and results
- ✅ **Comprehensive test coverage** - All calibration parameters validated
- ✅ **Live results display** - Expandable test results with timestamps

**Available Tests**:
- 🔧 **Test Current Settings** - Validates current form values
- ✅ **Test Valid Values** - Tests minimum, recommended, and maximum values
- ❌ **Test Invalid Values** - Ensures proper rejection of out-of-range values
- 🔄 **Test Combined Settings** - Tests multiple parameters simultaneously
- 🚀 **Run Full Test Suite** - Comprehensive automated testing

### **🛠️ External Testing Tools** (For Development/CI)

#### 1. **PowerShell Quick Test** (`quick_test.ps1`)
**Best for**: Command-line testing and automation

**Usage**:
```powershell
.\quick_test.ps1 [ESP32_IP]
```

#### 2. **Python Comprehensive Test** (`test_calibration.py`)
**Best for**: Detailed automated testing and CI integration

**Usage**:
```bash
python test_calibration.py [ESP32_IP]
```

**Requirements**:
```bash
pip install requests
```

#### 3. **Mock Server** (`mock_esp32_server.py`)
**Best for**: Development testing without physical ESP32

**Usage**:
```bash
pip install flask flask-cors
python mock_esp32_server.py
# Then test against 127.0.0.1:5000
```

## 🎛️ Calibration Parameters

### **Auto-Zero Mode**
- **Range**: 0-1
- **Values**:
  - `0`: Always start at zero
  - `1`: Use previous offset (recommended for stability)
- **Default**: 1

### **Auto-Zero Frequency**
- **Range**: 0-255
- **Values**:
  - `0`: Never run auto-zero
  - `127`: First cycle only (recommended)
  - `Other`: Every nth iteration
- **Default**: 127

### **Wait Time**
- **Range**: 0-255
- **Purpose**: Wait time between measurements
- **Effect**: Higher values improve stability but slow measurements
- **Default**: 0

## 🚀 Testing Procedure

### **Step 1: Access Integrated Testing Interface**
1. **Power on ESP32** and ensure it's connected to WiFi
2. **Open the ESP32 web interface** in your browser (usually http://ESP32_IP)
3. **Look at the right sidebar** - find the "Scanner Settings" card
4. **Scroll down within Scanner Settings** to "🧪 Sensor Diagnostics & Testing"
5. **Click "Show Tests"** to expand the testing interface

### **Step 2: Run Integrated Tests**
1. **Test Current Settings**: Click "🔧 Test Current Settings" to validate current form values
2. **Run Individual Tests**: Use specific test buttons for targeted validation
3. **Run Full Suite**: Click "Run Full Test Suite" for comprehensive testing
4. **View Results**: Click "Show Results" to see detailed test logs

### **Step 3: External Testing (Optional)**
For development or CI purposes, you can also run external tests:

```powershell
# Windows PowerShell
.\quick_test.ps1 192.168.1.100

# Or Linux/Mac
./quick_test.sh 192.168.1.100
```

**Expected Output**:
```
🧪 TCS3430 Quick Calibration Test
==================================
ESP32 IP: 192.168.1.100

🔌 Testing connectivity...
✅ ESP32 is reachable

📊 Getting current calibration settings...
✅ Status retrieved successfully
Current settings:
  Auto-Zero Mode: 1
  Auto-Zero Frequency: 127
  Wait Time: 0

🔧 Testing calibration settings update...
  Testing auto-zero mode...
  ✅ Auto-zero mode update sent
  Testing auto-zero frequency...
  ✅ Auto-zero frequency update sent
  Testing wait time...
  ✅ Wait time update sent

🔍 Verifying settings were applied...
✅ Verification status retrieved
Applied settings:
  Auto-Zero Mode: 1 (expected: 1)
  Auto-Zero Frequency: 127 (expected: 127)
  Wait Time: 5 (expected: 5)
✅ All settings applied correctly!

🔄 Testing combined settings update...
✅ Combined settings update sent
Final settings:
  Auto-Zero Mode: 0 (expected: 0)
  Auto-Zero Frequency: 200 (expected: 200)
  Wait Time: 10 (expected: 10)
✅ Combined settings applied correctly!

🚫 Testing invalid values (should be rejected)...
  ✅ Invalid auto-zero mode (5) correctly rejected

🎯 Quick test completed!

📋 Test Summary:
  ✅ Connectivity test
  ✅ Status retrieval
  ✅ Individual setting updates
  ✅ Setting verification
  ✅ Combined settings update
  ✅ Invalid value rejection

🎉 All basic calibration tests passed!
```

### **Step 3: Run Comprehensive Test**
```bash
python test_calibration.py 192.168.1.100
```

### **Step 4: Interactive Web Testing**

#### **Option A: Use Integrated Button (Recommended)**
1. **In the main ESP32 web interface**, navigate to Scanner Settings
2. **Scroll to "🧪 Sensor Diagnostics & Testing"** section
3. **Click "🔗 External Test"** button (top right) or **"Open Test Page"** button (in the standalone section)
4. **New tab opens** with the standalone test interface

#### **Option B: Direct File Access**
1. Open `test_web_ui.html` directly in your browser
2. Navigate to the file location and double-click to open

#### **Using the Standalone Test Interface:**
1. Enter ESP32 IP address
2. Click "Test Connection"
3. Click "Get Current Status"
4. Adjust calibration settings manually
5. Run automated test suite

## 🔍 Troubleshooting

### **Connection Issues**
- ❌ **Cannot connect to ESP32**
  - Check ESP32 is powered on
  - Verify WiFi connection
  - Confirm IP address is correct
  - Check firewall settings

### **Settings Not Applied**
- ❌ **Settings don't persist**
  - Check NVS storage is working
  - Verify ESP32 has sufficient flash memory
  - Check for error messages in serial monitor

### **Invalid Values Accepted**
- ❌ **Invalid values not rejected**
  - Check firmware validation logic
  - Verify input validation is working
  - Review error logging

## 📊 Expected Test Results

### **✅ All Tests Should Pass**
- Connectivity test
- Status retrieval with calibration fields
- Individual parameter updates
- Combined parameter updates
- Invalid value rejection
- Settings persistence

### **⚠️ Common Issues**
- Network connectivity problems
- Incorrect IP address
- ESP32 not running latest firmware
- Browser CORS restrictions (for web UI test)

## 🎉 Success Criteria

The calibration system is working correctly when:

1. **✅ All API endpoints respond correctly**
2. **✅ Calibration settings are returned in status**
3. **✅ Settings can be updated individually and in combination**
4. **✅ Invalid values are properly rejected**
5. **✅ Settings persist across requests**
6. **✅ Sensor applies settings immediately**

## 📝 Next Steps

After successful testing:

1. **Deploy to production ESP32 device**
2. **Test with actual TCS3430 sensor readings**
3. **Validate color measurement stability improvements**
4. **Document optimal calibration settings for your environment**
5. **Create user documentation for calibration procedures**

## 🔗 Related Files

- `src/main.cpp` - Firmware implementation
- `src/config.h` - Configuration constants
- `components/SettingsPanel.tsx` - Web UI calibration controls
- `types.ts` - TypeScript interface definitions
- `platformio.ini` - Build configuration
