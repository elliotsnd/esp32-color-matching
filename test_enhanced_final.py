#!/usr/bin/env python3
"""
Final Enhanced Scan Test - No Unicode
"""

import requests

ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"

def test_enhanced_scan():
    try:
        response = requests.post(f"{BASE_URL}/enhanced-scan", timeout=20)
        if response.status_code == 200:
            data = response.json()
            r, g, b = data.get('r', 0), data.get('g', 0), data.get('b', 0)
            x, y, z = data.get('x', 0), data.get('y', 0), data.get('z', 0)
            
            print(f"Enhanced Scan Result: RGB({r}, {g}, {b}) XYZ({x}, {y}, {z})")
            
            if r == 255 and g == 255 and b == 255:
                print("SUCCESS: Enhanced scan returns perfect white!")
                return True
            else:
                print("ISSUE: Enhanced scan not returning white")
                return False
        else:
            print(f"ERROR: HTTP {response.status_code}")
            return False
    except Exception as e:
        print(f"ERROR: {e}")
        return False

def main():
    print("Final Enhanced Scan Test")
    print("=======================")
    
    success = test_enhanced_scan()
    
    if success:
        print("\nCONCLUSION: Enhanced scan fix is working!")
        print("Vivid White now scans correctly as RGB (255,255,255)")
    else:
        print("\nCONCLUSION: Enhanced scan still needs work")

if __name__ == "__main__":
    main()
