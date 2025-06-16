#ifndef TCS3430_CALIBRATION_H
#define TCS3430_CALIBRATION_H

#include <Arduino.h>
#include <Preferences.h>
#include <DFRobot_TCS3430.h>
#include <ArduinoJson.h>

// ============================================================================
// CONSTANTS AND CONFIGURATION
// ============================================================================

// Calibration matrix dimensions (4x4 for R,G,B,IR -> X,Y,Z,1 transformation)
#define CALIBRATION_MATRIX_SIZE 16
#define MAX_CALIBRATION_POINTS 12
#define CALIBRATION_RETRIES 3
#define SENSOR_STABILIZE_DELAY_MS 200
#define AUTO_ZERO_TIMEOUT_MS 5000

// Delta E thresholds for calibration quality assessment
#define DELTA_E_EXCELLENT 2.0f
#define DELTA_E_ACCEPTABLE 5.0f
#define DELTA_E_POOR 10.0f

// IR blending thresholds (normalized IR content)
#define TCS3430_IR_THRESHOLD_LOW 0.15f
#define TCS3430_IR_THRESHOLD_HIGH 0.35f

// NVS storage keys
#define NVS_CALIBRATION_NAMESPACE "tcs3430_cal"
#define NVS_LOW_IR_MATRIX "low_ir_matrix"
#define NVS_HIGH_IR_MATRIX "high_ir_matrix"
#define NVS_LOW_IR_SCALING "low_ir_scale"
#define NVS_HIGH_IR_SCALING "high_ir_scale"
#define NVS_DUAL_MODE_ENABLED "dual_mode"
#define NVS_CALIBRATION_VALID "cal_valid"
#define NVS_CALIBRATION_TIMESTAMP "cal_timestamp"

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * @brief Raw sensor channel data with validation
 */
struct RawChannelData {
    uint16_t r;           // X channel (CH3)
    uint16_t g;           // Y channel (CH1) 
    uint16_t b;           // Z channel (CH0)
    uint16_t ir;          // IR channel (CH2/IR1)
    uint32_t timestamp;   // Reading timestamp
    bool valid;           // Data validity flag
    bool saturated;       // Saturation detection
};

/**
 * @brief TCS3430 Calibration matrix with metadata
 */
struct TCS3430CalibrationMatrix {
    float matrix[CALIBRATION_MATRIX_SIZE];  // 4x4 matrix (row-major)
    float kX, kY, kZ;                       // Scaling factors
    bool valid;                             // Matrix validity
    uint32_t timestamp;                     // Calibration timestamp
    char source[32];                        // Source description
    float quality_score;                    // Calibration quality (0-100)
};

/**
 * @brief Dual-matrix calibration system
 */
struct DualMatrixCalibration {
    TCS3430CalibrationMatrix lowIR;         // Low-IR source matrix (LED/CFL)
    TCS3430CalibrationMatrix highIR;        // High-IR source matrix (incandescent)
    bool dualModeEnabled;                   // Enable dual-matrix blending
    float irThresholdLow;                   // Low IR threshold
    float irThresholdHigh;                  // High IR threshold
};

/**
 * @brief Calibration reference point
 */
struct CalibrationReference {
    uint8_t ref_r, ref_g, ref_b;           // Reference sRGB values
    uint16_t sensor_r, sensor_g, sensor_b, sensor_ir; // Measured sensor values
    float delta_e;                          // Color difference (Delta E)
    char name[32];                          // Color name
    bool measured;                          // Measurement status
};

/**
 * @brief TCS3430 Calibration quality statistics
 */
struct TCS3430CalibrationStats {
    float mean_delta_e;                     // Mean color difference
    float std_delta_e;                      // Standard deviation
    float max_delta_e;                      // Maximum color difference
    uint8_t points_under_2;                 // Points with ΔE < 2
    uint8_t points_under_5;                 // Points with ΔE < 5
    uint8_t total_points;                   // Total calibration points
    float quality_score;                    // Overall quality (0-100)
    bool matrix_valid;                      // Matrix validity status
};

/**
 * @brief TCS3430 Sensor configuration parameters
 */
struct TCS3430SensorConfig {
    uint8_t atime;                          // Integration time
    uint8_t again;                          // Analog gain
    uint8_t wtime;                          // Wait time
    bool auto_zero_enabled;                 // Auto-zero mode
    uint8_t auto_zero_frequency;            // Auto-zero frequency
};

// ============================================================================
// ENUMERATIONS
// ============================================================================

enum class MatrixType {
    LOW_IR = 0,
    HIGH_IR = 1,
    BLENDED = 2
};

enum class CalibrationError {
    NONE = 0,
    SENSOR_NOT_INITIALIZED,
    I2C_READ_FAILED,
    SATURATION_DETECTED,
    INVALID_MATRIX,
    STORAGE_FAILED,
    AUTO_ZERO_FAILED,
    INSUFFICIENT_DATA,
    QUALITY_TOO_LOW
};

enum class TCS3430CalibrationState {
    UNINITIALIZED = 0,
    INITIALIZED,
    COLLECTING_DATA,
    COMPUTING_MATRIX,
    VALIDATING,
    COMPLETE,
    ERROR_STATE
};

// ============================================================================
// FACTORY CALIBRATION MATRICES (PROGMEM)
// ============================================================================

// Default low-IR calibration matrix (LED/CFL illumination)
// Based on AN000571 methodology for typical TCS3430 with diffuser
extern const float FACTORY_LOW_IR_MATRIX[CALIBRATION_MATRIX_SIZE] PROGMEM;

// Default high-IR calibration matrix (incandescent illumination)  
extern const float FACTORY_HIGH_IR_MATRIX[CALIBRATION_MATRIX_SIZE] PROGMEM;

// Default scaling factors for factory matrices
extern const float FACTORY_LOW_IR_SCALING[3] PROGMEM;  // kX, kY, kZ
extern const float FACTORY_HIGH_IR_SCALING[3] PROGMEM; // kX, kY, kZ

// ============================================================================
// TCS3430 CALIBRATION CLASS
// ============================================================================

/**
 * @brief Advanced TCS3430 colorimetric calibration system
 * 
 * Implements comprehensive colorimetric calibration based on AN000571 methodology:
 * - Dual-matrix system for low-IR and high-IR illumination sources
 * - 4x4 transformation matrices with IR compensation
 * - Auto-zero calibration for dark offset compensation
 * - Smooth-step IR blending for mixed illumination
 * - Delta E quality assessment and validation
 * - Field calibration data collection and export
 * - NVS storage for runtime calibration updates
 */
class TCS3430Calibration {
private:
    DFRobot_TCS3430* sensor;                // TCS3430 sensor instance
    DualMatrixCalibration calibration;      // Calibration matrices
    CalibrationReference references[MAX_CALIBRATION_POINTS]; // Reference points
    TCS3430CalibrationStats lastStats;      // Last calibration statistics
    TCS3430SensorConfig sensorConfig;       // Sensor configuration
    Preferences preferences;                // NVS storage
    
    uint8_t numReferences;                  // Number of reference points
    TCS3430CalibrationState currentState;   // Current calibration state
    CalibrationError lastError;             // Last error encountered
    bool initialized;                       // Initialization status
    uint32_t lastAutoZero;                  // Last auto-zero timestamp

public:
    /**
     * @brief Constructor
     * @param tcs3430 Pointer to initialized TCS3430 sensor
     */
    TCS3430Calibration(DFRobot_TCS3430* tcs3430);
    
    /**
     * @brief Destructor
     */
    ~TCS3430Calibration();
    
    // ========================================================================
    // INITIALIZATION AND CONFIGURATION
    // ========================================================================
    
    /**
     * @brief Initialize the calibration system
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Configure sensor parameters for optimal colorimetric performance
     * @param config Sensor configuration parameters
     * @return true if configuration successful
     */
    bool configureSensor(const TCS3430SensorConfig& config);
    
    /**
     * @brief Load factory default calibration matrices
     * @return true if factory defaults loaded successfully
     */
    bool loadFactoryDefaults();
    
    // ========================================================================
    // RAW SENSOR OPERATIONS
    // ========================================================================
    
    /**
     * @brief Read raw sensor channels with proper I2C timing
     * @return Raw channel data structure
     */
    RawChannelData readRawChannels();
    
    /**
     * @brief Perform auto-zero calibration sequence
     * @return true if auto-zero successful
     */
    bool performAutoZero();
    
    /**
     * @brief Check for sensor saturation
     * @param raw Raw channel data
     * @return true if any channel is saturated
     */
    bool checkSaturation(const RawChannelData& raw);
    
    /**
     * @brief Adjust sensor settings to avoid saturation
     * @return true if settings adjusted successfully
     */
    bool adjustSensorSettings();

    // ========================================================================
    // CALIBRATION MATRIX OPERATIONS
    // ========================================================================

    /**
     * @brief Apply color calibration matrix to raw sensor data
     * @param raw Raw sensor channel data
     * @param matrix 4x4 calibration matrix
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @param z Output Z coordinate
     * @return true if matrix application successful
     */
    bool applyColorMatrix(const RawChannelData& raw, const float matrix[CALIBRATION_MATRIX_SIZE],
                         float& x, float& y, float& z);

    /**
     * @brief Calculate IR weight for matrix blending
     * @param raw Raw sensor channel data
     * @return Normalized IR weight (0.0 to 1.0)
     */
    float calculateIRWeight(const RawChannelData& raw);

    /**
     * @brief Apply smooth-step blending between low-IR and high-IR matrices
     * @param raw Raw sensor channel data
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @param z Output Z coordinate
     * @return true if blending successful
     */
    bool applySmoothStepBlending(const RawChannelData& raw, float& x, float& y, float& z);

    /**
     * @brief Get calibrated XYZ coordinates from raw sensor data
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @param z Output Z coordinate
     * @return true if calibration successful
     */
    bool getCalibratedXYZ(float& x, float& y, float& z);

    /**
     * @brief Apply calibrated conversion to get sRGB output
     * @param raw_r Raw R channel (X)
     * @param raw_g Raw G channel (Y)
     * @param raw_b Raw B channel (Z)
     * @param raw_ir Raw IR channel
     * @param out_r Output red (0-255)
     * @param out_g Output green (0-255)
     * @param out_b Output blue (0-255)
     * @return true if conversion successful
     */
    bool applyCalibratedConversion(uint16_t raw_r, uint16_t raw_g, uint16_t raw_b, uint16_t raw_ir,
                                  uint8_t& out_r, uint8_t& out_g, uint8_t& out_b);

    // ========================================================================
    // CALIBRATION DATA MANAGEMENT
    // ========================================================================

    /**
     * @brief Set calibration matrix for specified type
     * @param matrix 4x4 calibration matrix (row-major)
     * @param type Matrix type (LOW_IR or HIGH_IR)
     * @return true if matrix set successfully
     */
    bool setCalibrationMatrix(const float matrix[CALIBRATION_MATRIX_SIZE], MatrixType type);

    /**
     * @brief Set scaling factors for specified matrix type
     * @param kX X scaling factor
     * @param kY Y scaling factor
     * @param kZ Z scaling factor
     * @param type Matrix type (LOW_IR or HIGH_IR)
     * @return true if scaling factors set successfully
     */
    bool setScalingFactors(float kX, float kY, float kZ, MatrixType type);

    /**
     * @brief Enable or disable dual-matrix mode
     * @param enable true to enable dual-matrix blending
     * @return true if mode set successfully
     */
    bool enableDualMatrixMode(bool enable);

    /**
     * @brief Set IR thresholds for matrix blending
     * @param lowThreshold Low IR threshold (0.0 to 1.0)
     * @param highThreshold High IR threshold (0.0 to 1.0)
     * @return true if thresholds set successfully
     */
    bool setIRThresholds(float lowThreshold, float highThreshold);

    // ========================================================================
    // CALIBRATION WORKFLOW
    // ========================================================================

    /**
     * @brief Add calibration reference point
     * @param ref_r Reference red value (0-255)
     * @param ref_g Reference green value (0-255)
     * @param ref_b Reference blue value (0-255)
     * @param name Color name for identification
     * @return true if reference point added successfully
     */
    bool addCalibrationPoint(uint8_t ref_r, uint8_t ref_g, uint8_t ref_b, const char* name);

    /**
     * @brief Compute calibration matrix from reference points
     * @param type Matrix type to compute (LOW_IR or HIGH_IR)
     * @return true if matrix computation successful
     */
    bool computeMatrix(MatrixType type);

    /**
     * @brief Evaluate calibration quality using Delta E assessment
     * @return Calibration statistics structure
     */
    TCS3430CalibrationStats evaluateCalibration();

    /**
     * @brief Clear all calibration reference points
     */
    void clearCalibrationPoints();

    // ========================================================================
    // DATA PERSISTENCE
    // ========================================================================

    /**
     * @brief Save calibration data to NVS storage
     * @return true if save successful
     */
    bool saveCalibration();

    /**
     * @brief Load calibration data from NVS storage
     * @return true if load successful
     */
    bool loadCalibration();

    /**
     * @brief Export calibration data for offline analysis
     * @param filename Output filename for calibration data
     * @return true if export successful
     */
    bool exportCalibrationData(const char* filename);

    /**
     * @brief Import calibration matrix from external source
     * @param matrix 4x4 calibration matrix
     * @param scaling Scaling factors [kX, kY, kZ]
     * @param type Matrix type
     * @return true if import successful
     */
    bool importCalibrationMatrix(const float matrix[CALIBRATION_MATRIX_SIZE],
                                const float scaling[3], MatrixType type);

    // ========================================================================
    // STATUS AND DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Get current calibration state
     * @return Current calibration state
     */
    TCS3430CalibrationState getState() const { return currentState; }

    /**
     * @brief Get last error encountered
     * @return Last calibration error
     */
    CalibrationError getLastError() const { return lastError; }

    /**
     * @brief Check if calibration system is properly initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Check if valid calibration matrices are available
     * @return true if calibration is valid
     */
    bool isCalibrationValid() const;

    /**
     * @brief Get sensor diagnostic information
     * @param doc JSON document to populate with diagnostics
     */
    void getSensorDiagnostics(JsonDocument& doc);

    /**
     * @brief Get calibration status information
     * @param doc JSON document to populate with status
     */
    void getCalibrationStatus(JsonDocument& doc);

private:
    // ========================================================================
    // PRIVATE HELPER METHODS
    // ========================================================================

    /**
     * @brief Validate calibration matrix
     * @param matrix Matrix to validate
     * @return true if matrix is valid
     */
    bool validateMatrix(const float matrix[CALIBRATION_MATRIX_SIZE]);

    /**
     * @brief Convert XYZ to sRGB color space
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param r Output red (0-255)
     * @param g Output green (0-255)
     * @param b Output blue (0-255)
     */
    void xyzToSRGB(float x, float y, float z, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Calculate Delta E color difference
     * @param r1 First color red
     * @param g1 First color green
     * @param b1 First color blue
     * @param r2 Second color red
     * @param g2 Second color green
     * @param b2 Second color blue
     * @return Delta E color difference
     */
    float calculateDeltaE(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2);

    /**
     * @brief Smooth step interpolation function
     * @param edge0 Lower edge
     * @param edge1 Upper edge
     * @param x Input value
     * @return Interpolated value
     */
    float smoothStep(float edge0, float edge1, float x);

    /**
     * @brief Set calibration error state
     * @param error Error code
     */
    void setError(CalibrationError error);

    /**
     * @brief Log calibration message with timestamp
     * @param level Log level
     * @param format Format string
     * @param ... Format arguments
     */
    void logCalibration(const char* level, const char* format, ...);
};

#endif // TCS3430_CALIBRATION_H
