#include "dynamic_sensor.h"
#include <cmath>

DynamicSensorManager::DynamicSensorManager(DFRobot_TCS3430* tcs3430) 
  : sensor(tcs3430), initialized(false), lastAdjustmentTime(0), 
    adjustmentAttempts(0), lastDetectedCondition(LIGHT_INDOOR),
    readingIndex(0), statisticsReady(false) {
  
  // Initialize current config with defaults
  currentConfig.atime = DEFAULT_ATIME;
  currentConfig.again = DEFAULT_AGAIN;
  currentConfig.brightness = DEFAULT_BRIGHTNESS;
  currentConfig.condition = LIGHT_INDOOR;
  currentConfig.isOptimal = false;
  currentConfig.timestamp = 0;
  
  // Clear readings array
  memset(recentReadings, 0, sizeof(recentReadings));
}

bool DynamicSensorManager::initialize() {
  if (!sensor) {
    LOG_SENSOR_ERROR("Dynamic sensor manager: No sensor provided");
    return false;
  }
  
  LOG_SENSOR_INFO("Initializing dynamic sensor management system");
  
  // Initialize optimal configurations for different lighting conditions
  initializeOptimalConfigs();
  
  // Apply default configuration
  if (!applySensorConfig(currentConfig)) {
    LOG_SENSOR_ERROR("Failed to apply default sensor configuration");
    return false;
  }
  
  initialized = true;
  LOG_SENSOR_INFO("Dynamic sensor manager initialized successfully");
  return true;
}

void DynamicSensorManager::initializeOptimalConfigs() {
  // Dark/Low-light conditions: High gain, longer integration
  optimalConfigs[LIGHT_DARK] = {
    .atime = ATIME_MAX,        // 200ms for maximum sensitivity
    .again = GAIN_64X,         // 64x gain for low light
    .brightness = MAX_LED_BRIGHTNESS, // Maximum LED brightness
    .condition = LIGHT_DARK,
    .isOptimal = true,
    .timestamp = 0
  };
  
  // Indoor conditions: Balanced settings
  optimalConfigs[LIGHT_INDOOR] = {
    .atime = ATIME_MID,        // 100ms balanced integration
    .again = GAIN_16X,         // 16x gain for indoor
    .brightness = DEFAULT_BRIGHTNESS, // Standard brightness
    .condition = LIGHT_INDOOR,
    .isOptimal = true,
    .timestamp = 0
  };
  
  // Bright conditions: Lower gain, shorter integration
  optimalConfigs[LIGHT_BRIGHT] = {
    .atime = ATIME_MIN,        // 20ms for fast response
    .again = GAIN_4X,          // 4x gain for bright light
    .brightness = MIN_LED_BRIGHTNESS + 32, // Reduced brightness
    .condition = LIGHT_BRIGHT,
    .isOptimal = true,
    .timestamp = 0
  };
  
  // Very bright conditions: Minimum gain and integration
  optimalConfigs[LIGHT_VERY_BRIGHT] = {
    .atime = ATIME_MIN,        // 20ms minimum integration
    .again = GAIN_1X,          // 1x gain for very bright
    .brightness = MIN_LED_BRIGHTNESS, // Minimum brightness
    .condition = LIGHT_VERY_BRIGHT,
    .isOptimal = true,
    .timestamp = 0
  };
  
  LOG_SENSOR_DEBUG("Optimal configurations initialized for all lighting conditions");
}

LightingCondition DynamicSensorManager::detectLightingCondition() {
  // Take a quick reading to assess lighting
  uint16_t yReading = sensor->getYData(); // Y channel for brightness assessment
  
  LOG_SENSOR_DEBUG("Lighting detection: Y channel = %u", yReading);
  
  if (yReading < LIGHT_CONDITION_DARK) {
    return LIGHT_DARK;
  } else if (yReading < LIGHT_CONDITION_INDOOR) {
    return LIGHT_INDOOR;
  } else if (yReading < LIGHT_CONDITION_BRIGHT) {
    return LIGHT_BRIGHT;
  } else {
    return LIGHT_VERY_BRIGHT;
  }
}

SensorConfig DynamicSensorManager::getOptimalConfig(LightingCondition condition) {
  if (condition >= 0 && condition < 4) {
    return optimalConfigs[condition];
  }
  
  // Fallback to indoor config
  LOG_SENSOR_WARN("Invalid lighting condition %d, using indoor config", condition);
  return optimalConfigs[LIGHT_INDOOR];
}

bool DynamicSensorManager::applySensorConfig(const SensorConfig& config) {
  if (!validateConfig(config)) {
    LOG_SENSOR_ERROR("Invalid sensor configuration");
    return false;
  }
  
  LOG_SENSOR_DEBUG("Applying sensor config: ATIME=%u AGAIN=%u Brightness=%u", 
                   config.atime, config.again, config.brightness);
  
  // Apply settings to sensor
  sensor->setIntegrationTime(config.atime);
  sensor->setALSGain(config.again);
  
  // Update current configuration
  currentConfig = config;
  currentConfig.timestamp = millis();
  
  // Allow sensor to stabilize
  delay(ADJUSTMENT_DELAY_MS);
  
  LOG_SENSOR_INFO("Sensor configuration applied successfully");
  return true;
}

bool DynamicSensorManager::optimizeSensorSettings() {
  if (!initialized) {
    LOG_SENSOR_ERROR("Dynamic sensor manager not initialized");
    return false;
  }
  
  // Prevent too frequent adjustments
  if (millis() - lastAdjustmentTime < ADJUSTMENT_DELAY_MS * 2) {
    return true; // Skip adjustment, too soon
  }
  
  LOG_SENSOR_DEBUG("Starting sensor optimization");
  
  // Detect current lighting condition
  LightingCondition condition = detectLightingCondition();
  
  // Check if we need to change configuration
  if (condition != currentConfig.condition || !currentConfig.isOptimal) {
    LOG_SENSOR_INFO("Lighting condition changed: %d -> %d", 
                     currentConfig.condition, condition);
    
    // Get optimal configuration for new condition
    SensorConfig newConfig = getOptimalConfig(condition);
    
    // Apply new configuration
    if (applySensorConfig(newConfig)) {
      lastAdjustmentTime = millis();
      lastDetectedCondition = condition;
      adjustmentAttempts = 0;
      LOG_SENSOR_INFO("Sensor optimized for lighting condition %d", condition);
      return true;
    } else {
      adjustmentAttempts++;
      LOG_SENSOR_ERROR("Failed to apply optimal configuration (attempt %d)", 
                       adjustmentAttempts);
      return false;
    }
  }
  
  // Check for saturation and adjust if needed
  if (checkSaturation()) {
    LOG_SENSOR_WARN("Saturation detected, reducing sensitivity");
    
    // Try reducing gain first
    if (currentConfig.again > GAIN_1X) {
      currentConfig.again--;
      return applySensorConfig(currentConfig);
    }
    
    // If gain is already minimum, reduce integration time
    if (currentConfig.atime > ATIME_MIN) {
      currentConfig.atime = max(ATIME_MIN, currentConfig.atime - 20);
      return applySensorConfig(currentConfig);
    }
    
    LOG_SENSOR_WARN("Cannot reduce sensitivity further, saturation may persist");
  }
  
  // Check for low signal and adjust if needed
  if (!checkSignalAdequacy()) {
    LOG_SENSOR_WARN("Low signal detected, increasing sensitivity");
    
    // Try increasing integration time first
    if (currentConfig.atime < ATIME_MAX) {
      currentConfig.atime = min(ATIME_MAX, currentConfig.atime + 20);
      return applySensorConfig(currentConfig);
    }
    
    // If integration time is already maximum, increase gain
    if (currentConfig.again < GAIN_64X) {
      currentConfig.again++;
      return applySensorConfig(currentConfig);
    }
    
    LOG_SENSOR_WARN("Cannot increase sensitivity further, signal may remain low");
  }
  
  return true;
}

bool DynamicSensorManager::checkSaturation() {
  uint8_t status = sensor->getDeviceStatus();
  bool hardwareSaturation = (status & 0x10) != 0; // ASAT bit
  
  if (hardwareSaturation) {
    LOG_SENSOR_DEBUG("Hardware saturation detected (ASAT bit set)");
    return true;
  }
  
  // Check individual channels for near-saturation
  uint16_t x = sensor->getXData();
  uint16_t y = sensor->getYData();
  uint16_t z = sensor->getZData();
  
  bool softSaturation = (x > ADC_TARGET_MAX || y > ADC_TARGET_MAX || z > ADC_TARGET_MAX);
  
  if (softSaturation) {
    LOG_SENSOR_DEBUG("Soft saturation detected: X=%u Y=%u Z=%u (limit=%u)", 
                     x, y, z, ADC_TARGET_MAX);
  }
  
  return softSaturation;
}

bool DynamicSensorManager::checkSignalAdequacy() {
  uint16_t x = sensor->getXData();
  uint16_t y = sensor->getYData();
  uint16_t z = sensor->getZData();
  
  bool adequate = (x > ADC_TARGET_MIN && y > ADC_TARGET_MIN && z > ADC_TARGET_MIN);
  
  if (!adequate) {
    LOG_SENSOR_DEBUG("Inadequate signal: X=%u Y=%u Z=%u (min=%u)", 
                     x, y, z, ADC_TARGET_MIN);
  }
  
  return adequate;
}

bool DynamicSensorManager::validateConfig(const SensorConfig& config) {
  // Validate ATIME range
  if (config.atime < ATIME_MIN || config.atime > 255) {
    LOG_SENSOR_ERROR("Invalid ATIME: %u (range: %u-255)", config.atime, ATIME_MIN);
    return false;
  }
  
  // Validate AGAIN range
  if (config.again > GAIN_64X) {
    LOG_SENSOR_ERROR("Invalid AGAIN: %u (range: 0-3)", config.again);
    return false;
  }
  
  // Validate brightness range
  if (config.brightness < MIN_LED_BRIGHTNESS || config.brightness > MAX_LED_BRIGHTNESS) {
    LOG_SENSOR_ERROR("Invalid brightness: %u (range: %u-%u)", 
                     config.brightness, MIN_LED_BRIGHTNESS, MAX_LED_BRIGHTNESS);
    return false;
  }
  
  return true;
}

void DynamicSensorManager::resetToDefaults() {
  LOG_SENSOR_INFO("Resetting dynamic sensor manager to defaults");
  
  currentConfig.atime = DEFAULT_ATIME;
  currentConfig.again = DEFAULT_AGAIN;
  currentConfig.brightness = DEFAULT_BRIGHTNESS;
  currentConfig.condition = LIGHT_INDOOR;
  currentConfig.isOptimal = false;
  currentConfig.timestamp = millis();
  
  if (initialized) {
    applySensorConfig(currentConfig);
  }
  
  // Reset state variables
  lastAdjustmentTime = 0;
  adjustmentAttempts = 0;
  lastDetectedCondition = LIGHT_INDOOR;
  readingIndex = 0;
  statisticsReady = false;
  
  memset(recentReadings, 0, sizeof(recentReadings));
}

bool DynamicSensorManager::performQualityReading(uint16_t& x, uint16_t& y, uint16_t& z,
                                               uint16_t& ir1, uint16_t& ir2, ReadingQuality& quality) {
  if (!initialized) {
    LOG_SENSOR_ERROR("Dynamic sensor manager not initialized");
    return false;
  }

  LOG_SENSOR_DEBUG("Performing quality reading with %d samples", RAPID_SCAN_SAMPLES);

  // Arrays to store multiple readings
  uint16_t xReadings[RAPID_SCAN_SAMPLES];
  uint16_t yReadings[RAPID_SCAN_SAMPLES];
  uint16_t zReadings[RAPID_SCAN_SAMPLES];
  uint16_t ir1Readings[RAPID_SCAN_SAMPLES];
  uint16_t ir2Readings[RAPID_SCAN_SAMPLES];

  // Collect multiple rapid samples
  for (int i = 0; i < RAPID_SCAN_SAMPLES; i++) {
    if (i > 0) {
      delay(RAPID_SCAN_INTERVAL_MS);
    }

    xReadings[i] = sensor->getXData();
    yReadings[i] = sensor->getYData();
    zReadings[i] = sensor->getZData();
    ir1Readings[i] = sensor->getIR1Data();
    ir2Readings[i] = sensor->getIR2Data();

    LOG_SENSOR_DEBUG("Sample %d: X=%u Y=%u Z=%u IR1=%u IR2=%u",
                     i+1, xReadings[i], yReadings[i], zReadings[i],
                     ir1Readings[i], ir2Readings[i]);
  }

  // Calculate statistics for each channel
  SampleStatistics xStats = calculateStatistics(xReadings, RAPID_SCAN_SAMPLES);
  SampleStatistics yStats = calculateStatistics(yReadings, RAPID_SCAN_SAMPLES);
  SampleStatistics zStats = calculateStatistics(zReadings, RAPID_SCAN_SAMPLES);

  // Calculate averages (using mean from statistics)
  x = (uint16_t)xStats.mean;
  y = (uint16_t)yStats.mean;
  z = (uint16_t)zStats.mean;

  // Calculate IR averages
  uint32_t ir1Sum = 0, ir2Sum = 0;
  for (int i = 0; i < RAPID_SCAN_SAMPLES; i++) {
    ir1Sum += ir1Readings[i];
    ir2Sum += ir2Readings[i];
  }
  ir1 = ir1Sum / RAPID_SCAN_SAMPLES;
  ir2 = ir2Sum / RAPID_SCAN_SAMPLES;

  // Analyze quality metrics
  quality.coefficientOfVariation = max({xStats.coefficientOfVariation,
                                       yStats.coefficientOfVariation,
                                       zStats.coefficientOfVariation});

  quality.maxReading = max({x, y, z});
  quality.minReading = min({x, y, z});

  quality.hasSaturation = (quality.maxReading > ADC_TARGET_MAX) || checkSaturation();
  quality.hasLowSignal = (quality.minReading < ADC_TARGET_MIN);

  // Calculate overall quality score
  quality.qualityScore = calculateQualityScore(yReadings, RAPID_SCAN_SAMPLES);

  LOG_SENSOR_INFO("Quality reading complete: X=%u Y=%u Z=%u IR1=%u IR2=%u CV=%.3f Score=%u",
                  x, y, z, ir1, ir2, quality.coefficientOfVariation, quality.qualityScore);

  return true;
}

void DynamicSensorManager::applyIRCompensation(uint8_t& r, uint8_t& g, uint8_t& b,
                                              uint16_t ir1, uint16_t ir2) {
  if (!IR_COMPENSATION_ENABLED) {
    return;
  }

  // Use average of IR1 and IR2 for compensation
  float irLevel = (ir1 + ir2) / 2.0f;

  // Only apply compensation if IR level is significant
  if (irLevel < IR_THRESHOLD_LOW) {
    LOG_SENSOR_DEBUG("IR level too low for compensation: %.1f", irLevel);
    return;
  }

  // Calculate IR correction factors based on IR level
  float irFactor = min(1.0f, irLevel / IR_THRESHOLD_HIGH);

  // Apply IR compensation (subtract IR contribution from each channel)
  float rCorrected = r - (irLevel * IR_CORRECTION_FACTOR_R * irFactor);
  float gCorrected = g - (irLevel * IR_CORRECTION_FACTOR_G * irFactor);
  float bCorrected = b - (irLevel * IR_CORRECTION_FACTOR_B * irFactor);

  // Clamp to valid range
  r = (uint8_t)max(0.0f, min(255.0f, rCorrected));
  g = (uint8_t)max(0.0f, min(255.0f, gCorrected));
  b = (uint8_t)max(0.0f, min(255.0f, bCorrected));

  LOG_SENSOR_DEBUG("IR compensation applied: IR=%.1f Factor=%.3f RGB(%u,%u,%u)",
                   irLevel, irFactor, r, g, b);
}

SampleStatistics DynamicSensorManager::calculateStatistics(uint16_t* readings, uint8_t count) {
  SampleStatistics stats = {0};

  if (count == 0) {
    return stats;
  }

  // Calculate mean
  uint32_t sum = 0;
  stats.min = readings[0];
  stats.max = readings[0];

  for (uint8_t i = 0; i < count; i++) {
    sum += readings[i];
    if (readings[i] < stats.min) stats.min = readings[i];
    if (readings[i] > stats.max) stats.max = readings[i];
  }

  stats.mean = (float)sum / count;

  // Calculate standard deviation
  float variance = 0;
  for (uint8_t i = 0; i < count; i++) {
    float diff = readings[i] - stats.mean;
    variance += diff * diff;
  }
  variance /= count;
  stats.standardDeviation = sqrt(variance);

  // Calculate coefficient of variation
  if (stats.mean > 0) {
    stats.coefficientOfVariation = stats.standardDeviation / stats.mean;
  }

  // Count outliers (readings beyond 2 standard deviations)
  stats.outlierCount = 0;
  float threshold = OUTLIER_DETECTION_SIGMA * stats.standardDeviation;
  for (uint8_t i = 0; i < count; i++) {
    if (abs(readings[i] - stats.mean) > threshold) {
      stats.outlierCount++;
    }
  }

  return stats;
}

uint8_t DynamicSensorManager::calculateQualityScore(uint16_t* readings, uint8_t count) {
  SampleStatistics stats = calculateStatistics(readings, count);

  uint8_t score = 100; // Start with perfect score

  // Penalize high coefficient of variation (inconsistency)
  if (stats.coefficientOfVariation > MAX_COEFFICIENT_VARIATION) {
    score -= 30;
  } else if (stats.coefficientOfVariation > MAX_COEFFICIENT_VARIATION / 2) {
    score -= 15;
  }

  // Penalize saturation
  if (stats.max > ADC_TARGET_MAX) {
    score -= 25;
  } else if (stats.max > ADC_TARGET_HIGH) {
    score -= 10;
  }

  // Penalize low signal
  if (stats.min < ADC_TARGET_MIN) {
    score -= 25;
  } else if (stats.min < ADC_TARGET_LOW) {
    score -= 10;
  }

  // Penalize outliers
  if (stats.outlierCount > count / 4) { // More than 25% outliers
    score -= 20;
  } else if (stats.outlierCount > 0) {
    score -= 5;
  }

  return max((uint8_t)0, score);
}

String DynamicSensorManager::getDiagnostics() {
  String diagnostics = "{";
  diagnostics += "\"initialized\":" + String(initialized ? "true" : "false") + ",";
  diagnostics += "\"currentConfig\":{";
  diagnostics += "\"atime\":" + String(currentConfig.atime) + ",";
  diagnostics += "\"again\":" + String(currentConfig.again) + ",";
  diagnostics += "\"brightness\":" + String(currentConfig.brightness) + ",";
  diagnostics += "\"condition\":" + String(currentConfig.condition) + ",";
  diagnostics += "\"isOptimal\":" + String(currentConfig.isOptimal ? "true" : "false");
  diagnostics += "},";
  diagnostics += "\"lastAdjustment\":" + String(millis() - lastAdjustmentTime) + ",";
  diagnostics += "\"adjustmentAttempts\":" + String(adjustmentAttempts) + ",";
  diagnostics += "\"saturation\":" + String(checkSaturation() ? "true" : "false") + ",";
  diagnostics += "\"signalAdequate\":" + String(checkSignalAdequacy() ? "true" : "false");
  diagnostics += "}";

  return diagnostics;
}

bool DynamicSensorManager::optimizeLEDBrightness(uint8_t& targetBrightness) {
  if (!initialized) {
    LOG_SENSOR_ERROR("Dynamic sensor manager not initialized");
    return false;
  }

  LOG_SENSOR_INFO("Starting LED brightness optimization");

  const int maxIterations = 6;
  const int stabilizationDelay = BRIGHTNESS_STABILIZATION_DELAY;
  uint8_t brightness = targetBrightness;

  for (int iteration = 0; iteration < maxIterations; iteration++) {
    // Calculate current control variable
    uint16_t controlVariable = calculateControlVariable();

    LOG_SENSOR_DEBUG("Brightness optimization iteration %d: Brightness=%u Control=%u",
                     iteration + 1, brightness, controlVariable);

    // Check if we're in the optimal range
    if (isInOptimalRange(controlVariable)) {
      LOG_SENSOR_INFO("Optimal brightness achieved: %u (Control: %u)", brightness, controlVariable);
      targetBrightness = brightness;
      return true;
    }

    // Adjust brightness based on control variable
    uint8_t oldBrightness = brightness;
    if (controlVariable > RGB_TARGET_MAX) {
      // Too bright - reduce brightness
      if (brightness > MIN_LED_BRIGHTNESS) {
        // Use larger steps for severe saturation (65535)
        uint8_t adjustmentStep = (controlVariable >= 65000) ? BRIGHTNESS_ADJUSTMENT_STEP * 4 : BRIGHTNESS_ADJUSTMENT_STEP;
        brightness = max(MIN_LED_BRIGHTNESS, brightness - adjustmentStep);
        LOG_SENSOR_DEBUG("Reducing brightness by %u (control=%u)", adjustmentStep, controlVariable);
      } else {
        LOG_SENSOR_WARN("Cannot reduce brightness further, at minimum");
        break;
      }
    } else if (controlVariable < RGB_TARGET_MIN) {
      // Too dim - increase brightness
      if (brightness < MAX_LED_BRIGHTNESS) {
        brightness = min(MAX_LED_BRIGHTNESS, brightness + BRIGHTNESS_ADJUSTMENT_STEP);
      } else {
        LOG_SENSOR_WARN("Cannot increase brightness further, at maximum");
        break;
      }
    }

    // If no change was made, we're at limits
    if (brightness == oldBrightness) {
      LOG_SENSOR_WARN("Brightness at limits, cannot optimize further");
      break;
    }

    // Apply new brightness and wait for stabilization
    // Note: Actual LED control should be done by caller
    delay(stabilizationDelay);
  }

  targetBrightness = brightness;
  LOG_SENSOR_INFO("Brightness optimization complete: %u", brightness);
  return true;
}

bool DynamicSensorManager::isInOptimalRange(uint16_t controlVariable) {
  return (controlVariable >= RGB_TARGET_MIN && controlVariable <= RGB_TARGET_MAX);
}

uint16_t DynamicSensorManager::calculateControlVariable() {
  if (!initialized) {
    return 0;
  }

  // Read all RGB channels
  uint16_t rawR = sensor->getXData();  // X channel maps to Red
  uint16_t rawG = sensor->getYData();  // Y channel maps to Green
  uint16_t rawB = sensor->getZData();  // Z channel maps to Blue

  // Use maximum of RGB channels as control variable
  uint16_t controlVariable = max(max(rawR, rawG), rawB);

  LOG_SENSOR_DEBUG("Control variable calculation: R=%u G=%u B=%u Max=%u",
                   rawR, rawG, rawB, controlVariable);

  return controlVariable;
}
