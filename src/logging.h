#ifndef LOGGING_H
#define LOGGING_H

#include "config.h"
#include <Arduino.h>

// Log levels
enum LogLevel {
  LOG_ERROR = 1,
  LOG_WARN = 2,
  LOG_INFO = 3,
  LOG_DEBUG = 4
};

// Log categories
enum LogCategory {
  CAT_SENSOR = 1,
  CAT_LED = 2,
  CAT_NETWORK = 3,
  CAT_WEB = 4,
  CAT_STORAGE = 5,
  CAT_API = 6,
  CAT_SYSTEM = 7,
  CAT_MEMORY = 8
};

class Logger {
private:
  static unsigned long startTime;
  static bool initialized;
  
  static const char* getLevelString(LogLevel level) {
    switch(level) {
      case LOG_ERROR: return "ERROR";
      case LOG_WARN:  return "WARN ";
      case LOG_INFO:  return "INFO ";
      case LOG_DEBUG: return "DEBUG";
      default:        return "UNKN ";
    }
  }
  
  static const char* getCategoryString(LogCategory category) {
    switch(category) {
      case CAT_SENSOR:  return "SENSOR";
      case CAT_LED:     return "LED   ";
      case CAT_NETWORK: return "NET   ";
      case CAT_WEB:     return "WEB   ";
      case CAT_STORAGE: return "STORE ";
      case CAT_API:     return "API   ";
      case CAT_SYSTEM:  return "SYS   ";
      case CAT_MEMORY:  return "MEM   ";
      default:          return "UNKN  ";
    }
  }
  
  static bool shouldLog(LogLevel level, LogCategory category) {
    if (!ENABLE_LOGGING) return false;
    if (level > GLOBAL_LOG_LEVEL) return false;
    
    switch(category) {
      case CAT_SENSOR:  return LOG_SENSOR_OPS;
      case CAT_LED:     return LOG_LED_CONTROL;
      case CAT_NETWORK: return LOG_NETWORK;
      case CAT_WEB:     return LOG_WEB_SERVER;
      case CAT_STORAGE: return LOG_DATA_STORAGE;
      case CAT_API:     return LOG_API_CALLS;
      case CAT_SYSTEM:  return LOG_SYSTEM_HEALTH;
      case CAT_MEMORY:  return LOG_MEMORY_USAGE;
      default:          return true;
    }
  }

public:
  static void init() {
    if (!initialized) {
      startTime = millis();
      initialized = true;
      if (ENABLE_LOGGING) {
        Serial.println();
        Serial.println("=== SCROFANI COLOR MATCHER LOGGING INITIALIZED ===");
        Serial.printf("Log Level: %d | Categories: S:%d L:%d N:%d W:%d D:%d A:%d Y:%d M:%d\n",
                     GLOBAL_LOG_LEVEL, LOG_SENSOR_OPS, LOG_LED_CONTROL, LOG_NETWORK,
                     LOG_WEB_SERVER, LOG_DATA_STORAGE, LOG_API_CALLS, LOG_SYSTEM_HEALTH, LOG_MEMORY_USAGE);
        Serial.println("Format: [TIMESTAMP] [LEVEL] [CATEGORY] [FREE_HEAP] Message");
        Serial.println("======================================================");
      }
    }
  }
  
  static void log(LogLevel level, LogCategory category, const char* format, ...) {
    if (!shouldLog(level, category)) return;
    
    unsigned long timestamp = millis() - startTime;
    uint32_t freeHeap = ESP.getFreeHeap();
    
    // Print structured log header
    Serial.printf("[%08lu] [%s] [%s] [%6u] ", 
                  timestamp, getLevelString(level), getCategoryString(category), freeHeap);
    
    // Print formatted message
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.println(buffer);
  }
  
  static void logMemoryUsage(const char* context) {
    if (!LOG_MEMORY_USAGE) return;
    
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float usagePercent = (float)usedHeap / totalHeap * 100.0;
    
    log(LOG_INFO, CAT_MEMORY, "%s - Used: %u/%u bytes (%.1f%%)", 
        context, usedHeap, totalHeap, usagePercent);
  }
  
  static void logPerformance(const char* operation, unsigned long startMs) {
    if (!LOG_RESPONSE_TIMES) return;
    
    unsigned long duration = millis() - startMs;
    log(LOG_DEBUG, CAT_SYSTEM, "%s completed in %lu ms", operation, duration);
  }
  
  static void logSensorData(uint16_t x, uint16_t y, uint16_t z, uint16_t ir, 
                           uint8_t r, uint8_t g, uint8_t b, float ambientLux) {
    log(LOG_INFO, CAT_SENSOR, "Scan - XYZ:(%u,%u,%u) IR:%u RGB:(%u,%u,%u) Ambient:%.1f lux", 
        x, y, z, ir, r, g, b, ambientLux);
  }
  
  static void logLEDBrightness(uint8_t oldBrightness, uint8_t newBrightness, 
                              const char* reason, float ambientLux, uint16_t rawCounts) {
    if (oldBrightness != newBrightness) {
      log(LOG_INFO, CAT_LED, "Brightness %u->%u (%s) Ambient:%.1f Raw:%u", 
          oldBrightness, newBrightness, reason, ambientLux, rawCounts);
    }
  }
  
  static void logWebRequest(const char* method, const char* uri, const char* clientIP) {
    log(LOG_INFO, CAT_WEB, "%s %s from %s", method, uri, clientIP);
  }
  
  static void logWebResponse(int statusCode, unsigned long responseTime) {
    log(LOG_DEBUG, CAT_WEB, "Response %d in %lu ms", statusCode, responseTime);
  }
  
  static void logAPICall(const char* endpoint, int responseCode, unsigned long duration) {
    log(LOG_INFO, CAT_API, "Call to %s - Status:%d Duration:%lu ms", 
        endpoint, responseCode, duration);
  }
  
  static void logStorageOperation(const char* operation, bool success, const char* details = "") {
    log(success ? LOG_DEBUG : LOG_ERROR, CAT_STORAGE, "%s %s %s", 
        operation, success ? "SUCCESS" : "FAILED", details);
  }
  
  static void logNetworkEvent(const char* event, const char* details = "") {
    log(LOG_INFO, CAT_NETWORK, "%s %s", event, details);
  }
  
  static void logSystemEvent(const char* event, const char* details = "") {
    log(LOG_INFO, CAT_SYSTEM, "%s %s", event, details);
  }
  
  static void logError(LogCategory category, const char* error, const char* details = "") {
    log(LOG_ERROR, category, "ERROR: %s %s", error, details);
  }
};

// Static member declarations (definitions in main.cpp)

// Convenience macros
#define LOG_ERROR_MSG(cat, msg, ...) Logger::log(LOG_ERROR, cat, msg, ##__VA_ARGS__)
#define LOG_WARN_MSG(cat, msg, ...) Logger::log(LOG_WARN, cat, msg, ##__VA_ARGS__)
#define LOG_INFO_MSG(cat, msg, ...) Logger::log(LOG_INFO, cat, msg, ##__VA_ARGS__)
#define LOG_DEBUG_MSG(cat, msg, ...) Logger::log(LOG_DEBUG, cat, msg, ##__VA_ARGS__)

// Category-specific macros
#define LOG_SENSOR_ERROR(msg, ...) LOG_ERROR_MSG(CAT_SENSOR, msg, ##__VA_ARGS__)
#define LOG_SENSOR_WARN(msg, ...) LOG_WARN_MSG(CAT_SENSOR, msg, ##__VA_ARGS__)
#define LOG_SENSOR_INFO(msg, ...) LOG_INFO_MSG(CAT_SENSOR, msg, ##__VA_ARGS__)
#define LOG_SENSOR_DEBUG(msg, ...) LOG_DEBUG_MSG(CAT_SENSOR, msg, ##__VA_ARGS__)

#define LOG_LED_INFO(msg, ...) LOG_INFO_MSG(CAT_LED, msg, ##__VA_ARGS__)
#define LOG_LED_DEBUG(msg, ...) LOG_DEBUG_MSG(CAT_LED, msg, ##__VA_ARGS__)

#define LOG_NET_INFO(msg, ...) LOG_INFO_MSG(CAT_NETWORK, msg, ##__VA_ARGS__)
#define LOG_NET_WARN(msg, ...) LOG_WARN_MSG(CAT_NETWORK, msg, ##__VA_ARGS__)
#define LOG_NET_DEBUG(msg, ...) LOG_DEBUG_MSG(CAT_NETWORK, msg, ##__VA_ARGS__)
#define LOG_NET_ERROR(msg, ...) LOG_ERROR_MSG(CAT_NETWORK, msg, ##__VA_ARGS__)

#define LOG_WEB_INFO(msg, ...) LOG_INFO_MSG(CAT_WEB, msg, ##__VA_ARGS__)
#define LOG_WEB_DEBUG(msg, ...) LOG_DEBUG_MSG(CAT_WEB, msg, ##__VA_ARGS__)
#define LOG_WEB_ERROR(msg, ...) LOG_ERROR_MSG(CAT_WEB, msg, ##__VA_ARGS__)

#define LOG_STORAGE_INFO(msg, ...) LOG_INFO_MSG(CAT_STORAGE, msg, ##__VA_ARGS__)
#define LOG_STORAGE_DEBUG(msg, ...) LOG_DEBUG_MSG(CAT_STORAGE, msg, ##__VA_ARGS__)
#define LOG_STORAGE_ERROR(msg, ...) LOG_ERROR_MSG(CAT_STORAGE, msg, ##__VA_ARGS__)

#define LOG_API_INFO(msg, ...) LOG_INFO_MSG(CAT_API, msg, ##__VA_ARGS__)
#define LOG_API_DEBUG(msg, ...) LOG_DEBUG_MSG(CAT_API, msg, ##__VA_ARGS__)
#define LOG_API_ERROR(msg, ...) LOG_ERROR_MSG(CAT_API, msg, ##__VA_ARGS__)

#define LOG_SYS_INFO(msg, ...) LOG_INFO_MSG(CAT_SYSTEM, msg, ##__VA_ARGS__)
#define LOG_SYS_ERROR(msg, ...) LOG_ERROR_MSG(CAT_SYSTEM, msg, ##__VA_ARGS__)

// Performance logging macros
#define LOG_PERF_START() unsigned long _perf_start = millis()
#define LOG_PERF_END(operation) Logger::logPerformance(operation, _perf_start)

#endif // LOGGING_H
