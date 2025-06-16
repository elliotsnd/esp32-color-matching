#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// ESP32 COLOR MATCHER CONFIGURATION
// ============================================================================

// Firmware Information
#define FIRMWARE_VERSION "1.0.0"
#define HARDWARE_VERSION "ESP32-S3"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// WiFi Configuration - UPDATE THESE WITH YOUR CREDENTIALS
#define WIFI_SSID "Wifi 6"
#define WIFI_PASSWORD "Scrofani1985"

// Google Apps Script Configuration (Optional)
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec"

// Hardware Pin Definitions
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define RGB_LED_PIN 8
#define LDO2_POWER_PIN 2

// Serial Configuration
#define SERIAL_BAUD_RATE 115200

// Web Server Configuration
#define WEB_SERVER_PORT 80

// Sensor Default Settings - Aligned with DFRobot TCS3430 Library
#define DEFAULT_ATIME 0x23              // DFRobot default integration time (35 decimal)
#define DEFAULT_AGAIN 3                 // DFRobot default gain (64x gain)
#define DEFAULT_BRIGHTNESS 128

// TCS3430 Advanced Settings - Following DFRobot Library Patterns
#define DEFAULT_AUTO_ZERO_MODE 0        // DFRobot default: start at zero when searching offset
#define DEFAULT_AUTO_ZERO_FREQUENCY 0x7F // DFRobot default: only at first ALS cycle
#define DEFAULT_WAIT_TIME 0             // DFRobot default wait time

// Hardware Pin Definitions for TCS3430
#define ILLUMINATION_LED_PIN 4          // PWM pin for illumination LED
#define INTERRUPT_PIN 2                 // GPIO2 for ambient light interrupt

// PWM Configuration for Illumination LED
#define PWM_CHANNEL 0
#define PWM_FREQUENCY 5000              // 5 kHz PWM frequency
#define PWM_RESOLUTION 8                // 8-bit resolution (0-255)

// Sensor Timing Constants - DFRobot Optimized
#define SENSOR_STABILIZE_MS 1000        // Extended stabilization time for DFRobot sensor
#define SENSOR_READING_DELAY_MS 100     // Delay between consecutive readings

// LED Brightness Control
#define MIN_LED_BRIGHTNESS 10
#define MAX_LED_BRIGHTNESS 255
#define LED_BRIGHTNESS_STEP 16

// Ambient Light Thresholds
#define LOW_AMBIENT_LUX 10.0
#define HIGH_AMBIENT_LUX 1000.0
#define SATURATION_THRESHOLD_LOW 6553   // 10% of 65535
#define SATURATION_THRESHOLD_HIGH 58981 // 90% of 65535

// Sample Storage
#define MAX_SAMPLES 30
#define SAMPLE_NAME_LENGTH 32
#define SAMPLE_CODE_LENGTH 16

// EEPROM Preferences Keys
#define PREF_NAMESPACE "colorMatcher"
#define PREF_ATIME "atime"
#define PREF_AGAIN "again"
#define PREF_BRIGHTNESS "brightness"
#define PREF_CALIBRATED "calibrated"
#define PREF_SAMPLE_COUNT "sampleCount"
#define PREF_SAMPLE_INDEX "sampleIndex"
#define PREF_SAMPLE_PREFIX "sample"

// TCS3430 Advanced Settings EEPROM Keys
#define PREF_AUTO_ZERO_MODE "autoZeroMode"
#define PREF_AUTO_ZERO_FREQ "autoZeroFreq"
#define PREF_WAIT_TIME "waitTime"

// Advanced Calibration Settings
#define CALIBRATION_COUNTDOWN_SECONDS 5
#define CALIBRATION_LED_STABILIZE_MS 500

// Calibration EEPROM Keys
#define PREF_HAS_WHITE_CAL "hasWhiteCal"
#define PREF_WHITE_CAL_X "whiteCalX"
#define PREF_WHITE_CAL_Y "whiteCalY"
#define PREF_WHITE_CAL_Z "whiteCalZ"
#define PREF_WHITE_CAL_IR "whiteCalIR"
#define PREF_WHITE_CAL_BRIGHTNESS "whiteCalBright"
#define PREF_WHITE_CAL_TIMESTAMP "whiteCalTime"

#define PREF_HAS_BLACK_CAL "hasBlackCal"
#define PREF_BLACK_CAL_X "blackCalX"
#define PREF_BLACK_CAL_Y "blackCalY"
#define PREF_BLACK_CAL_Z "blackCalZ"
#define PREF_BLACK_CAL_IR "blackCalIR"
#define PREF_BLACK_CAL_TIMESTAMP "blackCalTime"

// Logging Configuration
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR 3

// Enable/Disable specific logging categories
#define LOG_SYSTEM 1
#define LOG_SENSOR 1
#define LOG_LED_CONTROL 1
#define LOG_NETWORK 1
#define LOG_WEB_SERVER 1
#define LOG_STORAGE 1
#define LOG_API_CALLS 1
#define LOG_PERFORMANCE 1

// Memory and Performance Monitoring
#define ENABLE_MEMORY_LOGGING 1
#define MEMORY_LOG_INTERVAL_MS 30000

#endif // CONFIG_H