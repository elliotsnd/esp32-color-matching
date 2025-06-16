#!/usr/bin/env python3
"""
Color Accuracy Test
Tests both regular and enhanced scans with different colors
"""

import requests
import time

ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"

def regular_scan():
    """Perform regular scan"""
    try:
        response = requests.post(f"{BASE_URL}/scan", timeout=15)
        if response.status_code == 200:
            data = response.json()
            return True, data
        else:
            return False, {}
    except Exception as e:
        print(f"Regular scan error: {e}")
        return False, {}

def enhanced_scan():
    """Perform enhanced scan"""
    try:
        response = requests.post(f"{BASE_URL}/enhanced-scan", timeout=20)
        if response.status_code == 200:
            data = response.json()
            return True, data
        else:
            return False, {}
    except Exception as e:
        print(f"Enhanced scan error: {e}")
        return False, {}

def analyze_color(r, g, b, color_name):
    """Analyze if color reading makes sense"""
    if color_name.lower() == "vivid white":
        # Vivid White should be close to (255, 255, 255)
        if r >= 250 and g >= 250 and b >= 250:
            return "CORRECT - White color detected"
        else:
            return f"INCORRECT - Expected white, got RGB({r},{g},{b})"
    
    elif "beige" in color_name.lower() or "antique" in color_name.lower():
        # Beige/antique should NOT be pure white
        if r == 255 and g == 255 and b == 255:
            return "INCORRECT - Beige should not be pure white (255,255,255)"
        elif r > 200 and g > 180 and b > 150:
            return "CORRECT - Beige-like color detected"
        else:
            return f"UNCERTAIN - Got RGB({r},{g},{b}) for beige"
    
    else:
        return f"INFO - RGB({r},{g},{b}) for {color_name}"

def test_color(color_name):
    """Test a specific color with both scan types"""
    print(f"\n=== Testing {color_name} ===")
    print("Please position the sensor over the color and press Enter...")
    input()
    
    # Test regular scan
    print("Testing Regular Scan...")
    reg_success, reg_data = regular_scan()
    if reg_success:
        r, g, b = reg_data.get('r', 0), reg_data.get('g', 0), reg_data.get('b', 0)
        x, y, z = reg_data.get('x', 0), reg_data.get('y', 0), reg_data.get('z', 0)
        print(f"  Regular: RGB({r}, {g}, {b}) XYZ({x}, {y}, {z})")
        print(f"  Analysis: {analyze_color(r, g, b, color_name)}")
    else:
        print("  Regular scan failed")
    
    time.sleep(1)
    
    # Test enhanced scan
    print("Testing Enhanced Scan...")
    enh_success, enh_data = enhanced_scan()
    if enh_success:
        r, g, b = enh_data.get('r', 0), enh_data.get('g', 0), enh_data.get('b', 0)
        x, y, z = enh_data.get('x', 0), enh_data.get('y', 0), enh_data.get('z', 0)
        print(f"  Enhanced: RGB({r}, {g}, {b}) XYZ({x}, {y}, {z})")
        print(f"  Analysis: {analyze_color(r, g, b, color_name)}")
        
        # Show optimization info if available
        if 'brightnessOptimization' in enh_data:
            opt = enh_data['brightnessOptimization']
            print(f"  Brightness: {opt.get('optimizedBrightness', 'N/A')}")
            print(f"  Control Variable: {opt.get('controlVariable', 'N/A')}")
    else:
        print("  Enhanced scan failed")
    
    return reg_success and enh_success

def main():
    print("Color Accuracy Test")
    print("==================")
    print(f"ESP32 IP: {ESP32_IP}")
    
    # Test connectivity
    try:
        response = requests.get(f"{BASE_URL}/status", timeout=5)
        if response.status_code != 200:
            print("ERROR: Cannot connect to ESP32")
            return
    except:
        print("ERROR: Cannot connect to ESP32")
        return
    
    print("Connected to ESP32 successfully")
    
    # Test different colors
    colors_to_test = [
        "Vivid White",
        "Beige Antiquity"
    ]
    
    results = {}
    
    for color in colors_to_test:
        success = test_color(color)
        results[color] = success
    
    print("\n=== SUMMARY ===")
    for color, success in results.items():
        status = "PASS" if success else "FAIL"
        print(f"{color}: {status}")
    
    print("\nKey Points:")
    print("- Vivid White should read as RGB(255,255,255) or very close")
    print("- Beige Antiquity should NOT read as RGB(255,255,255)")
    print("- Enhanced scan should work for both colors")

if __name__ == "__main__":
    main()
