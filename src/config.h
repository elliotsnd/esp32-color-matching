#ifndef CONFIG_H
#define CONFIG_H

// Hardware Pin Definitions
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 4
#define ILLUMINATION_LED_PIN 5      // PWM-controlled illumination LED for scanning
#define RGB_LED_PIN 18               // Onboard RGB LED for error indication
#define TCS3430_INT_PIN 21
#define LDO2_POWER_PIN 17

// PWM Configuration for Illumination LED
#define PWM_CHANNEL 0                // LEDC channel (0-15)
#define PWM_FREQUENCY 5000           // 5kHz frequency
#define PWM_RESOLUTION 8             // 8-bit resolution (0-255)

// CIE 1931 Color Space Configuration
#define CIE_USE_2_DEGREE_OBSERVER    // Use CIE 1931 2° standard observer
#define CIE_ILLUMINANT_D65           // Use D65 standard illuminant (6504K)
#define CIE_SRGB_GAMMA 2.4           // sRGB gamma correction value

// WiFi Configuration
// Update these with your network credentials
#define WIFI_SSID "Wifi 6"
#define WIFI_PASSWORD "Scrofani1985"
#define WIFI_TIMEOUT_MS 50000

// Static IP Configuration
#define USE_STATIC_IP false
#define STATIC_IP_ADDRESS "192.168.0.152"
#define STATIC_GATEWAY "192.168.0.1"
#define STATIC_SUBNET "255.255.255.0"
#define STATIC_DNS1 "8.8.8.8"
#define STATIC_DNS2 "8.8.4.4"

// Google Apps Script Configuration for Dulux Color Matching
// Deploy the provided Google Apps Script as a web app and use the URL here
// Format: https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/AKfycbxcpG5EguFB6jOjAFWDWuZThoG3_GuCC5Et-dcExO3dgUL5-iCZW7sg16MA7MgFgQuI/exec"
#define COLOR_MATCH_TIMEOUT_MS 10000  // 10 second timeout for color matching

// Sensor Default Settings - Optimized for stability and consistency
#define DEFAULT_ATIME 150       // Integration time (optimal for stability)
#define DEFAULT_AGAIN 1         // Gain setting (16x - optimal for most conditions)
#define DEFAULT_BRIGHTNESS 128  // LED brightness (fixed for consistency)

// Dynamic Sensor Configuration for All-Around Light Matching
// Based on TCS3430 Application Note AN000571 recommendations
#define DYNAMIC_SENSOR_ENABLED 1            // Enable dynamic sensor adjustment

// Gain Settings (AGAIN register values)
#define GAIN_1X   0    // 1x gain for bright outdoor conditions
#define GAIN_4X   1    // 4x gain for normal indoor conditions
#define GAIN_16X  2    // 16x gain for dim indoor conditions
#define GAIN_64X  3    // 64x gain for low-light conditions

// Integration Time Ranges (ATIME register values)
#define ATIME_MIN 20   // 20ms for bright conditions (fast response)
#define ATIME_MID 100  // 100ms for normal conditions (balanced)
#define ATIME_MAX 200  // 200ms for low-light conditions (maximum sensitivity)

// ADC Target Ranges for Optimal Performance
#define ADC_TARGET_MIN 6553    // 10% of 65535 (minimum usable signal)
#define ADC_TARGET_LOW 13107   // 20% of 65535 (low signal threshold)
#define ADC_TARGET_HIGH 52428  // 80% of 65535 (high signal threshold)
#define ADC_TARGET_MAX 58981   // 90% of 65535 (saturation prevention)

// Lighting Condition Thresholds
#define LIGHT_CONDITION_DARK 1000      // Below this = low-light mode
#define LIGHT_CONDITION_INDOOR 10000   // Below this = indoor mode
#define LIGHT_CONDITION_BRIGHT 40000   // Above this = bright mode

// Dynamic Adjustment Parameters
#define ADJUSTMENT_STEP_SIZE 1         // How much to change gain/time per step
#define ADJUSTMENT_DELAY_MS 200        // Delay between adjustments
#define STABILITY_CHECK_SAMPLES 3      // Samples to check for stability

// IR Compensation Constants (from TCS3430 App Note AN000571)
#define IR_COMPENSATION_ENABLED 1      // Enable IR correction
#define IR_CORRECTION_FACTOR_R 0.15f   // IR correction factor for red channel
#define IR_CORRECTION_FACTOR_G 0.25f   // IR correction factor for green channel
#define IR_CORRECTION_FACTOR_B 0.10f   // IR correction factor for blue channel
#define IR_THRESHOLD_HIGH 5000         // High IR threshold for compensation
#define IR_THRESHOLD_LOW 500           // Low IR threshold for compensation

// Quality Control and Statistical Analysis
#define QUALITY_CONTROL_ENABLED 1      // Enable quality control system
#define MIN_SAMPLES_FOR_STATS 5        // Minimum samples for statistical analysis
#define MAX_COEFFICIENT_VARIATION 0.15f // Maximum allowed coefficient of variation (15%)
#define OUTLIER_DETECTION_SIGMA 2.0f   // Standard deviations for outlier detection
#define CONSISTENCY_THRESHOLD 0.05f    // 5% threshold for reading consistency

// Multi-Sample Reading Configuration
#define RAPID_SCAN_SAMPLES 10          // Number of rapid samples for averaging
#define RAPID_SCAN_INTERVAL_MS 50      // Interval between rapid samples
#define ADAPTIVE_SAMPLE_COUNT_MIN 5    // Minimum adaptive sample count
#define ADAPTIVE_SAMPLE_COUNT_MAX 20   // Maximum adaptive sample count

// TCS3430 Advanced Calibration Settings - Optimized for consecutive readings
#define DEFAULT_AUTO_ZERO_MODE 1        // 1 = use previous offset (recommended for stability)
#define DEFAULT_AUTO_ZERO_FREQUENCY 127 // 127 = auto-zero frequency for stability
#define DEFAULT_WAIT_TIME 50            // Wait time between measurements for stability

// TCS3430 Register Addresses (from datasheet for precise calibration)
#define TCS3430_ENABLE_REG 0x80         // Enable register
#define TCS3430_ATIME_REG 0x81          // Integration time register
#define TCS3430_WTIME_REG 0x83          // Wait time register
#define TCS3430_AILTL_REG 0x84          // ALS interrupt low threshold low byte
#define TCS3430_AILTH_REG 0x85          // ALS interrupt low threshold high byte
#define TCS3430_AIHTL_REG 0x86          // ALS interrupt high threshold low byte
#define TCS3430_AIHTH_REG 0x87          // ALS interrupt high threshold high byte
#define TCS3430_PERS_REG 0x8C           // ALS interrupt persistence filters
#define TCS3430_CFG0_REG 0x8D           // Configuration register 0
#define TCS3430_CFG1_REG 0x90           // Configuration register 1
#define TCS3430_STATUS_REG 0x93         // Status register
#define TCS3430_CH0DATAL_REG 0x94       // Z CH0 ADC low byte (Z tristimulus)
#define TCS3430_CH0DATAH_REG 0x95       // Z CH0 ADC high byte
#define TCS3430_CH1DATAL_REG 0x96       // Y CH1 ADC low byte (Y tristimulus)
#define TCS3430_CH1DATAH_REG 0x97       // Y CH1 ADC high byte
#define TCS3430_CH2DATAL_REG 0x98       // IR1 CH2 ADC low byte (IR1 infrared)
#define TCS3430_CH2DATAH_REG 0x99       // IR1 CH2 ADC high byte
#define TCS3430_CH3DATAL_REG 0x9A       // X or IR2 CH3 ADC low byte (X tristimulus or IR2)
#define TCS3430_CH3DATAH_REG 0x9B       // X or IR2 CH3 ADC high byte
#define TCS3430_CFG2_REG 0x9F           // Configuration register 2
#define TCS3430_CFG3_REG 0xAB           // Configuration register 3
#define TCS3430_AZ_CONFIG_REG 0xD6      // Auto-zero configuration
#define TCS3430_INTENAB_REG 0xDD        // Interrupt enables

// ENABLE Register Bits (datasheet 0x80) - Critical for proper initialization
#define TCS3430_PON_BIT 0x01            // Power ON bit
#define TCS3430_AEN_BIT 0x02            // ALS Enable bit
#define TCS3430_WEN_BIT 0x08            // Wait Enable bit

// DFRobot Library Default Values (from softReset() function)
#define DFROBOT_DEFAULT_ATIME 0x23      // DFRobot default integration time (35 decimal)
#define DFROBOT_DEFAULT_AGAIN 3         // DFRobot default gain (64x)
#define DFROBOT_DEFAULT_WTIME 0         // DFRobot default wait time
#define DFROBOT_DEFAULT_AZ_MODE 0       // DFRobot default auto-zero mode
#define DFROBOT_DEFAULT_AZ_FREQ 0x7F    // DFRobot default auto-zero frequency

// Calibration Constants for RGB (255,255,255) Achievement
#define CALIBRATION_TARGET_RGB 255      // Target RGB value for perfect white
#define CALIBRATION_TOLERANCE 0         // Zero tolerance for exact RGB (255,255,255)
#define CALIBRATION_MIN_SIGNAL 1000     // Minimum signal for valid calibration
#define CALIBRATION_MAX_SIGNAL 60000    // Maximum signal to avoid saturation
#define CALIBRATION_BALANCE_RATIO 2.5   // Maximum XYZ balance ratio for white

// Sample Storage Configuration
#define MAX_SAMPLES 30
#define SAMPLE_NAME_LENGTH 32
#define SAMPLE_CODE_LENGTH 16

// LED Brightness Control
#define MIN_LED_BRIGHTNESS 64
#define MAX_LED_BRIGHTNESS 255
#define LED_BRIGHTNESS_STEP 64

// Sensor Thresholds
#define SATURATION_THRESHOLD_HIGH 58981  // 90% of 65535
#define SATURATION_THRESHOLD_LOW 6553    // 10% of 65535
#define HIGH_AMBIENT_LUX 1000
#define LOW_AMBIENT_LUX 100

// Automatic LED Brightness Control - Target Range Optimization
#define RGB_TARGET_MIN 5000              // Lower threshold for RGB channel sum/max
#define RGB_TARGET_MAX 60000             // Upper threshold for RGB channel sum/max
#define RGB_TARGET_OPTIMAL 30000         // Optimal target for RGB channels
#define BRIGHTNESS_ADJUSTMENT_STEP 8     // Smaller steps for smoother control
#define BRIGHTNESS_SMOOTHING_ALPHA 0.3   // Exponential moving average factor (0.0-1.0)
#define IR_CONTAMINATION_THRESHOLD 0.15  // IR/RGB ratio threshold for contamination detection
#define BRIGHTNESS_STABILIZATION_DELAY 100  // ms delay after brightness change

// Web Server Configuration
#define WEB_SERVER_PORT 80
#define STATUS_UPDATE_INTERVAL 2000    // ms
#define SAMPLE_UPDATE_INTERVAL 5000    // ms

// Calibration Settings
#define CALIBRATION_LOW_IR_GAIN 3      // 64x gain for low IR
#define CALIBRATION_HIGH_IR_GAIN 2     // 16x gain for high IR
#define CALIBRATION_DELAY_MS 1000
#define CALIBRATION_STABILIZE_MS 500

// Advanced Calibration Sequence Settings - Increased for stability
#define CALIBRATION_COUNTDOWN_SECONDS 3
#define CALIBRATION_COUNTDOWN_INTERVAL_MS 1000
#define CALIBRATION_SCAN_DURATION_MS 2000
#define CALIBRATION_LED_STABILIZE_MS 1000  // Increased for better LED stabilization
#define CALIBRATION_TIMEOUT_MS 30000       // 30 second timeout for entire sequence

// Enhanced Multi-Sample Calibration Settings
#define ENHANCED_CALIBRATION_DURATION_MS 10000        // 10 second calibration duration
#define ENHANCED_CALIBRATION_SAMPLE_INTERVAL_MS 500   // 2 samples per second
#define ENHANCED_CALIBRATION_MAX_SAMPLES 20           // Maximum samples to collect
#define OUTLIER_DETECTION_THRESHOLD 2.0              // 2 standard deviations for outlier detection
#define QUALITY_CONSISTENCY_THRESHOLD 0.10           // 10% coefficient of variation threshold
#define TARGET_RGB_VALUE 255                         // Exact target for RGB channels
#define QUALITY_SCORE_EXCELLENT 90                   // Quality score thresholds
#define QUALITY_SCORE_GOOD 70
#define QUALITY_SCORE_FAIR 50

// Sensor Reading Stability Settings
#define SENSOR_STABILIZE_MS 300            // Delay after LED activation before reading
#define SENSOR_READING_DELAY_MS 150        // Delay between consecutive readings
#define SENSOR_VALIDATION_THRESHOLD 5      // Max allowed deviation between readings (%)

// Color Processing
#define SATURATION_BOOST 1.5
#define GAMMA_CORRECTION 2.4

// System Limits
#define MAX_JSON_SIZE 4096
#define SERIAL_BAUD_RATE 115200
#define LOOP_DELAY_MS 10
#define LED_RAINBOW_DELAY_MS 20

// EEPROM Keys
#define PREF_NAMESPACE "colorMatcher"
#define PREF_ATIME "atime"
#define PREF_AGAIN "again"
#define PREF_BRIGHTNESS "brightness"
#define PREF_CALIBRATED "calibrated"
#define PREF_SAMPLE_COUNT "sampleCount"
#define PREF_SAMPLE_INDEX "sampleIndex"
#define PREF_SAMPLE_PREFIX "sample"
#define PREF_ENHANCED_LED_MODE "enhancedLED"
#define PREF_MANUAL_LED_INTENSITY "manualLEDInt"

// TCS3430 Advanced Calibration EEPROM Keys
#define PREF_AUTO_ZERO_MODE "autoZeroMode"
#define PREF_AUTO_ZERO_FREQ "autoZeroFreq"
#define PREF_WAIT_TIME "waitTime"

// Advanced Calibration EEPROM Keys
#define PREF_WHITE_CAL_X "whiteCalX"
#define PREF_WHITE_CAL_Y "whiteCalY"
#define PREF_WHITE_CAL_Z "whiteCalZ"
#define PREF_WHITE_CAL_IR "whiteCalIR"
#define PREF_WHITE_CAL_BRIGHTNESS "whiteCalBright"
#define PREF_WHITE_CAL_TIMESTAMP "whiteCalTime"
#define PREF_BLACK_CAL_X "blackCalX"
#define PREF_BLACK_CAL_Y "blackCalY"
#define PREF_BLACK_CAL_Z "blackCalZ"
#define PREF_BLACK_CAL_IR "blackCalIR"
#define PREF_BLACK_CAL_TIMESTAMP "blackCalTime"
#define PREF_HAS_WHITE_CAL "hasWhiteCal"
#define PREF_HAS_BLACK_CAL "hasBlackCal"

// Matrix Calibration Configuration
#define MATRIX_SIZE 4                    // 3x4 matrix size for least squares
#define MAX_CALIBRATION_POINTS 12        // Maximum color patches for calibration
#define STANDARD_CALIBRATION_POINTS 7    // Standard 7-color patch set

// Matrix Calibration NVS Keys
#define PREF_MATRIX_VALID "matrixValid"
#define PREF_MATRIX_DATA "matrixData"
#define PREF_MATRIX_POINTS "matrixPoints"
#define PREF_MATRIX_TIMESTAMP "matrixTime"
#define PREF_MATRIX_STATS "matrixStats"
#define PREF_MATRIX_QUALITY "matrixQuality"

// Matrix Calibration Quality Thresholds
#define DELTA_E_EXCELLENT 2.0f           // ΔE < 2 = excellent calibration
#define DELTA_E_ACCEPTABLE 5.0f          // ΔE < 5 = acceptable calibration
#define MATRIX_MIN_POINTS 4              // Minimum points for matrix computation
#define MATRIX_CONDITION_THRESHOLD 100.0f // Maximum condition number for stability

// Standard Color References (sRGB values)
#define REF_RED_R 255
#define REF_RED_G 0
#define REF_RED_B 0
#define REF_YELLOW_R 255
#define REF_YELLOW_G 255
#define REF_YELLOW_B 0
#define REF_GREEN_R 0
#define REF_GREEN_G 255
#define REF_GREEN_B 0
#define REF_CYAN_R 0
#define REF_CYAN_G 255
#define REF_CYAN_B 255
#define REF_BLUE_R 0
#define REF_BLUE_G 0
#define REF_BLUE_B 255
#define REF_MAGENTA_R 255
#define REF_MAGENTA_G 0
#define REF_MAGENTA_B 255
#define REF_BLACK_R 0
#define REF_BLACK_G 0
#define REF_BLACK_B 0

// Removed simple and vivid white calibration constants - using advanced calibration only

// Logging Configuration
#define ENABLE_LOGGING true
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

// Set global log level (1=ERROR, 2=WARN, 3=INFO, 4=DEBUG)
#define GLOBAL_LOG_LEVEL LOG_LEVEL_INFO

// Category-specific logging flags
#define LOG_SENSOR_OPS true
#define LOG_LED_CONTROL true
#define LOG_NETWORK true
#define LOG_WEB_SERVER true
#define LOG_DATA_STORAGE true
#define LOG_API_CALLS true
#define LOG_SYSTEM_HEALTH true
#define LOG_MEMORY_USAGE true

// Performance logging
#define LOG_RESPONSE_TIMES true
#define LOG_SCAN_PERFORMANCE true

// Debug Configuration (legacy)
#define DEBUG_SERIAL true
#define DEBUG_SENSOR_READINGS false
#define DEBUG_LED_BRIGHTNESS false
#define DEBUG_API_CALLS false

// Version Information
#define FIRMWARE_VERSION "1.0.0"
#define HARDWARE_VERSION "ESP32-S3 ProS3 + TCS3430"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#endif // CONFIG_H
