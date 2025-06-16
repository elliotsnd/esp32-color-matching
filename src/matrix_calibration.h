#ifndef MATRIX_CALIBRATION_H
#define MATRIX_CALIBRATION_H

#include <Arduino.h>
#include <DFRobot_TCS3430.h>
#include "config.h"
#include "logging.h"

/**
 * @brief Matrix-based Color Calibration System for TCS3430
 *
 * Implements proper colorimetric calibration using transformation matrices
 * to map raw sensor readings to accurate sRGB values.
 */

// Maximum number of calibration points (color patches)
#define MAX_CALIBRATION_POINTS 12
#define MATRIX_COLS 4  // 3x4 matrix (RGB output, RGBC input + bias)

// Color reference structure for calibration points
struct ColorReference {
  // Reference sRGB values (ground truth)
  uint8_t ref_r, ref_g, ref_b;
  
  // Raw sensor readings for this color
  uint16_t sensor_r, sensor_g, sensor_b, sensor_c;
  uint16_t sensor_ir1, sensor_ir2;
  
  // Metadata
  char name[32];           // Color name (e.g., "Red", "Blue", "White")
  bool valid;              // Whether this calibration point is valid
  uint32_t timestamp;      // When this measurement was taken
  float delta_e;           // Color difference after calibration (quality metric)
};

// Calibration matrix structure
struct CalibrationMatrix {
  // 3x4 transformation matrix: sRGB = M × [R, G, B, 1]
  float matrix[3][4];
  
  // Matrix metadata
  uint8_t num_points;      // Number of calibration points used
  float avg_delta_e;       // Average color error across all points
  float max_delta_e;       // Maximum color error
  bool valid;              // Whether matrix is valid
  uint32_t timestamp;      // When matrix was computed
  char illuminant[16];     // Illuminant used (e.g., "D65", "A")
};

// Statistics for calibration quality assessment
struct CalibrationStats {
  float mean_delta_e;      // Mean color difference
  float std_delta_e;       // Standard deviation of color differences
  float max_delta_e;       // Maximum color difference
  uint8_t points_under_2;  // Number of points with ΔE < 2 (excellent)
  uint8_t points_under_5;  // Number of points with ΔE < 5 (acceptable)
  uint8_t total_points;    // Total calibration points
  float quality_score;     // Overall quality score (0-100)
};

class MatrixCalibration {
private:
  DFRobot_TCS3430* sensor;
  ColorReference calibrationPoints[MAX_CALIBRATION_POINTS];
  CalibrationMatrix currentMatrix;
  CalibrationStats lastStats;
  
  uint8_t numPoints;
  bool matrixValid;
  bool initialized;

public:
  /**
   * @brief Constructor
   * @param tcs3430 Pointer to initialized TCS3430 sensor
   */
  MatrixCalibration(DFRobot_TCS3430* tcs3430);
  
  /**
   * @brief Initialize the matrix calibration system
   * @return true if initialization successful
   */
  bool initialize();
  
  /**
   * @brief Add a calibration point by measuring a reference color
   * @param ref_r Reference red value (0-255)
   * @param ref_g Reference green value (0-255)
   * @param ref_b Reference blue value (0-255)
   * @param name Color name for identification
   * @return true if measurement successful
   */
  bool addCalibrationPoint(uint8_t ref_r, uint8_t ref_g, uint8_t ref_b, const char* name);
  
  /**
   * @brief Manually add a calibration point with known sensor values
   * @param ref_r Reference red value (0-255)
   * @param ref_g Reference green value (0-255)
   * @param ref_b Reference blue value (0-255)
   * @param sensor_r Raw sensor red reading
   * @param sensor_g Raw sensor green reading
   * @param sensor_b Raw sensor blue reading
   * @param sensor_c Raw sensor clear reading
   * @param name Color name for identification
   * @return true if point added successfully
   */
  bool addManualCalibrationPoint(uint8_t ref_r, uint8_t ref_g, uint8_t ref_b,
                                uint16_t sensor_r, uint16_t sensor_g, uint16_t sensor_b, uint16_t sensor_c,
                                const char* name);
  
  /**
   * @brief Compute calibration matrix from collected points
   * @return true if matrix computation successful
   */
  bool computeCalibrationMatrix();
  
  /**
   * @brief Apply calibration matrix to convert raw readings to sRGB
   * @param sensor_r Raw sensor red reading
   * @param sensor_g Raw sensor green reading
   * @param sensor_b Raw sensor blue reading
   * @param sensor_c Raw sensor clear reading
   * @param out_r Output calibrated red value (0-255)
   * @param out_g Output calibrated green value (0-255)
   * @param out_b Output calibrated blue value (0-255)
   * @return true if conversion successful
   */
  bool applyCalibratedConversion(uint16_t sensor_r, uint16_t sensor_g, uint16_t sensor_b, uint16_t sensor_c,
                                uint8_t& out_r, uint8_t& out_g, uint8_t& out_b);
  
  /**
   * @brief Calculate Delta E color difference between two sRGB colors
   * @param r1 First color red component
   * @param g1 First color green component
   * @param b1 First color blue component
   * @param r2 Second color red component
   * @param g2 Second color green component
   * @param b2 Second color blue component
   * @return Delta E color difference
   */
  float calculateDeltaE(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2);
  
  /**
   * @brief Evaluate calibration quality and compute statistics
   * @return Calibration statistics
   */
  CalibrationStats evaluateCalibration();
  
  /**
   * @brief Load pre-defined ColorChecker calibration points
   * @return Number of points loaded
   */
  uint8_t loadColorCheckerReferences();
  
  /**
   * @brief Load Dulux paint color references
   * @return Number of points loaded
   */
  uint8_t loadDuluxColorReferences();
  
  /**
   * @brief Clear all calibration points
   */
  void clearCalibrationPoints();
  
  /**
   * @brief Get current calibration matrix
   * @return Current calibration matrix
   */
  CalibrationMatrix getCurrentMatrix() const { return currentMatrix; }
  
  /**
   * @brief Get calibration point by index
   * @param index Point index (0-based)
   * @return Calibration point or nullptr if invalid
   */
  const ColorReference* getCalibrationPoint(uint8_t index) const;
  
  /**
   * @brief Get number of calibration points
   * @return Number of valid calibration points
   */
  uint8_t getNumPoints() const { return numPoints; }
  
  /**
   * @brief Check if matrix is valid
   * @return true if calibration matrix is valid
   */
  bool isMatrixValid() const { return matrixValid; }
  
  /**
   * @brief Save calibration data to NVS
   * @return true if save successful
   */
  bool saveCalibration();
  
  /**
   * @brief Load calibration data from NVS
   * @return true if load successful
   */
  bool loadCalibration();
  
  /**
   * @brief Get diagnostic information as JSON string
   * @return JSON diagnostic data
   */
  String getDiagnostics();
  
  /**
   * @brief Print calibration matrix to serial
   */
  void printMatrix();
  
  /**
   * @brief Print all calibration points to serial
   */
  void printCalibrationPoints();

private:
  /**
   * @brief Solve least squares matrix equation using Gaussian elimination
   * @param A Input matrix (modified during solving)
   * @param b Input vector (modified during solving)
   * @param x Output solution vector
   * @param n Matrix size
   * @return true if solution found
   */
  bool solveLeastSquares(float A[][MATRIX_COLS], float b[], float x[], int n);
  
  /**
   * @brief Validate calibration point data
   * @param point Point to validate
   * @return true if point is valid
   */
  bool validateCalibrationPoint(const ColorReference& point);
  
  /**
   * @brief Convert sRGB to LAB color space for Delta E calculation
   * @param r Red component (0-255)
   * @param g Green component (0-255)
   * @param b Blue component (0-255)
   * @param L Output L* component
   * @param a Output a* component
   * @param b_lab Output b* component
   */
  void srgbToLab(uint8_t r, uint8_t g, uint8_t b, float& L, float& a, float& b_lab);
  
  /**
   * @brief Apply gamma correction for sRGB conversion
   * @param linear Linear RGB value (0-1)
   * @return Gamma-corrected value (0-1)
   */
  float applyGammaCorrection(float linear);
  
  /**
   * @brief Remove gamma correction for linear RGB
   * @param srgb sRGB value (0-1)
   * @return Linear RGB value (0-1)
   */
  float removeGammaCorrection(float srgb);
};

#endif // MATRIX_CALIBRATION_H
