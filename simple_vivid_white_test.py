#!/usr/bin/env python3
"""
Simple Vivid White Calibration Test
Tests ESP32 color matcher to get Vivid White scanning as RGB (255,255,255)
"""

import requests
import json
import time
import sys

# Configuration
ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"
TIMEOUT = 10

def test_connectivity():
    """Test basic connectivity to ESP32"""
    print("\n--- Testing ESP32 Connectivity ---")
    try:
        response = requests.get(f"{BASE_URL}/status", timeout=5)
        if response.status_code == 200:
            data = response.json()
            print(f"SUCCESS: Connected to ESP32 at {ESP32_IP}")
            print(f"Current settings: ATIME={data.get('atime', 'unknown')}, AGAIN={data.get('again', 'unknown')}")
            return True, data
        else:
            print(f"ERROR: HTTP {response.status_code}")
            return False, {}
    except Exception as e:
        print(f"ERROR: Cannot connect - {e}")
        return False, {}

def scan_color():
    """Perform a color scan"""
    print("\n--- Scanning Color ---")
    try:
        response = requests.post(f"{BASE_URL}/scan", timeout=15)
        if response.status_code == 200:
            data = response.json()
            r, g, b = data.get('r', 0), data.get('g', 0), data.get('b', 0)
            x, y, z = data.get('x', 0), data.get('y', 0), data.get('z', 0)
            ir = data.get('ir', 0)
            
            print(f"Scan Result:")
            print(f"  RGB: ({r}, {g}, {b})")
            print(f"  XYZ: ({x}, {y}, {z})")
            print(f"  IR: {ir}")
            
            # Check if it's close to white
            if r > 200 and g > 200 and b > 200:
                print("SUCCESS: Color is close to white!")
            elif r == 255 and g == 255 and b == 255:
                print("PERFECT: Exact white RGB (255,255,255)!")
            else:
                print("ISSUE: Color is not white - needs calibration")
            
            return True, data
        else:
            print(f"ERROR: Scan failed with HTTP {response.status_code}")
            return False, {}
    except Exception as e:
        print(f"ERROR: Scan failed - {e}")
        return False, {}

def update_settings(settings):
    """Update ESP32 settings"""
    print(f"\n--- Updating Settings: {settings} ---")
    try:
        response = requests.post(f"{BASE_URL}/settings", 
                               json=settings, 
                               timeout=TIMEOUT)
        if response.status_code == 200:
            print("SUCCESS: Settings updated")
            return True
        else:
            print(f"ERROR: Settings update failed with HTTP {response.status_code}")
            return False
    except Exception as e:
        print(f"ERROR: Settings update failed - {e}")
        return False

def test_optimal_settings():
    """Test with optimal TCS3430 settings"""
    print("\n--- Testing Optimal Settings ---")
    
    # Your documented optimal settings
    optimal_settings = {
        "atime": 150,
        "again": 3,  # 16x gain
        "brightness": 128,
        "autoZeroMode": 1,
        "autoZeroFreq": 127,
        "waitTime": 5
    }
    
    if update_settings(optimal_settings):
        time.sleep(2)  # Allow settings to take effect
        return scan_color()
    else:
        return False, {}

def test_brightness_levels():
    """Test different brightness levels"""
    print("\n--- Testing Different Brightness Levels ---")
    
    brightness_levels = [64, 128, 192, 255]
    results = []
    
    for brightness in brightness_levels:
        print(f"\nTesting brightness: {brightness}")
        if update_settings({"brightness": brightness}):
            time.sleep(1)
            success, data = scan_color()
            if success:
                results.append((brightness, data))
    
    return results

def main():
    print("Simple Vivid White Calibration Test")
    print("===================================")
    print(f"ESP32 IP: {ESP32_IP}")
    
    # Test connectivity
    connected, status = test_connectivity()
    if not connected:
        print("\nCannot connect to ESP32. Please check:")
        print("  - ESP32 is powered on")
        print("  - WiFi connection is working")
        print("  - IP address is correct")
        return
    
    # Test current scan
    print("\n=== STEP 1: Current Scan ===")
    scan_color()
    
    # Test optimal settings
    print("\n=== STEP 2: Optimal Settings Test ===")
    test_optimal_settings()
    
    # Test brightness levels
    print("\n=== STEP 3: Brightness Level Tests ===")
    brightness_results = test_brightness_levels()
    
    # Summary
    print("\n=== SUMMARY ===")
    print("Brightness test results:")
    for brightness, data in brightness_results:
        r, g, b = data.get('r', 0), data.get('g', 0), data.get('b', 0)
        print(f"  Brightness {brightness}: RGB({r}, {g}, {b})")
    
    print("\nNext steps:")
    print("1. Check if any brightness level gives white results")
    print("2. If not, calibration may be needed")
    print("3. Try accessing the web interface at http://192.168.0.152")

if __name__ == "__main__":
    main()
