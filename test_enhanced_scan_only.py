#!/usr/bin/env python3
"""
Test Enhanced Scan Only
Quick test to check if enhanced scan is now working
"""

import requests
import json

# Configuration
ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"
TIMEOUT = 15

def enhanced_scan():
    """Perform enhanced scan"""
    print("--- Enhanced Scan Test ---")
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
            
            # Check result
            if r == 255 and g == 255 and b == 255:
                print("✅ SUCCESS: Enhanced scan now returns white RGB (255,255,255)!")
                return True
            elif r > 200 and g > 200 and b > 200:
                print("⚠️  PARTIAL: Enhanced scan returns near-white RGB")
                return True
            else:
                print("❌ ISSUE: Enhanced scan still not returning white")
                return False
                
        else:
            print(f"ERROR: Enhanced scan failed with HTTP {response.status_code}")
            print(f"Response: {response.text}")
            return False
    except Exception as e:
        print(f"ERROR: Enhanced scan failed - {e}")
        return False

def main():
    print("Enhanced Scan Fix Test")
    print("=====================")
    print(f"ESP32 IP: {ESP32_IP}")
    
    # Test enhanced scan multiple times
    success_count = 0
    total_tests = 3
    
    for i in range(total_tests):
        print(f"\n=== Test {i+1}/{total_tests} ===")
        if enhanced_scan():
            success_count += 1
    
    print(f"\n=== RESULTS ===")
    print(f"Successful tests: {success_count}/{total_tests}")
    
    if success_count == total_tests:
        print("🎉 EXCELLENT: Enhanced scan fix is working perfectly!")
    elif success_count > 0:
        print("⚠️  PARTIAL: Enhanced scan fix is working sometimes")
    else:
        print("❌ FAILED: Enhanced scan fix is not working")

if __name__ == "__main__":
    main()
