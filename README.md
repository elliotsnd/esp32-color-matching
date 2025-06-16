# ESP32 Color Matcher

A sophisticated ESP32-based color measurement device featuring a TCS3430 sensor with advanced colorimetric calibration and a modern React TypeScript web interface.

## 🌈 Features

- **High-Precision Color Measurement**: TCS3430 sensor with matrix-based colorimetric calibration
- **Advanced Calibration System**:
  - Matrix-based 3x4 transformation calibration using 7-color patches
  - White point calibration with Dulux Vivid White reference
  - Black point calibration for true darkness measurement
  - CIE 1931 color space conversion with chromatic adaptation
- **Enhanced LED Control**:
  - Automatic brightness optimization
  - Real-time sensor feedback with color-coded status indicators
  - Manual and enhanced auto-adjustment modes
- **Modern Web Interface**: React TypeScript application with real-time updates
- **Google Apps Script Integration**: Cloud-based color matching with Dulux paint database
- **Automated Deployment**: PowerShell scripts for streamlined firmware and web interface deployment

## 🛠 Hardware Requirements

- **ESP32-S3 DevKit-C-1** (or compatible ESP32 board)
- **TCS3430 Color Sensor** (DFRobot or compatible)
- **LED** for illumination (connected to pin 5)
- **I2C Connection**: SDA/SCL pins for sensor communication

## 📋 Prerequisites

- **Node.js** (v16 or higher)
- **PlatformIO** (for ESP32 firmware development)
- **Python** (for test scripts)
- **PowerShell** (for deployment scripts on Windows)

## 🚀 Quick Start

### 1. Clone and Setup
```bash
git clone <repository-url>
cd esp32-color-matcher
npm install
```

### 2. Configure Environment
Create a `.env` file with your device IP:
```
DEVICE_IP=192.168.0.152
```

### 3. Build and Deploy Web Interface
```bash
npm run build
npm run deploy
```

### 4. Flash ESP32 Firmware
```bash
pio run --target upload
```

### 5. Access the Interface
Open your browser to `http://192.168.0.152` (or your device IP)

## 🔧 Development

### Web Interface Development
```bash
npm run dev          # Start development server
npm run build        # Build for production
npm run deploy       # Deploy to ESP32
```

### ESP32 Firmware Development
```bash
pio run              # Build firmware
pio run --target upload    # Upload to device
pio device monitor   # Monitor serial output
```

### Testing
```bash
# Run various test scripts
./quick_test.ps1
python test_calibration.py
python mock_esp32_server.py  # For offline testing
```

## 📡 API Endpoints

The ESP32 device exposes a REST API for control and monitoring:

- `GET /status` - Device status and sensor readings
- `GET /samples` - Retrieve stored color samples
- `POST /settings` - Update sensor settings (ATIME, AGAIN, etc.)
- `POST /calibrate` - Perform calibration operations
- `POST /led` - Control LED brightness and modes
- `DELETE /samples/{id}` - Delete specific sample
- `DELETE /samples/clear` - Clear all samples

## 🎯 Calibration Process

### 1. Optimal Sensor Settings
- **ATIME**: 150
- **AGAIN**: 16x
- **Scan Brightness**: 128
- **Auto-Zero Mode**: 1
- **Auto-Zero Frequency**: 127
- **Wait Time**: 0-10

### 2. White Point Calibration
1. Use **Dulux Vivid White** paint (code W01A1, LRV 90.00)
2. Adjust settings to achieve RGB (255, 255, 255) reading
3. Maintain XYZ values in 30,000-50,000 range
4. Verify calibration persistence in NVS

### 3. Matrix Calibration
1. Prepare 7-color patch chart (Red, Yellow, Green, Cyan, Blue, Magenta, Black)
2. Scan each color patch following on-screen prompts
3. System computes 3x4 transformation matrix using least-squares fitting
4. Validation targets Delta E 2000 < 2 for accuracy

## 🔬 Technical Details

### Color Space Processing
- **Input**: TCS3430 raw sensor data (R, G, B, C, IR)
- **Processing**: CIE 1931 color matching functions with piecewise-Gaussian approximations
- **Intermediate**: XYZ coordinates with white point normalization
- **Output**: sRGB with gamma correction and chromatic adaptation

### Enhanced LED Control
- **Control Variable**: max(R_raw, G_raw, B_raw) targeting 5000-60000 range
- **Smoothing**: Exponential moving average to prevent flickering
- **IR Compensation**: Contamination detection and adjustment
- **Fallback**: Dynamic ATIME/AGAIN adjustments when LED limits reached

### Data Storage
- **NVS**: Calibration data, matrix coefficients, LED settings
- **EEPROM**: Color measurement samples with delete support
- **Google Drive**: Cloud-based Dulux color database via Apps Script

## 🌐 Google Apps Script Integration

The device integrates with a Google Apps Script for cloud-based color matching:

- **URL**: `https://script.google.com/macros/s/AKfycbwHl2P2eFVBePSZ0PbskN_YYiyDyfIwL7SbWEhqWNzZeVvhW6P-76Gb4Xy63ifSkzUi/exec`
- **Method**: GET with URL parameters
- **Database**: Dulux paint colors with ±1 RGB tolerance matching
- **Algorithm**: Euclidean distance calculation for closest matches

## 📁 Project Structure

```
esp32-color-matcher/
├── src/                          # ESP32 firmware source
│   ├── main.cpp                  # Main firmware file
│   ├── TCS3430Calibration.cpp/h  # Sensor calibration
│   ├── cie1931.cpp/h            # Color space conversion
│   ├── matrix_calibration.cpp/h  # Matrix calibration
│   └── color_matching.cpp/h     # Color matching algorithms
├── components/                   # React components
│   ├── CalibrationWizard.tsx    # Calibration interface
│   ├── LiveLedControl.tsx       # LED control panel
│   ├── SettingsPanel.tsx        # Device settings
│   └── ui/                      # UI components
├── services/                     # API services
├── scripts/                      # Deployment scripts
├── docs/                         # Documentation
├── platformio.ini               # PlatformIO configuration
├── package.json                 # Node.js dependencies
└── context.json                 # Project configuration
```

## 🚀 Deployment Scripts

### Automated Deployment
- `deploy-complete.ps1` - Full build and deployment pipeline
- `deploy-firmware.ps1` - ESP32 firmware upload only
- `deploy-simple.ps1` - Quick web interface deployment

### Manual Deployment
```bash
# Build web interface
npm run build

# Deploy to ESP32 filesystem
npm run esp32:upload

# Upload firmware
pio run --target upload
```

## 🧪 Testing & Diagnostics

### Integrated Testing
The web interface includes comprehensive testing tools in the Settings panel:
- **Test Current Settings** - Validate active sensor configuration
- **Test Valid/Invalid Values** - Parameter boundary testing
- **Test Combined Settings** - Multi-parameter validation
- **Run Full Test Suite** - Complete diagnostic sequence

### External Test Scripts
- `quick_test.ps1` - Quick connectivity and basic function test
- `test_calibration.py` - Calibration sequence validation
- `test_color_accuracy.py` - Color measurement accuracy testing
- `mock_esp32_server.py` - Offline development server

## 🔧 Configuration

### Device Settings
Key configuration parameters in `context.json`:
- **Device IP**: 192.168.0.152 (default)
- **Sensor Settings**: Optimal TCS3430 parameters
- **Calibration Targets**: Dulux Vivid White specifications
- **LED Control**: Pin assignments and brightness ranges

### Environment Variables
```bash
DEVICE_IP=192.168.0.152          # ESP32 device IP address
GEMINI_API_KEY=your_key_here     # For AI features (if used)
```

## 📖 Documentation

- `docs/DEPLOYMENT_GUIDE.md` - Detailed deployment instructions
- `docs/DEVELOPMENT_WORKFLOW.md` - Development best practices
- `CALIBRATION_ERROR_DURATION_CHANGES.md` - Calibration troubleshooting
- `TESTING_LOCATION_GUIDE.md` - Testing procedures and locations

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- **DFRobot** for the TCS3430 sensor library
- **Dulux** for paint color specifications
- **ESP32 Community** for extensive documentation and examples

## 📞 Support

For issues and questions:
1. Check the documentation in the `docs/` folder
2. Review existing issues in the repository
3. Create a new issue with detailed description and logs

---

**Device IP**: 192.168.0.152 | **Sensor**: TCS3430 | **Framework**: React + ESP32
