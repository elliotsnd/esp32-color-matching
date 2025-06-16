#!/usr/bin/env python3
"""
Enhanced Scan Issue Diagnostic Test
Compares regular scan vs enhanced scan to identify the problem
"""

import requests
import json
import time

# Configuration
ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"
TIMEOUT = 15

def get_status():
    """Get current device status"""
    try:
        response = requests.get(f"{BASE_URL}/status", timeout=5)
        if response.status_code == 200:
            return response.json()
        return {}
    except Exception as e:
        print(f"ERROR getting status: {e}")
        return {}

def regular_scan():
    """Perform regular scan"""
    print("\n--- Regular Scan ---")
    try:
        response = requests.post(f"{BASE_URL}/scan", timeout=TIMEOUT)
        if response.status_code == 200:
            data = response.json()
            r, g, b = data.get('r', 0), data.get('g', 0), data.get('b', 0)
            x, y, z = data.get('x', 0), data.get('y', 0), data.get('z', 0)
            ir = data.get('ir', 0)
            
            print(f"Regular Scan Result:")
            print(f"  RGB: ({r}, {g}, {b})")
            print(f"  XYZ: ({x}, {y}, {z})")
            print(f"  IR: {ir}")
            return True, data
        else:
            print(f"ERROR: Regular scan failed with HTTP {response.status_code}")
            return False, {}
    except Exception as e:
        print(f"ERROR: Regular scan failed - {e}")
        return False, {}

def enhanced_scan():
    """Perform enhanced scan"""
    print("\n--- Enhanced Scan ---")
    try:
        response = requests.post(f"{BASE_URL}/enhanced-scan", timeout=TIMEOUT)
        if response.status_code == 200:
            data = response.json()
            r, g, b = data.get('r', 0), data.get('g', 0), data.get('b', 0)
            x, y, z = data.get('x', 0), data.get('y', 0), data.get('z', 0)
            ir1 = data.get('ir1', 0)
            ir2 = data.get('ir2', 0)
            
            print(f"Enhanced Scan Result:")
            print(f"  RGB: ({r}, {g}, {b})")
            print(f"  XYZ: ({x}, {y}, {z})")
            print(f"  IR1: {ir1}, IR2: {ir2}")
            
            # Check for optimization info
            if 'brightnessOptimization' in data:
                opt = data['brightnessOptimization']
                print(f"  Brightness Optimization:")
                print(f"    Control Variable: {opt.get('controlVariable', 'N/A')}")
                print(f"    Target Range: {opt.get('targetMin', 'N/A')}-{opt.get('targetMax', 'N/A')}")
                print(f"    Optimized Brightness: {opt.get('optimizedBrightness', 'N/A')}")
                print(f"    In Optimal Range: {opt.get('inOptimalRange', 'N/A')}")
            
            if 'sensorConfig' in data:
                config = data['sensorConfig']
                print(f"  Sensor Config:")
                print(f"    ATIME: {config.get('atime', 'N/A')}")
                print(f"    AGAIN: {config.get('again', 'N/A')}")
                print(f"    Brightness: {config.get('brightness', 'N/A')}")
                print(f"    Is Optimal: {config.get('isOptimal', 'N/A')}")
            
            return True, data
        else:
            print(f"ERROR: Enhanced scan failed with HTTP {response.status_code}")
            print(f"Response: {response.text}")
            return False, {}
    except Exception as e:
        print(f"ERROR: Enhanced scan failed - {e}")
        return False, {}

def check_led_settings():
    """Check current LED settings"""
    print("\n--- LED Settings Check ---")
    status = get_status()
    
    enhanced_mode = status.get('enhancedLEDMode', False)
    manual_intensity = status.get('manualLEDIntensity', 128)
    current_brightness = status.get('currentBrightness', 128)
    
    print(f"Enhanced LED Mode: {enhanced_mode}")
    print(f"Manual LED Intensity: {manual_intensity}")
    print(f"Current Brightness: {current_brightness}")
    
    return enhanced_mode, manual_intensity, current_brightness

def update_led_settings(enhanced_mode=False, manual_intensity=128):
    """Update LED settings"""
    print(f"\n--- Updating LED Settings ---")
    print(f"Setting Enhanced Mode: {enhanced_mode}, Manual Intensity: {manual_intensity}")
    
    try:
        settings = {
            "enhancedLEDMode": enhanced_mode,
            "manualLEDIntensity": manual_intensity
        }
        response = requests.post(f"{BASE_URL}/settings", json=settings, timeout=TIMEOUT)
        if response.status_code == 200:
            print("SUCCESS: LED settings updated")
            return True
        else:
            print(f"ERROR: LED settings update failed with HTTP {response.status_code}")
            return False
    except Exception as e:
        print(f"ERROR: LED settings update failed - {e}")
        return False

def main():
    print("Enhanced Scan Issue Diagnostic Test")
    print("===================================")
    print(f"ESP32 IP: {ESP32_IP}")
    
    # Check connectivity
    status = get_status()
    if not status:
        print("ERROR: Cannot connect to ESP32")
        return
    
    print(f"Connected to ESP32 at {ESP32_IP}")
    
    # Check current LED settings
    enhanced_mode, manual_intensity, current_brightness = check_led_settings()
    
    print("\n=== COMPARISON TEST ===")
    
    # Test 1: Regular scan
    print("\n1. Testing Regular Scan")
    regular_success, regular_data = regular_scan()
    
    # Test 2: Enhanced scan with current settings
    print("\n2. Testing Enhanced Scan (current settings)")
    enhanced_success, enhanced_data = enhanced_scan()
    
    # Test 3: Try enhanced scan with enhanced mode disabled
    if enhanced_mode:
        print("\n3. Testing Enhanced Scan with Enhanced Mode DISABLED")
        if update_led_settings(enhanced_mode=False, manual_intensity=128):
            time.sleep(1)
            enhanced_success2, enhanced_data2 = enhanced_scan()
            
            # Restore original settings
            update_led_settings(enhanced_mode=enhanced_mode, manual_intensity=manual_intensity)
    
    # Test 4: Try enhanced scan with higher manual intensity
    print("\n4. Testing Enhanced Scan with Higher Manual Intensity")
    if update_led_settings(enhanced_mode=False, manual_intensity=255):
        time.sleep(1)
        enhanced_success3, enhanced_data3 = enhanced_scan()
        
        # Restore original settings
        update_led_settings(enhanced_mode=enhanced_mode, manual_intensity=manual_intensity)
    
    print("\n=== ANALYSIS ===")
    if regular_success and enhanced_success:
        reg_rgb = (regular_data['r'], regular_data['g'], regular_data['b'])
        enh_rgb = (enhanced_data['r'], enhanced_data['g'], enhanced_data['b'])
        
        print(f"Regular Scan RGB:  {reg_rgb}")
        print(f"Enhanced Scan RGB: {enh_rgb}")
        
        if reg_rgb == (255, 255, 255) and enh_rgb != (255, 255, 255):
            print("\nISSUE CONFIRMED: Enhanced scan is not reading white correctly")
            print("Possible causes:")
            print("1. Enhanced LED mode brightness optimization is too low")
            print("2. Dynamic sensor management is interfering")
            print("3. LED is being turned off during enhanced scan")
            print("4. Different sensor reading process in enhanced mode")
        elif reg_rgb == enh_rgb:
            print("\nBoth scans match - issue may be intermittent")
        else:
            print("\nBoth scans differ from expected white (255,255,255)")
    
    print("\nRecommendations:")
    print("1. Check enhanced LED mode brightness optimization")
    print("2. Verify LED is staying on during enhanced scan")
    print("3. Check dynamic sensor management settings")
    print("4. Test with enhanced mode disabled")

if __name__ == "__main__":
    main()
