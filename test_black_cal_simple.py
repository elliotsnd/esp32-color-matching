#!/usr/bin/env python3
"""
Simple Black Calibration Test
"""

import requests
import time

ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"
TIMEOUT = 30

def test_black_calibration():
    print("\n=== Black Calibration Test ===")
    
    # Step 1: Initial status check
    print("\nChecking initial status...")
    try:
        response = requests.get(f"{BASE_URL}/calibrate/standard/status")
        initial_status = response.json()
        print(f"Initial calibration status: {initial_status.get('hasCalibration', False)}")
    except Exception as e:
        print(f"Error checking status: {e}")
        return False

    # Step 2: Perform black calibration
    print("\nStarting black calibration...")
    print("Please ensure:")
    print("1. Black reference card is in position")
    print("2. No ambient light contamination")
    print("3. Sensor is completely covered")
    input("Press Enter when ready...")

    try:
        response = requests.post(f"{BASE_URL}/calibrate/standard/black")
        result = response.json()
        
        if result.get('success'):
            black_data = result.get('blackData', {})
            print("\n✅ Black calibration successful!")
            print(f"X: {black_data.get('x', 'N/A')}")
            print(f"Y: {black_data.get('y', 'N/A')}")
            print(f"Z: {black_data.get('z', 'N/A')}")
            print(f"IR: {black_data.get('ir', 'N/A')}")
            return True
        else:
            print(f"\n❌ Black calibration failed: {result.get('message', 'Unknown error')}")
            return False
            
    except Exception as e:
        print(f"\n❌ Error during black calibration: {e}")
        return False

def main():
    success = test_black_calibration()
    
    if success:
        print("\n🎉 Black calibration test completed successfully!")
    else:
        print("\n⚠️ Black calibration test failed!")
        print("Please check:")
        print("1. Sensor positioning")
        print("2. Ambient light conditions")
        print("3. Black reference card quality")

if __name__ == "__main__":
    main()
