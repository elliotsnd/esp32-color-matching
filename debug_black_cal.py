#!/usr/bin/env python3
"""
Debug Black Calibration Process
"""

import requests
import time
import json

ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"

def debug_black_calibration():
    print("🔍 Black Calibration Debug Tool")
    print("==============================")
    
    # 1. Check current status
    print("\nChecking current calibration status...")
    try:
        response = requests.get(f"{BASE_URL}/calibrate/standard/status")
        print(f"Status Response: {json.dumps(response.json(), indent=2)}")
    except Exception as e:
        print(f"Error getting status: {e}")
    
    # 2. Perform black calibration with debug output
    print("\n⚫ Starting Black Calibration")
    print("Please ensure complete darkness around sensor")
    input("Press Enter when ready...")
    
    try:
        response = requests.post(f"{BASE_URL}/calibrate/standard/black")
        print(f"\nResponse Status Code: {response.status_code}")
        print(f"Response Headers: {dict(response.headers)}")
        print(f"Response Body: {json.dumps(response.json(), indent=2)}")
    except Exception as e:
        print(f"Error during calibration: {e}")
    
    # 3. Check status after calibration
    print("\nChecking post-calibration status...")
    try:
        response = requests.get(f"{BASE_URL}/calibrate/standard/status")
        print(f"Status Response: {json.dumps(response.json(), indent=2)}")
    except Exception as e:
        print(f"Error getting status: {e}")

if __name__ == "__main__":
    debug_black_calibration()
