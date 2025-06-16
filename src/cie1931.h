/*!
 * @file cie1931.h
 * @brief CIE 1931 Color Space Implementation for TCS3430 Sensor
 * @details Implements scientifically accurate color space conversion following
 *          CIE 1931 standard with 2° observer and D65 illuminant
 * @copyright ESP32 Color Matcher Project
 * @version 1.0
 * @date 2024
 */

#ifndef CIE1931_H
#define CIE1931_H

#include <Arduino.h>
#include "config.h"

// CIE 1931 Standard Constants
#define CIE_XYZ_SCALE_FACTOR 100.0f     // Scale factor for XYZ normalization
#define CIE_RGB_MAX_VALUE 255.0f        // Maximum RGB output value
#define CIE_GAMMA_THRESHOLD 0.0031308f   // sRGB gamma correction threshold
#define CIE_GAMMA_LINEAR_COEFF 12.92f    // Linear coefficient for gamma correction
#define CIE_GAMMA_POWER_COEFF 1.055f     // Power coefficient for gamma correction
#define CIE_GAMMA_POWER_OFFSET 0.055f    // Power offset for gamma correction

// D65 Standard Illuminant White Point (normalized to Y=100)
#define CIE_D65_WHITE_X 95.047f
#define CIE_D65_WHITE_Y 100.000f
#define CIE_D65_WHITE_Z 108.883f

// TCS3430 Sensor Calibration Constants
#define TCS3430_XYZ_NORMALIZATION_FACTOR 1000.0f  // Sensor-specific scaling
#define TCS3430_IR_COMPENSATION_FACTOR 0.1f       // IR compensation coefficient

/**
 * @brief CIE 1931 XYZ color coordinates structure
 */
struct CIE_XYZ {
    float X;  // X tristimulus value
    float Y;  // Y tristimulus value (luminance)
    float Z;  // Z tristimulus value
};

/**
 * @brief CIE 1931 xyY chromaticity coordinates structure
 */
struct CIE_xyY {
    float x;  // x chromaticity coordinate
    float y;  // y chromaticity coordinate
    float Y;  // Y luminance value
};

/**
 * @brief sRGB color structure
 */
struct sRGB {
    uint8_t r;  // Red component (0-255)
    uint8_t g;  // Green component (0-255)
    uint8_t b;  // Blue component (0-255)
};

/**
 * @brief Floating-point RGB structure for intermediate calculations
 */
struct RGB_Float {
    float r;  // Red component (0.0-1.0)
    float g;  // Green component (0.0-1.0)
    float b;  // Blue component (0.0-1.0)
};

/**
 * @brief White reference calibration data in CIE XYZ space
 */
struct CIE_WhiteReference {
    CIE_XYZ whitePoint;      // Measured white point in XYZ
    float scalingFactor;     // Scaling factor for normalization
    uint32_t timestamp;      // Calibration timestamp
    bool valid;              // Calibration validity flag
};

// Function Declarations

/**
 * @brief Convert TCS3430 raw sensor data to CIE 1931 XYZ
 * @param rawX Raw X channel data from TCS3430
 * @param rawY Raw Y channel data from TCS3430
 * @param rawZ Raw Z channel data from TCS3430
 * @param rawIR Raw IR channel data for compensation
 * @param whiteRef White reference calibration data
 * @return CIE_XYZ Calibrated XYZ tristimulus values
 */
CIE_XYZ convertTCS3430ToXYZ(uint16_t rawX, uint16_t rawY, uint16_t rawZ, 
                           uint16_t rawIR, const CIE_WhiteReference& whiteRef);

/**
 * @brief Convert CIE 1931 XYZ to sRGB color space
 * @param xyz Input XYZ tristimulus values
 * @return sRGB Output RGB values (0-255)
 */
sRGB convertXYZToSRGB(const CIE_XYZ& xyz);

/**
 * @brief Convert CIE 1931 XYZ to chromaticity coordinates
 * @param xyz Input XYZ tristimulus values
 * @return CIE_xyY Chromaticity coordinates and luminance
 */
CIE_xyY convertXYZToxyY(const CIE_XYZ& xyz);

/**
 * @brief Convert chromaticity coordinates to XYZ
 * @param xyY Input chromaticity coordinates and luminance
 * @return CIE_XYZ XYZ tristimulus values
 */
CIE_XYZ convertxyYToXYZ(const CIE_xyY& xyY);

/**
 * @brief Apply sRGB gamma correction to linear RGB values
 * @param linear Linear RGB component (0.0-1.0)
 * @return float Gamma-corrected RGB component (0.0-1.0)
 */
float applySRGBGamma(float linear);

/**
 * @brief Calculate color temperature from chromaticity coordinates
 * @param x Chromaticity x coordinate
 * @param y Chromaticity y coordinate
 * @return float Color temperature in Kelvin
 */
float calculateColorTemperature(float x, float y);

/**
 * @brief Perform white reference calibration in CIE XYZ space
 * @param rawX Raw X channel data from white reference
 * @param rawY Raw Y channel data from white reference
 * @param rawZ Raw Z channel data from white reference
 * @param rawIR Raw IR channel data from white reference
 * @return CIE_WhiteReference Calibrated white reference data
 */
CIE_WhiteReference calibrateWhiteReference(uint16_t rawX, uint16_t rawY, 
                                          uint16_t rawZ, uint16_t rawIR);

/**
 * @brief Validate XYZ values for reasonable color measurement
 * @param xyz Input XYZ values to validate
 * @return bool True if values are within reasonable bounds
 */
bool validateXYZValues(const CIE_XYZ& xyz);

/**
 * @brief Calculate illuminance from Y tristimulus value
 * @param Y Y tristimulus value (luminance)
 * @param scalingFactor Calibration scaling factor
 * @return float Illuminance in lux
 */
float calculateIlluminance(float Y, float scalingFactor);

/**
 * @brief Apply IR compensation to XYZ values
 * @param xyz Input XYZ values
 * @param irValue IR channel measurement
 * @return CIE_XYZ IR-compensated XYZ values
 */
CIE_XYZ applyIRCompensation(const CIE_XYZ& xyz, uint16_t irValue);

/**
 * @brief Get D65 standard illuminant white point
 * @return CIE_XYZ D65 white point coordinates
 */
CIE_XYZ getD65WhitePoint();

/**
 * @brief Normalize XYZ values to D65 white point
 * @param xyz Input XYZ values
 * @param whiteRef White reference calibration
 * @return CIE_XYZ Normalized XYZ values
 */
CIE_XYZ normalizeToWhitePoint(const CIE_XYZ& xyz, const CIE_WhiteReference& whiteRef);

// Debug and utility functions

/**
 * @brief Print XYZ values to serial for debugging
 * @param xyz XYZ values to print
 * @param label Optional label for the output
 */
void printXYZ(const CIE_XYZ& xyz, const char* label = "XYZ");

/**
 * @brief Print chromaticity coordinates to serial for debugging
 * @param xyY Chromaticity coordinates to print
 * @param label Optional label for the output
 */
void printxyY(const CIE_xyY& xyY, const char* label = "xyY");

/**
 * @brief Print RGB values to serial for debugging
 * @param rgb RGB values to print
 * @param label Optional label for the output
 */
void printRGB(const sRGB& rgb, const char* label = "RGB");

#endif // CIE1931_H
