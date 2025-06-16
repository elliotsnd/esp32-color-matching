/*!
 * @file cie1931.cpp
 * @brief CIE 1931 Color Space Implementation for TCS3430 Sensor
 * @details Implements scientifically accurate color space conversion following
 *          CIE 1931 standard with 2° observer and D65 illuminant
 * @copyright ESP32 Color Matcher Project
 * @version 1.0
 * @date 2024
 */

#include "cie1931.h"
#include <math.h>

// CIE 1931 XYZ to sRGB transformation matrix (D65 illuminant)
// Based on IEC 61966-2-1:1999 sRGB standard
static const float XYZ_TO_SRGB_MATRIX[3][3] = {
    { 3.2406f, -1.5372f, -0.4986f},  // Red coefficients
    {-0.9689f,  1.8758f,  0.0415f},  // Green coefficients
    { 0.0557f, -0.2040f,  1.0570f}   // Blue coefficients
};

// TCS3430 spectral response correction factors
// These factors account for the difference between TCS3430 spectral response
// and the CIE 1931 standard observer functions
static const float TCS3430_SPECTRAL_CORRECTION[3] = {
    1.0f,    // X channel correction factor
    1.0f,    // Y channel correction factor  
    1.0f     // Z channel correction factor
};

CIE_XYZ convertTCS3430ToXYZ(uint16_t rawX, uint16_t rawY, uint16_t rawZ, 
                           uint16_t rawIR, const CIE_WhiteReference& whiteRef) {
    CIE_XYZ xyz;
    
    // Convert raw sensor values to floating point
    float x = (float)rawX;
    float y = (float)rawY;
    float z = (float)rawZ;
    float ir = (float)rawIR;
    
    Serial.printf("[CIE1931] Raw TCS3430 values - X:%.0f Y:%.0f Z:%.0f IR:%.0f\n", x, y, z, ir);
    
    // Apply spectral response correction
    x *= TCS3430_SPECTRAL_CORRECTION[0];
    y *= TCS3430_SPECTRAL_CORRECTION[1];
    z *= TCS3430_SPECTRAL_CORRECTION[2];
    
    // Apply IR compensation to reduce infrared interference
    float irCompensation = ir * TCS3430_IR_COMPENSATION_FACTOR;
    x = fmaxf(0.0f, x - irCompensation);
    y = fmaxf(0.0f, y - irCompensation);
    z = fmaxf(0.0f, z - irCompensation);
    
    Serial.printf("[CIE1931] After IR compensation - X:%.2f Y:%.2f Z:%.2f\n", x, y, z);

    // Apply white reference calibration if available
    if (whiteRef.valid) {
        // Normalize to white reference
        float whiteX = whiteRef.whitePoint.X;
        float whiteY = whiteRef.whitePoint.Y;
        float whiteZ = whiteRef.whitePoint.Z;

        if (whiteX > 0 && whiteY > 0 && whiteZ > 0) {
            // Calculate normalization factors to D65 white point
            float normX = CIE_D65_WHITE_X / whiteX;
            float normY = CIE_D65_WHITE_Y / whiteY;
            float normZ = CIE_D65_WHITE_Z / whiteZ;

            x *= normX * whiteRef.scalingFactor;
            y *= normY * whiteRef.scalingFactor;
            z *= normZ * whiteRef.scalingFactor;

            Serial.printf("[CIE1931] After white reference calibration - X:%.2f Y:%.2f Z:%.2f\n", x, y, z);
        }
    } else {
        // No calibration available, use default scaling
        x *= TCS3430_XYZ_NORMALIZATION_FACTOR / 65535.0f;
        y *= TCS3430_XYZ_NORMALIZATION_FACTOR / 65535.0f;
        z *= TCS3430_XYZ_NORMALIZATION_FACTOR / 65535.0f;

        Serial.printf("[CIE1931] Using default scaling - X:%.2f Y:%.2f Z:%.2f\n", x, y, z);
    }
    
    xyz.X = x;
    xyz.Y = y;
    xyz.Z = z;
    
    return xyz;
}

sRGB convertXYZToSRGB(const CIE_XYZ& xyz) {
    sRGB rgb;
    
    // Normalize XYZ values to 0-100 range for matrix multiplication
    float X = xyz.X / 100.0f;
    float Y = xyz.Y / 100.0f;
    float Z = xyz.Z / 100.0f;
    
    Serial.printf("[CIE1931] Normalized XYZ for sRGB conversion - X:%.4f Y:%.4f Z:%.4f\n", X, Y, Z);

    // Apply XYZ to sRGB transformation matrix
    float r = XYZ_TO_SRGB_MATRIX[0][0] * X + XYZ_TO_SRGB_MATRIX[0][1] * Y + XYZ_TO_SRGB_MATRIX[0][2] * Z;
    float g = XYZ_TO_SRGB_MATRIX[1][0] * X + XYZ_TO_SRGB_MATRIX[1][1] * Y + XYZ_TO_SRGB_MATRIX[1][2] * Z;
    float b = XYZ_TO_SRGB_MATRIX[2][0] * X + XYZ_TO_SRGB_MATRIX[2][1] * Y + XYZ_TO_SRGB_MATRIX[2][2] * Z;

    Serial.printf("[CIE1931] Linear RGB values - R:%.4f G:%.4f B:%.4f\n", r, g, b);

    // Apply gamma correction
    r = applySRGBGamma(r);
    g = applySRGBGamma(g);
    b = applySRGBGamma(b);

    Serial.printf("[CIE1931] Gamma-corrected RGB - R:%.4f G:%.4f B:%.4f\n", r, g, b);

    // Clamp to valid range and convert to 8-bit
    rgb.r = (uint8_t)constrain(r * 255.0f, 0.0f, 255.0f);
    rgb.g = (uint8_t)constrain(g * 255.0f, 0.0f, 255.0f);
    rgb.b = (uint8_t)constrain(b * 255.0f, 0.0f, 255.0f);

    Serial.printf("[CIE1931] Final sRGB values - R:%u G:%u B:%u\n", rgb.r, rgb.g, rgb.b);
    
    return rgb;
}

float applySRGBGamma(float linear) {
    if (linear <= CIE_GAMMA_THRESHOLD) {
        return CIE_GAMMA_LINEAR_COEFF * linear;
    } else {
        return CIE_GAMMA_POWER_COEFF * powf(linear, 1.0f / CIE_SRGB_GAMMA) - CIE_GAMMA_POWER_OFFSET;
    }
}

CIE_xyY convertXYZToxyY(const CIE_XYZ& xyz) {
    CIE_xyY xyY;
    
    float sum = xyz.X + xyz.Y + xyz.Z;
    
    if (sum > 0.0f) {
        xyY.x = xyz.X / sum;
        xyY.y = xyz.Y / sum;
        xyY.Y = xyz.Y;
    } else {
        // Default to D65 white point chromaticity if no valid data
        xyY.x = 0.31271f;  // D65 x chromaticity
        xyY.y = 0.32902f;  // D65 y chromaticity
        xyY.Y = 0.0f;
    }
    
    Serial.printf("[CIE1931] Chromaticity coordinates - x:%.4f y:%.4f Y:%.2f\n", xyY.x, xyY.y, xyY.Y);
    
    return xyY;
}

CIE_XYZ convertxyYToXYZ(const CIE_xyY& xyY) {
    CIE_XYZ xyz;
    
    if (xyY.y > 0.0f) {
        xyz.X = (xyY.x * xyY.Y) / xyY.y;
        xyz.Y = xyY.Y;
        xyz.Z = ((1.0f - xyY.x - xyY.y) * xyY.Y) / xyY.y;
    } else {
        xyz.X = xyz.Y = xyz.Z = 0.0f;
    }
    
    return xyz;
}

CIE_WhiteReference calibrateWhiteReference(uint16_t rawX, uint16_t rawY, 
                                          uint16_t rawZ, uint16_t rawIR) {
    CIE_WhiteReference whiteRef;
    
    Serial.printf("[CIE1931] Calibrating white reference with raw values - X:%u Y:%u Z:%u IR:%u\n",
                    rawX, rawY, rawZ, rawIR);
    
    // Convert raw values to floating point
    float x = (float)rawX;
    float y = (float)rawY;
    float z = (float)rawZ;
    float ir = (float)rawIR;
    
    // Apply basic IR compensation
    float irCompensation = ir * TCS3430_IR_COMPENSATION_FACTOR;
    x = fmaxf(0.0f, x - irCompensation);
    y = fmaxf(0.0f, y - irCompensation);
    z = fmaxf(0.0f, z - irCompensation);
    
    // Store the measured white point
    whiteRef.whitePoint.X = x;
    whiteRef.whitePoint.Y = y;
    whiteRef.whitePoint.Z = z;
    
    // Calculate scaling factor to normalize to reasonable XYZ range
    float maxComponent = fmaxf(fmaxf(x, y), z);
    if (maxComponent > 0.0f) {
        // Scale to target range that produces good RGB output
        whiteRef.scalingFactor = 50.0f / maxComponent;  // Target Y=50 for white
    } else {
        whiteRef.scalingFactor = 1.0f;
    }
    
    whiteRef.timestamp = millis();
    whiteRef.valid = (x > 0 && y > 0 && z > 0);
    
    Serial.printf("[CIE1931] White reference calibrated - XYZ:(%.2f,%.2f,%.2f) Scale:%.4f Valid:%s\n",
                    whiteRef.whitePoint.X, whiteRef.whitePoint.Y, whiteRef.whitePoint.Z,
                    whiteRef.scalingFactor, whiteRef.valid ? "true" : "false");
    
    return whiteRef;
}

bool validateXYZValues(const CIE_XYZ& xyz) {
    // Check for reasonable XYZ values
    bool valid = (xyz.X >= 0.0f && xyz.X <= 200.0f &&
                  xyz.Y >= 0.0f && xyz.Y <= 200.0f &&
                  xyz.Z >= 0.0f && xyz.Z <= 200.0f);
    
    // Check for non-zero values
    valid = valid && (xyz.X > 0.001f || xyz.Y > 0.001f || xyz.Z > 0.001f);
    
    return valid;
}

CIE_XYZ getD65WhitePoint() {
    CIE_XYZ d65;
    d65.X = CIE_D65_WHITE_X;
    d65.Y = CIE_D65_WHITE_Y;
    d65.Z = CIE_D65_WHITE_Z;
    return d65;
}

float calculateColorTemperature(float x, float y) {
    // McCamy's approximation for color temperature from chromaticity
    float n = (x - 0.3320f) / (0.1858f - y);
    float cct = 449.0f * powf(n, 3.0f) + 3525.0f * powf(n, 2.0f) + 6823.3f * n + 5520.33f;
    
    // Clamp to reasonable range
    return constrain(cct, 1000.0f, 25000.0f);
}

float calculateIlluminance(float Y, float scalingFactor) {
    // Convert Y tristimulus to illuminance in lux
    // This is a simplified conversion - actual conversion requires integration
    // over the entire visible spectrum with photopic luminosity function
    return Y * scalingFactor * 10.0f;  // Approximate conversion factor
}

CIE_XYZ applyIRCompensation(const CIE_XYZ& xyz, uint16_t irValue) {
    CIE_XYZ compensated = xyz;

    // Apply IR compensation to reduce infrared interference
    float irCompensation = (float)irValue * TCS3430_IR_COMPENSATION_FACTOR;

    compensated.X = fmaxf(0.0f, xyz.X - irCompensation);
    compensated.Y = fmaxf(0.0f, xyz.Y - irCompensation);
    compensated.Z = fmaxf(0.0f, xyz.Z - irCompensation);

    Serial.printf("[CIE1931] IR compensation applied - Original:(%.2f,%.2f,%.2f) IR:%.0f Compensated:(%.2f,%.2f,%.2f)\n",
                     xyz.X, xyz.Y, xyz.Z, irCompensation,
                     compensated.X, compensated.Y, compensated.Z);

    return compensated;
}

CIE_XYZ normalizeToWhitePoint(const CIE_XYZ& xyz, const CIE_WhiteReference& whiteRef) {
    CIE_XYZ normalized = xyz;

    if (whiteRef.valid) {
        // Calculate normalization factors based on white reference
        float whiteX = whiteRef.whitePoint.X;
        float whiteY = whiteRef.whitePoint.Y;
        float whiteZ = whiteRef.whitePoint.Z;

        if (whiteX > 0 && whiteY > 0 && whiteZ > 0) {
            // Normalize to D65 white point
            normalized.X = (xyz.X / whiteX) * CIE_D65_WHITE_X;
            normalized.Y = (xyz.Y / whiteY) * CIE_D65_WHITE_Y;
            normalized.Z = (xyz.Z / whiteZ) * CIE_D65_WHITE_Z;

            Serial.printf("[CIE1931] White point normalization - Original:(%.2f,%.2f,%.2f) Normalized:(%.2f,%.2f,%.2f)\n",
                             xyz.X, xyz.Y, xyz.Z, normalized.X, normalized.Y, normalized.Z);
        }
    }

    return normalized;
}

// Debug functions
void printXYZ(const CIE_XYZ& xyz, const char* label) {
    Serial.printf("[%s] X:%.2f Y:%.2f Z:%.2f\n", label, xyz.X, xyz.Y, xyz.Z);
}

void printxyY(const CIE_xyY& xyY, const char* label) {
    Serial.printf("[%s] x:%.4f y:%.4f Y:%.2f\n", label, xyY.x, xyY.y, xyY.Y);
}

void printRGB(const sRGB& rgb, const char* label) {
    Serial.printf("[%s] R:%u G:%u B:%u\n", label, rgb.r, rgb.g, rgb.b);
}
