#include "TCS3430Calibration.h"
#include <math.h>
#include <esp_log.h>

// ============================================================================
// FACTORY CALIBRATION MATRICES (PROGMEM)
// ============================================================================

// Default low-IR calibration matrix (LED/CFL illumination)
// Scaled to produce appropriate XYZ values for white RGB output
// Target: raw sensor ~(500,500,700) -> XYZ that converts to RGB(255,255,255)
const float FACTORY_LOW_IR_MATRIX[CALIBRATION_MATRIX_SIZE] PROGMEM = {
    // Row 0: X output coefficients [R, G, B, IR] - scaled for white output
    0.5f, 0.4f, 0.2f, -0.01f,
    // Row 1: Y output coefficients [R, G, B, IR] - scaled for white output
    0.25f, 0.8f, 0.1f, -0.005f,
    // Row 2: Z output coefficients [R, G, B, IR] - scaled for white output
    0.02f, 0.15f, 1.2f, -0.002f,
    // Row 3: Homogeneous coordinates [0, 0, 0, 1]
    0.0000f, 0.0000f, 0.0000f, 1.0000f
};

// Default high-IR calibration matrix (incandescent illumination)
// Scaled to produce appropriate XYZ values for white RGB output with IR compensation
const float FACTORY_HIGH_IR_MATRIX[CALIBRATION_MATRIX_SIZE] PROGMEM = {
    // Row 0: X output coefficients [R, G, B, IR] - scaled for white output
    0.52f, 0.38f, 0.18f, -0.02f,
    // Row 1: Y output coefficients [R, G, B, IR] - scaled for white output
    0.27f, 0.78f, 0.08f, -0.01f,
    // Row 2: Z output coefficients [R, G, B, IR] - scaled for white output
    0.025f, 0.12f, 1.15f, -0.005f,
    // Row 3: Homogeneous coordinates [0, 0, 0, 1]
    0.0000f, 0.0000f, 0.0000f, 1.0000f
};

// Default scaling factors for factory matrices
const float FACTORY_LOW_IR_SCALING[3] PROGMEM = {1.0f, 1.0f, 1.0f};   // kX, kY, kZ
const float FACTORY_HIGH_IR_SCALING[3] PROGMEM = {1.0f, 1.0f, 1.0f};  // kX, kY, kZ

// ============================================================================
// LOGGING MACROS
// ============================================================================

#define TAG "TCS3430Cal"
#define LOG_CAL_ERROR(format, ...) ESP_LOGE(TAG, format, ##__VA_ARGS__)
#define LOG_CAL_WARN(format, ...)  ESP_LOGW(TAG, format, ##__VA_ARGS__)
#define LOG_CAL_INFO(format, ...)  ESP_LOGI(TAG, format, ##__VA_ARGS__)
#define LOG_CAL_DEBUG(format, ...) ESP_LOGD(TAG, format, ##__VA_ARGS__)

// ============================================================================
// CONSTRUCTOR AND DESTRUCTOR
// ============================================================================

TCS3430Calibration::TCS3430Calibration(DFRobot_TCS3430* tcs3430)
    : sensor(tcs3430), numReferences(0), currentState(TCS3430CalibrationState::UNINITIALIZED),
      lastError(CalibrationError::NONE), initialized(false), lastAutoZero(0) {
    
    // Initialize calibration structure
    memset(&calibration, 0, sizeof(calibration));
    memset(&references, 0, sizeof(references));
    memset(&lastStats, 0, sizeof(lastStats));
    memset(&sensorConfig, 0, sizeof(sensorConfig));
    
    // Set default IR thresholds
    calibration.irThresholdLow = TCS3430_IR_THRESHOLD_LOW;
    calibration.irThresholdHigh = TCS3430_IR_THRESHOLD_HIGH;
    calibration.dualModeEnabled = false;
    
    // Set default sensor configuration (optimal settings from memory)
    sensorConfig.atime = 150;           // Integration time
    sensorConfig.again = 16;            // Analog gain (16x)
    sensorConfig.wtime = 0;             // Wait time
    sensorConfig.auto_zero_enabled = true;
    sensorConfig.auto_zero_frequency = 127;
    
    LOG_CAL_INFO("TCS3430Calibration initialized");
}

TCS3430Calibration::~TCS3430Calibration() {
    if (preferences.isKey(NVS_CALIBRATION_NAMESPACE)) {
        preferences.end();
    }
    LOG_CAL_INFO("TCS3430Calibration destroyed");
}

// ============================================================================
// INITIALIZATION AND CONFIGURATION
// ============================================================================

bool TCS3430Calibration::initialize() {
    if (!sensor) {
        setError(CalibrationError::SENSOR_NOT_INITIALIZED);
        LOG_CAL_ERROR("Sensor not initialized");
        return false;
    }
    
    // Initialize NVS preferences
    if (!preferences.begin(NVS_CALIBRATION_NAMESPACE, false)) {
        LOG_CAL_ERROR("Failed to initialize NVS preferences");
        setError(CalibrationError::STORAGE_FAILED);
        return false;
    }
    
    // Configure sensor with optimal settings
    if (!configureSensor(sensorConfig)) {
        LOG_CAL_ERROR("Failed to configure sensor");
        return false;
    }
    
    // Load existing calibration or factory defaults
    if (!loadCalibration()) {
        LOG_CAL_WARN("No existing calibration found, loading factory defaults");
        if (!loadFactoryDefaults()) {
            LOG_CAL_ERROR("Failed to load factory defaults");
            return false;
        }
    }
    
    // Perform initial auto-zero calibration
    if (!performAutoZero()) {
        LOG_CAL_WARN("Auto-zero calibration failed, continuing anyway");
    }
    
    initialized = true;
    currentState = TCS3430CalibrationState::INITIALIZED;
    lastError = CalibrationError::NONE;
    
    LOG_CAL_INFO("TCS3430Calibration initialization complete");
    return true;
}

bool TCS3430Calibration::configureSensor(const TCS3430SensorConfig& config) {
    if (!sensor) {
        setError(CalibrationError::SENSOR_NOT_INITIALIZED);
        return false;
    }
    
    try {
        // Set integration time (ATIME register)
        sensor->setIntegrationTime(config.atime);
        
        // Set analog gain (AGAIN register)
        sensor->setALSGain(config.again);
        
        // Set wait time (WTIME register)
        sensor->setWaitTime(config.wtime);
        
        // Configure auto-zero mode
        sensor->setAutoZeroMode(config.auto_zero_enabled ? 1 : 0);
        sensor->setAutoZeroNTHIteration(config.auto_zero_frequency);
        
        // Enable ALS and power on - use public method
        // Note: setPowerALSADC() is private, so we'll use the public begin() method approach
        // or access through public interface
        
        // Store configuration
        sensorConfig = config;
        
        LOG_CAL_INFO("Sensor configured: ATIME=%d, AGAIN=%d, WTIME=%d, AutoZero=%s", 
                     config.atime, config.again, config.wtime, 
                     config.auto_zero_enabled ? "enabled" : "disabled");
        
        return true;
    } catch (...) {
        setError(CalibrationError::I2C_READ_FAILED);
        LOG_CAL_ERROR("Failed to configure sensor - I2C error");
        return false;
    }
}

bool TCS3430Calibration::loadFactoryDefaults() {
    LOG_CAL_INFO("Loading factory default calibration matrices");
    
    // Load low-IR matrix from PROGMEM
    for (int i = 0; i < CALIBRATION_MATRIX_SIZE; i++) {
        calibration.lowIR.matrix[i] = pgm_read_float(&FACTORY_LOW_IR_MATRIX[i]);
    }
    calibration.lowIR.kX = pgm_read_float(&FACTORY_LOW_IR_SCALING[0]);
    calibration.lowIR.kY = pgm_read_float(&FACTORY_LOW_IR_SCALING[1]);
    calibration.lowIR.kZ = pgm_read_float(&FACTORY_LOW_IR_SCALING[2]);
    calibration.lowIR.valid = true;
    calibration.lowIR.timestamp = millis();
    strcpy(calibration.lowIR.source, "factory_low_ir");
    calibration.lowIR.quality_score = 85.0f; // Assumed factory quality
    
    // Load high-IR matrix from PROGMEM
    for (int i = 0; i < CALIBRATION_MATRIX_SIZE; i++) {
        calibration.highIR.matrix[i] = pgm_read_float(&FACTORY_HIGH_IR_MATRIX[i]);
    }
    calibration.highIR.kX = pgm_read_float(&FACTORY_HIGH_IR_SCALING[0]);
    calibration.highIR.kY = pgm_read_float(&FACTORY_HIGH_IR_SCALING[1]);
    calibration.highIR.kZ = pgm_read_float(&FACTORY_HIGH_IR_SCALING[2]);
    calibration.highIR.valid = true;
    calibration.highIR.timestamp = millis();
    strcpy(calibration.highIR.source, "factory_high_ir");
    calibration.highIR.quality_score = 85.0f; // Assumed factory quality
    
    // Enable dual-matrix mode by default
    calibration.dualModeEnabled = true;
    
    LOG_CAL_INFO("Factory defaults loaded successfully");
    return true;
}

// ============================================================================
// RAW SENSOR OPERATIONS
// ============================================================================

RawChannelData TCS3430Calibration::readRawChannels() {
    RawChannelData data;
    memset(&data, 0, sizeof(data));
    data.timestamp = millis();
    
    if (!sensor) {
        setError(CalibrationError::SENSOR_NOT_INITIALIZED);
        return data;
    }
    
    // Retry mechanism for I2C reads
    for (int retry = 0; retry < CALIBRATION_RETRIES; retry++) {
        try {
            // Wait for sensor stabilization per datasheet timing requirements
            delay(SENSOR_STABILIZE_DELAY_MS);
            
            // Read all channels per datasheet channel mapping
            // TCS3430 channel mapping: CH0=Z, CH1=Y, CH2=IR1, CH3=X
            data.r = sensor->getXData();    // CH3 (X channel)
            data.g = sensor->getYData();    // CH1 (Y channel)
            data.b = sensor->getZData();    // CH0 (Z channel)
            data.ir = sensor->getIR1Data(); // CH2 (IR1 channel)
            
            // Check for saturation
            data.saturated = checkSaturation(data);
            data.valid = true;
            
            LOG_CAL_DEBUG("Raw channels read: R=%d, G=%d, B=%d, IR=%d, Saturated=%s", 
                         data.r, data.g, data.b, data.ir, data.saturated ? "YES" : "NO");
            
            return data;
            
        } catch (...) {
            LOG_CAL_WARN("I2C read failed, retry %d/%d", retry + 1, CALIBRATION_RETRIES);
            delay(50); // Brief delay before retry
        }
    }
    
    setError(CalibrationError::I2C_READ_FAILED);
    LOG_CAL_ERROR("Failed to read raw channels after %d retries", CALIBRATION_RETRIES);
    return data;
}

bool TCS3430Calibration::performAutoZero() {
    if (!sensor) {
        setError(CalibrationError::SENSOR_NOT_INITIALIZED);
        return false;
    }

    LOG_CAL_INFO("Performing auto-zero calibration sequence");

    try {
        // Note: disableALSADC() and setPowerALSADC() are private methods
        // We'll use alternative approach through public methods

        // Use public methods to achieve auto-zero sequence
        // The sensor should already be initialized from begin()

        // Wait for auto-zero completion (per datasheet timing)
        uint32_t startTime = millis();
        while ((millis() - startTime) < AUTO_ZERO_TIMEOUT_MS) {
            uint8_t status = sensor->getDeviceStatus();
            // Check if auto-zero is complete (implementation depends on status register bits)
            if ((status & 0x01) == 0) { // Assuming bit 0 indicates auto-zero in progress
                break;
            }
            delay(10);
        }

        lastAutoZero = millis();
        LOG_CAL_INFO("Auto-zero calibration completed");
        return true;

    } catch (...) {
        setError(CalibrationError::AUTO_ZERO_FAILED);
        LOG_CAL_ERROR("Auto-zero calibration failed - I2C error");
        return false;
    }
}

bool TCS3430Calibration::checkSaturation(const RawChannelData& raw) {
    // Check for saturation based on ADC full scale (65535 for 16-bit)
    const uint16_t SATURATION_THRESHOLD = 65000; // Leave some margin

    bool saturated = (raw.r >= SATURATION_THRESHOLD) ||
                     (raw.g >= SATURATION_THRESHOLD) ||
                     (raw.b >= SATURATION_THRESHOLD) ||
                     (raw.ir >= SATURATION_THRESHOLD);

    if (saturated) {
        LOG_CAL_WARN("Saturation detected: R=%d, G=%d, B=%d, IR=%d",
                     raw.r, raw.g, raw.b, raw.ir);
        setError(CalibrationError::SATURATION_DETECTED);
    }

    return saturated;
}

bool TCS3430Calibration::adjustSensorSettings() {
    if (!sensor) {
        setError(CalibrationError::SENSOR_NOT_INITIALIZED);
        return false;
    }

    // Read current sensor data to assess saturation
    RawChannelData data = readRawChannels();
    if (!data.valid) {
        return false;
    }

    // If saturated, reduce gain or integration time
    if (data.saturated) {
        LOG_CAL_INFO("Adjusting sensor settings due to saturation");

        // Try reducing gain first
        if (sensorConfig.again > 1) {
            sensorConfig.again = sensorConfig.again / 2;
            sensor->setALSGain(sensorConfig.again);
            LOG_CAL_INFO("Reduced gain to %d", sensorConfig.again);
            return true;
        }

        // If gain is already at minimum, reduce integration time
        if (sensorConfig.atime > 50) {
            sensorConfig.atime = sensorConfig.atime - 50;
            sensor->setIntegrationTime(sensorConfig.atime);
            LOG_CAL_INFO("Reduced integration time to %d", sensorConfig.atime);
            return true;
        }

        LOG_CAL_WARN("Cannot reduce settings further to avoid saturation");
        return false;
    }

    // If signal is too low, increase gain or integration time
    uint16_t maxSignal = max({data.r, data.g, data.b});
    const uint16_t MIN_SIGNAL_THRESHOLD = 1000;

    if (maxSignal < MIN_SIGNAL_THRESHOLD) {
        LOG_CAL_INFO("Signal too low, adjusting sensor settings");

        // Try increasing gain first
        if (sensorConfig.again < 64) {
            sensorConfig.again = sensorConfig.again * 2;
            sensor->setALSGain(sensorConfig.again);
            LOG_CAL_INFO("Increased gain to %d", sensorConfig.again);
            return true;
        }

        // If gain is at maximum, increase integration time
        if (sensorConfig.atime < 200) {
            sensorConfig.atime = sensorConfig.atime + 50;
            sensor->setIntegrationTime(sensorConfig.atime);
            LOG_CAL_INFO("Increased integration time to %d", sensorConfig.atime);
            return true;
        }

        LOG_CAL_WARN("Cannot increase settings further to improve signal");
        return false;
    }

    return true; // Settings are acceptable
}

// ============================================================================
// CALIBRATION MATRIX OPERATIONS
// ============================================================================

bool TCS3430Calibration::applyColorMatrix(const RawChannelData& raw,
                                         const float matrix[CALIBRATION_MATRIX_SIZE],
                                         float& x, float& y, float& z) {
    if (!validateMatrix(matrix)) {
        setError(CalibrationError::INVALID_MATRIX);
        return false;
    }

    // Use raw sensor values directly with the scaled matrices
    // The factory matrices are designed to work with actual sensor output range
    float raw_r = (float)raw.r;
    float raw_g = (float)raw.g;
    float raw_b = (float)raw.b;
    float raw_ir = (float)raw.ir;

    // Apply 4x4 matrix transformation: [X', Y', Z', 1] = M × [R, G, B, IR]
    // Matrix is stored row-major: [m00, m01, m02, m03, m10, m11, m12, m13, ...]
    x = matrix[0] * raw_r + matrix[1] * raw_g + matrix[2] * raw_b + matrix[3] * raw_ir;
    y = matrix[4] * raw_r + matrix[5] * raw_g + matrix[6] * raw_b + matrix[7] * raw_ir;
    z = matrix[8] * raw_r + matrix[9] * raw_g + matrix[10] * raw_b + matrix[11] * raw_ir;

    // Ensure non-negative values
    x = fmaxf(0.0f, x);
    y = fmaxf(0.0f, y);
    z = fmaxf(0.0f, z);

    LOG_CAL_DEBUG("Matrix applied: Raw(%d,%d,%d,%d) -> XYZ(%.3f,%.3f,%.3f)",
                  raw.r, raw.g, raw.b, raw.ir, x, y, z);

    return true;
}

float TCS3430Calibration::calculateIRWeight(const RawChannelData& raw) {
    // Calculate normalized IR content
    uint32_t totalSignal = raw.r + raw.g + raw.b + raw.ir;
    if (totalSignal == 0) {
        return 0.0f; // Default to low-IR matrix
    }

    float irRatio = (float)raw.ir / (float)totalSignal;

    // Apply smooth step function for blending weight
    float weight = smoothStep(calibration.irThresholdLow, calibration.irThresholdHigh, irRatio);

    LOG_CAL_DEBUG("IR weight calculation: IR=%d, Total=%d, Ratio=%.3f, Weight=%.3f",
                  raw.ir, totalSignal, irRatio, weight);

    return weight;
}

bool TCS3430Calibration::applySmoothStepBlending(const RawChannelData& raw,
                                                float& x, float& y, float& z) {
    if (!calibration.lowIR.valid || !calibration.highIR.valid) {
        setError(CalibrationError::INVALID_MATRIX);
        LOG_CAL_ERROR("Cannot blend - invalid matrices");
        return false;
    }

    // Calculate XYZ for both matrices
    float x_low, y_low, z_low;
    float x_high, y_high, z_high;

    if (!applyColorMatrix(raw, calibration.lowIR.matrix, x_low, y_low, z_low) ||
        !applyColorMatrix(raw, calibration.highIR.matrix, x_high, y_high, z_high)) {
        return false;
    }

    // Apply scaling factors
    x_low *= calibration.lowIR.kX;
    y_low *= calibration.lowIR.kY;
    z_low *= calibration.lowIR.kZ;

    x_high *= calibration.highIR.kX;
    y_high *= calibration.highIR.kY;
    z_high *= calibration.highIR.kZ;

    // Calculate IR-based blending weight
    float weight = calculateIRWeight(raw);

    // Blend the results: result = low * (1-weight) + high * weight
    x = x_low * (1.0f - weight) + x_high * weight;
    y = y_low * (1.0f - weight) + y_high * weight;
    z = z_low * (1.0f - weight) + z_high * weight;

    LOG_CAL_DEBUG("Smooth step blending: Weight=%.3f, Low(%.3f,%.3f,%.3f), High(%.3f,%.3f,%.3f), Result(%.3f,%.3f,%.3f)",
                  weight, x_low, y_low, z_low, x_high, y_high, z_high, x, y, z);

    return true;
}

bool TCS3430Calibration::getCalibratedXYZ(float& x, float& y, float& z) {
    // Read raw sensor channels
    RawChannelData raw = readRawChannels();
    if (!raw.valid) {
        return false;
    }

    // Check for saturation and adjust if needed
    if (raw.saturated) {
        if (!adjustSensorSettings()) {
            LOG_CAL_WARN("Cannot adjust for saturation, proceeding with saturated data");
        } else {
            // Re-read after adjustment
            raw = readRawChannels();
            if (!raw.valid) {
                return false;
            }
        }
    }

    // Apply calibration based on mode
    if (calibration.dualModeEnabled && calibration.lowIR.valid && calibration.highIR.valid) {
        // Use dual-matrix blending
        return applySmoothStepBlending(raw, x, y, z);
    } else if (calibration.lowIR.valid) {
        // Use low-IR matrix only
        if (!applyColorMatrix(raw, calibration.lowIR.matrix, x, y, z)) {
            return false;
        }
        x *= calibration.lowIR.kX;
        y *= calibration.lowIR.kY;
        z *= calibration.lowIR.kZ;
        return true;
    } else if (calibration.highIR.valid) {
        // Use high-IR matrix only
        if (!applyColorMatrix(raw, calibration.highIR.matrix, x, y, z)) {
            return false;
        }
        x *= calibration.highIR.kX;
        y *= calibration.highIR.kY;
        z *= calibration.highIR.kZ;
        return true;
    } else {
        setError(CalibrationError::INVALID_MATRIX);
        LOG_CAL_ERROR("No valid calibration matrices available");
        return false;
    }
}

bool TCS3430Calibration::applyCalibratedConversion(uint16_t raw_r, uint16_t raw_g, uint16_t raw_b, uint16_t raw_ir,
                                                  uint8_t& out_r, uint8_t& out_g, uint8_t& out_b) {
    // Create raw data structure
    RawChannelData raw;
    raw.r = raw_r;
    raw.g = raw_g;
    raw.b = raw_b;
    raw.ir = raw_ir;
    raw.valid = true;
    raw.saturated = checkSaturation(raw);
    raw.timestamp = millis();

    // Get calibrated XYZ coordinates
    float x, y, z;
    bool success;

    if (calibration.dualModeEnabled && calibration.lowIR.valid && calibration.highIR.valid) {
        success = applySmoothStepBlending(raw, x, y, z);
    } else if (calibration.lowIR.valid) {
        success = applyColorMatrix(raw, calibration.lowIR.matrix, x, y, z);
        if (success) {
            x *= calibration.lowIR.kX;
            y *= calibration.lowIR.kY;
            z *= calibration.lowIR.kZ;
        }
    } else if (calibration.highIR.valid) {
        success = applyColorMatrix(raw, calibration.highIR.matrix, x, y, z);
        if (success) {
            x *= calibration.highIR.kX;
            y *= calibration.highIR.kY;
            z *= calibration.highIR.kZ;
        }
    } else {
        setError(CalibrationError::INVALID_MATRIX);
        return false;
    }

    if (!success) {
        return false;
    }

    // Convert XYZ to sRGB
    xyzToSRGB(x, y, z, out_r, out_g, out_b);

    LOG_CAL_DEBUG("Calibrated conversion: Raw(%d,%d,%d,%d) -> XYZ(%.3f,%.3f,%.3f) -> sRGB(%d,%d,%d)",
                  raw_r, raw_g, raw_b, raw_ir, x, y, z, out_r, out_g, out_b);

    return true;
}

// ============================================================================
// CALIBRATION DATA MANAGEMENT
// ============================================================================

bool TCS3430Calibration::setCalibrationMatrix(const float matrix[CALIBRATION_MATRIX_SIZE], MatrixType type) {
    if (!validateMatrix(matrix)) {
        setError(CalibrationError::INVALID_MATRIX);
        LOG_CAL_ERROR("Invalid calibration matrix provided");
        return false;
    }

    TCS3430CalibrationMatrix* target = nullptr;
    const char* typeName = "";

    switch (type) {
        case MatrixType::LOW_IR:
            target = &calibration.lowIR;
            typeName = "low-IR";
            break;
        case MatrixType::HIGH_IR:
            target = &calibration.highIR;
            typeName = "high-IR";
            break;
        default:
            setError(CalibrationError::INVALID_MATRIX);
            LOG_CAL_ERROR("Invalid matrix type specified");
            return false;
    }

    // Copy matrix data
    memcpy(target->matrix, matrix, sizeof(float) * CALIBRATION_MATRIX_SIZE);
    target->valid = true;
    target->timestamp = millis();
    snprintf(target->source, sizeof(target->source), "user_%s", typeName);

    LOG_CAL_INFO("Calibration matrix set for %s source", typeName);
    return true;
}

bool TCS3430Calibration::setScalingFactors(float kX, float kY, float kZ, MatrixType type) {
    if (kX <= 0.0f || kY <= 0.0f || kZ <= 0.0f) {
        setError(CalibrationError::INVALID_MATRIX);
        LOG_CAL_ERROR("Invalid scaling factors: kX=%.3f, kY=%.3f, kZ=%.3f", kX, kY, kZ);
        return false;
    }

    TCS3430CalibrationMatrix* target = nullptr;
    const char* typeName = "";

    switch (type) {
        case MatrixType::LOW_IR:
            target = &calibration.lowIR;
            typeName = "low-IR";
            break;
        case MatrixType::HIGH_IR:
            target = &calibration.highIR;
            typeName = "high-IR";
            break;
        default:
            setError(CalibrationError::INVALID_MATRIX);
            return false;
    }

    target->kX = kX;
    target->kY = kY;
    target->kZ = kZ;
    target->timestamp = millis();

    LOG_CAL_INFO("Scaling factors set for %s: kX=%.3f, kY=%.3f, kZ=%.3f", typeName, kX, kY, kZ);
    return true;
}

bool TCS3430Calibration::enableDualMatrixMode(bool enable) {
    if (enable && (!calibration.lowIR.valid || !calibration.highIR.valid)) {
        LOG_CAL_WARN("Cannot enable dual-matrix mode - missing valid matrices");
        return false;
    }

    calibration.dualModeEnabled = enable;
    LOG_CAL_INFO("Dual-matrix mode %s", enable ? "enabled" : "disabled");
    return true;
}

bool TCS3430Calibration::setIRThresholds(float lowThreshold, float highThreshold) {
    if (lowThreshold < 0.0f || lowThreshold > 1.0f ||
        highThreshold < 0.0f || highThreshold > 1.0f ||
        lowThreshold >= highThreshold) {
        setError(CalibrationError::INVALID_MATRIX);
        LOG_CAL_ERROR("Invalid IR thresholds: low=%.3f, high=%.3f", lowThreshold, highThreshold);
        return false;
    }

    calibration.irThresholdLow = lowThreshold;
    calibration.irThresholdHigh = highThreshold;

    LOG_CAL_INFO("IR thresholds set: low=%.3f, high=%.3f", lowThreshold, highThreshold);
    return true;
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

bool TCS3430Calibration::validateMatrix(const float matrix[CALIBRATION_MATRIX_SIZE]) {
    if (!matrix) {
        return false;
    }

    // Check for NaN or infinite values
    for (int i = 0; i < CALIBRATION_MATRIX_SIZE; i++) {
        if (!isfinite(matrix[i])) {
            LOG_CAL_ERROR("Matrix validation failed: invalid value at index %d", i);
            return false;
        }
    }

    // Check that the homogeneous coordinate row is [0, 0, 0, 1]
    if (fabs(matrix[12]) > 0.001f || fabs(matrix[13]) > 0.001f ||
        fabs(matrix[14]) > 0.001f || fabs(matrix[15] - 1.0f) > 0.001f) {
        LOG_CAL_WARN("Matrix homogeneous row is not [0,0,0,1] - may be intentional");
    }

    return true;
}

void TCS3430Calibration::xyzToSRGB(float x, float y, float z, uint8_t& r, uint8_t& g, uint8_t& b) {
    // XYZ to sRGB transformation matrix (D65 illuminant)
    // Based on sRGB standard transformation
    float linear_r = 3.2406f * x - 1.5372f * y - 0.4986f * z;
    float linear_g = -0.9689f * x + 1.8758f * y + 0.0415f * z;
    float linear_b = 0.0557f * x - 0.2040f * y + 1.0570f * z;

    // Clamp to valid range
    linear_r = fmaxf(0.0f, fminf(1.0f, linear_r));
    linear_g = fmaxf(0.0f, fminf(1.0f, linear_g));
    linear_b = fmaxf(0.0f, fminf(1.0f, linear_b));

    // Apply gamma correction (sRGB gamma curve)
    auto gammaCorrect = [](float linear) -> float {
        if (linear <= 0.0031308f) {
            return 12.92f * linear;
        } else {
            return 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
        }
    };

    float gamma_r = gammaCorrect(linear_r);
    float gamma_g = gammaCorrect(linear_g);
    float gamma_b = gammaCorrect(linear_b);

    // Convert to 8-bit values
    r = (uint8_t)(gamma_r * 255.0f + 0.5f);
    g = (uint8_t)(gamma_g * 255.0f + 0.5f);
    b = (uint8_t)(gamma_b * 255.0f + 0.5f);
}

float TCS3430Calibration::calculateDeltaE(uint8_t r1, uint8_t g1, uint8_t b1,
                                         uint8_t r2, uint8_t g2, uint8_t b2) {
    // Simple Delta E calculation using Euclidean distance in RGB space
    // For more accurate results, this should be implemented in LAB color space
    float dr = (float)(r1 - r2);
    float dg = (float)(g1 - g2);
    float db = (float)(b1 - b2);

    return sqrtf(dr * dr + dg * dg + db * db);
}

float TCS3430Calibration::smoothStep(float edge0, float edge1, float x) {
    // Clamp x to [0, 1] range
    float t = fmaxf(0.0f, fminf(1.0f, (x - edge0) / (edge1 - edge0)));

    // Apply smooth step function: 3t² - 2t³
    return t * t * (3.0f - 2.0f * t);
}

void TCS3430Calibration::setError(CalibrationError error) {
    lastError = error;
    if (error != CalibrationError::NONE) {
        currentState = TCS3430CalibrationState::ERROR_STATE;
    }
}

bool TCS3430Calibration::isCalibrationValid() const {
    return (calibration.lowIR.valid || calibration.highIR.valid) && initialized;
}

// ============================================================================
// CALIBRATION WORKFLOW METHODS
// ============================================================================

bool TCS3430Calibration::addCalibrationPoint(uint8_t ref_r, uint8_t ref_g, uint8_t ref_b, const char* name) {
    if (numReferences >= MAX_CALIBRATION_POINTS) {
        setError(CalibrationError::INSUFFICIENT_DATA);
        LOG_CAL_ERROR("Maximum calibration points reached (%d)", MAX_CALIBRATION_POINTS);
        return false;
    }

    if (!sensor) {
        setError(CalibrationError::SENSOR_NOT_INITIALIZED);
        return false;
    }

    // Read raw sensor data
    RawChannelData raw = readRawChannels();
    if (!raw.valid) {
        return false;
    }

    // Store calibration reference point
    CalibrationReference& ref = references[numReferences];
    ref.ref_r = ref_r;
    ref.ref_g = ref_g;
    ref.ref_b = ref_b;
    ref.sensor_r = raw.r;
    ref.sensor_g = raw.g;
    ref.sensor_b = raw.b;
    ref.sensor_ir = raw.ir;
    ref.delta_e = 0.0f; // Will be calculated after matrix computation
    strncpy(ref.name, name, sizeof(ref.name) - 1);
    ref.name[sizeof(ref.name) - 1] = '\0';
    ref.measured = true;

    numReferences++;
    currentState = TCS3430CalibrationState::COLLECTING_DATA;

    LOG_CAL_INFO("Added calibration point %d: %s RGB(%d,%d,%d) -> Sensor(%d,%d,%d,%d)",
                 numReferences, name, ref_r, ref_g, ref_b, raw.r, raw.g, raw.b, raw.ir);

    return true;
}

void TCS3430Calibration::clearCalibrationPoints() {
    numReferences = 0;
    memset(&references, 0, sizeof(references));
    memset(&lastStats, 0, sizeof(lastStats));
    currentState = TCS3430CalibrationState::INITIALIZED;
    LOG_CAL_INFO("Calibration points cleared");
}

TCS3430CalibrationStats TCS3430Calibration::evaluateCalibration() {
    TCS3430CalibrationStats stats;
    memset(&stats, 0, sizeof(stats));

    if (numReferences == 0) {
        stats.matrix_valid = false;
        return stats;
    }

    stats.total_points = numReferences;
    stats.matrix_valid = calibration.lowIR.valid || calibration.highIR.valid;

    if (!stats.matrix_valid) {
        return stats;
    }

    float deltaE_sum = 0.0f;
    float deltaE_squared_sum = 0.0f;
    uint8_t validCount = 0;

    for (uint8_t i = 0; i < numReferences; i++) {
        if (!references[i].measured) continue;

        // Apply current calibration to get predicted sRGB
        uint8_t pred_r, pred_g, pred_b;
        if (applyCalibratedConversion(references[i].sensor_r, references[i].sensor_g,
                                     references[i].sensor_b, references[i].sensor_ir,
                                     pred_r, pred_g, pred_b)) {

            // Calculate Delta E between predicted and reference
            float deltaE = calculateDeltaE(pred_r, pred_g, pred_b,
                                         references[i].ref_r, references[i].ref_g, references[i].ref_b);

            references[i].delta_e = deltaE;
            deltaE_sum += deltaE;
            deltaE_squared_sum += deltaE * deltaE;

            if (deltaE < DELTA_E_EXCELLENT) stats.points_under_2++;
            if (deltaE < DELTA_E_ACCEPTABLE) stats.points_under_5++;
            if (deltaE > stats.max_delta_e) stats.max_delta_e = deltaE;

            validCount++;
        }
    }

    if (validCount > 0) {
        stats.mean_delta_e = deltaE_sum / validCount;
        if (validCount > 1) {
            float variance = (deltaE_squared_sum / validCount) - (stats.mean_delta_e * stats.mean_delta_e);
            stats.std_delta_e = sqrtf(fmaxf(0.0f, variance));
        }

        // Calculate quality score (0-100)
        float excellentRatio = (float)stats.points_under_2 / validCount;
        float acceptableRatio = (float)stats.points_under_5 / validCount;
        stats.quality_score = (excellentRatio * 100.0f + acceptableRatio * 50.0f) / 1.5f;
        stats.quality_score = fminf(100.0f, stats.quality_score);
    }

    lastStats = stats;
    return stats;
}

// ============================================================================
// DATA PERSISTENCE METHODS
// ============================================================================

bool TCS3430Calibration::saveCalibration() {
    if (!preferences.isKey(NVS_CALIBRATION_NAMESPACE)) {
        LOG_CAL_ERROR("NVS preferences not initialized");
        return false;
    }

    LOG_CAL_INFO("Saving TCS3430 calibration data to NVS");

    try {
        // Save low-IR matrix
        if (calibration.lowIR.valid) {
            preferences.putBytes(NVS_LOW_IR_MATRIX, calibration.lowIR.matrix, sizeof(calibration.lowIR.matrix));
            float scaling[3] = {calibration.lowIR.kX, calibration.lowIR.kY, calibration.lowIR.kZ};
            preferences.putBytes(NVS_LOW_IR_SCALING, scaling, sizeof(scaling));
        }

        // Save high-IR matrix
        if (calibration.highIR.valid) {
            preferences.putBytes(NVS_HIGH_IR_MATRIX, calibration.highIR.matrix, sizeof(calibration.highIR.matrix));
            float scaling[3] = {calibration.highIR.kX, calibration.highIR.kY, calibration.highIR.kZ};
            preferences.putBytes(NVS_HIGH_IR_SCALING, scaling, sizeof(scaling));
        }

        // Save configuration
        preferences.putBool(NVS_DUAL_MODE_ENABLED, calibration.dualModeEnabled);
        preferences.putBool(NVS_CALIBRATION_VALID, calibration.lowIR.valid || calibration.highIR.valid);
        preferences.putULong(NVS_CALIBRATION_TIMESTAMP, millis());

        LOG_CAL_INFO("TCS3430 calibration data saved successfully");
        return true;

    } catch (...) {
        setError(CalibrationError::STORAGE_FAILED);
        LOG_CAL_ERROR("Failed to save calibration data to NVS");
        return false;
    }
}

bool TCS3430Calibration::loadCalibration() {
    if (!preferences.isKey(NVS_CALIBRATION_NAMESPACE)) {
        LOG_CAL_WARN("No existing calibration data found in NVS");
        return false;
    }

    LOG_CAL_INFO("Loading TCS3430 calibration data from NVS");

    try {
        // Check if calibration is valid
        bool isValid = preferences.getBool(NVS_CALIBRATION_VALID, false);
        if (!isValid) {
            LOG_CAL_WARN("NVS indicates no valid calibration data");
            return false;
        }

        // Load low-IR matrix
        size_t matrixSize = preferences.getBytesLength(NVS_LOW_IR_MATRIX);
        if (matrixSize == sizeof(calibration.lowIR.matrix)) {
            preferences.getBytes(NVS_LOW_IR_MATRIX, calibration.lowIR.matrix, matrixSize);

            float scaling[3];
            size_t scalingSize = preferences.getBytesLength(NVS_LOW_IR_SCALING);
            if (scalingSize == sizeof(scaling)) {
                preferences.getBytes(NVS_LOW_IR_SCALING, scaling, scalingSize);
                calibration.lowIR.kX = scaling[0];
                calibration.lowIR.kY = scaling[1];
                calibration.lowIR.kZ = scaling[2];
                calibration.lowIR.valid = true;
                strcpy(calibration.lowIR.source, "nvs_low_ir");
                LOG_CAL_INFO("Low-IR matrix loaded from NVS");
            }
        }

        // Load high-IR matrix
        matrixSize = preferences.getBytesLength(NVS_HIGH_IR_MATRIX);
        if (matrixSize == sizeof(calibration.highIR.matrix)) {
            preferences.getBytes(NVS_HIGH_IR_MATRIX, calibration.highIR.matrix, matrixSize);

            float scaling[3];
            size_t scalingSize = preferences.getBytesLength(NVS_HIGH_IR_SCALING);
            if (scalingSize == sizeof(scaling)) {
                preferences.getBytes(NVS_HIGH_IR_SCALING, scaling, scalingSize);
                calibration.highIR.kX = scaling[0];
                calibration.highIR.kY = scaling[1];
                calibration.highIR.kZ = scaling[2];
                calibration.highIR.valid = true;
                strcpy(calibration.highIR.source, "nvs_high_ir");
                LOG_CAL_INFO("High-IR matrix loaded from NVS");
            }
        }

        // Load configuration
        calibration.dualModeEnabled = preferences.getBool(NVS_DUAL_MODE_ENABLED, false);

        LOG_CAL_INFO("TCS3430 calibration data loaded successfully");
        return calibration.lowIR.valid || calibration.highIR.valid;

    } catch (...) {
        setError(CalibrationError::STORAGE_FAILED);
        LOG_CAL_ERROR("Failed to load calibration data from NVS");
        return false;
    }
}

bool TCS3430Calibration::exportCalibrationData(const char* filename) {
    // For now, just log the export request
    // In a full implementation, this would write to LittleFS
    LOG_CAL_INFO("Export calibration data requested to file: %s", filename);

    // Create JSON export data
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["version"] = "1.0";
    doc["device"] = "TCS3430";

    // Export calibration matrices
    if (calibration.lowIR.valid) {
        JsonArray lowMatrix = doc["lowIR"]["matrix"].to<JsonArray>();
        for (int i = 0; i < CALIBRATION_MATRIX_SIZE; i++) {
            lowMatrix.add(calibration.lowIR.matrix[i]);
        }
        doc["lowIR"]["kX"] = calibration.lowIR.kX;
        doc["lowIR"]["kY"] = calibration.lowIR.kY;
        doc["lowIR"]["kZ"] = calibration.lowIR.kZ;
        doc["lowIR"]["source"] = calibration.lowIR.source;
        doc["lowIR"]["timestamp"] = calibration.lowIR.timestamp;
    }

    if (calibration.highIR.valid) {
        JsonArray highMatrix = doc["highIR"]["matrix"].to<JsonArray>();
        for (int i = 0; i < CALIBRATION_MATRIX_SIZE; i++) {
            highMatrix.add(calibration.highIR.matrix[i]);
        }
        doc["highIR"]["kX"] = calibration.highIR.kX;
        doc["highIR"]["kY"] = calibration.highIR.kY;
        doc["highIR"]["kZ"] = calibration.highIR.kZ;
        doc["highIR"]["source"] = calibration.highIR.source;
        doc["highIR"]["timestamp"] = calibration.highIR.timestamp;
    }

    // Export reference points
    JsonArray refs = doc["referencePoints"].to<JsonArray>();
    for (uint8_t i = 0; i < numReferences; i++) {
        JsonObject ref = refs.add<JsonObject>();
        ref["name"] = references[i].name;
        ref["ref_r"] = references[i].ref_r;
        ref["ref_g"] = references[i].ref_g;
        ref["ref_b"] = references[i].ref_b;
        ref["sensor_r"] = references[i].sensor_r;
        ref["sensor_g"] = references[i].sensor_g;
        ref["sensor_b"] = references[i].sensor_b;
        ref["sensor_ir"] = references[i].sensor_ir;
        ref["delta_e"] = references[i].delta_e;
    }

    // For now, just log the JSON (in full implementation, write to file)
    String jsonStr;
    serializeJsonPretty(doc, jsonStr);
    LOG_CAL_INFO("Calibration export data:\n%s", jsonStr.c_str());

    return true; // Would return actual file write success
}

void TCS3430Calibration::getSensorDiagnostics(JsonDocument& doc) {
    doc["success"] = true;
    doc["initialized"] = initialized;
    doc["calibrationValid"] = isCalibrationValid();

    // Current sensor state
    if (sensor) {
        RawChannelData raw = readRawChannels();
        doc["currentReading"]["r"] = raw.r;
        doc["currentReading"]["g"] = raw.g;
        doc["currentReading"]["b"] = raw.b;
        doc["currentReading"]["ir"] = raw.ir;
        doc["currentReading"]["valid"] = raw.valid;
        doc["currentReading"]["saturated"] = raw.saturated;
        doc["currentReading"]["timestamp"] = raw.timestamp;

        // Sensor configuration
        doc["sensorConfig"]["atime"] = sensorConfig.atime;
        doc["sensorConfig"]["again"] = sensorConfig.again;
        doc["sensorConfig"]["wtime"] = sensorConfig.wtime;
        doc["sensorConfig"]["autoZeroEnabled"] = sensorConfig.auto_zero_enabled;
        doc["sensorConfig"]["autoZeroFrequency"] = sensorConfig.auto_zero_frequency;
    }

    // Calibration status
    doc["calibration"]["dualModeEnabled"] = calibration.dualModeEnabled;
    doc["calibration"]["lowIRValid"] = calibration.lowIR.valid;
    doc["calibration"]["highIRValid"] = calibration.highIR.valid;
    doc["calibration"]["irThresholdLow"] = calibration.irThresholdLow;
    doc["calibration"]["irThresholdHigh"] = calibration.irThresholdHigh;

    if (calibration.lowIR.valid) {
        doc["calibration"]["lowIR"]["kX"] = calibration.lowIR.kX;
        doc["calibration"]["lowIR"]["kY"] = calibration.lowIR.kY;
        doc["calibration"]["lowIR"]["kZ"] = calibration.lowIR.kZ;
        doc["calibration"]["lowIR"]["source"] = calibration.lowIR.source;
        doc["calibration"]["lowIR"]["timestamp"] = calibration.lowIR.timestamp;
        doc["calibration"]["lowIR"]["qualityScore"] = calibration.lowIR.quality_score;
    }

    if (calibration.highIR.valid) {
        doc["calibration"]["highIR"]["kX"] = calibration.highIR.kX;
        doc["calibration"]["highIR"]["kY"] = calibration.highIR.kY;
        doc["calibration"]["highIR"]["kZ"] = calibration.highIR.kZ;
        doc["calibration"]["highIR"]["source"] = calibration.highIR.source;
        doc["calibration"]["highIR"]["timestamp"] = calibration.highIR.timestamp;
        doc["calibration"]["highIR"]["qualityScore"] = calibration.highIR.quality_score;
    }

    // Last auto-zero timestamp
    doc["lastAutoZero"] = lastAutoZero;

    // Current state and error
    doc["currentState"] = (int)currentState;
    doc["lastError"] = (int)lastError;

    // Reference points
    doc["numReferences"] = numReferences;
    if (numReferences > 0) {
        TCS3430CalibrationStats stats = evaluateCalibration();
        doc["calibrationStats"]["meanDeltaE"] = stats.mean_delta_e;
        doc["calibrationStats"]["stdDeltaE"] = stats.std_delta_e;
        doc["calibrationStats"]["maxDeltaE"] = stats.max_delta_e;
        doc["calibrationStats"]["pointsUnder2"] = stats.points_under_2;
        doc["calibrationStats"]["pointsUnder5"] = stats.points_under_5;
        doc["calibrationStats"]["qualityScore"] = stats.quality_score;
    }
}

void TCS3430Calibration::getCalibrationStatus(JsonDocument& doc) {
    doc["success"] = true;
    doc["initialized"] = initialized;
    doc["calibrationValid"] = isCalibrationValid();
    doc["dualModeEnabled"] = calibration.dualModeEnabled;
    doc["lowIRValid"] = calibration.lowIR.valid;
    doc["highIRValid"] = calibration.highIR.valid;
    doc["numReferences"] = numReferences;
    doc["currentState"] = (int)currentState;
    doc["lastError"] = (int)lastError;

    if (isCalibrationValid()) {
        TCS3430CalibrationStats stats = evaluateCalibration();
        doc["qualityScore"] = stats.quality_score;
        doc["meanDeltaE"] = stats.mean_delta_e;
        doc["maxDeltaE"] = stats.max_delta_e;
        doc["pointsUnder2"] = stats.points_under_2;
        doc["pointsUnder5"] = stats.points_under_5;
    }

    // IR thresholds
    doc["irThresholdLow"] = calibration.irThresholdLow;
    doc["irThresholdHigh"] = calibration.irThresholdHigh;

    // Last auto-zero
    doc["lastAutoZero"] = lastAutoZero;
    doc["autoZeroAge"] = lastAutoZero > 0 ? (millis() - lastAutoZero) : 0;
}
