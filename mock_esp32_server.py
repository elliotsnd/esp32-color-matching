#!/usr/bin/env python3
"""
Mock ESP32 Server for Testing TCS3430 Calibration
Simulates the ESP32 color matcher API for testing purposes
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import json
import time

app = Flask(__name__)
CORS(app)  # Enable CORS for web UI testing

# Simulated device state
device_state = {
    "isScanning": False,
    "ledState": False,
    "isCalibrated": True,
    "currentR": 128,
    "currentG": 128,
    "currentB": 128,
    "sampleCount": 5,
    "atime": 56,
    "again": 3,
    "brightness": 128,
    "ambientLux": 150.5,
    # TCS3430 Advanced Calibration Settings
    "autoZeroMode": 1,
    "autoZeroFreq": 127,
    "waitTime": 0,
    "esp32IP": "192.168.1.100",
    "clientIP": "192.168.1.50",
    "gateway": "192.168.1.1",
    "subnet": "255.255.255.0",
    "macAddress": "AA:BB:CC:DD:EE:FF",
    "rssi": -45
}

@app.route('/status', methods=['GET'])
def get_status():
    """Return current device status"""
    print(f"[STATUS] Returning device status")
    return jsonify(device_state)

@app.route('/settings', methods=['POST'])
def update_settings():
    """Update device settings with validation"""
    try:
        data = request.get_json()
        print(f"[SETTINGS] Received update: {data}")
        
        updated_fields = []
        
        # Validate and update autoZeroMode
        if 'autoZeroMode' in data:
            value = int(data['autoZeroMode'])
            if 0 <= value <= 1:
                device_state['autoZeroMode'] = value
                updated_fields.append(f"autoZeroMode={value}")
                print(f"[SETTINGS] Auto-zero mode updated to: {value}")
            else:
                print(f"[SETTINGS] Invalid auto-zero mode: {value} (must be 0-1)")
                return jsonify({"error": f"Invalid auto-zero mode: {value} (must be 0-1)"}), 400
        
        # Validate and update autoZeroFreq
        if 'autoZeroFreq' in data:
            value = int(data['autoZeroFreq'])
            if 0 <= value <= 255:
                device_state['autoZeroFreq'] = value
                updated_fields.append(f"autoZeroFreq={value}")
                print(f"[SETTINGS] Auto-zero frequency updated to: {value}")
            else:
                print(f"[SETTINGS] Invalid auto-zero frequency: {value} (must be 0-255)")
                return jsonify({"error": f"Invalid auto-zero frequency: {value} (must be 0-255)"}), 400
        
        # Validate and update waitTime
        if 'waitTime' in data:
            value = int(data['waitTime'])
            if 0 <= value <= 255:
                device_state['waitTime'] = value
                updated_fields.append(f"waitTime={value}")
                print(f"[SETTINGS] Wait time updated to: {value}")
            else:
                print(f"[SETTINGS] Invalid wait time: {value} (must be 0-255)")
                return jsonify({"error": f"Invalid wait time: {value} (must be 0-255)"}), 400
        
        # Update other standard settings
        if 'atime' in data:
            device_state['atime'] = int(data['atime'])
            updated_fields.append(f"atime={data['atime']}")
        
        if 'again' in data:
            device_state['again'] = int(data['again'])
            updated_fields.append(f"again={data['again']}")
        
        if 'brightness' in data:
            device_state['brightness'] = int(data['brightness'])
            updated_fields.append(f"brightness={data['brightness']}")
        
        if 'ledState' in data:
            device_state['ledState'] = bool(data['ledState'])
            updated_fields.append(f"ledState={data['ledState']}")
        
        print(f"[SETTINGS] Updated fields: {', '.join(updated_fields)}")
        return "Settings saved", 200
        
    except Exception as e:
        print(f"[SETTINGS] Error: {e}")
        return jsonify({"error": str(e)}), 400

@app.route('/scan', methods=['POST'])
def start_scan():
    """Simulate color scan"""
    print(f"[SCAN] Starting color scan simulation")
    device_state['isScanning'] = True
    
    # Simulate scan delay
    time.sleep(0.1)
    
    # Simulate color reading
    device_state['currentR'] = 180
    device_state['currentG'] = 120
    device_state['currentB'] = 90
    device_state['isScanning'] = False
    
    print(f"[SCAN] Scan complete: RGB({device_state['currentR']}, {device_state['currentG']}, {device_state['currentB']})")
    return jsonify({
        "success": True,
        "r": device_state['currentR'],
        "g": device_state['currentG'],
        "b": device_state['currentB']
    })

@app.route('/brightness', methods=['POST'])
def update_brightness():
    """Update LED brightness"""
    try:
        data = request.get_json()
        brightness = int(data.get('brightness', 128))
        
        if 0 <= brightness <= 255:
            device_state['brightness'] = brightness
            print(f"[BRIGHTNESS] Updated to: {brightness}")
            return jsonify({"success": True, "brightness": brightness})
        else:
            return jsonify({"error": "Brightness must be 0-255"}), 400
            
    except Exception as e:
        return jsonify({"error": str(e)}), 400

@app.route('/', methods=['GET'])
def root():
    """Root endpoint"""
    return jsonify({
        "device": "ESP32 Color Matcher (Mock)",
        "version": "1.0.0",
        "status": "running",
        "endpoints": ["/status", "/settings", "/scan", "/brightness"]
    })

def main():
    print("🧪 Mock ESP32 Color Matcher Server")
    print("==================================")
    print("This server simulates the ESP32 color matcher API")
    print("for testing TCS3430 calibration functionality.")
    print("")
    print("Available endpoints:")
    print("  GET  /status     - Get device status")
    print("  POST /settings   - Update settings")
    print("  POST /scan       - Start color scan")
    print("  POST /brightness - Update brightness")
    print("")
    print("Starting server on http://localhost:5000")
    print("Use this IP in your test scripts: 127.0.0.1:5000")
    print("")
    print("To test:")
    print("  python test_calibration.py 127.0.0.1:5000")
    print("  .\\quick_test.ps1 127.0.0.1:5000")
    print("  Open test_web_ui.html and use 127.0.0.1:5000")
    print("")
    
    app.run(host='0.0.0.0', port=5000, debug=True)

if __name__ == '__main__':
    main()
