#ifndef DYNAMIC_SENSOR_H
#define DYNAMIC_SENSOR_H

#include <Arduino.h>
#include <DFRobot_TCS3430.h>
#include "config.h"
#include "logging.h"

/**
 * @brief Dynamic TCS3430 Sensor Management System
 * 
 * Implements intelligent sensor configuration for all-around light-to-dark color matching
 * Based on TCS3430 Application Note AN000571 recommendations
 */

// Lighting condition enumeration
enum LightingCondition {
  LIGHT_DARK = 0,      // Low-light conditions (< 1000 counts)
  LIGHT_INDOOR = 1,    // Normal indoor lighting (1000-10000 counts)  
  LIGHT_BRIGHT = 2,    // Bright indoor/outdoor (10000-40000 counts)
  LIGHT_VERY_BRIGHT = 3 // Very bright outdoor (> 40000 counts)
};

// Sensor configuration structure
struct SensorConfig {
  uint8_t atime;           // Integration time (ATIME register)
  uint8_t again;           // Analog gain (AGAIN register)
  uint8_t brightness;      // LED brightness
  LightingCondition condition; // Detected lighting condition
  bool isOptimal;          // Whether configuration is optimal
  uint32_t timestamp;      // When configuration was set
};

// Reading quality metrics
struct ReadingQuality {
  float coefficientOfVariation; // CV for consistency measurement
  bool hasSaturation;          // Whether any channel is saturated
  bool hasLowSignal;           // Whether signal is too low
  uint8_t qualityScore;        // Overall quality score (0-100)
  uint16_t maxReading;         // Highest channel reading
  uint16_t minReading;         // Lowest channel reading
};

// Statistical data for multi-sample analysis
struct SampleStatistics {
  float mean;
  float standardDeviation;
  float coefficientOfVariation;
  uint16_t min;
  uint16_t max;
  uint8_t outlierCount;
};

class DynamicSensorManager {
private:
  DFRobot_TCS3430* sensor;
  SensorConfig currentConfig;
  SensorConfig optimalConfigs[4]; // Presets for each lighting condition
  
  // Internal state
  bool initialized;
  uint32_t lastAdjustmentTime;
  uint8_t adjustmentAttempts;
  LightingCondition lastDetectedCondition;
  
  // Statistics tracking
  uint16_t recentReadings[RAPID_SCAN_SAMPLES];
  uint8_t readingIndex;
  bool statisticsReady;

public:
  /**
   * @brief Constructor
   * @param tcs3430 Pointer to initialized TCS3430 sensor
   */
  DynamicSensorManager(DFRobot_TCS3430* tcs3430);
  
  /**
   * @brief Initialize the dynamic sensor manager
   * @return true if initialization successful
   */
  bool initialize();
  
  /**
   * @brief Detect current lighting conditions based on sensor readings
   * @return Detected lighting condition
   */
  LightingCondition detectLightingCondition();
  
  /**
   * @brief Get optimal sensor configuration for current lighting
   * @param condition Current lighting condition
   * @return Optimal sensor configuration
   */
  SensorConfig getOptimalConfig(LightingCondition condition);
  
  /**
   * @brief Apply sensor configuration
   * @param config Configuration to apply
   * @return true if configuration applied successfully
   */
  bool applySensorConfig(const SensorConfig& config);
  
  /**
   * @brief Perform automatic sensor optimization
   * @return true if optimization successful
   */
  bool optimizeSensorSettings();
  
  /**
   * @brief Check if current readings are saturated
   * @return true if any channel is saturated
   */
  bool checkSaturation();
  
  /**
   * @brief Check if current readings have sufficient signal
   * @return true if signal is adequate
   */
  bool checkSignalAdequacy();
  
  /**
   * @brief Perform rapid multi-sample reading with quality analysis
   * @param x Output X channel average
   * @param y Output Y channel average  
   * @param z Output Z channel average
   * @param ir1 Output IR1 channel average
   * @param ir2 Output IR2 channel average
   * @param quality Output quality metrics
   * @return true if reading successful
   */
  bool performQualityReading(uint16_t& x, uint16_t& y, uint16_t& z, 
                           uint16_t& ir1, uint16_t& ir2, ReadingQuality& quality);
  
  /**
   * @brief Apply IR compensation to RGB readings
   * @param r Red channel (input/output)
   * @param g Green channel (input/output)
   * @param b Blue channel (input/output)
   * @param ir1 IR1 channel reading
   * @param ir2 IR2 channel reading
   */
  void applyIRCompensation(uint8_t& r, uint8_t& g, uint8_t& b,
                          uint16_t ir1, uint16_t ir2);

  /**
   * @brief Perform automatic LED brightness optimization
   * @param targetBrightness Reference to brightness value to optimize
   * @return true if optimization successful
   */
  bool optimizeLEDBrightness(uint8_t& targetBrightness);

  /**
   * @brief Check if current readings are in optimal range
   * @param controlVariable RGB control variable (max or average)
   * @return true if in optimal range
   */
  bool isInOptimalRange(uint16_t controlVariable);

  /**
   * @brief Calculate RGB control variable from current readings
   * @return Control variable value (max of RGB channels)
   */
  uint16_t calculateControlVariable();
  
  /**
   * @brief Calculate statistics for a set of readings
   * @param readings Array of readings
   * @param count Number of readings
   * @return Statistical analysis
   */
  SampleStatistics calculateStatistics(uint16_t* readings, uint8_t count);
  
  /**
   * @brief Get current sensor configuration
   * @return Current configuration
   */
  SensorConfig getCurrentConfig() const { return currentConfig; }
  
  /**
   * @brief Check if sensor manager is initialized
   * @return true if initialized
   */
  bool isInitialized() const { return initialized; }
  
  /**
   * @brief Reset to default configuration
   */
  void resetToDefaults();
  
  /**
   * @brief Get diagnostic information
   * @return JSON string with diagnostic data
   */
  String getDiagnostics();

private:
  /**
   * @brief Initialize optimal configurations for different lighting conditions
   */
  void initializeOptimalConfigs();
  
  /**
   * @brief Validate sensor configuration
   * @param config Configuration to validate
   * @return true if configuration is valid
   */
  bool validateConfig(const SensorConfig& config);
  
  /**
   * @brief Calculate quality score based on readings
   * @param readings Array of readings
   * @param count Number of readings
   * @return Quality score (0-100)
   */
  uint8_t calculateQualityScore(uint16_t* readings, uint8_t count);
  
  /**
   * @brief Remove outliers from readings array
   * @param readings Array of readings (modified in place)
   * @param count Number of readings (modified)
   * @return Number of outliers removed
   */
  uint8_t removeOutliers(uint16_t* readings, uint8_t& count);
};

#endif // DYNAMIC_SENSOR_H
