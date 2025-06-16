#include "matrix_calibration.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <math.h>

// External preferences object from main.cpp
extern Preferences preferences;

MatrixCalibration::MatrixCalibration(DFRobot_TCS3430* tcs3430) {
  sensor = tcs3430;
  numPoints = 0;
  matrixValid = false;
  initialized = false;
  
  // Initialize calibration matrix to identity
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      currentMatrix.matrix[i][j] = (i == j) ? 1.0f : 0.0f;
    }
  }
  currentMatrix.valid = false;
  currentMatrix.num_points = 0;
  currentMatrix.avg_delta_e = 0.0f;
  currentMatrix.max_delta_e = 0.0f;
  currentMatrix.timestamp = 0;
  strcpy(currentMatrix.illuminant, "D65");
}

bool MatrixCalibration::initialize() {
  if (!sensor) {
    LOG_SENSOR_ERROR("Matrix calibration: sensor pointer is null");
    return false;
  }
  
  // Clear calibration points
  clearCalibrationPoints();
  
  // Try to load existing calibration from NVS
  bool loaded = loadCalibration();
  
  initialized = true;
  LOG_SENSOR_INFO("Matrix calibration initialized, existing data loaded: %s", 
                  loaded ? "YES" : "NO");
  
  return true;
}

bool MatrixCalibration::addCalibrationPoint(uint8_t ref_r, uint8_t ref_g, uint8_t ref_b, const char* name) {
  if (numPoints >= MAX_CALIBRATION_POINTS) {
    LOG_SENSOR_ERROR("Matrix calibration: maximum points reached (%d)", MAX_CALIBRATION_POINTS);
    return false;
  }
  
  if (!sensor) {
    LOG_SENSOR_ERROR("Matrix calibration: sensor not initialized");
    return false;
  }
  
  // Take multiple readings for stability
  const int numReadings = 5;
  uint32_t sumR = 0, sumG = 0, sumB = 0, sumC = 0, sumIR1 = 0, sumIR2 = 0;
  
  LOG_SENSOR_INFO("Matrix calibration: measuring %s (target RGB: %d,%d,%d)", 
                  name, ref_r, ref_g, ref_b);
  
  for (int i = 0; i < numReadings; i++) {
    delay(200); // Stabilization delay
    
    uint16_t r = sensor->getXData();    // Red channel
    uint16_t g = sensor->getYData();    // Green channel  
    uint16_t b = sensor->getZData();    // Blue channel
    uint16_t c = (r + g + b) / 3;       // Clear channel approximation
    uint16_t ir1 = sensor->getIR1Data();
    uint16_t ir2 = sensor->getIR2Data();
    
    sumR += r;
    sumG += g;
    sumB += b;
    sumC += c;
    sumIR1 += ir1;
    sumIR2 += ir2;
    
    LOG_SENSOR_DEBUG("Reading %d: R=%d G=%d B=%d C=%d IR1=%d IR2=%d", 
                     i+1, r, g, b, c, ir1, ir2);
  }
  
  // Average the readings
  ColorReference& point = calibrationPoints[numPoints];
  point.ref_r = ref_r;
  point.ref_g = ref_g;
  point.ref_b = ref_b;
  point.sensor_r = sumR / numReadings;
  point.sensor_g = sumG / numReadings;
  point.sensor_b = sumB / numReadings;
  point.sensor_c = sumC / numReadings;
  point.sensor_ir1 = sumIR1 / numReadings;
  point.sensor_ir2 = sumIR2 / numReadings;
  point.valid = true;
  point.timestamp = millis();
  point.delta_e = 0.0f; // Will be calculated after matrix computation
  strncpy(point.name, name, sizeof(point.name) - 1);
  point.name[sizeof(point.name) - 1] = '\0';
  
  // Validate the measurement
  if (!validateCalibrationPoint(point)) {
    LOG_SENSOR_ERROR("Matrix calibration: invalid measurement for %s", name);
    point.valid = false;
    return false;
  }
  
  numPoints++;
  LOG_SENSOR_INFO("Matrix calibration: added point %d/%d - %s (sensor: %d,%d,%d,%d)", 
                  numPoints, MAX_CALIBRATION_POINTS, name, 
                  point.sensor_r, point.sensor_g, point.sensor_b, point.sensor_c);
  
  return true;
}

bool MatrixCalibration::addManualCalibrationPoint(uint8_t ref_r, uint8_t ref_g, uint8_t ref_b,
                                                 uint16_t sensor_r, uint16_t sensor_g, 
                                                 uint16_t sensor_b, uint16_t sensor_c,
                                                 const char* name) {
  if (numPoints >= MAX_CALIBRATION_POINTS) {
    LOG_SENSOR_ERROR("Matrix calibration: maximum points reached (%d)", MAX_CALIBRATION_POINTS);
    return false;
  }
  
  ColorReference& point = calibrationPoints[numPoints];
  point.ref_r = ref_r;
  point.ref_g = ref_g;
  point.ref_b = ref_b;
  point.sensor_r = sensor_r;
  point.sensor_g = sensor_g;
  point.sensor_b = sensor_b;
  point.sensor_c = sensor_c;
  point.sensor_ir1 = 0; // Not used in manual mode
  point.sensor_ir2 = 0;
  point.valid = true;
  point.timestamp = millis();
  point.delta_e = 0.0f;
  strncpy(point.name, name, sizeof(point.name) - 1);
  point.name[sizeof(point.name) - 1] = '\0';
  
  if (!validateCalibrationPoint(point)) {
    LOG_SENSOR_ERROR("Matrix calibration: invalid manual point for %s", name);
    point.valid = false;
    return false;
  }
  
  numPoints++;
  LOG_SENSOR_INFO("Matrix calibration: added manual point %d - %s", numPoints, name);
  
  return true;
}

bool MatrixCalibration::computeCalibrationMatrix() {
  if (numPoints < MATRIX_MIN_POINTS) {
    LOG_SENSOR_ERROR("Matrix calibration: insufficient points (%d < %d)", 
                     numPoints, MATRIX_MIN_POINTS);
    return false;
  }
  
  LOG_SENSOR_INFO("Matrix calibration: computing matrix from %d points", numPoints);
  
  // Count valid points
  int validPoints = 0;
  for (int i = 0; i < numPoints; i++) {
    if (calibrationPoints[i].valid) {
      validPoints++;
    }
  }
  
  if (validPoints < MATRIX_MIN_POINTS) {
    LOG_SENSOR_ERROR("Matrix calibration: insufficient valid points (%d < %d)", 
                     validPoints, MATRIX_MIN_POINTS);
    return false;
  }
  
  // Build matrices for least squares: A * x = b
  // A is (validPoints x 4) matrix of sensor readings [R, G, B, 1]
  // b is (validPoints x 3) matrix of target sRGB values
  // x is (4 x 3) matrix we want to solve for (transposed calibration matrix)
  
  float A[MAX_CALIBRATION_POINTS][MATRIX_COLS];
  float b[MAX_CALIBRATION_POINTS][3];
  
  int row = 0;
  for (int i = 0; i < numPoints; i++) {
    if (!calibrationPoints[i].valid) continue;
    
    // Normalize sensor readings to 0-1 range for numerical stability
    A[row][0] = calibrationPoints[i].sensor_r / 65535.0f;
    A[row][1] = calibrationPoints[i].sensor_g / 65535.0f;
    A[row][2] = calibrationPoints[i].sensor_b / 65535.0f;
    A[row][3] = 1.0f; // Bias term
    
    // Target sRGB values (0-1 range)
    b[row][0] = calibrationPoints[i].ref_r / 255.0f;
    b[row][1] = calibrationPoints[i].ref_g / 255.0f;
    b[row][2] = calibrationPoints[i].ref_b / 255.0f;
    
    row++;
  }
  
  // Solve least squares for each RGB channel separately
  for (int channel = 0; channel < 3; channel++) {
    float AtA[MATRIX_COLS][MATRIX_COLS];
    float Atb[MATRIX_COLS];
    float x[MATRIX_COLS];
    
    // Compute A^T * A
    for (int i = 0; i < MATRIX_COLS; i++) {
      for (int j = 0; j < MATRIX_COLS; j++) {
        AtA[i][j] = 0.0f;
        for (int k = 0; k < validPoints; k++) {
          AtA[i][j] += A[k][i] * A[k][j];
        }
      }
    }
    
    // Compute A^T * b
    for (int i = 0; i < MATRIX_COLS; i++) {
      Atb[i] = 0.0f;
      for (int k = 0; k < validPoints; k++) {
        Atb[i] += A[k][i] * b[k][channel];
      }
    }
    
    // Solve AtA * x = Atb using Gaussian elimination
    if (!solveLeastSquares(AtA, Atb, x, MATRIX_COLS)) {
      LOG_SENSOR_ERROR("Matrix calibration: failed to solve least squares for channel %d", channel);
      return false;
    }
    
    // Store solution in calibration matrix
    for (int i = 0; i < MATRIX_COLS; i++) {
      currentMatrix.matrix[channel][i] = x[i];
    }
  }
  
  // Update matrix metadata
  currentMatrix.valid = true;
  currentMatrix.num_points = validPoints;
  currentMatrix.timestamp = millis();
  
  // Evaluate calibration quality
  lastStats = evaluateCalibration();
  currentMatrix.avg_delta_e = lastStats.mean_delta_e;
  currentMatrix.max_delta_e = lastStats.max_delta_e;
  
  matrixValid = true;
  
  LOG_SENSOR_INFO("Matrix calibration: computation complete");
  LOG_SENSOR_INFO("  Points used: %d", validPoints);
  LOG_SENSOR_INFO("  Average ΔE: %.2f", currentMatrix.avg_delta_e);
  LOG_SENSOR_INFO("  Maximum ΔE: %.2f", currentMatrix.max_delta_e);
  LOG_SENSOR_INFO("  Quality score: %.1f", lastStats.quality_score);
  
  return true;
}

bool MatrixCalibration::applyCalibratedConversion(uint16_t sensor_r, uint16_t sensor_g,
                                                 uint16_t sensor_b, uint16_t sensor_c,
                                                 uint8_t& out_r, uint8_t& out_g, uint8_t& out_b) {
  if (!matrixValid || !currentMatrix.valid) {
    LOG_SENSOR_DEBUG("Matrix calibration: no valid matrix, using fallback");
    return false;
  }

  // Normalize sensor readings to 0-1 range
  float norm_r = sensor_r / 65535.0f;
  float norm_g = sensor_g / 65535.0f;
  float norm_b = sensor_b / 65535.0f;

  // Apply calibration matrix: sRGB = M × [R, G, B, 1]
  float linear_r = currentMatrix.matrix[0][0] * norm_r +
                   currentMatrix.matrix[0][1] * norm_g +
                   currentMatrix.matrix[0][2] * norm_b +
                   currentMatrix.matrix[0][3];

  float linear_g = currentMatrix.matrix[1][0] * norm_r +
                   currentMatrix.matrix[1][1] * norm_g +
                   currentMatrix.matrix[1][2] * norm_b +
                   currentMatrix.matrix[1][3];

  float linear_b = currentMatrix.matrix[2][0] * norm_r +
                   currentMatrix.matrix[2][1] * norm_g +
                   currentMatrix.matrix[2][2] * norm_b +
                   currentMatrix.matrix[2][3];

  // Clamp to valid range and convert to 8-bit
  out_r = (uint8_t)(constrain(linear_r * 255.0f, 0.0f, 255.0f));
  out_g = (uint8_t)(constrain(linear_g * 255.0f, 0.0f, 255.0f));
  out_b = (uint8_t)(constrain(linear_b * 255.0f, 0.0f, 255.0f));

  LOG_SENSOR_DEBUG("Matrix conversion: sensor(%d,%d,%d) -> sRGB(%d,%d,%d)",
                   sensor_r, sensor_g, sensor_b, out_r, out_g, out_b);

  return true;
}

float MatrixCalibration::calculateDeltaE(uint8_t r1, uint8_t g1, uint8_t b1,
                                        uint8_t r2, uint8_t g2, uint8_t b2) {
  // Convert both colors to LAB color space
  float L1, a1, b1_lab, L2, a2, b2_lab;
  srgbToLab(r1, g1, b1, L1, a1, b1_lab);
  srgbToLab(r2, g2, b2, L2, a2, b2_lab);

  // Calculate Delta E (CIE76 - simpler than CIEDE2000 but adequate for calibration)
  float deltaL = L1 - L2;
  float deltaA = a1 - a2;
  float deltaB = b1_lab - b2_lab;

  float deltaE = sqrt(deltaL * deltaL + deltaA * deltaA + deltaB * deltaB);

  return deltaE;
}

CalibrationStats MatrixCalibration::evaluateCalibration() {
  CalibrationStats stats;
  memset(&stats, 0, sizeof(stats));

  if (!matrixValid || numPoints == 0) {
    return stats;
  }

  float deltaE_sum = 0.0f;
  float deltaE_squared_sum = 0.0f;
  int validCount = 0;

  // Calculate Delta E for each calibration point
  for (int i = 0; i < numPoints; i++) {
    if (!calibrationPoints[i].valid) continue;

    // Apply current matrix to get predicted sRGB
    uint8_t pred_r, pred_g, pred_b;
    if (applyCalibratedConversion(calibrationPoints[i].sensor_r,
                                 calibrationPoints[i].sensor_g,
                                 calibrationPoints[i].sensor_b,
                                 calibrationPoints[i].sensor_c,
                                 pred_r, pred_g, pred_b)) {

      // Calculate Delta E between predicted and reference
      float deltaE = calculateDeltaE(pred_r, pred_g, pred_b,
                                    calibrationPoints[i].ref_r,
                                    calibrationPoints[i].ref_g,
                                    calibrationPoints[i].ref_b);

      calibrationPoints[i].delta_e = deltaE;
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
    stats.std_delta_e = sqrt((deltaE_squared_sum / validCount) -
                            (stats.mean_delta_e * stats.mean_delta_e));
    stats.total_points = validCount;

    // Calculate quality score (0-100)
    float excellent_ratio = (float)stats.points_under_2 / validCount;
    float acceptable_ratio = (float)stats.points_under_5 / validCount;

    stats.quality_score = (excellent_ratio * 100.0f + acceptable_ratio * 50.0f) / 1.5f;
    stats.quality_score = constrain(stats.quality_score, 0.0f, 100.0f);
  }

  return stats;
}

uint8_t MatrixCalibration::loadColorCheckerReferences() {
  clearCalibrationPoints();

  // Standard ColorChecker patches (simplified set)
  struct { uint8_t r, g, b; const char* name; } colorChecker[] = {
    {115, 82, 68, "Dark Skin"},
    {194, 150, 130, "Light Skin"},
    {98, 122, 157, "Blue Sky"},
    {87, 108, 67, "Foliage"},
    {133, 128, 177, "Blue Flower"},
    {103, 189, 170, "Bluish Green"},
    {214, 126, 44, "Orange"},
    {80, 91, 166, "Purplish Blue"},
    {193, 90, 99, "Moderate Red"},
    {94, 60, 108, "Purple"},
    {157, 188, 64, "Yellow Green"},
    {224, 163, 46, "Orange Yellow"}
  };

  int loaded = 0;
  for (int i = 0; i < 12 && loaded < MAX_CALIBRATION_POINTS; i++) {
    if (addManualCalibrationPoint(colorChecker[i].r, colorChecker[i].g, colorChecker[i].b,
                                 0, 0, 0, 0, colorChecker[i].name)) {
      loaded++;
    }
  }

  LOG_SENSOR_INFO("Matrix calibration: loaded %d ColorChecker references", loaded);
  return loaded;
}

uint8_t MatrixCalibration::loadDuluxColorReferences() {
  clearCalibrationPoints();

  // Standard primary colors for basic calibration
  struct { uint8_t r, g, b; const char* name; } duluxColors[] = {
    {REF_RED_R, REF_RED_G, REF_RED_B, "Red"},
    {REF_YELLOW_R, REF_YELLOW_G, REF_YELLOW_B, "Yellow"},
    {REF_GREEN_R, REF_GREEN_G, REF_GREEN_B, "Green"},
    {REF_CYAN_R, REF_CYAN_G, REF_CYAN_B, "Cyan"},
    {REF_BLUE_R, REF_BLUE_G, REF_BLUE_B, "Blue"},
    {REF_MAGENTA_R, REF_MAGENTA_G, REF_MAGENTA_B, "Magenta"},
    {REF_BLACK_R, REF_BLACK_G, REF_BLACK_B, "Black"}
  };

  int loaded = 0;
  for (int i = 0; i < STANDARD_CALIBRATION_POINTS && loaded < MAX_CALIBRATION_POINTS; i++) {
    if (addManualCalibrationPoint(duluxColors[i].r, duluxColors[i].g, duluxColors[i].b,
                                 0, 0, 0, 0, duluxColors[i].name)) {
      loaded++;
    }
  }

  LOG_SENSOR_INFO("Matrix calibration: loaded %d Dulux color references", loaded);
  return loaded;
}

void MatrixCalibration::clearCalibrationPoints() {
  for (int i = 0; i < MAX_CALIBRATION_POINTS; i++) {
    calibrationPoints[i].valid = false;
    calibrationPoints[i].delta_e = 0.0f;
    memset(calibrationPoints[i].name, 0, sizeof(calibrationPoints[i].name));
  }
  numPoints = 0;
  matrixValid = false;
  currentMatrix.valid = false;

  LOG_SENSOR_INFO("Matrix calibration: cleared all calibration points");
}

const ColorReference* MatrixCalibration::getCalibrationPoint(uint8_t index) const {
  if (index >= numPoints || !calibrationPoints[index].valid) {
    return nullptr;
  }
  return &calibrationPoints[index];
}

bool MatrixCalibration::saveCalibration() {
  if (!initialized) {
    LOG_SENSOR_ERROR("Matrix calibration: not initialized");
    return false;
  }

  LOG_SENSOR_INFO("Matrix calibration: saving to NVS");

  // Save matrix validity flag
  preferences.putBool(PREF_MATRIX_VALID, matrixValid);

  if (matrixValid && currentMatrix.valid) {
    // Save matrix data as blob
    size_t matrixSize = sizeof(currentMatrix.matrix);
    preferences.putBytes(PREF_MATRIX_DATA, currentMatrix.matrix, matrixSize);

    // Save matrix metadata
    preferences.putUInt(PREF_MATRIX_POINTS, currentMatrix.num_points);
    preferences.putULong(PREF_MATRIX_TIMESTAMP, currentMatrix.timestamp);
    preferences.putFloat(PREF_MATRIX_QUALITY, currentMatrix.avg_delta_e);

    // Save calibration statistics
    preferences.putBytes(PREF_MATRIX_STATS, &lastStats, sizeof(lastStats));

    LOG_SENSOR_INFO("Matrix calibration: saved matrix with %d points, ΔE=%.2f",
                    currentMatrix.num_points, currentMatrix.avg_delta_e);
  }

  return true;
}

bool MatrixCalibration::loadCalibration() {
  if (!initialized) {
    LOG_SENSOR_ERROR("Matrix calibration: not initialized");
    return false;
  }

  LOG_SENSOR_INFO("Matrix calibration: loading from NVS");

  // Load matrix validity flag
  matrixValid = preferences.getBool(PREF_MATRIX_VALID, false);

  if (matrixValid) {
    // Load matrix data
    size_t matrixSize = sizeof(currentMatrix.matrix);
    size_t loaded = preferences.getBytes(PREF_MATRIX_DATA, currentMatrix.matrix, matrixSize);

    if (loaded == matrixSize) {
      // Load matrix metadata
      currentMatrix.num_points = preferences.getUInt(PREF_MATRIX_POINTS, 0);
      currentMatrix.timestamp = preferences.getULong(PREF_MATRIX_TIMESTAMP, 0);
      currentMatrix.avg_delta_e = preferences.getFloat(PREF_MATRIX_QUALITY, 0.0f);
      currentMatrix.valid = true;

      // Load calibration statistics
      size_t statsSize = sizeof(lastStats);
      preferences.getBytes(PREF_MATRIX_STATS, &lastStats, statsSize);

      LOG_SENSOR_INFO("Matrix calibration: loaded matrix with %d points, ΔE=%.2f",
                      currentMatrix.num_points, currentMatrix.avg_delta_e);
      return true;
    } else {
      LOG_SENSOR_ERROR("Matrix calibration: failed to load matrix data");
      matrixValid = false;
      currentMatrix.valid = false;
    }
  }

  return false;
}

String MatrixCalibration::getDiagnostics() {
  JsonDocument doc;

  doc["initialized"] = initialized;
  doc["matrixValid"] = matrixValid;
  doc["numPoints"] = numPoints;

  if (matrixValid && currentMatrix.valid) {
    JsonArray matrix = doc["matrix"].to<JsonArray>();
    for (int i = 0; i < 3; i++) {
      JsonArray row = matrix.add<JsonArray>();
      for (int j = 0; j < 4; j++) {
        row.add(currentMatrix.matrix[i][j]);
      }
    }

    doc["avgDeltaE"] = currentMatrix.avg_delta_e;
    doc["maxDeltaE"] = currentMatrix.max_delta_e;
    doc["qualityScore"] = lastStats.quality_score;
    doc["pointsUnder2"] = lastStats.points_under_2;
    doc["pointsUnder5"] = lastStats.points_under_5;
  }

  JsonArray points = doc["calibrationPoints"].to<JsonArray>();
  for (int i = 0; i < numPoints; i++) {
    if (calibrationPoints[i].valid) {
      JsonObject point = points.add<JsonObject>();
      point["name"] = calibrationPoints[i].name;
      point["refR"] = calibrationPoints[i].ref_r;
      point["refG"] = calibrationPoints[i].ref_g;
      point["refB"] = calibrationPoints[i].ref_b;
      point["sensorR"] = calibrationPoints[i].sensor_r;
      point["sensorG"] = calibrationPoints[i].sensor_g;
      point["sensorB"] = calibrationPoints[i].sensor_b;
      point["deltaE"] = calibrationPoints[i].delta_e;
    }
  }

  String result;
  serializeJson(doc, result);
  return result;
}

void MatrixCalibration::printMatrix() {
  if (!matrixValid || !currentMatrix.valid) {
    Serial.println("Matrix calibration: No valid matrix");
    return;
  }

  Serial.println("=== Calibration Matrix ===");
  for (int i = 0; i < 3; i++) {
    Serial.printf("Row %d: [%8.4f %8.4f %8.4f %8.4f]\n", i,
                  currentMatrix.matrix[i][0], currentMatrix.matrix[i][1],
                  currentMatrix.matrix[i][2], currentMatrix.matrix[i][3]);
  }
  Serial.printf("Points: %d, Avg ΔE: %.2f, Max ΔE: %.2f\n",
                currentMatrix.num_points, currentMatrix.avg_delta_e, currentMatrix.max_delta_e);
}

void MatrixCalibration::printCalibrationPoints() {
  Serial.println("=== Calibration Points ===");
  for (int i = 0; i < numPoints; i++) {
    if (calibrationPoints[i].valid) {
      Serial.printf("%d. %s: Ref(%d,%d,%d) Sensor(%d,%d,%d) ΔE=%.2f\n",
                    i+1, calibrationPoints[i].name,
                    calibrationPoints[i].ref_r, calibrationPoints[i].ref_g, calibrationPoints[i].ref_b,
                    calibrationPoints[i].sensor_r, calibrationPoints[i].sensor_g, calibrationPoints[i].sensor_b,
                    calibrationPoints[i].delta_e);
    }
  }
}

// Private helper methods

bool MatrixCalibration::solveLeastSquares(float A[][MATRIX_COLS], float b[], float x[], int n) {
  // Gaussian elimination with partial pivoting
  for (int i = 0; i < n; i++) {
    // Find pivot
    int maxRow = i;
    for (int k = i + 1; k < n; k++) {
      if (abs(A[k][i]) > abs(A[maxRow][i])) {
        maxRow = k;
      }
    }

    // Swap rows
    if (maxRow != i) {
      for (int j = 0; j < n; j++) {
        float temp = A[i][j];
        A[i][j] = A[maxRow][j];
        A[maxRow][j] = temp;
      }
      float temp = b[i];
      b[i] = b[maxRow];
      b[maxRow] = temp;
    }

    // Check for singular matrix
    if (abs(A[i][i]) < 1e-10) {
      LOG_SENSOR_ERROR("Matrix calibration: singular matrix detected");
      return false;
    }

    // Eliminate column
    for (int k = i + 1; k < n; k++) {
      float factor = A[k][i] / A[i][i];
      for (int j = i; j < n; j++) {
        A[k][j] -= factor * A[i][j];
      }
      b[k] -= factor * b[i];
    }
  }

  // Back substitution
  for (int i = n - 1; i >= 0; i--) {
    x[i] = b[i];
    for (int j = i + 1; j < n; j++) {
      x[i] -= A[i][j] * x[j];
    }
    x[i] /= A[i][i];
  }

  return true;
}

bool MatrixCalibration::validateCalibrationPoint(const ColorReference& point) {
  // Check for reasonable sensor values
  if (point.sensor_r < 100 || point.sensor_g < 100 || point.sensor_b < 100) {
    LOG_SENSOR_WARN("Matrix calibration: low sensor values for %s", point.name);
    return false;
  }

  if (point.sensor_r > 60000 || point.sensor_g > 60000 || point.sensor_b > 60000) {
    LOG_SENSOR_WARN("Matrix calibration: saturated sensor values for %s", point.name);
    return false;
  }

  return true;
}

void MatrixCalibration::srgbToLab(uint8_t r, uint8_t g, uint8_t b, float& L, float& a, float& b_lab) {
  // Convert sRGB to linear RGB
  float linear_r = removeGammaCorrection(r / 255.0f);
  float linear_g = removeGammaCorrection(g / 255.0f);
  float linear_b = removeGammaCorrection(b / 255.0f);

  // Convert linear RGB to XYZ (sRGB matrix)
  float X = 0.4124564f * linear_r + 0.3575761f * linear_g + 0.1804375f * linear_b;
  float Y = 0.2126729f * linear_r + 0.7151522f * linear_g + 0.0721750f * linear_b;
  float Z = 0.0193339f * linear_r + 0.1191920f * linear_g + 0.9503041f * linear_b;

  // Normalize by D65 white point
  X /= 0.95047f;
  Y /= 1.00000f;
  Z /= 1.08883f;

  // Convert XYZ to LAB
  auto f = [](float t) -> float {
    return (t > 0.008856f) ? pow(t, 1.0f/3.0f) : (7.787f * t + 16.0f/116.0f);
  };

  float fx = f(X);
  float fy = f(Y);
  float fz = f(Z);

  L = 116.0f * fy - 16.0f;
  a = 500.0f * (fx - fy);
  b_lab = 200.0f * (fy - fz);
}

float MatrixCalibration::applyGammaCorrection(float linear) {
  if (linear <= 0.0031308f) {
    return 12.92f * linear;
  } else {
    return 1.055f * pow(linear, 1.0f/2.4f) - 0.055f;
  }
}

float MatrixCalibration::removeGammaCorrection(float srgb) {
  if (srgb <= 0.04045f) {
    return srgb / 12.92f;
  } else {
    return pow((srgb + 0.055f) / 1.055f, 2.4f);
  }
}
