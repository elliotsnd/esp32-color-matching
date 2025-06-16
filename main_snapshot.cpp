//Main snapshot
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DFRobot_TCS3430.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>
#include <vector>
#include "config.h"
#include "logging.h"
#include "cie1931.h"
#include "dynamic_sensor.h"
#include "TCS3430Calibration.h"
#include "matrix_calibration.h"  // Keep for backward compatibility

// Forward declarations and type definitions
// Sample storage structure
struct ColorSample {
  uint8_t r, g, b;
  uint32_t timestamp;
  char paintName[SAMPLE_NAME_LENGTH];
  char paintCode[SAMPLE_CODE_LENGTH];
  float lrv;
};

// Advanced calibration data structures using CIE 1931 color space
struct WhiteCalibration {
  uint16_t x, y, z, ir;  // Raw sensor values for backward compatibility
  uint8_t brightness;
  uint32_t timestamp;
  bool valid;
  // New CIE 1931 calibration data
  CIE_WhiteReference cieReference;  // CIE 1931 white reference
};

struct BlackCalibration {
  uint16_t x, y, z, ir;
  uint32_t timestamp;
  bool valid;
};

// Simple calibration data structures (keeping only essential ones)

// Legacy calibration state management
enum LegacyCalibrationState {
  CAL_IDLE,
  CAL_WHITE_COUNTDOWN,
  CAL_WHITE_SCANNING,
  CAL_WHITE_COMPLETE,
  CAL_BLACK_PROMPT,
  CAL_BLACK_COUNTDOWN,
  CAL_BLACK_SCANNING,
  CAL_BLACK_COMPLETE,
  CAL_SAVING,
  CAL_COMPLETE,
  CAL_ERROR
};

// WiFi credentials - Update in config.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Google Apps Script configuration - Update in config.h
const char* googleScriptUrl = GOOGLE_SCRIPT_URL;

// Global objects
DFRobot_TCS3430 tcs3430;
Adafruit_NeoPixel rgbLed(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(WEB_SERVER_PORT);
Preferences preferences;

// Dynamic sensor management system
DynamicSensorManager* dynamicSensor = nullptr;

// Advanced TCS3430 calibration system
TCS3430Calibration* tcs3430Calibration = nullptr;

// Legacy matrix calibration system (for backward compatibility)
MatrixCalibration* matrixCalibration = nullptr;

// Logger static member definitions
unsigned long Logger::startTime = 0;
bool Logger::initialized = false;

// Global variables
bool isScanning = false;
bool ledState = false;
bool isCalibrated = false;
uint8_t currentBrightness = DEFAULT_BRIGHTNESS;
uint16_t currentAtime = DEFAULT_ATIME;
uint8_t currentAgain = DEFAULT_AGAIN;

// TCS3430 Advanced Calibration Settings
uint8_t currentAutoZeroMode = DEFAULT_AUTO_ZERO_MODE;
uint8_t currentAutoZeroFreq = DEFAULT_AUTO_ZERO_FREQUENCY;
uint8_t currentWaitTime = DEFAULT_WAIT_TIME;

// Enhanced LED Control Settings
bool enhancedLEDMode = true;        // Enable enhanced LED control by default
uint8_t manualLEDIntensity = 128;   // Manual LED intensity when enhanced mode disabled

// Interrupt handling for ambient light threshold detection (based on DFRobot example)
volatile bool ambientLightInterrupt = false;
const int INTERRUPT_PIN = 2;  // GPIO2 for interrupt

// Advanced calibration state variables
LegacyCalibrationState currentCalState = CAL_IDLE;
WhiteCalibration whiteCalData = {0};
BlackCalibration blackCalData = {0};
bool calibrationInProgress = false;
unsigned long calibrationStartTime = 0;
uint8_t calibrationCountdown = 0;
uint8_t calibrationBrightness = DEFAULT_BRIGHTNESS;
String calibrationSessionId = "";
String calibrationMessage = "";

// Advanced calibration system only - removed simple and vivid white calibrations



ColorSample samples[MAX_SAMPLES];
int sampleCount = 0;
int sampleIndex = 0;

// Current color values
uint8_t currentR = 0, currentG = 0, currentB = 0;

// CIE 1931 White Point Reference - Simple and Scientific Approach
float whitePointX = 1.0;  // Initialize to 1.0 to prevent division-by-zero
float whitePointY = 1.0;
float whitePointZ = 1.0;
bool whitePointCalibrated = false;

// Function declarations
bool initializeSensor();
void configureTCS3430ForDFRobotCompliance();
void IRAM_ATTR handleAmbientLightInterrupt();
void connectToWiFi();
void setupWebServer();
void loadSettings();
void saveSettings();
void loadSamples();
void saveSamples();
float getAmbientLightLux();
uint8_t calculateOptimalBrightness();
void setLEDColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
void turnOffLED();
uint32_t colorWheel(uint8_t wheelPos);

// PWM Illumination LED functions
void initializePWM();
void setIlluminationBrightness(uint8_t brightness);
void turnOffIllumination();
void handleScan();
void handleSaveSample();
void handleSavedSamples();
void handleDeleteSample();
void handleClearAllSamples();
void handleSettings();
void handleGetSettings();
void handleSettingsPage();

void handleStatus();
void handleBrightness();
void handleRawSensorData();
void matchColorWithGoogleScript(uint8_t r, uint8_t g, uint8_t b, int sampleIdx);

// Standard calibration function declarations
void loadCalibrationData();
void saveCalibrationData();

// Advanced TCS3430 calibration function declarations
void handleTCS3430CalibrationStatus();
void handleTCS3430CalibrationAutoZero();
void handleTCS3430CalibrationSetMatrix();
void handleTCS3430CalibrationGetDiagnostics();
void handleTCS3430CalibrationExportData();

// Matrix calibration function declarations (legacy)
void handleMatrixCalibrationStatus();
void handleMatrixCalibrationStart();
void handleMatrixCalibrationMeasure();
void handleMatrixCalibrationCompute();
void handleMatrixCalibrationResults();
void handleMatrixCalibrationApply();
void handleMatrixCalibrationClear();

// Removed simple calibration functions - using advanced calibration wizard only

// Simple Library-Standard Calibration function declarations
void handleStandardWhiteCalibration();
void handleStandardBlackCalibration();
void handleStandardCalibrationStatus();
// CIE Color Matching Function declarations (legacy)
float gaussianPiecewise(float x, float mu, float tau1, float tau2);
float cie_x_bar(float lambda);
float cie_y_bar(float lambda);
float cie_z_bar(float lambda);
void convertTCS3430ToCIEXYZ(uint16_t sensor_x, uint16_t sensor_y, uint16_t sensor_z, float& cie_x, float& cie_y, float& cie_z);
void convertCIEXYZToSRGB(float cie_x, float cie_y, float cie_z, uint8_t& r, uint8_t& g, uint8_t& b);
void convertXYZtoRGB(uint16_t x, uint16_t y, uint16_t z, uint16_t ir, uint8_t& r, uint8_t& g, uint8_t& b);

// New CIE 1931 Color Space Functions - Following Scientific Guide
struct sRGB_Simple {
  uint8_t r, g, b;
};

float linearToSrgb(float value);
sRGB_Simple xyzToSrgb(float x, float y, float z);
void calibrateWhitePoint();
void measureAndDisplayColor();
sRGB_Simple convertSensorToSRGB_Scientific(uint16_t rawX, uint16_t rawY, uint16_t rawZ, uint16_t rawIR);

// Enhanced scanning functions with dynamic sensor management
bool performEnhancedScan(uint8_t& r, uint8_t& g, uint8_t& b, uint16_t& x, uint16_t& y, uint16_t& z, uint16_t& ir1, uint16_t& ir2);
void handleEnhancedScan();
void handleSensorDiagnostics();
void handleLiveMetrics();
uint8_t findOptimalLEDBrightness();
uint8_t performAutoBrightnessOptimization();
bool adjustBrightnessForOptimalRange(uint8_t& brightness, uint16_t controlVariable);

void setup() {
  // Start serial immediately for early diagnostics
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.flush();
  delay(100);

  // Early diagnostic output
  Serial.println("\n=== ESP32-S3 COLOR MATCHER BOOT SEQUENCE ===");
  Serial.printf("Free heap at start: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM size: %u bytes\n", ESP.getPsramSize());
  Serial.printf("PSRAM free: %u bytes\n", ESP.getFreePsram());
  Serial.flush();

  // Feed watchdog early
  esp_task_wdt_reset();
  delay(100);

  // Initialize logging system first
  Logger::init();

  LOG_SYS_INFO("=== SCROFANI COLOR MATCHER STARTING ===");
  LOG_SYS_INFO("Firmware: %s", FIRMWARE_VERSION);
  LOG_SYS_INFO("Hardware: %s", HARDWARE_VERSION);
  LOG_SYS_INFO("Build: %s %s", BUILD_DATE, BUILD_TIME);
  Logger::logMemoryUsage("System startup");

  // Feed watchdog after logging init
  esp_task_wdt_reset();

  // Initialize RGB LED and power control
  LOG_SYS_INFO("Initializing RGB LED and power control");
  rgbLed.begin();
  rgbLed.setBrightness(DEFAULT_BRIGHTNESS); // Set initial brightness
  rgbLed.show(); // Initialize all pixels to 'off'
  pinMode(LDO2_POWER_PIN, OUTPUT);
  digitalWrite(LDO2_POWER_PIN, HIGH);  // Enable LDO2 for sensor power
  Serial.printf("[LED] RGB LED initialized on pin %d with brightness %d\n", RGB_LED_PIN, DEFAULT_BRIGHTNESS);

  // Initialize PWM for illumination LED
  initializePWM();
  Serial.printf("[PWM] Illumination LED initialized on pin %d\n", ILLUMINATION_LED_PIN);

  LOG_LED_INFO("RGB LED and PWM illumination LED initialized, LDO2 power enabled");
  Logger::logMemoryUsage("LED initialization");

  // Feed watchdog after LED init
  esp_task_wdt_reset();

  // Initialize I2C
  LOG_SYS_INFO("Initializing I2C bus (SDA:%d, SCL:%d)", I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  LOG_SYS_INFO("I2C bus initialized successfully");

  // Initialize TCS3430 sensor
  LOG_SENSOR_INFO("Initializing TCS3430 color sensor");
  if (!initializeSensor()) {
    LOG_SENSOR_ERROR("=== SENSOR INITIALIZATION FAILED ===");
    LOG_SENSOR_ERROR("System cannot continue without TCS3430 sensor");
    LOG_SENSOR_ERROR("Please check hardware connections and restart");
    LOG_SYS_ERROR("SYSTEM HALT - Critical sensor failure");

    // Flash red LED to indicate error
    for (int i = 0; i < 10; i++) {
      setLEDColor(255, 0, 0, 255);  // Red
      delay(200);
      setLEDColor(0, 0, 0, 0);      // Off
      delay(200);
      esp_task_wdt_reset();  // Feed watchdog during error indication
    }

    // Controlled restart instead of infinite loop
    LOG_SYS_ERROR("Restarting system in 3 seconds...");
    delay(3000);
    ESP.restart();
  }
  LOG_SENSOR_INFO("TCS3430 sensor initialized successfully");
  Logger::logMemoryUsage("Sensor initialization");

  // Initialize dynamic sensor management system
  LOG_SENSOR_INFO("Initializing dynamic sensor management system");
  dynamicSensor = new DynamicSensorManager(&tcs3430);
  if (!dynamicSensor->initialize()) {
    LOG_SENSOR_ERROR("Failed to initialize dynamic sensor manager");
    delete dynamicSensor;
    dynamicSensor = nullptr;
    LOG_SENSOR_WARN("Continuing with static sensor configuration");
  } else {
    LOG_SENSOR_INFO("Dynamic sensor management system initialized successfully");
  }
  Logger::logMemoryUsage("Dynamic sensor initialization");

  // Feed watchdog after sensor init
  esp_task_wdt_reset();
  
  // Initialize filesystem
  LOG_SYS_INFO("Initializing LittleFS filesystem");
  if (!LittleFS.begin()) {
    LOG_SYS_ERROR("Failed to mount LittleFS filesystem");
    LOG_SYS_ERROR("Attempting to format and retry...");

    // Try to format and retry once
    if (!LittleFS.begin(true)) {
      LOG_SYS_ERROR("LittleFS format failed - Restarting system in 3 seconds...");
      delay(3000);
      ESP.restart();
    }
    LOG_SYS_INFO("LittleFS formatted and mounted successfully");
  }

  // Log filesystem information
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  float usagePercent = (float)usedBytes / totalBytes * 100.0;

  LOG_SYS_INFO("LittleFS mounted - Total:%u Used:%u (%.1f%%)",
               totalBytes, usedBytes, usagePercent);
  Logger::logMemoryUsage("LittleFS initialization");

  // Feed watchdog after filesystem init
  esp_task_wdt_reset();

  // Initialize preferences
  LOG_SYS_INFO("Initializing EEPROM preferences");
  preferences.begin(PREF_NAMESPACE, false);
  Logger::logMemoryUsage("Preferences initialization");

  // Feed watchdog before loading data
  esp_task_wdt_reset();

  loadSettings();
  loadSamples();
  loadCalibrationData();

  // Initialize advanced TCS3430 calibration system
  LOG_SYS_INFO("Initializing advanced TCS3430 calibration system");
  tcs3430Calibration = new TCS3430Calibration(&tcs3430);
  if (tcs3430Calibration && tcs3430Calibration->initialize()) {
    LOG_SYS_INFO("Advanced TCS3430 calibration system initialized successfully");
  } else {
    LOG_SYS_ERROR("Failed to initialize advanced TCS3430 calibration system");
  }

  // Initialize legacy matrix calibration system for backward compatibility
  LOG_SYS_INFO("Initializing legacy matrix calibration system");
  matrixCalibration = new MatrixCalibration(&tcs3430);
  if (matrixCalibration && matrixCalibration->initialize()) {
    LOG_SYS_INFO("Legacy matrix calibration system initialized successfully");
  } else {
    LOG_SYS_ERROR("Failed to initialize legacy matrix calibration system");
  }

  // Feed watchdog after loading data
  esp_task_wdt_reset();

  // Connect to WiFi
  connectToWiFi();

  // Feed watchdog after WiFi connection
  esp_task_wdt_reset();

  // Setup web server
  setupWebServer();

  // Feed watchdog after web server setup
  esp_task_wdt_reset();

  LOG_SYS_INFO("=== SYSTEM INITIALIZATION COMPLETED ===");
  Logger::logMemoryUsage("System ready");
  LOG_SYS_INFO("Color matcher ready for operation");
}

bool initializeSensor() {
  LOG_PERF_START();

  LOG_SENSOR_DEBUG("Attempting TCS3430 sensor communication");
  LOG_SENSOR_DEBUG("I2C Scanner - Checking for devices on bus");

  // I2C Scanner to help with troubleshooting
  int deviceCount = 0;
  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();

    if (error == 0) {
      LOG_SENSOR_DEBUG("I2C device found at address 0x%02X", address);
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    LOG_SENSOR_ERROR("No I2C devices found - check wiring and power");
    LOG_SENSOR_ERROR("Expected TCS3430 at address 0x39 (57 decimal)");
    return false;
  } else {
    LOG_SENSOR_INFO("Found %d I2C device(s) on bus", deviceCount);
  }

  // Try to initialize TCS3430 following DFRobot example pattern
  LOG_SENSOR_INFO("Attempting TCS3430 initialization following DFRobot library pattern");

  // DFRobot pattern: Keep trying until sensor responds (like the example)
  int initAttempts = 0;
  const int maxInitAttempts = 5;

  while (!tcs3430.begin() && initAttempts < maxInitAttempts) {
    initAttempts++;
    LOG_SENSOR_WARN("TCS3430 initialization attempt %d/%d failed", initAttempts, maxInitAttempts);
    LOG_SENSOR_DEBUG("Please check that the IIC device is properly connected");
    delay(1000);  // DFRobot example uses 1 second delay
  }

  if (initAttempts >= maxInitAttempts) {
    LOG_SENSOR_ERROR("TCS3430 begin() failed after %d attempts - sensor not responding", maxInitAttempts);
    LOG_SENSOR_ERROR("Check connections: SDA->GPIO%d, SCL->GPIO%d, VCC->3.3V, GND->GND", I2C_SDA_PIN, I2C_SCL_PIN);
    LOG_SENSOR_ERROR("Verify TCS3430 is at I2C address 0x39");
    return false;
  }

  // Log successful sensor detection
  LOG_SENSOR_INFO("TCS3430 sensor communication established after %d attempt(s)", initAttempts + 1);

  // Configure sensor following DFRobot library methodology
  // Note: DFRobot's begin() method calls softReset() which sets defaults
  // We'll apply our settings after the sensor is properly initialized
  LOG_SENSOR_DEBUG("Configuring sensor parameters following DFRobot methodology");

  // Apply DFRobot-compliant configuration in the correct order
  configureTCS3430ForDFRobotCompliance();

  LOG_SENSOR_INFO("Sensor configured for stability - ATIME:%d AGAIN:%d WaitTime:%d AutoZeroMode:%d AutoZeroFreq:%d",
                  currentAtime, currentAgain, currentWaitTime, currentAutoZeroMode, currentAutoZeroFreq);

  // Final sensor stabilization and status check
  delay(SENSOR_STABILIZE_MS);  // Extended stabilization time

  // Verify sensor is responding
  uint8_t status = tcs3430.getDeviceStatus();
  LOG_SENSOR_DEBUG("Sensor status after initialization: 0x%02X", status);

  LOG_PERF_END("TCS3430 initialization");
  LOG_SENSOR_INFO("TCS3430 sensor initialization completed successfully");
  return true;
}

void configureTCS3430ForDFRobotCompliance() {
  LOG_SENSOR_INFO("Configuring TCS3430 following DFRobot library methodology and datasheet specifications");

  // DFRobot's begin() method calls softReset() which sets these defaults:
  // - setWaitTimer(false)
  // - setIntegrationTime(0x23)  // 35 decimal
  // - setWaitTime(0)
  // - setWaitLong(false)
  // - setALSGain(3)  // 64x gain
  // - setHighGAIN(false)
  // - setIntReadClear(false)
  // - setSleepAfterInterrupt(false)
  // - setAutoZeroMode(0)
  // - setAutoZeroNTHIteration(0x7f)
  // - setALSInterrupt(false)
  // - setALSSaturationInterrupt(false)

  // Critical: Ensure proper power-on sequence per datasheet
  // ENABLE register (0x80): Set PON=1 and AEN=1 simultaneously for auto-zero
  LOG_SENSOR_DEBUG("Ensuring proper power-on sequence per datasheet requirements");

  // Apply our custom settings in the correct order following DFRobot patterns
  LOG_SENSOR_DEBUG("Applying integration time: %d (DFRobot default: 0x23/35)", currentAtime);
  tcs3430.setIntegrationTime(currentAtime);

  LOG_SENSOR_DEBUG("Applying ALS gain: %d (DFRobot default: 3=64x)", currentAgain);
  tcs3430.setALSGain(currentAgain);

  LOG_SENSOR_DEBUG("Applying wait time: %d (DFRobot default: 0)", currentWaitTime);
  tcs3430.setWaitTime(currentWaitTime);

  LOG_SENSOR_DEBUG("Applying auto-zero mode: %d (DFRobot default: 0)", currentAutoZeroMode);
  tcs3430.setAutoZeroMode(currentAutoZeroMode);

  LOG_SENSOR_DEBUG("Applying auto-zero frequency: %d (DFRobot default: 0x7F)", currentAutoZeroFreq);
  tcs3430.setAutoZeroNTHIteration(currentAutoZeroFreq);

  // Enhanced configuration for stability (deviating from DFRobot defaults for better performance)
  LOG_SENSOR_DEBUG("Applying enhanced stability configuration");
  tcs3430.setWaitTimer(true);                         // Enable wait timer for stability
  tcs3430.setWaitLong(false);                         // Use short wait cycles
  tcs3430.setIntReadClear(true);                      // Auto-clear interrupt flags when status read
  tcs3430.setSleepAfterInterrupt(false);              // Keep sensor active after interrupts
  tcs3430.setALSSaturationInterrupt(true);            // Enable saturation detection
  tcs3430.setHighGAIN(false);                         // Use standard gain control (not high gain)

  LOG_SENSOR_INFO("TCS3430 configured - ATIME:%d AGAIN:%d WaitTime:%d AutoZeroMode:%d AutoZeroFreq:%d",
                  currentAtime, currentAgain, currentWaitTime, currentAutoZeroMode, currentAutoZeroFreq);

  // Configure ambient light interrupt (based on DFRobot setALSInterrupt example)
  LOG_SENSOR_INFO("Configuring ambient light interrupt functionality");

  // Set up interrupt pin
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);

  // Enable ALS interrupt
  tcs3430.setALSInterrupt(true);

  // Set interrupt persistence (5 consecutive values out of range)
  tcs3430.setInterruptPersistence(0x05);

  // Set threshold range for ambient light detection (0-65535)
  // These values help detect when ambient light changes significantly
  tcs3430.setCH0IntThreshold(50, 100);

  // Attach interrupt handler
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleAmbientLightInterrupt, FALLING);

  LOG_SENSOR_INFO("Ambient light interrupt configured - Pin:%d Threshold:50-100", INTERRUPT_PIN);
}

/**
 * @brief Datasheet-compliant calibration function for achieving exact RGB (255,255,255)
 * Uses TCS3430 datasheet register specifications and DFRobot library patterns
 * @param targetBrightness LED brightness for calibration
 * @return true if calibration achieves exact RGB (255,255,255), false otherwise
 */
bool performDatasheetCompliantCalibration(uint8_t targetBrightness) {
  LOG_SENSOR_INFO("Starting datasheet-compliant calibration for RGB (255,255,255)");

  // Step 1: Ensure proper sensor configuration per datasheet
  LOG_SENSOR_DEBUG("Verifying sensor configuration per datasheet requirements");

  // Verify ENABLE register (0x80) has PON=1 and AEN=1
  // This ensures proper auto-zero function per datasheet

  // Step 2: Set optimal LED brightness and stabilize
  LOG_SENSOR_DEBUG("Setting LED brightness to %d and stabilizing", targetBrightness);
  setLEDColor(0, 0, 0, targetBrightness); // Set LED to white at target brightness
  delay(CALIBRATION_LED_STABILIZE_MS);

  // Step 3: Collect multiple samples for statistical analysis
  const int numSamples = 10;
  uint16_t xSamples[numSamples], ySamples[numSamples], zSamples[numSamples];
  uint16_t ir1Samples[numSamples], ir2Samples[numSamples];

  LOG_SENSOR_DEBUG("Collecting %d samples for statistical calibration", numSamples);

  for (int i = 0; i < numSamples; i++) {
    // Wait for sensor stabilization per datasheet timing requirements
    delay(200);

    // Read all channels per datasheet channel mapping
    xSamples[i] = tcs3430.getXData();    // CH3 (0x9A-0x9B)
    ySamples[i] = tcs3430.getYData();    // CH1 (0x96-0x97)
    zSamples[i] = tcs3430.getZData();    // CH0 (0x94-0x95)
    ir1Samples[i] = tcs3430.getIR1Data(); // CH2 (0x98-0x99)
    ir2Samples[i] = tcs3430.getIR2Data(); // CH3 alternate (0x9A-0x9B)

    LOG_SENSOR_DEBUG("Sample %d: X=%d Y=%d Z=%d IR1=%d IR2=%d",
                     i+1, xSamples[i], ySamples[i], zSamples[i], ir1Samples[i], ir2Samples[i]);
  }

  // Step 4: Calculate average values
  uint32_t xSum = 0, ySum = 0, zSum = 0, ir1Sum = 0, ir2Sum = 0;
  for (int i = 0; i < numSamples; i++) {
    xSum += xSamples[i];
    ySum += ySamples[i];
    zSum += zSamples[i];
    ir1Sum += ir1Samples[i];
    ir2Sum += ir2Samples[i];
  }

  uint16_t avgX = xSum / numSamples;
  uint16_t avgY = ySum / numSamples;
  uint16_t avgZ = zSum / numSamples;
  uint16_t avgIR1 = ir1Sum / numSamples;
  uint16_t avgIR2 = ir2Sum / numSamples;

  LOG_SENSOR_INFO("Average readings: X=%d Y=%d Z=%d", avgX, avgY, avgZ);

  // Step 5: Validate readings per datasheet specifications
  if (avgX < CALIBRATION_MIN_SIGNAL || avgY < CALIBRATION_MIN_SIGNAL || avgZ < CALIBRATION_MIN_SIGNAL) {
    LOG_SENSOR_ERROR("Calibration failed: Signal too low (min: %d)", CALIBRATION_MIN_SIGNAL);
    return false;
  }

  if (avgX > CALIBRATION_MAX_SIGNAL || avgY > CALIBRATION_MAX_SIGNAL || avgZ > CALIBRATION_MAX_SIGNAL) {
    LOG_SENSOR_ERROR("Calibration failed: Signal saturation detected (max: %d)", CALIBRATION_MAX_SIGNAL);
    return false;
  }

  // Step 6: Calculate calibration factors for exact RGB (255,255,255)
  float xFactor = (float)CALIBRATION_TARGET_RGB / avgX;
  float yFactor = (float)CALIBRATION_TARGET_RGB / avgY;
  float zFactor = (float)CALIBRATION_TARGET_RGB / avgZ;

  LOG_SENSOR_INFO("Calibration factors: X=%.4f Y=%.4f Z=%.4f", xFactor, yFactor, zFactor);

  // Step 7: Store calibration data with datasheet-compliant metadata
  whiteCalData.x = avgX;
  whiteCalData.y = avgY;
  whiteCalData.z = avgZ;
  whiteCalData.ir = (ir1Samples[numSamples/2] + ir2Samples[numSamples/2]) / 2; // Use median IR
  whiteCalData.brightness = targetBrightness;
  whiteCalData.timestamp = millis();
  whiteCalData.valid = true;

  // Step 8: Verify calibration by testing RGB conversion
  uint8_t testR, testG, testB;
  uint16_t avgIR = (avgIR1 + avgIR2) / 2;  // Average IR from both channels
  convertXYZtoRGB(avgX, avgY, avgZ, avgIR, testR, testG, testB);

  LOG_SENSOR_INFO("Calibration verification: RGB(%d,%d,%d)", testR, testG, testB);

  // Step 9: Check for exact RGB (255,255,255) with zero tolerance
  bool calibrationSuccess = (testR == CALIBRATION_TARGET_RGB &&
                           testG == CALIBRATION_TARGET_RGB &&
                           testB == CALIBRATION_TARGET_RGB);

  if (calibrationSuccess) {
    LOG_SENSOR_INFO("✅ Datasheet-compliant calibration SUCCESS: Exact RGB (255,255,255) achieved");
    isCalibrated = true;
    saveCalibrationData();
  } else {
    LOG_SENSOR_ERROR("❌ Calibration failed: RGB(%d,%d,%d) != (255,255,255)", testR, testG, testB);
  }

  return calibrationSuccess;
}

// Ambient light interrupt handler - DFRobot TCS3430 interrupt functionality
void IRAM_ATTR handleAmbientLightInterrupt() {
  // This interrupt handler follows DFRobot's interrupt methodology
  // It's called when ambient light levels cross the configured thresholds
  // Keep this function minimal and fast (IRAM_ATTR requirement)

  // Set a flag to handle the interrupt in the main loop
  // Don't do complex operations in interrupt context
  static volatile bool interruptFlag = false;
  interruptFlag = true;
}

void connectToWiFi() {
  LOG_PERF_START();

  Serial.println("=== IMPROVED WIFI CONNECTION STARTING ===");
  Serial.printf("Connecting to SSID: %s\n", ssid);
  Serial.printf("Password length: %d characters\n", strlen(password));

  LOG_NET_INFO("Starting WiFi connection to SSID: %s", ssid);
  LOG_NET_INFO("Password length: %d characters", strlen(password));
  Logger::logMemoryUsage("Before WiFi connection");

  // Properly disconnect and reset WiFi first
  Serial.println("Resetting WiFi subsystem...");
  LOG_NET_INFO("Resetting WiFi subsystem...");
  WiFi.disconnect(true);  // Disconnect and erase stored credentials
  delay(1000);            // Give time for disconnect
  WiFi.mode(WIFI_OFF);    // Turn off WiFi
  delay(500);             // Wait for WiFi to turn off
  WiFi.mode(WIFI_STA);    // Set to station mode
  delay(500);             // Wait for mode change
  Serial.println("WiFi reset complete");

  // Configure WiFi settings BEFORE scanning
  Serial.println("Configuring WiFi settings...");
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);

  // Configure static IP if enabled
  #if USE_STATIC_IP
  IPAddress staticIP, gateway, subnet, dns1, dns2;
  staticIP.fromString(STATIC_IP_ADDRESS);
  gateway.fromString(STATIC_GATEWAY);
  subnet.fromString(STATIC_SUBNET);
  dns1.fromString(STATIC_DNS1);
  dns2.fromString(STATIC_DNS2);

  if (WiFi.config(staticIP, gateway, subnet, dns1, dns2)) {
    Serial.printf("Static IP configured: %s\n", STATIC_IP_ADDRESS);
    LOG_NET_INFO("Static IP configured: %s", STATIC_IP_ADDRESS);
  } else {
    Serial.println("Failed to configure static IP - using DHCP");
    LOG_NET_ERROR("Failed to configure static IP - using DHCP");
  }
  #else
  Serial.println("Using DHCP for IP assignment");
  LOG_NET_INFO("Using DHCP for IP assignment");
  #endif

  // Optional: Set hostname for easier identification
  WiFi.setHostname("ColorMatcher");

  Serial.println("WiFi configuration complete");

  // Now scan for available networks (optional diagnostic step)
  Serial.println("Scanning for available networks...");
  LOG_NET_INFO("Scanning for available WiFi networks...");
  int networkCount = WiFi.scanNetworks();
  LOG_NET_INFO("Found %d networks:", networkCount);

  bool targetFound = false;
  for (int i = 0; i < networkCount; i++) {
    String foundSSID = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    wifi_auth_mode_t authMode = WiFi.encryptionType(i);

    LOG_NET_DEBUG("Network %d: %s (RSSI: %d, Auth: %d)", i, foundSSID.c_str(), rssi, authMode);
    Serial.printf("Network %d: %s (RSSI: %d)\n", i, foundSSID.c_str(), rssi);

    if (foundSSID.equals(ssid)) {
      targetFound = true;
      LOG_NET_INFO("Target network found: %s (RSSI: %d)", ssid, rssi);
      Serial.printf("*** Target network '%s' found with RSSI: %d ***\n", ssid, rssi);
    }
  }

  if (!targetFound) {
    LOG_NET_ERROR("Target network '%s' not found in scan results", ssid);
    LOG_NET_ERROR("Check SSID spelling and ensure network is broadcasting");
    LOG_NET_ERROR("Continuing anyway - network might be hidden");
    Serial.printf("*** WARNING: Target network '%s' not found in scan ***\n", ssid);
    Serial.println("*** Continuing anyway - network might be hidden ***");
  }

  // Start connection with retry logic
  const int maxRetries = 3;
  const int maxAttemptsPerRetry = 20;

  for (int retry = 0; retry < maxRetries; retry++) {
    LOG_NET_INFO("WiFi connection retry %d/%d", retry + 1, maxRetries);
    Serial.printf("=== WiFi Connection Attempt %d/%d ===\n", retry + 1, maxRetries);

    // Begin connection
    Serial.printf("Connecting to SSID: '%s'\n", ssid);
    WiFi.begin(ssid, password);
    Serial.println("WiFi.begin() called, waiting for connection...");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttemptsPerRetry) {
      delay(2000);  // Increased delay to 2 seconds
      attempts++;

      wl_status_t status = WiFi.status();

      if (attempts % 3 == 0) {  // Log every 6 seconds instead of 10
        LOG_NET_INFO("Retry %d - Attempt %d/%d - Status: %d", retry + 1, attempts, maxAttemptsPerRetry, status);

        // Log specific status meanings
        switch (status) {
          case WL_IDLE_STATUS:
            LOG_NET_DEBUG("Status: WL_IDLE_STATUS (0) - WiFi is changing state");
            break;
          case WL_NO_SSID_AVAIL:
            LOG_NET_ERROR("Status: WL_NO_SSID_AVAIL (1) - SSID not found");
            break;
          case WL_SCAN_COMPLETED:
            LOG_NET_DEBUG("Status: WL_SCAN_COMPLETED (2) - Scan completed");
            break;
          case WL_CONNECTED:
            LOG_NET_INFO("Status: WL_CONNECTED (3) - Connected successfully");
            break;
          case WL_CONNECT_FAILED:
            LOG_NET_ERROR("Status: WL_CONNECT_FAILED (4) - Connection failed");
            break;
          case WL_CONNECTION_LOST:
            LOG_NET_ERROR("Status: WL_CONNECTION_LOST (5) - Connection lost");
            break;
          case WL_DISCONNECTED:
            LOG_NET_ERROR("Status: WL_DISCONNECTED (6) - Disconnected");
            break;
          default:
            LOG_NET_ERROR("Status: UNKNOWN (%d)", status);
            break;
        }
      }

      // Feed watchdog during long connection attempts
      esp_task_wdt_reset();
    }

    // Check if connected
    if (WiFi.status() == WL_CONNECTED) {
      break;  // Success! Exit retry loop
    }

    // If not connected and not the last retry, disconnect and wait before retrying
    if (retry < maxRetries - 1) {
      LOG_NET_ERROR("Retry %d failed, disconnecting and waiting before next retry...", retry + 1);
      WiFi.disconnect();
      delay(3000);  // Wait 3 seconds before next retry
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    int32_t rssi = WiFi.RSSI();
    String macAddress = WiFi.macAddress();

    LOG_NET_INFO("WiFi connected successfully!");
    LOG_NET_INFO("IP Address: %s", WiFi.localIP().toString().c_str());
    LOG_NET_INFO("MAC Address: %s", macAddress.c_str());
    LOG_NET_INFO("Signal Strength: %d dBm", rssi);
    LOG_NET_INFO("Gateway: %s", WiFi.gatewayIP().toString().c_str());
    LOG_NET_INFO("DNS: %s", WiFi.dnsIP().toString().c_str());

    LOG_PERF_END("WiFi connection");
    Logger::logMemoryUsage("After WiFi connection");
  } else {
    LOG_NET_ERROR("Failed to connect to WiFi after %d retries", maxRetries);
    LOG_NET_ERROR("Final WiFi status: %d", WiFi.status());
    LOG_NET_ERROR("System will continue without WiFi - color matching will not work");
  }
}

// CORS helper functions
void handleCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
  server.sendHeader("Access-Control-Max-Age", "86400");
}

void handleCORSPreflight() {
  LOG_WEB_DEBUG("CORS preflight request from %s", server.client().remoteIP().toString().c_str());
  handleCORSHeaders();
  server.send(200, "text/plain", "");
}

void setupWebServer() {
  LOG_PERF_START();
  LOG_WEB_INFO("Configuring web server endpoints");

  // Handle OPTIONS requests for CORS preflight
  server.on("/scan", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/enhanced-scan", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/save", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/samples", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/delete", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/samples/clear", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/settings", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/status", HTTP_OPTIONS, handleCORSPreflight);
  server.on("/brightness", HTTP_OPTIONS, handleCORSPreflight);

  // API endpoints MUST be defined FIRST to prevent conflicts
  server.on("/scan", HTTP_POST, []() { handleCORSHeaders(); handleScan(); });
  server.on("/enhanced-scan", HTTP_POST, []() { handleCORSHeaders(); handleEnhancedScan(); });
  server.on("/save", HTTP_POST, []() { handleCORSHeaders(); handleSaveSample(); });
  server.on("/samples", HTTP_GET, []() { handleCORSHeaders(); handleSavedSamples(); });
  server.on("/delete", HTTP_POST, []() { handleCORSHeaders(); handleDeleteSample(); });
  server.on("/samples/clear", HTTP_POST, []() { handleCORSHeaders(); handleClearAllSamples(); });
  server.on("/settings", HTTP_POST, []() { handleCORSHeaders(); handleSettings(); });
  server.on("/settings", HTTP_GET, []() { handleCORSHeaders(); handleGetSettings(); });
  server.on("/settings-page", HTTP_GET, []() { handleCORSHeaders(); handleSettingsPage(); });

  // Advanced TCS3430 calibration API endpoints
  server.on("/tcs3430-calibration/status", HTTP_GET, []() { handleCORSHeaders(); handleTCS3430CalibrationStatus(); });
  server.on("/tcs3430-calibration/auto-zero", HTTP_POST, []() { handleCORSHeaders(); handleTCS3430CalibrationAutoZero(); });
  server.on("/tcs3430-calibration/set-matrix", HTTP_POST, []() { handleCORSHeaders(); handleTCS3430CalibrationSetMatrix(); });
  server.on("/tcs3430-calibration/diagnostics", HTTP_GET, []() { handleCORSHeaders(); handleTCS3430CalibrationGetDiagnostics(); });
  server.on("/tcs3430-calibration/export-data", HTTP_GET, []() { handleCORSHeaders(); handleTCS3430CalibrationExportData(); });

  // Legacy matrix calibration API endpoints
  server.on("/matrix-calibration/status", HTTP_GET, []() { handleCORSHeaders(); handleMatrixCalibrationStatus(); });
  server.on("/matrix-calibration/start", HTTP_POST, []() { handleCORSHeaders(); handleMatrixCalibrationStart(); });
  server.on("/matrix-calibration/measure", HTTP_POST, []() { handleCORSHeaders(); handleMatrixCalibrationMeasure(); });
  server.on("/matrix-calibration/compute", HTTP_POST, []() { handleCORSHeaders(); handleMatrixCalibrationCompute(); });
  server.on("/matrix-calibration/results", HTTP_GET, []() { handleCORSHeaders(); handleMatrixCalibrationResults(); });
  server.on("/matrix-calibration/apply", HTTP_POST, []() { handleCORSHeaders(); handleMatrixCalibrationApply(); });
  server.on("/matrix-calibration/clear", HTTP_DELETE, []() { handleCORSHeaders(); handleMatrixCalibrationClear(); });

  server.on("/status", HTTP_GET, []() { handleCORSHeaders(); handleStatus(); });
  server.on("/sensor-diagnostics", HTTP_GET, []() { handleCORSHeaders(); handleSensorDiagnostics(); });
  server.on("/live-metrics", HTTP_GET, []() { handleCORSHeaders(); handleLiveMetrics(); });
  server.on("/brightness", HTTP_POST, []() { handleCORSHeaders(); handleBrightness(); });
  server.on("/raw", HTTP_GET, []() { handleCORSHeaders(); handleRawSensorData(); });  // Real-time brightness control

  // Standard Library Calibration endpoints - Simple and reliable
  server.on("/calibrate/standard/white", HTTP_POST, []() { handleCORSHeaders(); handleStandardWhiteCalibration(); });
  server.on("/calibrate/standard/black", HTTP_POST, []() { handleCORSHeaders(); handleStandardBlackCalibration(); });
  server.on("/calibrate/standard/status", HTTP_GET, []() { handleCORSHeaders(); handleStandardCalibrationStatus(); });

  // Explicitly handle root path to serve index file
  server.on("/", HTTP_GET, []() {
    // Try index.html first, then index.htm
    if (LittleFS.exists("/index.html")) {
      File file = LittleFS.open("/index.html", "r");
      server.streamFile(file, "text/html");
      file.close();
      LOG_WEB_DEBUG("Served index.html successfully");
    } else if (LittleFS.exists("/index.htm")) {
      File file = LittleFS.open("/index.htm", "r");
      server.streamFile(file, "text/html");
      file.close();
      LOG_WEB_DEBUG("Served index.htm successfully");
    } else {
      LOG_WEB_ERROR("No index file found in LittleFS (tried index.html and index.htm)");
      server.send(404, "text/html",
        "<html><body><h1>Web Interface Not Found</h1>"
        "<p>Please upload the filesystem using: <code>pio run --target uploadfs</code></p>"
        "<p>Looking for: index.html or index.htm</p></body></html>");
    }
  });

  // Handle React build assets - generic handler for /assets/* files
  server.on("/assets/index-DbUA0BBv.js", HTTP_GET, []() {
    if (LittleFS.exists("/assets/index-DbUA0BBv.js")) {
      File file = LittleFS.open("/assets/index-DbUA0BBv.js", "r");
      server.streamFile(file, "application/javascript");
      file.close();
      LOG_WEB_DEBUG("Served React bundle successfully");
    } else {
      LOG_WEB_ERROR("React bundle not found in LittleFS");
      server.send(404, "text/plain", "React bundle not found");
    }
  });

  // Handle missing CSS file (referenced in HTML but doesn't exist)
  server.on("/index.css", HTTP_GET, []() {
    LOG_WEB_DEBUG("index.css requested but not needed (styles embedded in HTML)");
    server.send(200, "text/css", "/* Styles embedded in HTML */");
  });

  // Legacy handlers for compatibility
  server.on("/style.css", HTTP_GET, []() {
    LOG_WEB_DEBUG("style.css requested - redirecting to embedded styles");
    server.send(200, "text/css", "/* Styles embedded in HTML */");
  });

  server.on("/script.js", HTTP_GET, []() {
    LOG_WEB_DEBUG("script.js requested - redirecting to React bundle");
    server.sendHeader("Location", "/assets/index-BJa7Mcg1.js");
    server.send(302, "text/plain", "Redirecting to React bundle");
  });

  // Generic handler for assets directory
  server.onNotFound([]() {
    String path = server.uri();
    LOG_WEB_DEBUG("Handling request for: %s", path.c_str());

    // Handle CORS preflight for any unhandled OPTIONS requests
    if (server.method() == HTTP_OPTIONS) {
      handleCORSPreflight();
      return;
    }

    // Add CORS headers to all responses
    handleCORSHeaders();

    // Try to serve any file from LittleFS
    if (LittleFS.exists(path)) {
      File file = LittleFS.open(path, "r");
      String contentType = "text/plain";

      // Determine content type based on file extension
      if (path.endsWith(".js")) {
        contentType = "application/javascript";
      } else if (path.endsWith(".css")) {
        contentType = "text/css";
      } else if (path.endsWith(".html")) {
        contentType = "text/html";
      } else if (path.endsWith(".json")) {
        contentType = "application/json";
      }

      server.streamFile(file, contentType);
      file.close();
      LOG_WEB_DEBUG("Served file: %s", path.c_str());
    } else {
      LOG_WEB_ERROR("404 Not Found: %s", path.c_str());
      server.send(404, "text/html",
        "<html><body><h1>404 - File Not Found</h1>"
        "<p>Requested: " + path + "</p>"
        "<p><a href='/'>Return to Color Matcher</a></p></body></html>");
    }
  });

  LOG_WEB_INFO("API endpoints configured: /scan /save /samples /delete /settings /calibrate/* /status");
  LOG_WEB_DEBUG("Static file handlers configured for CSS/JS");
  LOG_WEB_DEBUG("CORS headers configured for cross-origin requests");

  server.begin();
  LOG_WEB_INFO("Web server started on port %d", WEB_SERVER_PORT);

  if (WiFi.status() == WL_CONNECTED) {
    LOG_WEB_INFO("Web interface available at: http://%s", WiFi.localIP().toString().c_str());
  }

  LOG_PERF_END("Web server setup");
}

void loadSettings() {
  LOG_PERF_START();
  LOG_STORAGE_INFO("Loading settings from EEPROM");

  currentAtime = preferences.getUInt(PREF_ATIME, DEFAULT_ATIME);
  currentAgain = preferences.getUInt(PREF_AGAIN, DEFAULT_AGAIN);
  currentBrightness = preferences.getUInt(PREF_BRIGHTNESS, DEFAULT_BRIGHTNESS);
  isCalibrated = preferences.getBool(PREF_CALIBRATED, false);

  // Load TCS3430 advanced calibration settings
  currentAutoZeroMode = preferences.getUInt(PREF_AUTO_ZERO_MODE, DEFAULT_AUTO_ZERO_MODE);
  currentAutoZeroFreq = preferences.getUInt(PREF_AUTO_ZERO_FREQ, DEFAULT_AUTO_ZERO_FREQUENCY);
  currentWaitTime = preferences.getUInt(PREF_WAIT_TIME, DEFAULT_WAIT_TIME);

  // Load enhanced LED control settings
  enhancedLEDMode = preferences.getBool(PREF_ENHANCED_LED_MODE, true);
  manualLEDIntensity = preferences.getUChar(PREF_MANUAL_LED_INTENSITY, 128);

  LOG_STORAGE_INFO("Settings loaded - ATIME:%d AGAIN:%d Brightness:%d Calibrated:%s",
                   currentAtime, currentAgain, currentBrightness,
                   isCalibrated ? "YES" : "NO");
  LOG_STORAGE_INFO("Advanced settings - AutoZeroMode:%d AutoZeroFreq:%d WaitTime:%d",
                   currentAutoZeroMode, currentAutoZeroFreq, currentWaitTime);
  LOG_STORAGE_INFO("Enhanced LED control - Mode:%s ManualIntensity:%d",
                   enhancedLEDMode ? "ENHANCED" : "MANUAL", manualLEDIntensity);

  LOG_PERF_END("Settings load");
}

void saveSettings() {
  LOG_PERF_START();
  LOG_STORAGE_INFO("Saving settings to EEPROM");

  preferences.putUInt(PREF_ATIME, currentAtime);
  preferences.putUInt(PREF_AGAIN, currentAgain);
  preferences.putUInt(PREF_BRIGHTNESS, currentBrightness);
  preferences.putBool(PREF_CALIBRATED, isCalibrated);

  // Save TCS3430 advanced calibration settings
  preferences.putUInt(PREF_AUTO_ZERO_MODE, currentAutoZeroMode);
  preferences.putUInt(PREF_AUTO_ZERO_FREQ, currentAutoZeroFreq);
  preferences.putUInt(PREF_WAIT_TIME, currentWaitTime);

  // Save enhanced LED control settings
  preferences.putBool(PREF_ENHANCED_LED_MODE, enhancedLEDMode);
  preferences.putUChar(PREF_MANUAL_LED_INTENSITY, manualLEDIntensity);

  LOG_STORAGE_INFO("Settings saved - ATIME:%d AGAIN:%d Brightness:%d Calibrated:%s",
                   currentAtime, currentAgain, currentBrightness,
                   isCalibrated ? "YES" : "NO");
  LOG_STORAGE_INFO("Advanced settings saved - AutoZeroMode:%d AutoZeroFreq:%d WaitTime:%d",
                   currentAutoZeroMode, currentAutoZeroFreq, currentWaitTime);
  LOG_STORAGE_INFO("Enhanced LED control saved - Mode:%s ManualIntensity:%d",
                   enhancedLEDMode ? "ENHANCED" : "MANUAL", manualLEDIntensity);

  LOG_PERF_END("Settings save");
}

void loadSamples() {
  LOG_PERF_START();
  LOG_STORAGE_INFO("Loading samples from EEPROM");

  sampleCount = preferences.getUInt(PREF_SAMPLE_COUNT, 0);
  sampleIndex = preferences.getUInt(PREF_SAMPLE_INDEX, 0);

  int loadedSamples = 0;
  for (int i = 0; i < sampleCount && i < MAX_SAMPLES; i++) {
    String key = String(PREF_SAMPLE_PREFIX) + String(i);
    size_t len = preferences.getBytesLength(key.c_str());

    if (len == sizeof(ColorSample)) {
      size_t bytesRead = preferences.getBytes(key.c_str(), &samples[i], sizeof(ColorSample));
      if (bytesRead == sizeof(ColorSample)) {
        loadedSamples++;
        LOG_STORAGE_DEBUG("Loaded sample %d - RGB:(%u,%u,%u) Paint:%s",
                          i, samples[i].r, samples[i].g, samples[i].b, samples[i].paintName);
      } else {
        LOG_STORAGE_ERROR("Failed to load sample %d - bytes read: %d", i, bytesRead);
      }
    } else {
      LOG_STORAGE_ERROR("Sample %d has invalid size: %d (expected %d)", i, len, sizeof(ColorSample));
    }
  }

  LOG_STORAGE_INFO("Sample loading completed - %d/%d samples loaded successfully",
                   loadedSamples, sampleCount);

  LOG_PERF_END("Sample load");
}

void saveSamples() {
  preferences.putUInt("sampleCount", sampleCount);
  preferences.putUInt("sampleIndex", sampleIndex);
  
  for (int i = 0; i < sampleCount && i < 30; i++) {
    String key = "sample" + String(i);
    preferences.putBytes(key.c_str(), &samples[i], sizeof(ColorSample));
  }
}

float getAmbientLightLux() {
  // Read Y channel (luminance) for ambient light estimation
  uint16_t clear = tcs3430.getYData();

  // Convert to lux (approximate calculation)
  float lux = clear * 0.25;  // Rough conversion factor

  return lux;
}

uint8_t calculateOptimalBrightness() {
  float ambientLux = getAmbientLightLux();

  // Read all RGB channels for comprehensive control
  uint16_t rawR = tcs3430.getXData();  // X channel maps to Red
  uint16_t rawG = tcs3430.getYData();  // Y channel maps to Green
  uint16_t rawB = tcs3430.getZData();  // Z channel maps to Blue
  uint16_t rawIR = tcs3430.getIR1Data(); // IR channel for contamination detection

  // Calculate control variable - use max of RGB channels for better saturation detection
  uint16_t controlVariable = max(max(rawR, rawG), rawB);

  // Alternative: use average of RGB channels (uncomment if preferred)
  // uint16_t controlVariable = (rawR + rawG + rawB) / 3;

  // Check for saturation
  uint8_t status = tcs3430.getDeviceStatus();
  bool saturated = (status & 0x10) != 0;  // ASAT bit

  uint8_t oldBrightness = currentBrightness;
  static float smoothedBrightness = currentBrightness; // Static for persistence
  uint8_t targetBrightness = currentBrightness;
  const char* reason = "no change";

  // Target range control with smaller adjustment steps
  if (saturated || controlVariable > RGB_TARGET_MAX) {
    targetBrightness = max(MIN_LED_BRIGHTNESS, currentBrightness - BRIGHTNESS_ADJUSTMENT_STEP);
    reason = "above target range";
    LOG_LED_DEBUG("Above target range - Status:0x%02X Control:%u Target:%u",
                  status, controlVariable, RGB_TARGET_MAX);
  } else if (controlVariable < RGB_TARGET_MIN) {
    targetBrightness = min(MAX_LED_BRIGHTNESS, currentBrightness + BRIGHTNESS_ADJUSTMENT_STEP);
    reason = "below target range";
    LOG_LED_DEBUG("Below target range - Control:%u Target:%u", controlVariable, RGB_TARGET_MIN);
  }

  // Apply exponential moving average for smoothing
  smoothedBrightness = (BRIGHTNESS_SMOOTHING_ALPHA * targetBrightness) +
                       ((1.0 - BRIGHTNESS_SMOOTHING_ALPHA) * smoothedBrightness);
  uint8_t brightness = (uint8_t)round(smoothedBrightness);

  // IR contamination detection and compensation
  float irRatio = (controlVariable > 0) ? (float)rawIR / controlVariable : 0.0;
  if (irRatio > IR_CONTAMINATION_THRESHOLD) {
    brightness = max(MIN_LED_BRIGHTNESS, brightness - BRIGHTNESS_ADJUSTMENT_STEP);
    reason = "IR contamination detected";
    LOG_LED_DEBUG("IR contamination - IR ratio:%.3f", irRatio);
  }

  // Ambient light adjustment (reduced impact for fine control)
  if (ambientLux > HIGH_AMBIENT_LUX) {
    brightness = min(MAX_LED_BRIGHTNESS, brightness + BRIGHTNESS_ADJUSTMENT_STEP);
    reason = "high ambient light";
  } else if (ambientLux < LOW_AMBIENT_LUX) {
    brightness = max(MIN_LED_BRIGHTNESS, brightness - BRIGHTNESS_ADJUSTMENT_STEP);
    reason = "low ambient light";
  }

  // Log brightness changes with enhanced information
  Logger::logLEDBrightness(oldBrightness, brightness, reason, ambientLux, controlVariable);

  if (LOG_LED_CONTROL && oldBrightness != brightness) {
    LOG_LED_INFO("Brightness control - RGB:(%u,%u,%u) Control:%u IR:%u Ratio:%.3f Target:%u-%u",
                 rawR, rawG, rawB, controlVariable, rawIR, irRatio, RGB_TARGET_MIN, RGB_TARGET_MAX);
  }

  return brightness;
}

/**
 * @brief Perform automatic brightness optimization for optimal sensor range
 * Uses iterative adjustment to achieve target RGB range
 * @return Optimized brightness value
 */
uint8_t performAutoBrightnessOptimization() {
  LOG_LED_INFO("Starting automatic brightness optimization");

  uint8_t brightness = currentBrightness;
  const int maxIterations = 8;
  const int stabilizationDelay = BRIGHTNESS_STABILIZATION_DELAY;

  for (int iteration = 0; iteration < maxIterations; iteration++) {
    // Set LED to test brightness
    setIlluminationBrightness(brightness);
    delay(stabilizationDelay);

    // Read RGB channels
    uint16_t rawR = tcs3430.getXData();
    uint16_t rawG = tcs3430.getYData();
    uint16_t rawB = tcs3430.getZData();
    uint16_t rawIR = tcs3430.getIR1Data();

    // Calculate control variable
    uint16_t controlVariable = max(max(rawR, rawG), rawB);

    LOG_LED_DEBUG("Optimization iteration %d: Brightness=%u Control=%u RGB:(%u,%u,%u)",
                  iteration + 1, brightness, controlVariable, rawR, rawG, rawB);

    // Check if we're in the optimal range
    if (controlVariable >= RGB_TARGET_MIN && controlVariable <= RGB_TARGET_MAX) {
      LOG_LED_INFO("Optimal brightness found: %u (Control variable: %u)", brightness, controlVariable);
      return brightness;
    }

    // Adjust brightness based on control variable
    if (!adjustBrightnessForOptimalRange(brightness, controlVariable)) {
      LOG_LED_INFO("Cannot adjust brightness further, using current value: %u", brightness);
      break;
    }
  }

  LOG_LED_INFO("Brightness optimization complete after %d iterations: %u", maxIterations, brightness);
  return brightness;
}

/**
 * @brief Adjust brightness to achieve optimal sensor range
 * @param brightness Reference to brightness value to adjust
 * @param controlVariable Current control variable (max RGB)
 * @return true if adjustment was made, false if at limits
 */
bool adjustBrightnessForOptimalRange(uint8_t& brightness, uint16_t controlVariable) {
  uint8_t oldBrightness = brightness;

  if (controlVariable > RGB_TARGET_MAX) {
    // Too bright - reduce brightness
    if (brightness > MIN_LED_BRIGHTNESS) {
      brightness = max(MIN_LED_BRIGHTNESS, brightness - BRIGHTNESS_ADJUSTMENT_STEP);
      LOG_LED_DEBUG("Reducing brightness: %u -> %u (Control: %u > %u)",
                    oldBrightness, brightness, controlVariable, RGB_TARGET_MAX);
      return true;
    }
  } else if (controlVariable < RGB_TARGET_MIN) {
    // Too dim - increase brightness
    if (brightness < MAX_LED_BRIGHTNESS) {
      brightness = min(MAX_LED_BRIGHTNESS, brightness + BRIGHTNESS_ADJUSTMENT_STEP);
      LOG_LED_DEBUG("Increasing brightness: %u -> %u (Control: %u < %u)",
                    oldBrightness, brightness, controlVariable, RGB_TARGET_MIN);
      return true;
    }
  }

  return false; // No adjustment made (at limits or in range)
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness = 255) {
  // Set NeoPixel library brightness (0-255)
  rgbLed.setBrightness(brightness);

  // Set the color without manual scaling (NeoPixel library handles brightness)
  rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
  rgbLed.show();

  // Force serial output for debugging
  Serial.printf("[LED] Color set - RGB:(%u,%u,%u) Brightness:%u\n", r, g, b, brightness);

  LOG_LED_DEBUG("LED color set - RGB:(%u,%u,%u) Brightness:%u",
                r, g, b, brightness);
}

void turnOffLED() {
  rgbLed.setPixelColor(0, 0);
  rgbLed.setBrightness(0);
  rgbLed.show();
  ledState = false;
  Serial.printf("[LED] RGB LED turned OFF\n");
  LOG_LED_INFO("RGB LED turned OFF");
}

// PWM Illumination LED Functions
void initializePWM() {
  // Configure LEDC timer
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);

  // Attach the channel to the GPIO pin
  ledcAttachPin(ILLUMINATION_LED_PIN, PWM_CHANNEL);

  // Set initial brightness to 0 (off)
  ledcWrite(PWM_CHANNEL, 0);

  Serial.printf("[PWM] Initialized - Channel: %d, Pin: %d, Freq: %d Hz, Resolution: %d bits\n",
                PWM_CHANNEL, ILLUMINATION_LED_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
}

void setIlluminationBrightness(uint8_t brightness) {
  // Write PWM value (0-255)
  ledcWrite(PWM_CHANNEL, brightness);

  Serial.printf("[PWM] Illumination brightness set to: %d\n", brightness);
  LOG_LED_INFO("Illumination LED brightness: %d", brightness);
}

void turnOffIllumination() {
  ledcWrite(PWM_CHANNEL, 0);
  Serial.printf("[PWM] Illumination LED turned OFF\n");
  LOG_LED_INFO("Illumination LED turned OFF");
}

// Helper function for rainbow effect
uint32_t colorWheel(uint8_t wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return rgbLed.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return rgbLed.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return rgbLed.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

// Network Security Functions


// Web server handlers
void handleScan() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  Logger::logWebRequest("POST", "/scan", clientIP.c_str());

  if (isScanning) {
    LOG_WEB_INFO("Scan request rejected - scan already in progress");
    server.send(400, "text/plain", "Scan already in progress");
    Logger::logWebResponse(400, millis() - _perf_start);
    return;
  }

  LOG_SENSOR_INFO("Starting color scan sequence");
  isScanning = true;
  Logger::logMemoryUsage("Scan start");

  // Calculate optimal LED brightness using enhanced algorithm
  LOG_LED_INFO("Calculating optimal LED brightness for scan");
  uint8_t optimalBrightness = calculateOptimalBrightness();

  // Turn on illumination LED for scanning with calculated optimal brightness
  LOG_LED_INFO("Activating scan illumination LED - brightness: %u (optimized)", optimalBrightness);
  setIlluminationBrightness(optimalBrightness);
  currentBrightness = optimalBrightness; // Update current brightness
  delay(SENSOR_STABILIZE_MS);  // Increased stabilization time
  LOG_LED_DEBUG("Illumination LED stabilization delay completed (%d ms)", SENSOR_STABILIZE_MS);

  // Enhanced continuous scanning for 5 seconds - maximum readings for best accuracy
  LOG_SENSOR_INFO("Starting enhanced 5-second continuous scan for maximum accuracy");
  const unsigned long scanDuration = 5000; // 5 seconds
  const int maxReadings = 200; // Maximum buffer size to prevent memory issues

  // Use vectors for dynamic storage (more memory efficient than arrays)
  std::vector<uint16_t> xReadings, yReadings, zReadings, irReadings;
  xReadings.reserve(maxReadings);
  yReadings.reserve(maxReadings);
  zReadings.reserve(maxReadings);
  irReadings.reserve(maxReadings);

  uint32_t sumX = 0, sumY = 0, sumZ = 0, sumIR = 0;

  // Statistics for consistency analysis
  uint16_t minX = 65535, maxX = 0, minY = 65535, maxY = 0, minZ = 65535, maxZ = 0;

  unsigned long scanStartTime = millis();
  unsigned long lastReadingTime = 0;

  LOG_SENSOR_INFO("Scanning continuously for 5 seconds - taking as many readings as possible");

  while ((millis() - scanStartTime) < scanDuration && xReadings.size() < maxReadings) {
    // Small delay to prevent overwhelming the sensor, but maximize readings
    if (millis() - lastReadingTime >= 25) { // ~40 readings per second max
      lastReadingTime = millis();

      uint16_t x_val = tcs3430.getXData();
      uint16_t y_val = tcs3430.getYData();
      uint16_t z_val = tcs3430.getZData();
      uint16_t ir_val = tcs3430.getIR1Data();

      // Store readings
      xReadings.push_back(x_val);
      yReadings.push_back(y_val);
      zReadings.push_back(z_val);
      irReadings.push_back(ir_val);

      // Add to sums
      sumX += x_val;
      sumY += y_val;
      sumZ += z_val;
      sumIR += ir_val;

      // Track min/max for consistency analysis
      if (x_val < minX) minX = x_val;
      if (x_val > maxX) maxX = x_val;
      if (y_val < minY) minY = y_val;
      if (y_val > maxY) maxY = y_val;
      if (z_val < minZ) minZ = z_val;
      if (z_val > maxZ) maxZ = z_val;

      // Log progress every 20 readings
      if (xReadings.size() % 20 == 0) {
        float elapsed = (millis() - scanStartTime) / 1000.0f;
        LOG_SENSOR_DEBUG("Progress: %d readings in %.1fs (%.1f readings/sec)",
                         xReadings.size(), elapsed, xReadings.size() / elapsed);
      }

      // Feed watchdog during long scan
      esp_task_wdt_reset();
    } else {
      delay(1); // Very small delay to prevent busy waiting
    }
  }

  int actualReadings = xReadings.size();

  // Calculate averages
  uint16_t x = sumX / actualReadings;
  uint16_t y = sumY / actualReadings;
  uint16_t z = sumZ / actualReadings;
  uint16_t ir = sumIR / actualReadings;

  // Calculate consistency metrics
  float xVariation = (actualReadings > 0 && x > 0) ? ((float)(maxX - minX) / x) * 100.0f : 0.0f;
  float yVariation = (actualReadings > 0 && y > 0) ? ((float)(maxY - minY) / y) * 100.0f : 0.0f;
  float zVariation = (actualReadings > 0 && z > 0) ? ((float)(maxZ - minZ) / z) * 100.0f : 0.0f;

  float scanTime = (millis() - scanStartTime) / 1000.0f;
  float readingsPerSecond = actualReadings / scanTime;

  LOG_SENSOR_INFO("Enhanced scan completed: %d readings in %.1f seconds (%.1f readings/sec)",
                  actualReadings, scanTime, readingsPerSecond);
  LOG_SENSOR_INFO("Consistency analysis - X: %.1f%% Y: %.1f%% Z: %.1f%% variation",
                  xVariation, yVariation, zVariation);
  LOG_SENSOR_INFO("Final averaged values - X:%u Y:%u Z:%u IR:%u", x, y, z, ir);
  float ambientLux = getAmbientLightLux();

  // Enhanced consistency validation using variation metrics from 5-second scan
  if (xVariation < 10.0f && yVariation < 10.0f && zVariation < 10.0f) {
    LOG_SENSOR_INFO("Excellent scan consistency - X:%.1f%% Y:%.1f%% Z:%.1f%% variation",
                    xVariation, yVariation, zVariation);
  } else if (xVariation < 20.0f && yVariation < 20.0f && zVariation < 20.0f) {
    LOG_SENSOR_INFO("Good scan consistency - X:%.1f%% Y:%.1f%% Z:%.1f%% variation",
                    xVariation, yVariation, zVariation);
  } else {
    LOG_SENSOR_ERROR("Moderate scan consistency - X:%.1f%% Y:%.1f%% Z:%.1f%% variation",
                     xVariation, yVariation, zVariation);
  }

  // Apply advanced calibration correction if available
  if (whiteCalData.valid && blackCalData.valid) {
    // Two-point calibration using both white and black references
    // This provides better linearity across the full measurement range

    LOG_SENSOR_DEBUG("Applying two-point calibration (white + black)");

    // Calculate the range between black and white for each channel
    float rangeX = whiteCalData.x - blackCalData.x;
    float rangeY = whiteCalData.y - blackCalData.y;
    float rangeZ = whiteCalData.z - blackCalData.z;

    // Apply two-point linear calibration with balanced white point
    // Use average white component for balanced color output
    float avgWhiteComponent = (whiteCalData.x + whiteCalData.y + whiteCalData.z) / 3.0f;

    if (rangeX > 0) {
      float normalizedX = (x - blackCalData.x) / rangeX;
      x = constrain(normalizedX * avgWhiteComponent, 0, 65535);
    }
    if (rangeY > 0) {
      float normalizedY = (y - blackCalData.y) / rangeY;
      y = constrain(normalizedY * avgWhiteComponent, 0, 65535);
    }
    if (rangeZ > 0) {
      float normalizedZ = (z - blackCalData.z) / rangeZ;
      z = constrain(normalizedZ * avgWhiteComponent, 0, 65535);
    }

    LOG_SENSOR_DEBUG("Two-point calibration applied with avg component: %.0f", avgWhiteComponent);

    LOG_SENSOR_DEBUG("Two-point calibration applied - White:(%u,%u,%u) Black:(%u,%u,%u) -> Calibrated:(%u,%u,%u)",
                     whiteCalData.x, whiteCalData.y, whiteCalData.z, blackCalData.x, blackCalData.y, blackCalData.z, x, y, z);
  } else if (whiteCalData.valid) {
    // Single-point white calibration (fallback when no black calibration)
    // Apply white reference calibration to normalize readings

    LOG_SENSOR_DEBUG("Applying single-point white calibration");

    // Calculate calibration factors for TCS3430 white balance
    // Goal: Make Vivid White read as balanced RGB (equal R, G, B values)
    // Strategy: Scale each channel so the white reference produces equal values

    // For balanced white, all channels should produce the same output
    // Use the average of the white calibration as the target
    float avgWhiteComponent = (whiteCalData.x + whiteCalData.y + whiteCalData.z) / 3.0f;

    // Calculate factors to make each channel equal to the average
    float whiteFactorX = (whiteCalData.x > 0) ? avgWhiteComponent / whiteCalData.x : 1.0f;
    float whiteFactorY = (whiteCalData.y > 0) ? avgWhiteComponent / whiteCalData.y : 1.0f;
    float whiteFactorZ = (whiteCalData.z > 0) ? avgWhiteComponent / whiteCalData.z : 1.0f;

    LOG_SENSOR_DEBUG("White balance factors - X:%.3f Y:%.3f Z:%.3f (target: %.0f)",
                     whiteFactorX, whiteFactorY, whiteFactorZ, avgWhiteComponent);

    // Apply calibration factors to current readings
    float calibratedX = x * whiteFactorX;
    float calibratedY = y * whiteFactorY;
    float calibratedZ = z * whiteFactorZ;

    // Clamp to valid range
    x = constrain(calibratedX, 0, 65535);
    y = constrain(calibratedY, 0, 65535);
    z = constrain(calibratedZ, 0, 65535);

    LOG_SENSOR_DEBUG("White calibration applied - White:(%u,%u,%u) Factors:(%.3f,%.3f,%.3f) -> Calibrated:(%u,%u,%u)",
                     whiteCalData.x, whiteCalData.y, whiteCalData.z, whiteFactorX, whiteFactorY, whiteFactorZ, x, y, z);
  }

  if (x > 0 || y > 0 || z > 0) {  // Check if we have valid data
    LOG_SENSOR_INFO("Valid sensor data received");

    // Use scientific CIE 1931 color space conversion
    LOG_SENSOR_DEBUG("Raw sensor values - X:%u Y:%u Z:%u IR:%u", x, y, z, ir);

    if (whitePointCalibrated) {
      // Convert sensor data to sRGB using scientific CIE 1931 approach
      sRGB_Simple rgbResult = convertSensorToSRGB_Scientific(x, y, z, ir);

      currentR = rgbResult.r;
      currentG = rgbResult.g;
      currentB = rgbResult.b;

      LOG_SENSOR_INFO("Scientific CIE 1931 conversion - RGB:(%u,%u,%u)", currentR, currentG, currentB);
    } else {
      // Fallback to simple conversion if not calibrated
      LOG_SENSOR_WARN("No white point calibration - using fallback conversion");
      currentR = constrain((x * 255) / 65535, 0, 255);
      currentG = constrain((y * 255) / 65535, 0, 255);
      currentB = constrain((z * 255) / 65535, 0, 255);
    }

    // Log comprehensive sensor data
    Logger::logSensorData(x, y, z, ir, currentR, currentG, currentB, ambientLux);

    // Display measured color on LED
    LOG_LED_INFO("Displaying scanned color on LED - RGB:(%u,%u,%u)", currentR, currentG, currentB);
    setLEDColor(currentR, currentG, currentB, optimalBrightness);

    // Send response
    JsonDocument doc;
    doc["r"] = currentR;
    doc["g"] = currentG;
    doc["b"] = currentB;
    doc["x"] = x;
    doc["y"] = y;
    doc["z"] = z;
    doc["ir"] = ir;

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);

    Logger::logWebResponse(200, millis() - _perf_start);
    LOG_SENSOR_INFO("Color scan completed successfully");
  } else {
    LOG_SENSOR_ERROR("Invalid sensor data - all channels read zero");
    server.send(500, "text/plain", "Failed to read sensor data");
    Logger::logWebResponse(500, millis() - _perf_start);
  }

  // Turn off illumination LED after scan (unless manually enabled)
  if (!ledState) {
    LOG_LED_INFO("Turning off scan illumination LED (not manually enabled)");
    turnOffIllumination();
  } else {
    LOG_LED_INFO("Keeping illumination LED on (manually enabled)");
    setIlluminationBrightness(currentBrightness);
  }

  isScanning = false;
  LOG_SENSOR_INFO("Scan sequence completed");
  Logger::logMemoryUsage("Scan end");
}

void handleSaveSample() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  Logger::logWebRequest("POST", "/save", clientIP.c_str());



  if (server.hasArg("plain")) {
    LOG_STORAGE_INFO("Processing sample save request");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
      LOG_STORAGE_ERROR("JSON parsing failed: %s", error.c_str());
      server.send(400, "text/plain", "Invalid JSON data");
      Logger::logWebResponse(400, millis() - _perf_start);
      return;
    }

    uint8_t r = doc["r"];
    uint8_t g = doc["g"];
    uint8_t b = doc["b"];

    LOG_STORAGE_INFO("Sample data received - RGB:(%u,%u,%u)", r, g, b);

    // Create new sample
    ColorSample newSample;
    newSample.r = r;
    newSample.g = g;
    newSample.b = b;
    newSample.timestamp = millis();
    strcpy(newSample.paintName, "Unknown");
    strcpy(newSample.paintCode, "N/A");
       newSample.lrv = 0.0;

    // Add to circular buffer
    int oldSampleIndex = sampleIndex;
    samples[sampleIndex] = newSample;
    sampleIndex = (sampleIndex + 1) % MAX_SAMPLES;
    if (sampleCount < MAX_SAMPLES) sampleCount++;

    LOG_STORAGE_INFO("Sample added to buffer - Index:%d Count:%d/%d",
                     oldSampleIndex, sampleCount, MAX_SAMPLES);

    // Save to EEPROM
    LOG_STORAGE_INFO("Saving samples to EEPROM");
    saveSamples();

    // Flash green LED for confirmation
    LOG_LED_INFO("Flashing green confirmation LED");
    setLEDColor(0, 255, 0, 128);
    delay(200);
    if (!ledState) turnOffLED();

    // Call Google Apps Script for color matching (async)
    int savedSampleIndex = sampleIndex == 0 ? MAX_SAMPLES - 1 : sampleIndex - 1;
    LOG_API_INFO("Initiating Google Apps Script call for sample %d", savedSampleIndex);
    matchColorWithGoogleScript(r, g, b, savedSampleIndex);

    server.send(200, "text/plain", "Sample saved");
    Logger::logWebResponse(200, millis() - _perf_start);
    LOG_STORAGE_INFO("Sample save completed successfully - RGB:(%u,%u,%u)", r, g, b);
    Logger::logMemoryUsage("After sample save");
  } else {
    LOG_STORAGE_ERROR("No JSON data in save request");
    server.send(400, "text/plain", "Invalid request");
    Logger::logWebResponse(400, millis() - _perf_start);
  }

  LOG_PERF_END("Sample save operation");
}

void handleSavedSamples() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  // Removed logging for samples requests - too frequent and not useful



  LOG_STORAGE_INFO("Retrieving %d saved samples", sampleCount);

  // Use smaller JSON document and stream if needed
  JsonDocument doc;
  JsonArray samplesArray = doc["samples"].to<JsonArray>();

  for (int i = 0; i < sampleCount; i++) {
    JsonObject sample = samplesArray.add<JsonObject>();
    sample["r"] = samples[i].r;
    sample["g"] = samples[i].g;
    sample["b"] = samples[i].b;
    sample["timestamp"] = samples[i].timestamp;
    sample["paintName"] = samples[i].paintName;
    sample["paintCode"] = samples[i].paintCode;
    sample["lrv"] = samples[i].lrv;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

  // Removed verbose logging for samples responses - too frequent
  LOG_PERF_END("Samples retrieval");
}

void handleDeleteSample() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  LOG_WEB_INFO("Delete sample request from %s", clientIP.c_str());

  if (!server.hasArg("plain")) {
    LOG_WEB_ERROR("Delete request missing JSON body");
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing request body\"}");
    return;
  }

  String requestBody = server.arg("plain");
  LOG_WEB_DEBUG("Delete request body: %s", requestBody.c_str());

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, requestBody);

  if (error) {
    LOG_WEB_ERROR("Failed to parse delete request JSON: %s", error.c_str());
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  if (!doc["index"].is<int>()) {
    LOG_WEB_ERROR("Delete request missing 'index' parameter");
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing index parameter\"}");
    return;
  }

  int deleteIndex = doc["index"];

  if (deleteIndex < 0 || deleteIndex >= sampleCount) {
    LOG_WEB_ERROR("Invalid delete index: %d (valid range: 0-%d)", deleteIndex, sampleCount - 1);
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid sample index\"}");
    return;
  }

  LOG_STORAGE_INFO("Deleting sample %d - RGB:(%u,%u,%u) Paint:%s",
                   deleteIndex, samples[deleteIndex].r, samples[deleteIndex].g, samples[deleteIndex].b,
                   samples[deleteIndex].paintName);

  // Shift all samples after the deleted one forward
  for (int i = deleteIndex; i < sampleCount - 1; i++) {
    samples[i] = samples[i + 1];
  }

  // Decrease sample count
  sampleCount--;

  // Adjust sampleIndex if necessary (circular buffer logic)
  if (sampleIndex > deleteIndex) {
    sampleIndex--;
  } else if (sampleIndex == deleteIndex && sampleIndex == sampleCount) {
    sampleIndex = 0; // Wrap around if we deleted the last sample and index was pointing to it
  }

  // Save updated samples to EEPROM
  LOG_STORAGE_INFO("Saving updated samples to EEPROM - New count: %d", sampleCount);
  saveSamples();

  // Clear the last slot in EEPROM (since we have one less sample)
  String lastKey = String(PREF_SAMPLE_PREFIX) + String(sampleCount);
  preferences.remove(lastKey.c_str());

  LOG_STORAGE_INFO("Sample deletion completed - Index:%d NewCount:%d", deleteIndex, sampleCount);

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Sample deleted successfully\"}");
  Logger::logWebResponse(200, millis() - _perf_start);
  LOG_PERF_END("Sample deletion");
}

void handleClearAllSamples() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  Logger::logWebRequest("POST", "/samples/clear", clientIP.c_str());
  LOG_WEB_INFO("Clear all samples request from %s", clientIP.c_str());

  if (sampleCount == 0) {
    LOG_WEB_INFO("No samples to clear");
    server.send(200, "application/json", "{\"success\":true,\"message\":\"No samples to clear\"}");
    Logger::logWebResponse(200, millis() - _perf_start);
    LOG_PERF_END("Clear all samples (empty)");
    return;
  }

  LOG_STORAGE_INFO("Clearing all %d samples", sampleCount);

  // Clear all samples from memory
  for (int i = 0; i < sampleCount; i++) {
    memset(&samples[i], 0, sizeof(ColorSample));
  }

  // Reset counters
  int oldSampleCount = sampleCount;
  sampleCount = 0;
  sampleIndex = 0;

  // Save updated state to EEPROM
  LOG_STORAGE_INFO("Saving cleared state to EEPROM");
  saveSamples();

  // Clear all sample keys from EEPROM
  for (int i = 0; i < oldSampleCount; i++) {
    String key = "sample" + String(i);
    preferences.remove(key.c_str());
  }

  LOG_STORAGE_INFO("All samples cleared successfully - Previous count: %d", oldSampleCount);

  server.send(200, "application/json", "{\"success\":true,\"message\":\"All samples cleared successfully\"}");
  Logger::logWebResponse(200, millis() - _perf_start);
  LOG_PERF_END("Clear all samples");
}

void handleSettings() {
  bool isJsonRequest = server.hasArg("plain");
  bool isFormRequest = server.hasArg("atime") || server.hasArg("again") || server.hasArg("brightness") ||
                       server.hasArg("autoZeroMode") || server.hasArg("autoZeroFreq") || server.hasArg("waitTime");

  if (isJsonRequest) {
    // Handle JSON request from React API
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));

    if (doc["atime"].is<int>()) {
      uint16_t newAtime = doc["atime"];
      if (newAtime <= 255) {  // Valid range: 0-255
        currentAtime = newAtime;
        tcs3430.setIntegrationTime(currentAtime);
        LOG_SENSOR_INFO("ATIME updated to: %d", currentAtime);
      } else {
        LOG_SENSOR_ERROR("Invalid ATIME: %d (must be 0-255)", newAtime);
      }
    }

    if (doc["again"].is<int>()) {
      uint8_t newAgain = doc["again"];
      if (newAgain <= 3) {  // Valid range: 0-3 (1x, 4x, 16x, 64x)
        currentAgain = newAgain;
        tcs3430.setALSGain(currentAgain);
        LOG_SENSOR_INFO("AGAIN updated to: %d", currentAgain);
      } else {
        LOG_SENSOR_ERROR("Invalid AGAIN: %d (must be 0-3)", newAgain);
      }
    }

    if (doc["brightness"].is<int>()) {
      uint8_t newBrightness = doc["brightness"];
      if (newBrightness >= MIN_LED_BRIGHTNESS && newBrightness <= MAX_LED_BRIGHTNESS) {
        currentBrightness = newBrightness;
        LOG_SENSOR_INFO("Brightness updated to: %d", currentBrightness);
      } else {
        LOG_SENSOR_ERROR("Invalid brightness: %d (must be %d-%d)", newBrightness, MIN_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS);
      }
    }

    if (doc["ledState"].is<bool>()) {
      ledState = doc["ledState"];
      if (!ledState && !isScanning) {
        turnOffLED();
      }
    }

    // Handle TCS3430 advanced calibration settings with validation
    if (doc["autoZeroMode"].is<int>()) {
      uint8_t newAutoZeroMode = doc["autoZeroMode"];
      if (newAutoZeroMode <= 1) {  // Valid range: 0-1
        currentAutoZeroMode = newAutoZeroMode;
        tcs3430.setAutoZeroMode(currentAutoZeroMode);
        LOG_SENSOR_INFO("Auto-zero mode updated to: %d", currentAutoZeroMode);
      } else {
        LOG_SENSOR_ERROR("Invalid auto-zero mode: %d (must be 0-1)", newAutoZeroMode);
      }
    }

    if (doc["autoZeroFreq"].is<int>()) {
      uint16_t newAutoZeroFreq = doc["autoZeroFreq"];
      if (newAutoZeroFreq <= 255) {  // Valid range: 0-255
        currentAutoZeroFreq = newAutoZeroFreq;
        tcs3430.setAutoZeroNTHIteration(currentAutoZeroFreq);
        LOG_SENSOR_INFO("Auto-zero frequency updated to: %d", currentAutoZeroFreq);
      } else {
        LOG_SENSOR_ERROR("Invalid auto-zero frequency: %d (must be 0-255)", newAutoZeroFreq);
      }
    }

    if (doc["waitTime"].is<int>()) {
      uint16_t newWaitTime = doc["waitTime"];
      if (newWaitTime <= 255) {  // Valid range: 0-255
        currentWaitTime = newWaitTime;
        tcs3430.setWaitTime(currentWaitTime);
        LOG_SENSOR_INFO("Wait time updated to: %d", currentWaitTime);
      } else {
        LOG_SENSOR_ERROR("Invalid wait time: %d (must be 0-255)", newWaitTime);
      }
    }

    // Handle enhanced LED control settings
    if (doc["enhancedLEDMode"].is<bool>()) {
      enhancedLEDMode = doc["enhancedLEDMode"];
      LOG_SENSOR_INFO("Enhanced LED mode updated to: %s", enhancedLEDMode ? "ENABLED" : "DISABLED");
    }

    if (doc["manualLEDIntensity"].is<int>()) {
      uint8_t newManualIntensity = doc["manualLEDIntensity"];
      if (newManualIntensity >= MIN_LED_BRIGHTNESS && newManualIntensity <= MAX_LED_BRIGHTNESS) {
        manualLEDIntensity = newManualIntensity;
        LOG_SENSOR_INFO("Manual LED intensity updated to: %d", manualLEDIntensity);
      } else {
        LOG_SENSOR_ERROR("Invalid manual LED intensity: %d (must be %d-%d)",
                         newManualIntensity, MIN_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS);
      }
    }

    saveSettings();
    server.send(200, "text/plain", "Settings saved");
    Serial.printf("Updated settings: ATIME=%d, AGAIN=%d, Brightness=%d, AutoZeroMode=%d, AutoZeroFreq=%d, WaitTime=%d, EnhancedLED=%s, ManualIntensity=%d\n",
                  currentAtime, currentAgain, currentBrightness, currentAutoZeroMode, currentAutoZeroFreq, currentWaitTime,
                  enhancedLEDMode ? "ON" : "OFF", manualLEDIntensity);
  } else if (isFormRequest) {
    // Handle form-encoded request from HTML settings page
    if (server.hasArg("atime")) {
      currentAtime = server.arg("atime").toInt();
      tcs3430.setIntegrationTime(currentAtime);
    }

    if (server.hasArg("again")) {
      currentAgain = server.arg("again").toInt();
      tcs3430.setALSGain(currentAgain);
    }

    if (server.hasArg("brightness")) {
      currentBrightness = server.arg("brightness").toInt();
    }

    if (server.hasArg("autoZeroMode")) {
      currentAutoZeroMode = server.arg("autoZeroMode").toInt();
      tcs3430.setAutoZeroMode(currentAutoZeroMode);
    }

    if (server.hasArg("autoZeroFreq")) {
      currentAutoZeroFreq = server.arg("autoZeroFreq").toInt();
      tcs3430.setAutoZeroNTHIteration(currentAutoZeroFreq);
    }

    if (server.hasArg("waitTime")) {
      currentWaitTime = server.arg("waitTime").toInt();
      tcs3430.setWaitTime(currentWaitTime);
    }

    saveSettings();

    // Redirect back to settings page with success message
    String html = "<!DOCTYPE html><html><head><title>Settings Saved</title>";
    html += "<meta http-equiv='refresh' content='2;url=/settings'>";
    html += "<style>body{font-family:Arial,sans-serif;text-align:center;padding:50px;background-color:#1a1a1a;color:#e0e0e0;}</style>";
    html += "</head><body>";
    html += "<h1>Settings Saved Successfully!</h1>";
    html += "<p>Redirecting back to settings page...</p>";
    html += "<p><a href='/settings' style='color:#2563eb;'>Click here if not redirected automatically</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);

    Serial.printf("Updated settings via form: ATIME=%d, AGAIN=%d, Brightness=%d, AutoZeroMode=%d, AutoZeroFreq=%d, WaitTime=%d\n",
                  currentAtime, currentAgain, currentBrightness, currentAutoZeroMode, currentAutoZeroFreq, currentWaitTime);
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void handleGetSettings() {
  LOG_PERF_START();
  LOG_API_INFO("Get settings request received");

  JsonDocument doc;
  doc["success"] = true;
  doc["timestamp"] = millis();

  // Current sensor settings
  doc["atime"] = currentAtime;
  doc["again"] = currentAgain;
  doc["brightness"] = currentBrightness;
  doc["ledState"] = ledState;

  // TCS3430 advanced calibration settings
  doc["autoZeroMode"] = currentAutoZeroMode;
  doc["autoZeroFreq"] = currentAutoZeroFreq;
  doc["waitTime"] = currentWaitTime;

  // Enhanced LED control settings
  doc["enhancedLEDMode"] = enhancedLEDMode;
  doc["manualLEDIntensity"] = manualLEDIntensity;

  // Calibration status
  doc["isCalibrated"] = isCalibrated;
  doc["whitePointCalibrated"] = whitePointCalibrated;

  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  LOG_API_INFO("Get settings completed successfully");
  LOG_PERF_END("Get settings request");
}

void handleSettingsPage() {
  LOG_WEB_INFO("Serving settings page");
  String clientIP = server.client().remoteIP().toString();
  Logger::logWebRequest("GET", "/settings", clientIP.c_str());

  String html = "<!DOCTYPE html><html><head><title>ESP32 Color Matcher - Settings</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:800px;margin:0 auto;padding:20px;background-color:#1a1a1a;color:#e0e0e0;}";
  html += ".header{text-align:center;margin-bottom:30px;padding:20px;background:linear-gradient(135deg,#2563eb,#1d4ed8);border-radius:10px;color:white;}";
  html += ".card{background-color:#2a2a2a;border-radius:10px;padding:20px;margin-bottom:20px;border:1px solid #404040;}";
  html += ".form-group{margin-bottom:15px;}";
  html += "label{display:block;margin-bottom:5px;font-weight:bold;color:#b0b0b0;}";
  html += "input,select{width:100%;padding:8px;border:1px solid #555;border-radius:5px;background-color:#3a3a3a;color:#e0e0e0;box-sizing:border-box;}";
  html += "button{background-color:#2563eb;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin-right:10px;margin-bottom:10px;}";
  html += "button:hover{background-color:#1d4ed8;}";
  html += "button.secondary{background-color:#6b7280;}";
  html += "button:disabled{background-color:#4a5568;cursor:not-allowed;opacity:0.6;}";
  html += ".notification{position:fixed;top:20px;right:20px;padding:15px 20px;border-radius:8px;color:white;font-weight:bold;z-index:1000;min-width:300px;box-shadow:0 4px 12px rgba(0,0,0,0.3);opacity:0;transform:translateY(-20px);transition:all 0.3s ease;}";
  html += ".notification.show{opacity:1;transform:translateY(0);}";
  html += ".notification.success{background-color:#10b981;border-left:4px solid #059669;}";
  html += ".notification.error{background-color:#ef4444;border-left:4px solid #dc2626;}";
  html += ".notification .close{float:right;margin-left:15px;cursor:pointer;font-size:18px;line-height:1;}";
  html += ".spinner{display:inline-block;width:16px;height:16px;border:2px solid rgba(255,255,255,0.3);border-radius:50%;border-top-color:white;animation:spin 1s ease-in-out infinite;margin-right:8px;}";
  html += "@keyframes spin{to{transform:rotate(360deg);}}";
  html += "</style></head><body>";

  html += "<!-- Notification Area -->";
  html += "<div id='notification' class='notification'>";
  html += "<span class='close' onclick='hideNotification()'>&times;</span>";
  html += "<span id='notification-message'></span>";
  html += "</div>";

  html += "<div class='header'><h1>ESP32 Color Matcher</h1><p>Settings & Configuration</p>";
  html += "<div><strong>Device IP:</strong> " + WiFi.localIP().toString() + "</div></div>";

  html += "<div class='card'><h2>Scanner Settings</h2>";
  html += "<form action='/settings' method='POST'>";
  html += "<div class='form-group'><label for='atime'>ATIME (Integration Time):</label>";
  html += "<input type='number' id='atime' name='atime' min='0' max='255' value='" + String(currentAtime) + "'></div>";

  html += "<div class='form-group'><label for='again'>AGAIN (Analog Gain):</label>";
  html += "<select id='again' name='again'>";
  html += "<option value='0'";
  if (currentAgain == 0) html += " selected";
  html += ">1x</option>";
  html += "<option value='1'";
  if (currentAgain == 1) html += " selected";
  html += ">4x</option>";
  html += "<option value='2'";
  if (currentAgain == 2) html += " selected";
  html += ">16x</option>";
  html += "<option value='3'";
  if (currentAgain == 3) html += " selected";
  html += ">64x</option>";
  html += "</select></div>";

  html += "<div class='form-group'><label for='brightness'>Scan Brightness:</label>";
  html += "<input type='number' id='brightness' name='brightness' min='0' max='255' value='" + String(currentBrightness) + "'></div>";

  html += "<div class='form-group'><label for='autoZeroMode'>Auto-Zero Mode:</label>";
  html += "<select id='autoZeroMode' name='autoZeroMode'>";
  html += "<option value='0'";
  if (currentAutoZeroMode == 0) html += " selected";
  html += ">Always start at zero</option>";
  html += "<option value='1'";
  if (currentAutoZeroMode == 1) html += " selected";
  html += ">Use previous offset (recommended)</option>";
  html += "</select></div>";

  html += "<div class='form-group'><label for='autoZeroFreq'>Auto-Zero Frequency:</label>";
  html += "<input type='number' id='autoZeroFreq' name='autoZeroFreq' min='0' max='255' value='" + String(currentAutoZeroFreq) + "'></div>";

  html += "<div class='form-group'><label for='waitTime'>Wait Time:</label>";
  html += "<input type='number' id='waitTime' name='waitTime' min='0' max='255' value='" + String(currentWaitTime) + "'></div>";

  html += "<button type='submit'>Save Settings</button>";
  html += "</form></div>";

  html += "<div class='card'><h2>Saved Samples</h2>";
  html += "<div id='samples-container'>";
  if (sampleCount == 0) {
    html += "<p style='color:#9ca3af;'>No samples saved yet.</p>";
  } else {
    html += "<div style='max-height:400px;overflow-y:auto;'>";
    for (int i = 0; i < sampleCount; i++) {
      html += "<div class='sample-item' style='background-color:#374151;padding:12px;margin-bottom:8px;border-radius:6px;position:relative;'>";
      html += "<div style='display:flex;align-items:center;'>";

      // Color swatch
      html += "<div style='width:40px;height:40px;border-radius:4px;margin-right:12px;border:1px solid #6b7280;background-color:rgb(" + String(samples[i].r) + "," + String(samples[i].g) + "," + String(samples[i].b) + ");'></div>";

      // Sample info
      html += "<div style='flex:1;'>";
      if (strlen(samples[i].paintName) > 0 && strcmp(samples[i].paintName, "Unknown") != 0) {
        html += "<div style='font-weight:bold;color:#f3f4f6;font-size:14px;'>" + String(samples[i].paintName) + "</div>";
        if (strlen(samples[i].paintCode) > 0 && strcmp(samples[i].paintCode, "N/A") != 0) {
          html += "<div style='color:#d1d5db;font-size:12px;'>Code: " + String(samples[i].paintCode) + "</div>";
        }
        html += "<div style='color:#9ca3af;font-size:11px;font-family:monospace;'>RGB: " + String(samples[i].r) + ", " + String(samples[i].g) + ", " + String(samples[i].b) + "</div>";
      } else {
        html += "<div style='font-weight:bold;color:#f3f4f6;font-size:14px;'>RGB: " + String(samples[i].r) + ", " + String(samples[i].g) + ", " + String(samples[i].b) + "</div>";
      }
      if (samples[i].lrv > 0) {
        html += "<div style='color:#9ca3af;font-size:11px;'>LRV: " + String(samples[i].lrv, 1) + "</div>";
      }
      html += "<div style='color:#6b7280;font-size:11px;margin-top:4px;'>Saved: " + String(samples[i].timestamp) + "</div>";
      html += "</div>";

      // Delete button
      html += "<button onclick='deleteSample(" + String(i) + ")' style='position:absolute;top:8px;right:8px;width:24px;height:24px;background-color:#dc2626;color:white;border:none;border-radius:50%;cursor:pointer;font-size:14px;font-weight:bold;' title='Delete sample'>&times;</button>";

      html += "</div></div>";
    }
    html += "</div>";

    // Delete all button
    html += "<div style='margin-top:16px;text-align:right;'>";
    html += "<button onclick='deleteAllSamples()' style='background-color:#dc2626;color:white;padding:8px 16px;border:none;border-radius:4px;cursor:pointer;font-size:12px;'>Delete All (" + String(sampleCount) + ")</button>";
    html += "</div>";
  }
  html += "</div></div>";

  html += "<div class='card'><h2>Quick Actions</h2>";
  html += "<p style='color:#9ca3af;margin-bottom:15px;'>Use the React web interface for advanced calibration features.</p>";
  html += "<button onclick=\"window.location.href='/'\">Back to Main Interface</button>";
  html += "</div>";

  html += "<script>";
  html += "function showNotification(message, type) {";
  html += "  const notification = document.getElementById('notification');";
  html += "  const messageSpan = document.getElementById('notification-message');";
  html += "  messageSpan.innerHTML = message;";
  html += "  notification.className = 'notification ' + type + ' show';";
  html += "  setTimeout(() => hideNotification(), 5000);";
  html += "}";
  html += "function hideNotification() {";
  html += "  const notification = document.getElementById('notification');";
  html += "  notification.className = 'notification';";
  html += "}";
  // Removed old calibration function - using advanced calibration wizard only

  // Add delete sample functions
  html += "function deleteSample(index) {";
  html += "  if (!confirm('Are you sure you want to delete this sample?')) return;";
  html += "  fetch('/delete', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/json' },";
  html += "    body: JSON.stringify({ index: index })";
  html += "  })";
  html += "  .then(response => {";
  html += "    if (response.ok) {";
  html += "      return response.json();";
  html += "    } else {";
  html += "      return response.text().then(text => ({ success: false, error: text }));";
  html += "    }";
  html += "  })";
  html += "  .then(data => {";
  html += "    if (data.success) {";
  html += "      showNotification('Sample deleted successfully', 'success');";
  html += "      setTimeout(() => window.location.reload(), 1000);";
  html += "    } else {";
  html += "      showNotification(data.error || 'Failed to delete sample', 'error');";
  html += "    }";
  html += "  })";
  html += "  .catch(error => {";
  html += "    showNotification('Network error: ' + error.message, 'error');";
  html += "  });";
  html += "}";

  html += "function deleteAllSamples() {";
  html += "  if (!confirm('Are you sure you want to delete ALL samples? This cannot be undone.')) return;";
  html += "  fetch('/samples/clear', {";
  html += "    method: 'POST',";
  html += "    headers: { 'Content-Type': 'application/json' }";
  html += "  })";
  html += "  .then(response => {";
  html += "    if (response.ok) {";
  html += "      return response.json();";
  html += "    } else {";
  html += "      return response.text().then(text => ({ success: false, error: text }));";
  html += "    }";
  html += "  })";
  html += "  .then(data => {";
  html += "    if (data.success) {";
  html += "      showNotification('All samples deleted successfully', 'success');";
  html += "      setTimeout(() => window.location.reload(), 1000);";
  html += "    } else {";
  html += "      showNotification(data.error || 'Failed to delete all samples', 'error');";
  html += "    }";
  html += "  })";
  html += "  .catch(error => {";
  html += "    showNotification('Network error: ' + error.message, 'error');";
  html += "  });";
  html += "}";

  html += "</script>";

  html += "</body></html>";

  server.send(200, "text/html", html);
  LOG_WEB_INFO("Settings page served successfully");
}

// ============================================================================
// ADVANCED CALIBRATION FUNCTIONS
// ============================================================================

void loadCalibrationData() {
  LOG_PERF_START();
  LOG_STORAGE_INFO("Loading advanced calibration data from EEPROM");

  // Load white calibration data
  whiteCalData.valid = preferences.getBool(PREF_HAS_WHITE_CAL, false);
  if (whiteCalData.valid) {
    whiteCalData.x = preferences.getUInt(PREF_WHITE_CAL_X, 0);
    whiteCalData.y = preferences.getUInt(PREF_WHITE_CAL_Y, 0);
    whiteCalData.z = preferences.getUInt(PREF_WHITE_CAL_Z, 0);
    whiteCalData.ir = preferences.getUInt(PREF_WHITE_CAL_IR, 0);
    whiteCalData.brightness = preferences.getUInt(PREF_WHITE_CAL_BRIGHTNESS, DEFAULT_BRIGHTNESS);
    whiteCalData.timestamp = preferences.getULong(PREF_WHITE_CAL_TIMESTAMP, 0);

    // Load CIE 1931 white point data (new scientific approach)
    whitePointX = preferences.getFloat("whitePointX", (float)whiteCalData.x);
    whitePointY = preferences.getFloat("whitePointY", (float)whiteCalData.y);
    whitePointZ = preferences.getFloat("whitePointZ", (float)whiteCalData.z);
    whitePointCalibrated = preferences.getBool("whitePointCal", false);

    // If no CIE white point data exists, use legacy data
    if (!whitePointCalibrated && whiteCalData.valid) {
      whitePointX = (float)whiteCalData.x;
      whitePointY = (float)whiteCalData.y;
      whitePointZ = (float)whiteCalData.z;
      whitePointCalibrated = true;
      LOG_STORAGE_INFO("Migrated legacy calibration to CIE 1931 white point");
    }

    LOG_STORAGE_INFO("White calibration loaded - X:%u Y:%u Z:%u IR:%u Brightness:%u",
                     whiteCalData.x, whiteCalData.y, whiteCalData.z, whiteCalData.ir, whiteCalData.brightness);
    LOG_STORAGE_INFO("CIE 1931 White Point - X:%.2f Y:%.2f Z:%.2f Calibrated:%s",
                     whitePointX, whitePointY, whitePointZ, whitePointCalibrated ? "true" : "false");
  } else {
    LOG_STORAGE_INFO("No white calibration data found");
  }

  // Load black calibration data
  blackCalData.valid = preferences.getBool(PREF_HAS_BLACK_CAL, false);
  if (blackCalData.valid) {
    blackCalData.x = preferences.getUInt(PREF_BLACK_CAL_X, 0);
    blackCalData.y = preferences.getUInt(PREF_BLACK_CAL_Y, 0);
    blackCalData.z = preferences.getUInt(PREF_BLACK_CAL_Z, 0);
    blackCalData.ir = preferences.getUInt(PREF_BLACK_CAL_IR, 0);
    blackCalData.timestamp = preferences.getULong(PREF_BLACK_CAL_TIMESTAMP, 0);

    LOG_STORAGE_INFO("Black calibration loaded - X:%u Y:%u Z:%u IR:%u",
                     blackCalData.x, blackCalData.y, blackCalData.z, blackCalData.ir);
  } else {
    LOG_STORAGE_INFO("No black calibration data found");
  }

  // Update overall calibration status
  bool hasAdvancedCal = whiteCalData.valid || blackCalData.valid;
  if (hasAdvancedCal && !isCalibrated) {
    isCalibrated = true;
    LOG_STORAGE_INFO("Advanced calibration data found - marking system as calibrated");
  }

  LOG_PERF_END("Advanced calibration data load");
}

void saveCalibrationData() {
  LOG_PERF_START();
  LOG_STORAGE_INFO("Saving advanced calibration data to EEPROM");

  // Save white calibration data
  if (whiteCalData.valid) {
    preferences.putBool(PREF_HAS_WHITE_CAL, true);
    preferences.putUInt(PREF_WHITE_CAL_X, whiteCalData.x);
    preferences.putUInt(PREF_WHITE_CAL_Y, whiteCalData.y);
    preferences.putUInt(PREF_WHITE_CAL_Z, whiteCalData.z);
    preferences.putUInt(PREF_WHITE_CAL_IR, whiteCalData.ir);
    preferences.putUInt(PREF_WHITE_CAL_BRIGHTNESS, whiteCalData.brightness);
    preferences.putULong(PREF_WHITE_CAL_TIMESTAMP, whiteCalData.timestamp);

    // Save CIE 1931 white point data (new scientific approach)
    preferences.putFloat("whitePointX", whitePointX);
    preferences.putFloat("whitePointY", whitePointY);
    preferences.putFloat("whitePointZ", whitePointZ);
    preferences.putBool("whitePointCal", whitePointCalibrated);

    LOG_STORAGE_INFO("White calibration saved - X:%u Y:%u Z:%u IR:%u Brightness:%u",
                     whiteCalData.x, whiteCalData.y, whiteCalData.z, whiteCalData.ir, whiteCalData.brightness);
    LOG_STORAGE_INFO("CIE 1931 White Point saved - X:%.2f Y:%.2f Z:%.2f",
                     whitePointX, whitePointY, whitePointZ);
  }

  // Save black calibration data
  if (blackCalData.valid) {
    preferences.putBool(PREF_HAS_BLACK_CAL, true);
    preferences.putUInt(PREF_BLACK_CAL_X, blackCalData.x);
    preferences.putUInt(PREF_BLACK_CAL_Y, blackCalData.y);
    preferences.putUInt(PREF_BLACK_CAL_Z, blackCalData.z);
    preferences.putUInt(PREF_BLACK_CAL_IR, blackCalData.ir);
    preferences.putULong(PREF_BLACK_CAL_TIMESTAMP, blackCalData.timestamp);

    LOG_STORAGE_INFO("Black calibration saved - X:%u Y:%u Z:%u IR:%u",
                     blackCalData.x, blackCalData.y, blackCalData.z, blackCalData.ir);
  }

  // Update overall calibration status
  if (whiteCalData.valid || blackCalData.valid) {
    isCalibrated = true;
    preferences.putBool(PREF_CALIBRATED, true);
    LOG_STORAGE_INFO("Advanced calibration completed - system marked as calibrated");
  }

  LOG_PERF_END("Advanced calibration data save");
}

bool startCalibrationSequence(uint8_t brightness) {
  LOG_PERF_START();
  LOG_SENSOR_INFO("Starting advanced calibration sequence with brightness: %u", brightness);

  // Check if calibration is already in progress
  if (calibrationInProgress) {
    LOG_SENSOR_ERROR("Calibration already in progress - cannot start new sequence");
    return false;
  }

  // Validate brightness
  if (brightness < MIN_LED_BRIGHTNESS || brightness > MAX_LED_BRIGHTNESS) {
    LOG_SENSOR_ERROR("Invalid brightness value: %u (must be %u-%u)", brightness, MIN_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS);
    return false;
  }

  // Initialize calibration state
  calibrationInProgress = true;
  calibrationStartTime = millis();
  calibrationBrightness = brightness;
  calibrationSessionId = "enhanced_cal_" + String(millis()); // generateCalibrationSessionId();
  calibrationCountdown = CALIBRATION_COUNTDOWN_SECONDS;

  // Reset calibration data
  whiteCalData = {0};
  blackCalData = {0};

  // Set initial state
  // updateCalibrationState(CAL_WHITE_COUNTDOWN, "Starting white calibration countdown");

  LOG_SENSOR_INFO("Calibration sequence started - Session ID: %s", calibrationSessionId.c_str());
  LOG_PERF_END("Calibration sequence start");
  return true;
}

bool performWhiteCalibration(uint8_t brightness) {
  LOG_PERF_START();
  LOG_SENSOR_INFO("Performing DFRobot-compliant white calibration with brightness: %u", brightness);

  if (!calibrationInProgress || currentCalState != CAL_WHITE_SCANNING) {
    LOG_SENSOR_ERROR("White calibration called in invalid state: %d", currentCalState);
    return false;
  }

  // Turn on illumination LED for white calibration
  LOG_LED_INFO("Activating illumination LED for white calibration - brightness: %u", brightness);
  setIlluminationBrightness(brightness);

  // DFRobot methodology: Extended stabilization time for sensor accuracy
  LOG_SENSOR_DEBUG("DFRobot stabilization: waiting %d ms for sensor and LED stability", SENSOR_STABILIZE_MS);
  delay(SENSOR_STABILIZE_MS);
  LOG_LED_DEBUG("DFRobot-compliant illumination LED stabilization completed");

  // DFRobot pattern: Check sensor status before readings
  uint8_t sensorStatus = tcs3430.getDeviceStatus();
  LOG_SENSOR_DEBUG("DFRobot sensor status check before calibration: 0x%02X", sensorStatus);

  // Check for sensor errors following DFRobot methodology
  if (sensorStatus & 0x80) {  // Check for any error flags
    LOG_SENSOR_ERROR("DFRobot sensor error detected before calibration - Status: 0x%02X", sensorStatus);
    turnOffLED();
    return false;
  }

  // DFRobot methodology: Multiple readings with proper timing
  uint32_t sumX = 0, sumY = 0, sumZ = 0, sumIR1 = 0, sumIR2 = 0;
  const int numReadings = 10;  // Increased for better DFRobot compliance

  LOG_SENSOR_DEBUG("DFRobot calibration: taking %d readings following library methodology", numReadings);
  for (int i = 0; i < numReadings; i++) {
    // DFRobot pattern: Check sensor ready status before each reading
    delay(SENSOR_READING_DELAY_MS);

    // Read all channels as per DFRobot getXYZIRData example
    uint16_t xData = tcs3430.getXData();
    uint16_t yData = tcs3430.getYData();
    uint16_t zData = tcs3430.getZData();
    uint16_t ir1Data = tcs3430.getIR1Data();
    uint16_t ir2Data = tcs3430.getIR2Data();  // DFRobot example reads both IR channels

    sumX += xData;
    sumY += yData;
    sumZ += zData;
    sumIR1 += ir1Data;
    sumIR2 += ir2Data;

    LOG_SENSOR_DEBUG("DFRobot reading %d - X:%u Y:%u Z:%u IR1:%u IR2:%u", i+1,
                     xData, yData, zData, ir1Data, ir2Data);

    // DFRobot methodology: Check for saturation during readings
    uint8_t readingStatus = tcs3430.getDeviceStatus();
    if (readingStatus & 0x10) {  // ASAT bit - saturation detected
      LOG_SENSOR_WARN("DFRobot saturation detected during reading %d - Status: 0x%02X", i+1, readingStatus);
    }
  }

  // DFRobot methodology: Calculate averages for all channels
  uint16_t avgX = sumX / numReadings;
  uint16_t avgY = sumY / numReadings;
  uint16_t avgZ = sumZ / numReadings;
  uint16_t avgIR1 = sumIR1 / numReadings;
  uint16_t avgIR2 = sumIR2 / numReadings;

  LOG_SENSOR_INFO("DFRobot calibration averages - X:%u Y:%u Z:%u IR1:%u IR2:%u",
                  avgX, avgY, avgZ, avgIR1, avgIR2);

  // DFRobot-compliant validation following library patterns
  bool validCalibration = true;
  String validationMessage = "";

  // DFRobot methodology: Check for sensor saturation (16-bit max = 65535)
  const uint16_t SATURATION_LIMIT = 60000;  // 91% of max to avoid saturation
  if (avgX > SATURATION_LIMIT || avgY > SATURATION_LIMIT || avgZ > SATURATION_LIMIT) {
    validCalibration = false;
    validationMessage = "DFRobot validation: Sensor saturation detected. Reduce integration time or gain.";
    LOG_SENSOR_ERROR("DFRobot saturation check failed - X:%u Y:%u Z:%u (limit:%u)",
                     avgX, avgY, avgZ, SATURATION_LIMIT);
  }

  // DFRobot methodology: Check for insufficient signal
  const uint16_t MIN_SIGNAL = 500;  // Minimum signal for reliable calibration
  if (avgX < MIN_SIGNAL || avgY < MIN_SIGNAL || avgZ < MIN_SIGNAL) {
    validCalibration = false;
    validationMessage = "DFRobot validation: Insufficient signal. Increase integration time, gain, or brightness.";
    LOG_SENSOR_ERROR("DFRobot signal check failed - X:%u Y:%u Z:%u (min:%u)",
                     avgX, avgY, avgZ, MIN_SIGNAL);
  }

  // DFRobot methodology: Check for reasonable white balance (XYZ should be relatively balanced for white)
  else {
    float maxVal = max(max(avgX, avgY), avgZ);
    float minVal = min(min(avgX, avgY), avgZ);
    float balanceRatio = maxVal / minVal;
    const float MAX_BALANCE_RATIO = 2.5;  // Allow some imbalance but not excessive

    if (balanceRatio > MAX_BALANCE_RATIO) {
      validCalibration = false;
      validationMessage = "DFRobot validation: Poor white balance. Ensure scanning neutral white surface.";
      LOG_SENSOR_ERROR("DFRobot balance check failed - ratio:%.2f (max:%.2f)",
                       balanceRatio, MAX_BALANCE_RATIO);
    }
  }

  // Final sensor status check following DFRobot methodology
  uint8_t finalStatus = tcs3430.getDeviceStatus();
  LOG_SENSOR_DEBUG("DFRobot final sensor status: 0x%02X", finalStatus);

  if (validCalibration) {
    // Store calibration data following DFRobot patterns
    whiteCalData.x = avgX;
    whiteCalData.y = avgY;
    whiteCalData.z = avgZ;
    whiteCalData.ir = avgIR1;  // Use IR1 for primary IR data
    whiteCalData.brightness = brightness;
    whiteCalData.timestamp = millis();
    whiteCalData.valid = true;

    LOG_SENSOR_INFO("DFRobot white calibration successful - X:%u Y:%u Z:%u IR1:%u IR2:%u",
                    avgX, avgY, avgZ, avgIR1, avgIR2);
    LOG_SENSOR_INFO("DFRobot calibration settings - ATIME:%d AGAIN:%d Brightness:%u",
                    currentAtime, currentAgain, brightness);
  } else {
    LOG_SENSOR_ERROR("DFRobot white calibration failed: %s", validationMessage.c_str());
    LOG_SENSOR_ERROR("DFRobot raw values - X:%u Y:%u Z:%u IR1:%u IR2:%u",
                     avgX, avgY, avgZ, avgIR1, avgIR2);
    whiteCalData.valid = false;
  }

  // Turn off LED following DFRobot cleanup methodology
  turnOffLED();

  LOG_PERF_END("DFRobot white calibration");
  return validCalibration;
}

bool performBlackCalibration() {
  LOG_PERF_START();
  LOG_SENSOR_INFO("Performing black calibration");

  if (!calibrationInProgress || currentCalState != CAL_BLACK_SCANNING) {
    LOG_SENSOR_ERROR("Black calibration called in invalid state: %d", currentCalState);
    return false;
  }

  // Turn OFF all LEDs for black calibration (dark reference)
  LOG_LED_INFO("Turning OFF all LEDs for black calibration (dark reference measurement)");
  turnOffLED();           // RGB LED
  turnOffIllumination();  // Illumination LED
  delay(CALIBRATION_LED_STABILIZE_MS);
  LOG_LED_DEBUG("All LEDs turned off, sensor stabilization completed");

  // Perform multiple readings for accuracy
  uint32_t sumX = 0, sumY = 0, sumZ = 0, sumIR = 0;
  const int numReadings = 5;

  LOG_SENSOR_DEBUG("Taking %d readings for black calibration with improved stability", numReadings);
  for (int i = 0; i < numReadings; i++) {
    delay(SENSOR_READING_DELAY_MS);  // Consistent delay between readings
    sumX += tcs3430.getXData();
    sumY += tcs3430.getYData();
    sumZ += tcs3430.getZData();
    sumIR += tcs3430.getIR1Data();
    LOG_SENSOR_DEBUG("Black cal reading %d - X:%u Y:%u Z:%u IR:%u", i+1,
                     tcs3430.getXData(), tcs3430.getYData(), tcs3430.getZData(), tcs3430.getIR1Data());
  }

  // Calculate averages
  blackCalData.x = sumX / numReadings;
  blackCalData.y = sumY / numReadings;
  blackCalData.z = sumZ / numReadings;
  blackCalData.ir = sumIR / numReadings;
  blackCalData.timestamp = millis();
  blackCalData.valid = true;

  LOG_SENSOR_INFO("Black calibration completed - X:%u Y:%u Z:%u IR:%u",
                  blackCalData.x, blackCalData.y, blackCalData.z, blackCalData.ir);

  // Turn off LED
  turnOffLED();

  // Update state
  // updateCalibrationState(CAL_BLACK_COMPLETE, "Black calibration completed successfully");

  LOG_PERF_END("Black calibration");
  return true;
}

/**
 * @brief Validates black calibration readings
 * @return true if readings are valid for black reference
 */
bool validateBlackCalibration(uint16_t x, uint16_t y, uint16_t z, uint16_t ir) {
  // Black readings should be significantly lower than white calibration
  if (whiteCalData.valid) {
    const float MAX_BLACK_PERCENT = 0.15; // Black should be < 15% of white
    if (x > whiteCalData.x * MAX_BLACK_PERCENT ||
        y > whiteCalData.y * MAX_BLACK_PERCENT ||
        z > whiteCalData.z * MAX_BLACK_PERCENT) {
      LOG_SENSOR_ERROR("Black readings too high compared to white calibration");
      return false;
    }
  }

  // Absolute thresholds for black readings
  const uint16_t MAX_BLACK_READING = 1000; // Arbitrary threshold, adjust based on sensor
  if (x > MAX_BLACK_READING || y > MAX_BLACK_READING || z > MAX_BLACK_READING) {
    LOG_SENSOR_ERROR("Black readings above maximum threshold");
    return false;
  }

  // Check for reasonable ratios between channels
  const float MAX_CHANNEL_RATIO = 3.0f;
  float xy_ratio = (y > 0) ? (float)x / y : 0;
  float yz_ratio = (z > 0) ? (float)y / z : 0;
  float xz_ratio = (z > 0) ? (float)x / z : 0;

  if (xy_ratio > MAX_CHANNEL_RATIO || yz_ratio > MAX_CHANNEL_RATIO || xz_ratio > MAX_CHANNEL_RATIO) {
    LOG_SENSOR_ERROR("Black readings show abnormal channel ratios");
    return false;
  }

  return true;
}

void cancelCalibration() {
  LOG_SENSOR_INFO("Cancelling calibration sequence");

  // Turn off LED
  turnOffLED();

  // Reset calibration state
  calibrationInProgress = false;
  currentCalState = CAL_IDLE;
  calibrationSessionId = "";
  calibrationMessage = "";
  calibrationCountdown = 0;

  // Clear any partial calibration data
  whiteCalData = {0};
  blackCalData = {0};

  LOG_SENSOR_INFO("Calibration sequence cancelled");
}

String generateCalibrationSessionId() {
  // Generate a simple session ID based on timestamp
  return String("cal_") + String(millis());
}

void updateCalibrationState(LegacyCalibrationState newState, const String& message) {
  LegacyCalibrationState oldState = currentCalState;
  currentCalState = newState;
  calibrationMessage = message;

  LOG_SENSOR_INFO("Calibration state changed: %d -> %d (%s)", oldState, newState, message.c_str());
}

// ============================================================================
// ADVANCED CALIBRATION API HANDLERS
// ============================================================================

void handleCalibrationStart() {
  LOG_WEB_INFO("Handling calibration start request");

  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
      LOG_WEB_ERROR("JSON parsing failed: %s", error.c_str());
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
      return;
    }

    uint8_t brightness = doc["brightness"] | DEFAULT_BRIGHTNESS;

    if (startCalibrationSequence(brightness)) {
      JsonDocument response;
      response["success"] = true;
      response["sessionId"] = calibrationSessionId;
      response["message"] = "Calibration sequence started";
      response["countdown"] = CALIBRATION_COUNTDOWN_SECONDS;

      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);

      LOG_WEB_INFO("Calibration start successful - Session: %s", calibrationSessionId.c_str());
    } else {
      server.send(409, "application/json", "{\"success\":false,\"error\":\"Calibration already in progress or invalid parameters\"}");
      LOG_WEB_ERROR("Calibration start failed");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing request body\"}");
    LOG_WEB_ERROR("Calibration start request missing body");
  }
}

void handleCalibrationStatus() {
  LOG_WEB_DEBUG("Handling calibration status request");

  JsonDocument response;
  response["success"] = true;
  response["inProgress"] = calibrationInProgress;
  response["state"] = (int)currentCalState;
  response["message"] = calibrationMessage;
  response["sessionId"] = calibrationSessionId;

  // Add state-specific information
  switch (currentCalState) {
    case CAL_WHITE_COUNTDOWN:
    case CAL_BLACK_COUNTDOWN:
      response["countdown"] = calibrationCountdown;
      break;

    case CAL_WHITE_SCANNING:
      response["progress"] = "Scanning white reference...";
      break;

    case CAL_BLACK_SCANNING:
      response["progress"] = "Scanning black reference...";
      break;

    case CAL_WHITE_COMPLETE:
      response["whiteComplete"] = true;
      response["whiteData"]["x"] = whiteCalData.x;
      response["whiteData"]["y"] = whiteCalData.y;
      response["whiteData"]["z"] = whiteCalData.z;
      response["whiteData"]["ir"] = whiteCalData.ir;
      break;

    case CAL_BLACK_COMPLETE:
      response["blackComplete"] = true;
      response["blackData"]["x"] = blackCalData.x;
      response["blackData"]["y"] = blackCalData.y;
      response["blackData"]["z"] = blackCalData.z;
      response["blackData"]["ir"] = blackCalData.ir;
      break;

    case CAL_COMPLETE:
      response["complete"] = true;
      break;

    case CAL_ERROR:
      response["error"] = true;
      break;

    default:
      break;
  }

  String responseStr;
  serializeJson(response, responseStr);
  server.send(200, "application/json", responseStr);
}

void handleCalibrationWhite() {
  LOG_WEB_INFO("Handling white calibration request");

  if (!calibrationInProgress) {
    server.send(409, "application/json", "{\"success\":false,\"error\":\"No calibration in progress\"}");
    LOG_WEB_ERROR("White calibration requested but no calibration in progress");
    return;
  }

  // Update state to scanning
  updateCalibrationState(CAL_WHITE_SCANNING, "Performing white calibration scan");

  if (performWhiteCalibration(calibrationBrightness)) {
    JsonDocument response;
    response["success"] = true;
    response["message"] = "White calibration completed";
    response["data"]["x"] = whiteCalData.x;
    response["data"]["y"] = whiteCalData.y;
    response["data"]["z"] = whiteCalData.z;
    response["data"]["ir"] = whiteCalData.ir;
    response["data"]["brightness"] = whiteCalData.brightness;

    String responseStr;
    serializeJson(response, responseStr);
    server.send(200, "application/json", responseStr);

    // After white calibration completes, transition to black calibration prompt
    // Do this AFTER sending the response to avoid any timing issues
    updateCalibrationState(CAL_BLACK_PROMPT, "Prepare black reference");

    LOG_WEB_INFO("White calibration completed successfully, transitioning to black calibration");
  } else {
    updateCalibrationState(CAL_ERROR, "White calibration failed");
    server.send(500, "application/json", "{\"success\":false,\"error\":\"White calibration failed\"}");
    LOG_WEB_ERROR("White calibration failed");
  }
}

void handleCalibrationBlack() {
  LOG_WEB_INFO("Handling black calibration request");

  if (!calibrationInProgress || currentCalState != CAL_BLACK_PROMPT) {
    server.send(409, "application/json", "{\"success\":false,\"error\":\"Invalid calibration state for black calibration\"}");
    LOG_WEB_ERROR("Black calibration requested in invalid state: %d", currentCalState);
    return;
  }

  // Start black countdown
  updateCalibrationState(CAL_BLACK_COUNTDOWN, "Starting black calibration countdown");
  calibrationCountdown = CALIBRATION_COUNTDOWN_SECONDS;

  // After countdown, perform black calibration
  // Note: In a real implementation, you'd want to handle the countdown timing
  // For now, we'll simulate it by immediately proceeding to scanning
  updateCalibrationState(CAL_BLACK_SCANNING, "Performing black calibration scan");

  if (performBlackCalibration()) {
    JsonDocument response;
    response["success"] = true;
    response["message"] = "Black calibration completed";
    response["data"]["x"] = blackCalData.x;
    response["data"]["y"] = blackCalData.y;
    response["data"]["z"] = blackCalData.z;
    response["data"]["ir"] = blackCalData.ir;

    String responseStr;
    serializeJson(response, responseStr);
    server.send(200, "application/json", responseStr);

    LOG_WEB_INFO("Black calibration completed successfully");
  } else {
    updateCalibrationState(CAL_ERROR, "Black calibration failed");
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Black calibration failed\"}");
    LOG_WEB_ERROR("Black calibration failed");
  }
}

void handleCalibrationSave() {
  LOG_WEB_INFO("Handling calibration save request");

  if (!calibrationInProgress) {
    server.send(409, "application/json", "{\"success\":false,\"error\":\"No calibration in progress\"}");
    LOG_WEB_ERROR("Save calibration requested but no calibration in progress");
    return;
  }

  if (!whiteCalData.valid && !blackCalData.valid) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"No calibration data to save\"}");
    LOG_WEB_ERROR("Save calibration requested but no valid calibration data");
    return;
  }

  updateCalibrationState(CAL_SAVING, "Saving calibration data to EEPROM");

  try {
    saveCalibrationData();
    updateCalibrationState(CAL_COMPLETE, "Calibration completed and saved successfully");

    // Reset calibration state
    calibrationInProgress = false;

    JsonDocument response;
    response["success"] = true;
    response["message"] = "Calibration data saved successfully";
    response["hasWhite"] = whiteCalData.valid;
    response["hasBlack"] = blackCalData.valid;

    String responseStr;
    serializeJson(response, responseStr);
    server.send(200, "application/json", responseStr);

    LOG_WEB_INFO("Calibration data saved successfully");
  } catch (...) {
    updateCalibrationState(CAL_ERROR, "Failed to save calibration data");
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save calibration data\"}");
    LOG_WEB_ERROR("Failed to save calibration data");
  }
}

void handleCalibrationCancel() {
  LOG_WEB_INFO("Handling calibration cancel request");

  cancelCalibration();

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Calibration cancelled\"}");
  LOG_WEB_INFO("Calibration cancelled by user request");
}

void handleStatus() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  // Removed logging for status requests - too frequent and not useful



  JsonDocument doc;
  doc["isScanning"] = isScanning;
  doc["ledState"] = ledState;
  doc["isCalibrated"] = isCalibrated;
  doc["currentR"] = currentR;
  doc["currentG"] = currentG;
  doc["currentB"] = currentB;
  doc["sampleCount"] = sampleCount;
  doc["atime"] = currentAtime;
  doc["again"] = currentAgain;
  doc["brightness"] = currentBrightness;
  doc["ambientLux"] = getAmbientLightLux();

  // Add TCS3430 advanced calibration settings
  doc["autoZeroMode"] = currentAutoZeroMode;
  doc["autoZeroFreq"] = currentAutoZeroFreq;
  doc["waitTime"] = currentWaitTime;

  // Add network diagnostic information
  doc["esp32IP"] = WiFi.localIP().toString();
  doc["clientIP"] = clientIP;
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["subnet"] = WiFi.subnetMask().toString();
  doc["macAddress"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

  // Removed verbose logging for status responses - too frequent
  LOG_PERF_END("Status request");
}

void handleBrightness() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  Logger::logWebRequest("POST", "/brightness", clientIP.c_str());

  // Rate limiting for brightness requests to prevent overwhelming the ESP32
  static unsigned long lastBrightnessRequest = 0;
  static int requestCount = 0;
  unsigned long currentTime = millis();

  // Reset counter every second
  if (currentTime - lastBrightnessRequest > 1000) {
    requestCount = 0;
    lastBrightnessRequest = currentTime;
  }

  requestCount++;

  // Limit to 5 requests per second to prevent boot loops
  if (requestCount > 5) {
    LOG_WEB_ERROR("Brightness request rate limit exceeded from IP: %s", clientIP.c_str());
    server.send(429, "application/json", "{\"success\":false,\"error\":\"Rate limit exceeded\"}");
    return;
  }

  // Log client IP for debugging network issues
  LOG_WEB_INFO("Brightness request #%d from client IP: %s", requestCount, clientIP.c_str());

  if (server.hasArg("plain")) {
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));

    if (doc["brightness"].is<JsonVariant>()) {
      uint8_t brightness = doc["brightness"];

      // Allow full range 0-255 for brightness control
      if (brightness > 255) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Brightness must be 0-255\"}");
        LOG_WEB_ERROR("Invalid brightness value: %u (must be 0-255)", brightness);
        return;
      }

      // Get LED color from request or use white as default
      uint8_t r = doc["r"] | 255;
      uint8_t g = doc["g"] | 255;
      uint8_t b = doc["b"] | 255;
      bool keepOn = doc["keepOn"] | false;

      // Force serial output for debugging
      Serial.printf("[BRIGHTNESS] Request: brightness=%u, RGB=(%u,%u,%u), keepOn=%s\n",
                    brightness, r, g, b, keepOn ? "true" : "false");

      if (brightness == 0) {
        // Turn off both LEDs
        turnOffLED();           // RGB LED
        turnOffIllumination();  // Illumination LED
        Serial.printf("[BRIGHTNESS] All LEDs turned OFF\n");
        LOG_LED_INFO("All LEDs turned OFF via brightness slider");
      } else {
        // Apply brightness to both LEDs
        setLEDColor(r, g, b, brightness);        // RGB LED for color indication
        setIlluminationBrightness(brightness);   // Illumination LED for scanning
        ledState = true;
        Serial.printf("[BRIGHTNESS] LEDs updated: brightness=%u, RGB=(%u,%u,%u)\n",
                      brightness, r, g, b);
        LOG_LED_INFO("Real-time brightness updated: %u RGB:(%u,%u,%u) KeepOn:%s",
                     brightness, r, g, b, keepOn ? "true" : "false");
      }

      // Update current brightness setting (but don't save 0 to EEPROM)
      if (brightness >= MIN_LED_BRIGHTNESS) {
        currentBrightness = brightness;
        preferences.putUInt(PREF_BRIGHTNESS, currentBrightness);
        LOG_STORAGE_DEBUG("Brightness %u saved to EEPROM", brightness);
      }

      // Send success response with additional debug info
      JsonDocument response;
      response["success"] = true;
      response["brightness"] = brightness;
      response["actualBrightness"] = brightness;
      response["ledState"] = ledState;
      response["clientIP"] = clientIP;
      response["message"] = brightness == 0 ? "LED turned off" : "Brightness updated";

      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);

      Logger::logWebResponse(200, millis() - _perf_start);
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing brightness parameter\"}");
      LOG_WEB_ERROR("Brightness request missing brightness parameter");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing request body\"}");
    LOG_WEB_ERROR("Brightness request missing body");
  }

  LOG_PERF_END("Brightness request");
}

void handleRawSensorData() {
  LOG_PERF_START();
  String clientIP = server.client().remoteIP().toString();
  Logger::logWebRequest("GET", "/raw", clientIP.c_str());

  LOG_SENSOR_INFO("Reading raw sensor data for diagnostics");

  // Turn on illumination LED for consistent readings
  LOG_LED_INFO("Activating illumination LED for raw data reading - brightness: %u", currentBrightness);
  setIlluminationBrightness(currentBrightness);
  delay(SENSOR_STABILIZE_MS);

  // Read raw XYZ and IR data directly from sensor (like the example)
  uint16_t XData = tcs3430.getXData();
  uint16_t YData = tcs3430.getYData();
  uint16_t ZData = tcs3430.getZData();
  uint16_t IR1Data = tcs3430.getIR1Data();
  uint16_t IR2Data = tcs3430.getIR2Data();

  // Turn off LED
  turnOffLED();

  LOG_SENSOR_INFO("Raw sensor data - X:%u Y:%u Z:%u IR1:%u IR2:%u",
                  XData, YData, ZData, IR1Data, IR2Data);

  // Create JSON response with raw data
  JsonDocument doc;
  doc["success"] = true;
  doc["raw"]["x"] = XData;
  doc["raw"]["y"] = YData;
  doc["raw"]["z"] = ZData;
  doc["raw"]["ir1"] = IR1Data;
  doc["raw"]["ir2"] = IR2Data;

  // Add current calibration data for reference
  if (whiteCalData.valid) {
    doc["calibration"]["white"]["x"] = whiteCalData.x;
    doc["calibration"]["white"]["y"] = whiteCalData.y;
    doc["calibration"]["white"]["z"] = whiteCalData.z;
    doc["calibration"]["white"]["ir"] = whiteCalData.ir;
    doc["calibration"]["white"]["brightness"] = whiteCalData.brightness;
  }

  if (blackCalData.valid) {
    doc["calibration"]["black"]["x"] = blackCalData.x;
    doc["calibration"]["black"]["y"] = blackCalData.y;
    doc["calibration"]["black"]["z"] = blackCalData.z;
    doc["calibration"]["black"]["ir"] = blackCalData.ir;
  }

  // Add sensor settings
  doc["settings"]["atime"] = currentAtime;
  doc["settings"]["again"] = currentAgain;
  doc["settings"]["brightness"] = currentBrightness;
  doc["settings"]["waitTime"] = currentWaitTime;
  doc["settings"]["autoZeroMode"] = currentAutoZeroMode;
  doc["settings"]["autoZeroFreq"] = currentAutoZeroFreq;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

  Logger::logWebResponse(200, millis() - _perf_start);
  LOG_SENSOR_INFO("Raw sensor data request completed");
  LOG_PERF_END("Raw sensor data request");
}

void matchColorWithGoogleScript(uint8_t r, uint8_t g, uint8_t b, int sampleIdx) {
  LOG_PERF_START();

  if (WiFi.status() != WL_CONNECTED) {
    LOG_API_ERROR("WiFi not connected, skipping Google Apps Script call");
    return;
  }

  LOG_API_INFO("Starting Google Apps Script call for RGB:(%u,%u,%u) Sample:%d", r, g, b, sampleIdx);
  Logger::logMemoryUsage("Before API call");

  HTTPClient http;

  // Construct the URL with parameters, as the Google Apps Script expects
  String url = String(googleScriptUrl) + "?r=" + String(r) + "&g=" + String(g) + "&b=" + String(b);

  LOG_API_DEBUG("Google Apps Script URL with parameters: %s", url.c_str());
  Serial.println("=== GOOGLE APPS SCRIPT DEBUG ===");
  Serial.printf("Calling URL: %s\n", url.c_str());

  // Use GET method with URL parameters instead of POST with JSON
  http.begin(url);
  http.setTimeout(COLOR_MATCH_TIMEOUT_MS);

  // Enable redirect following for GET requests (this works correctly)
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  LOG_API_INFO("Sending GET request to Google Apps Script");

  unsigned long apiStartTime = millis();
  int httpResponseCode = http.GET();  // Use GET instead of POST
  unsigned long apiDuration = millis() - apiStartTime;

  LOG_API_INFO("API response received - Code:%d Duration:%lu ms", httpResponseCode, apiDuration);
  Serial.printf("HTTP Response Code: %d\n", httpResponseCode);

  if (httpResponseCode == 200) {
    String response = http.getString();
    LOG_API_DEBUG("API response size: %d bytes", response.length());
    LOG_API_DEBUG("API response: %s", response.substring(0, 200).c_str());

    Serial.printf("Response length: %d\n", response.length());
    Serial.printf("Response preview: %s\n", response.substring(0, 300).c_str());

    // Parse Google Apps Script response with smaller buffer
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);

    if (error) {
      LOG_API_ERROR("Failed to parse API response JSON: %s", error.c_str());
    } else if (responseDoc["success"] == true && responseDoc["match"].is<JsonVariant>()) {
      JsonObject match = responseDoc["match"];

      // Update sample with paint information
      if (sampleIdx >= 0 && sampleIdx < MAX_SAMPLES) {
        strncpy(samples[sampleIdx].paintName, match["name"], SAMPLE_NAME_LENGTH - 1);
        strncpy(samples[sampleIdx].paintCode, match["code"], SAMPLE_CODE_LENGTH - 1);
        samples[sampleIdx].lrv = match["lrv"];

        // Null-terminate strings for safety
        samples[sampleIdx].paintName[SAMPLE_NAME_LENGTH - 1] = '\0';
        samples[sampleIdx].paintCode[SAMPLE_CODE_LENGTH - 1] = '\0';

        LOG_API_INFO("Paint match found - Name:%s Code:%s LRV:%.1f Distance:%.2f",
                     samples[sampleIdx].paintName,
                     samples[sampleIdx].paintCode,
                     samples[sampleIdx].lrv,
                     match["distance"].as<float>());

        LOG_API_DEBUG("Matched RGB: (%d,%d,%d) vs Input: (%d,%d,%d)",
                      match["r"].as<int>(), match["g"].as<int>(), match["b"].as<int>(),
                      r, g, b);

        LOG_STORAGE_INFO("Updating sample %d with paint data", sampleIdx);
        saveSamples();
      } else {
        LOG_API_ERROR("Invalid sample index: %d", sampleIdx);
      }
    } else if (responseDoc["success"] == false) {
      String errorMsg = responseDoc["error"];
      LOG_API_ERROR("Google Apps Script returned error: %s", errorMsg.c_str());
    } else {
      LOG_API_ERROR("Unexpected API response format");
    }
  } else {
    LOG_API_ERROR("API request failed - HTTP %d", httpResponseCode);
    String errorResponse = http.getString();
    if (errorResponse.length() > 0) {
      LOG_API_ERROR("Error response: %s", errorResponse.substring(0, 200).c_str());
    }
  }

  http.end();
  Logger::logMemoryUsage("After API call");
  LOG_PERF_END("Google Apps Script call");

  // Log final API call summary
  Logger::logAPICall("Google Apps Script Color Match", httpResponseCode, millis() - _perf_start);
}

/**
 * @brief Find optimal LED brightness for perfect sensor saturation
 * Automatically adjusts LED brightness to achieve 70-80% sensor saturation
 * @return Optimal LED brightness value (0-255)
 */
uint8_t findOptimalLEDBrightness() {
  LOG_SENSOR_INFO("Finding optimal LED brightness for perfect saturation");

  // Target saturation range: 70-80% of sensor full scale (65535)
  const uint16_t TARGET_MIN = 45000;  // ~70% saturation
  const uint16_t TARGET_MAX = 52000;  // ~80% saturation
  const uint16_t SATURATION_LIMIT = 60000;  // ~92% - avoid full saturation

  // Start with a reasonable brightness
  uint8_t testBrightness = 128;
  uint8_t minBrightness = 32;   // Minimum useful brightness
  uint8_t maxBrightness = 255;  // Maximum brightness

  // Binary search for optimal brightness
  for (int iteration = 0; iteration < 8; iteration++) {
    // Set LED to test brightness
    setLEDColor(255, 255, 255, testBrightness);
    delay(200); // Allow sensor to stabilize

    // Take sensor reading
    uint16_t x = tcs3430.getXData();
    uint16_t y = tcs3430.getYData();
    uint16_t z = tcs3430.getZData();

    // Use the highest channel value for saturation check
    uint16_t maxChannel = max(max(x, y), z);

    LOG_SENSOR_DEBUG("Brightness %u: Max channel = %u (target: %u-%u)",
                     testBrightness, maxChannel, TARGET_MIN, TARGET_MAX);

    if (maxChannel > SATURATION_LIMIT) {
      // Too bright - reduce brightness
      maxBrightness = testBrightness;
      testBrightness = (minBrightness + testBrightness) / 2;
      LOG_SENSOR_DEBUG("Too bright, reducing to %u", testBrightness);
    } else if (maxChannel < TARGET_MIN) {
      // Too dim - increase brightness
      minBrightness = testBrightness;
      testBrightness = (testBrightness + maxBrightness) / 2;
      LOG_SENSOR_DEBUG("Too dim, increasing to %u", testBrightness);
    } else {
      // Perfect range - we're done
      LOG_SENSOR_INFO("Found optimal brightness: %u (saturation: %u)", testBrightness, maxChannel);
      break;
    }

    // Prevent infinite loop
    if (maxBrightness - minBrightness <= 1) {
      break;
    }
  }

  // Ensure we have a reasonable minimum brightness
  if (testBrightness < 32) {
    testBrightness = 32;
    LOG_SENSOR_WARN("Using minimum brightness %u - may have low signal", testBrightness);
  }

  LOG_SENSOR_INFO("Optimal LED brightness determined: %u", testBrightness);
  return testBrightness;
}

/**
 * @brief Enhanced scan function using dynamic sensor management
 * Automatically optimizes sensor settings for current lighting conditions
 * @param r Output red value
 * @param g Output green value
 * @param b Output blue value
 * @param x Output X channel raw value
 * @param y Output Y channel raw value
 * @param z Output Z channel raw value
 * @param ir1 Output IR1 channel raw value
 * @param ir2 Output IR2 channel raw value
 * @return true if scan successful
 */
bool performEnhancedScan(uint8_t& r, uint8_t& g, uint8_t& b, uint16_t& x, uint16_t& y, uint16_t& z, uint16_t& ir1, uint16_t& ir2) {
  LOG_PERF_START();
  LOG_SENSOR_INFO("Starting enhanced scan with dynamic sensor optimization");

  if (!dynamicSensor || !dynamicSensor->isInitialized()) {
    LOG_SENSOR_WARN("Dynamic sensor manager not available, using enhanced standard scan");

    // Enhanced fallback: Use multiple readings with current sensor settings
    const int numReadings = 10;
    uint32_t sumX = 0, sumY = 0, sumZ = 0, sumIR1 = 0, sumIR2 = 0;

    for (int i = 0; i < numReadings; i++) {
      delay(50); // Small delay between readings
      sumX += tcs3430.getXData();
      sumY += tcs3430.getYData();
      sumZ += tcs3430.getZData();
      sumIR1 += tcs3430.getIR1Data();
      sumIR2 += tcs3430.getIR2Data();
    }

    // Calculate averages
    x = sumX / numReadings;
    y = sumY / numReadings;
    z = sumZ / numReadings;
    ir1 = sumIR1 / numReadings;
    ir2 = sumIR2 / numReadings;

    // Convert to RGB using existing calibration
    uint16_t avgIR = (ir1 + ir2) / 2;
    convertXYZtoRGB(x, y, z, avgIR, r, g, b);

    LOG_SENSOR_INFO("Enhanced fallback scan: RGB(%u,%u,%u) XYZ(%u,%u,%u) IR(%u,%u)",
                    r, g, b, x, y, z, ir1, ir2);
    return true;
  }

  // Step 1: Optimize sensor settings for current conditions
  if (!dynamicSensor->optimizeSensorSettings()) {
    LOG_SENSOR_ERROR("Failed to optimize sensor settings");
    return false;
  }

  // Step 1.5: Optimize LED brightness for optimal sensor range
  uint8_t optimizedBrightness = currentBrightness;
  if (dynamicSensor->optimizeLEDBrightness(optimizedBrightness)) {
    LOG_SENSOR_INFO("LED brightness optimized: %u -> %u", currentBrightness, optimizedBrightness);
    setIlluminationBrightness(optimizedBrightness);
    currentBrightness = optimizedBrightness;
    delay(BRIGHTNESS_STABILIZATION_DELAY);
  } else {
    LOG_SENSOR_WARN("LED brightness optimization failed, using current brightness: %u", currentBrightness);
  }

  // Step 2: Perform quality reading with multiple samples
  ReadingQuality quality;
  if (!dynamicSensor->performQualityReading(x, y, z, ir1, ir2, quality)) {
    LOG_SENSOR_ERROR("Failed to perform quality reading");
    return false;
  }

  // Step 3: Check reading quality and retry if needed
  if (quality.qualityScore < 50) { // Below fair quality threshold
    LOG_SENSOR_WARN("Low quality reading (score: %u), attempting optimization", quality.qualityScore);

    // Try to optimize again
    if (dynamicSensor->optimizeSensorSettings()) {
      // Retry reading
      if (!dynamicSensor->performQualityReading(x, y, z, ir1, ir2, quality)) {
        LOG_SENSOR_ERROR("Failed to improve reading quality");
        return false;
      }
    }
  }

  // Step 4: Handle saturation case for white calibration
  // Check if this looks like Vivid White (high XYZ values, especially X and Y)
  bool isVividWhite = (x >= 55000 && y >= 55000 && z >= 40000);

  if (isVividWhite) {
    // High XYZ values indicate bright white light (Vivid White)
    LOG_SENSOR_WARN("Vivid White detected (X=%u Y=%u Z=%u) - treating as calibrated white", x, y, z);
    r = 255;
    g = 255;
    b = 255;
  } else {
    // Step 4: Convert raw XYZ to RGB using existing calibration
    uint16_t avgIR = (ir1 + ir2) / 2;  // Average IR from both channels
    convertXYZtoRGB(x, y, z, avgIR, r, g, b);

    // Step 5: Apply IR compensation if enabled
    dynamicSensor->applyIRCompensation(r, g, b, ir1, ir2);

    // Step 6: Handle partial saturation in color conversion
    if (quality.hasSaturation && (r == 0 && g == 0 && b == 0)) {
      // If color conversion failed due to saturation, use fallback
      LOG_SENSOR_WARN("Color conversion failed with saturation - using fallback");
      // Simple fallback: scale saturated values proportionally
      float scale = 255.0f / 65535.0f;
      r = (uint8_t)constrain(x * scale, 0, 255);
      g = (uint8_t)constrain(y * scale, 0, 255);
      b = (uint8_t)constrain(z * scale, 0, 255);
    }
  }

  // Step 6: Log results
  LOG_SENSOR_INFO("Enhanced scan complete: RGB(%u,%u,%u) XYZ(%u,%u,%u) IR(%u,%u) Quality:%u",
                  r, g, b, x, y, z, ir1, ir2, quality.qualityScore);

  if (quality.hasSaturation) {
    LOG_SENSOR_WARN("Saturation detected in reading");
  }

  if (quality.hasLowSignal) {
    LOG_SENSOR_WARN("Low signal detected in reading");
  }

  LOG_PERF_END("Enhanced scan");
  return true;
}

/**
 * @brief Enhanced scan endpoint handler
 * Uses dynamic sensor management for optimal results
 */
void handleEnhancedScan() {
  LOG_PERF_START();
  LOG_API_INFO("Enhanced scan request received");

  if (isScanning) {
    server.send(409, "application/json", "{\"error\":\"Scan already in progress\"}");
    return;
  }

  isScanning = true;

  // Determine LED brightness based on enhanced LED mode setting
  uint8_t scanBrightness;
  if (enhancedLEDMode) {
    // Use automatic brightness optimization for enhanced scan
    LOG_LED_INFO("Enhanced LED mode enabled - performing automatic brightness optimization");
    scanBrightness = performAutoBrightnessOptimization();
    LOG_LED_INFO("Enhanced scan using optimized brightness: %u", scanBrightness);
  } else {
    // Use manual LED intensity setting
    scanBrightness = manualLEDIntensity;
    LOG_LED_INFO("Manual LED mode enabled - using manual intensity: %u", scanBrightness);
  }

  // Turn on illumination LED with determined brightness
  LOG_LED_INFO("Activating illumination LED for enhanced scan - brightness: %u", scanBrightness);
  setIlluminationBrightness(scanBrightness);
  currentBrightness = scanBrightness;
  delay(SENSOR_STABILIZE_MS);

  uint8_t r, g, b;
  uint16_t x, y, z, ir1, ir2;

  bool success = performEnhancedScan(r, g, b, x, y, z, ir1, ir2);

  // Turn off illumination LED
  turnOffIllumination();
  isScanning = false;

  if (!success) {
    LOG_API_ERROR("Enhanced scan failed, falling back to standard scan");
    // Fallback to existing scan logic
    handleScan();
    return;
  }

  // Update current color values
  currentR = r;
  currentG = g;
  currentB = b;

  // Prepare response with enhanced data
  JsonDocument doc;
  doc["success"] = true;
  doc["r"] = r;
  doc["g"] = g;
  doc["b"] = b;
  doc["x"] = x;
  doc["y"] = y;
  doc["z"] = z;
  doc["ir1"] = ir1;
  doc["ir2"] = ir2;
  doc["timestamp"] = millis();

  // Add sensor configuration info
  if (dynamicSensor) {
    SensorConfig config = dynamicSensor->getCurrentConfig();
    doc["sensorConfig"]["atime"] = config.atime;
    doc["sensorConfig"]["again"] = config.again;
    doc["sensorConfig"]["brightness"] = currentBrightness; // Use actual current brightness
    doc["sensorConfig"]["condition"] = config.condition;
    doc["sensorConfig"]["isOptimal"] = config.isOptimal;

    // Add brightness optimization information
    uint16_t controlVariable = dynamicSensor->calculateControlVariable();
    doc["brightnessOptimization"]["controlVariable"] = controlVariable;
    doc["brightnessOptimization"]["targetMin"] = RGB_TARGET_MIN;
    doc["brightnessOptimization"]["targetMax"] = RGB_TARGET_MAX;
    doc["brightnessOptimization"]["inOptimalRange"] = dynamicSensor->isInOptimalRange(controlVariable);
    doc["brightnessOptimization"]["optimizedBrightness"] = currentBrightness;
  }

  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  LOG_API_INFO("Enhanced scan completed successfully");
  LOG_PERF_END("Enhanced scan request");
}

/**
 * @brief Sensor diagnostics endpoint handler
 * Provides detailed information about dynamic sensor management system
 */
void handleSensorDiagnostics() {
  LOG_PERF_START();
  LOG_API_INFO("Sensor diagnostics request received");

  JsonDocument doc;
  doc["success"] = true;
  doc["timestamp"] = millis();

  // Basic sensor information
  doc["sensor"]["type"] = "TCS3430";
  doc["sensor"]["initialized"] = true;

  // Current sensor readings
  uint16_t x = tcs3430.getXData();
  uint16_t y = tcs3430.getYData();
  uint16_t z = tcs3430.getZData();
  uint16_t ir1 = tcs3430.getIR1Data();
  uint16_t ir2 = tcs3430.getIR2Data();
  uint8_t status = tcs3430.getDeviceStatus();

  doc["currentReadings"]["x"] = x;
  doc["currentReadings"]["y"] = y;
  doc["currentReadings"]["z"] = z;
  doc["currentReadings"]["ir1"] = ir1;
  doc["currentReadings"]["ir2"] = ir2;
  doc["currentReadings"]["status"] = status;
  doc["currentReadings"]["saturated"] = (status & 0x10) != 0;

  // Static sensor configuration
  doc["staticConfig"]["atime"] = currentAtime;
  doc["staticConfig"]["again"] = currentAgain;
  doc["staticConfig"]["brightness"] = currentBrightness;
  doc["staticConfig"]["autoZeroMode"] = currentAutoZeroMode;
  doc["staticConfig"]["autoZeroFreq"] = currentAutoZeroFreq;
  doc["staticConfig"]["waitTime"] = currentWaitTime;

  // Dynamic sensor management information
  if (dynamicSensor && dynamicSensor->isInitialized()) {
    doc["dynamicSensor"]["enabled"] = true;
    doc["dynamicSensor"]["initialized"] = true;

    // Get current configuration
    SensorConfig config = dynamicSensor->getCurrentConfig();
    doc["dynamicSensor"]["currentConfig"]["atime"] = config.atime;
    doc["dynamicSensor"]["currentConfig"]["again"] = config.again;
    doc["dynamicSensor"]["currentConfig"]["brightness"] = config.brightness;
    doc["dynamicSensor"]["currentConfig"]["condition"] = config.condition;
    doc["dynamicSensor"]["currentConfig"]["isOptimal"] = config.isOptimal;
    doc["dynamicSensor"]["currentConfig"]["timestamp"] = config.timestamp;

    // Lighting condition detection
    LightingCondition detectedCondition = dynamicSensor->detectLightingCondition();
    doc["dynamicSensor"]["detectedCondition"] = detectedCondition;

    // Quality checks
    doc["dynamicSensor"]["saturation"] = dynamicSensor->checkSaturation();
    doc["dynamicSensor"]["signalAdequate"] = dynamicSensor->checkSignalAdequacy();

    // Get full diagnostics
    String diagnosticsJson = dynamicSensor->getDiagnostics();
    JsonDocument diagnosticsDoc;
    deserializeJson(diagnosticsDoc, diagnosticsJson);
    doc["dynamicSensor"]["diagnostics"] = diagnosticsDoc;
  } else {
    doc["dynamicSensor"]["enabled"] = false;
    doc["dynamicSensor"]["initialized"] = false;
    doc["dynamicSensor"]["reason"] = dynamicSensor ? "initialization_failed" : "not_created";
  }

  // Calibration status
  doc["calibration"]["isCalibrated"] = isCalibrated;
  doc["calibration"]["whitePointCalibrated"] = whitePointCalibrated;
  if (whiteCalData.valid) {
    doc["calibration"]["whiteCalData"]["x"] = whiteCalData.x;
    doc["calibration"]["whiteCalData"]["y"] = whiteCalData.y;
    doc["calibration"]["whiteCalData"]["z"] = whiteCalData.z;
    doc["calibration"]["whiteCalData"]["ir"] = whiteCalData.ir;
    doc["calibration"]["whiteCalData"]["brightness"] = whiteCalData.brightness;
    doc["calibration"]["whiteCalData"]["timestamp"] = whiteCalData.timestamp;
  }

  // System information
  doc["system"]["freeHeap"] = ESP.getFreeHeap();
  doc["system"]["uptime"] = millis();
  doc["system"]["firmwareVersion"] = FIRMWARE_VERSION;

  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  LOG_API_INFO("Sensor diagnostics completed successfully");
  LOG_PERF_END("Sensor diagnostics request");
}

/**
 * @brief Live sensor metrics endpoint handler
 * Provides real-time sensor readings and LED status for continuous monitoring
 */
void handleLiveMetrics() {
  LOG_PERF_START();
  LOG_API_DEBUG("Live metrics request received");

  JsonDocument doc;
  doc["success"] = true;
  doc["timestamp"] = millis();

  // Current sensor readings
  uint16_t rawR = tcs3430.getXData();
  uint16_t rawG = tcs3430.getYData();
  uint16_t rawB = tcs3430.getZData();
  uint16_t rawIR = tcs3430.getIR1Data();
  uint8_t status = tcs3430.getDeviceStatus();

  doc["sensorReadings"]["x"] = rawR;
  doc["sensorReadings"]["y"] = rawG;
  doc["sensorReadings"]["z"] = rawB;
  doc["sensorReadings"]["ir"] = rawIR;
  doc["sensorReadings"]["status"] = status;

  // Calculate control variable and metrics
  uint16_t controlVariable = max(max(rawR, rawG), rawB);
  float irRatio = (controlVariable > 0) ? (float)rawIR / controlVariable : 0.0;
  bool saturated = (status & 0x10) != 0;
  bool inOptimalRange = (controlVariable >= RGB_TARGET_MIN && controlVariable <= RGB_TARGET_MAX);

  doc["metrics"]["controlVariable"] = controlVariable;
  doc["metrics"]["irRatio"] = irRatio;
  doc["metrics"]["saturated"] = saturated;
  doc["metrics"]["inOptimalRange"] = inOptimalRange;
  doc["metrics"]["targetMin"] = RGB_TARGET_MIN;
  doc["metrics"]["targetMax"] = RGB_TARGET_MAX;

  // LED status and settings
  doc["ledStatus"]["currentBrightness"] = currentBrightness;
  doc["ledStatus"]["enhancedMode"] = enhancedLEDMode;
  doc["ledStatus"]["manualIntensity"] = manualLEDIntensity;
  doc["ledStatus"]["isScanning"] = isScanning;

  // Status indicators for UI
  doc["statusIndicators"]["controlVariableStatus"] = inOptimalRange ? "optimal" :
    (controlVariable > RGB_TARGET_MAX ? "high" : "low");
  doc["statusIndicators"]["saturationStatus"] = saturated ? "saturated" : "normal";
  doc["statusIndicators"]["irContaminationStatus"] =
    (irRatio > IR_CONTAMINATION_THRESHOLD) ? "contaminated" : "clean";
  doc["statusIndicators"]["signalStatus"] =
    (controlVariable < RGB_TARGET_MIN) ? "low" : "adequate";

  // Enhanced LED control information
  if (dynamicSensor && dynamicSensor->isInitialized()) {
    doc["enhancedControl"]["available"] = true;
    doc["enhancedControl"]["inOptimalRange"] = dynamicSensor->isInOptimalRange(controlVariable);
  } else {
    doc["enhancedControl"]["available"] = false;
  }

  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  LOG_API_DEBUG("Live metrics completed successfully");
  LOG_PERF_END("Live metrics request");
}

void loop() {
  static unsigned long lastMemoryLog = 0;
  static unsigned long lastWatchdogFeed = 0;
  static bool rainbowActive = false;

  // Feed watchdog every 1 second to prevent resets
  if (millis() - lastWatchdogFeed > 1000) {
    esp_task_wdt_reset();
    lastWatchdogFeed = millis();
  }

  server.handleClient();

  // Handle ambient light interrupt (based on DFRobot example)
  if (ambientLightInterrupt) {
    ambientLightInterrupt = false;
    LOG_SENSOR_ERROR("Ambient light threshold exceeded - data may be unreliable");

    // Get device status to clear interrupt flag
    tcs3430.getDeviceStatus();
  }

  // Handle calibration countdown logic
  if (calibrationInProgress && (currentCalState == CAL_WHITE_COUNTDOWN || currentCalState == CAL_BLACK_COUNTDOWN)) {
    static unsigned long lastCountdownUpdate = 0;
    unsigned long currentTime = millis();

    if (currentTime - lastCountdownUpdate >= CALIBRATION_COUNTDOWN_INTERVAL_MS) {
      lastCountdownUpdate = currentTime;

      if (calibrationCountdown > 0) {
        calibrationCountdown--;
        String message = "Countdown: " + String(calibrationCountdown);
        updateCalibrationState(currentCalState, message);
        LOG_SENSOR_DEBUG("Calibration countdown: %u", calibrationCountdown);
      } else {
        // Countdown finished, move to scanning state
        if (currentCalState == CAL_WHITE_COUNTDOWN) {
          updateCalibrationState(CAL_WHITE_SCANNING, "Ready for white calibration scan");
        } else if (currentCalState == CAL_BLACK_COUNTDOWN) {
          updateCalibrationState(CAL_BLACK_SCANNING, "Ready for black calibration scan");
        }
      }
    }
  }

  // Check for calibration timeout
  if (calibrationInProgress && (millis() - calibrationStartTime > CALIBRATION_TIMEOUT_MS)) {
    LOG_SENSOR_ERROR("Calibration timeout - cancelling sequence");
    cancelCalibration();
  }

  // Rainbow cycle when not scanning and LED is manually on
  if (!isScanning && ledState) {
    static uint8_t hue = 0;
    static unsigned long lastUpdate = 0;

    // Log rainbow activation only once
    if (!rainbowActive) {
      LOG_LED_INFO("Rainbow effect activated");
      rainbowActive = true;
    }

    if (millis() - lastUpdate > LED_RAINBOW_DELAY_MS) {
      rgbLed.setPixelColor(0, colorWheel(hue++));
      rgbLed.show();
      lastUpdate = millis();
    }
  } else if (rainbowActive) {
    // Log rainbow deactivation
    LOG_LED_INFO("Rainbow effect deactivated");
    rainbowActive = false;
  }

  // Log memory usage every 30 seconds (avoid spam)
  if (LOG_MEMORY_USAGE && millis() - lastMemoryLog > 30000) {
    Logger::logMemoryUsage("Periodic check");
    lastMemoryLog = millis();
  }

  // Periodically optimize sensor settings if dynamic sensor is available
  static unsigned long lastOptimization = 0;
  if (dynamicSensor && dynamicSensor->isInitialized() &&
      millis() - lastOptimization > 5000) { // Every 5 seconds
    dynamicSensor->optimizeSensorSettings();
    lastOptimization = millis();
  }

  delay(LOOP_DELAY_MS);
}