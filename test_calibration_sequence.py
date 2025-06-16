#!/usr/bin/env python3
"""
Comprehensive Calibration and Scan Test
Tests white calibration, black calibration, and then scans white object
"""

import requests
import time
import json

ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"
TIMEOUT = 30  # Increased timeout for more reliable calibration

def perform_white_calibration():
    print("\n🔆 Performing White Calibration...")
    print("Please place white reference object under sensor")
    input("Press Enter when ready...")
    
    try:
        response = requests.post(f"{BASE_URL}/calibrate/standard/white", timeout=TIMEOUT)
        if response.status_code == 200:
            data = response.json()
            if data.get('success'):
                print("✅ White calibration successful!")
                print(f"Calibration values: X={data['calibration']['x']}, Y={data['calibration']['y']}, Z={data['calibration']['z']}")
                return True
            else:
                print("❌ White calibration failed:", data.get('message', 'Unknown error'))
                return False
        else:
            print(f"❌ HTTP Error: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error during white calibration: {e}")
        return False

def perform_black_calibration():
    print("\n⚫ Performing Black Calibration...")
    print("Please place black reference object under sensor")
    input("Press Enter when ready...")
    
    try:
        response = requests.post(f"{BASE_URL}/calibrate/standard/black", timeout=TIMEOUT)
        if response.status_code == 200:
            data = response.json()
            if data.get('success'):
                print("✅ Black calibration successful!")
                print(f"Calibration values: X={data['calibration']['x']}, Y={data['calibration']['y']}, Z={data['calibration']['z']}")
                return True
            else:
                print("❌ Black calibration failed:", data.get('message', 'Unknown error'))
                return False
        else:
            print(f"❌ HTTP Error: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error during black calibration: {e}")
        return False

def test_white_scan():
    print("\n📝 Testing Enhanced Scan with White Object...")
    print("Please place white object under sensor")
    input("Press Enter when ready...")
    
    try:
        response = requests.post(f"{BASE_URL}/enhanced-scan", timeout=TIMEOUT)
        if response.status_code == 200:
            data = response.json()
            r, g, b = data.get('r', 0), data.get('g', 0), data.get('b', 0)
            x, y, z = data.get('x', 0), data.get('y', 0), data.get('z', 0)
            
            print(f"\nScan Results:")
            print(f"RGB: ({r}, {g}, {b})")
            print(f"XYZ: ({x}, {y}, {z})")
            
            # Check if values are close to white (allowing some tolerance)
            rgb_diff = abs(r - 255) + abs(g - 255) + abs(b - 255)
            if rgb_diff < 30:  # Allow small variance
                print("✅ SUCCESS: Scan shows near-perfect white!")
                return True
            else:
                print("❌ ISSUE: Scan values deviate from expected white")
                print(f"RGB difference from pure white: {rgb_diff}")
                return False
        else:
            print(f"❌ HTTP Error: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error during scan: {e}")
        return False

def main():
    print("🧪 Comprehensive Calibration and Scan Test")
    print("=========================================")
    
    # Step 1: White Calibration
    if not perform_white_calibration():
        print("❌ Stopping test due to white calibration failure")
        return
    
    time.sleep(2)  # Brief pause between calibrations
    
    # Step 2: Black Calibration
    if not perform_black_calibration():
        print("❌ Stopping test due to black calibration failure")
        return
    
    time.sleep(2)  # Brief pause before scanning
    
    # Step 3: Test Scan
    success = test_white_scan()
    
    print("\n📊 Test Summary")
    print("==============")
    if success:
        print("✅ Complete calibration sequence successful!")
        print("✅ White scan verification passed")
    else:
        print("❌ Test sequence failed")
        print("👉 Please check calibration objects and sensor positioning")

if __name__ == "__main__":
    main()
