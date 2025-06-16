#!/bin/bash

# Quick TCS3430 Calibration Test Script
# Usage: ./quick_test.sh [ESP32_IP]

ESP32_IP=${1:-"192.168.1.100"}
BASE_URL="http://$ESP32_IP"

echo "🧪 TCS3430 Quick Calibration Test"
echo "=================================="
echo "ESP32 IP: $ESP32_IP"
echo ""

# Test 1: Check connectivity
echo "🔌 Testing connectivity..."
if curl -s --connect-timeout 5 "$BASE_URL/status" > /dev/null; then
    echo "✅ ESP32 is reachable"
else
    echo "❌ Cannot connect to ESP32 at $ESP32_IP"
    echo "Please check:"
    echo "  - ESP32 is powered on"
    echo "  - WiFi connection is working"
    echo "  - IP address is correct"
    exit 1
fi

# Test 2: Get current status
echo ""
echo "📊 Getting current calibration settings..."
CURRENT_STATUS=$(curl -s "$BASE_URL/status")
if [ $? -eq 0 ]; then
    echo "✅ Status retrieved successfully"
    echo "Current settings:"
    echo "$CURRENT_STATUS" | grep -o '"autoZeroMode":[0-9]*' | sed 's/"autoZeroMode":/  Auto-Zero Mode: /'
    echo "$CURRENT_STATUS" | grep -o '"autoZeroFreq":[0-9]*' | sed 's/"autoZeroFreq":/  Auto-Zero Frequency: /'
    echo "$CURRENT_STATUS" | grep -o '"waitTime":[0-9]*' | sed 's/"waitTime":/  Wait Time: /'
else
    echo "❌ Failed to get status"
    exit 1
fi

# Test 3: Update calibration settings
echo ""
echo "🔧 Testing calibration settings update..."

# Test auto-zero mode
echo "  Testing auto-zero mode..."
curl -s -X POST -H "Content-Type: application/json" \
     -d '{"autoZeroMode": 1}' \
     "$BASE_URL/settings" > /dev/null

if [ $? -eq 0 ]; then
    echo "  ✅ Auto-zero mode update sent"
else
    echo "  ❌ Auto-zero mode update failed"
fi

sleep 1

# Test auto-zero frequency
echo "  Testing auto-zero frequency..."
curl -s -X POST -H "Content-Type: application/json" \
     -d '{"autoZeroFreq": 127}' \
     "$BASE_URL/settings" > /dev/null

if [ $? -eq 0 ]; then
    echo "  ✅ Auto-zero frequency update sent"
else
    echo "  ❌ Auto-zero frequency update failed"
fi

sleep 1

# Test wait time
echo "  Testing wait time..."
curl -s -X POST -H "Content-Type: application/json" \
     -d '{"waitTime": 5}' \
     "$BASE_URL/settings" > /dev/null

if [ $? -eq 0 ]; then
    echo "  ✅ Wait time update sent"
else
    echo "  ❌ Wait time update failed"
fi

sleep 1

# Test 4: Verify settings were applied
echo ""
echo "🔍 Verifying settings were applied..."
NEW_STATUS=$(curl -s "$BASE_URL/status")
if [ $? -eq 0 ]; then
    echo "✅ Verification status retrieved"
    
    AUTO_ZERO_MODE=$(echo "$NEW_STATUS" | grep -o '"autoZeroMode":[0-9]*' | grep -o '[0-9]*')
    AUTO_ZERO_FREQ=$(echo "$NEW_STATUS" | grep -o '"autoZeroFreq":[0-9]*' | grep -o '[0-9]*')
    WAIT_TIME=$(echo "$NEW_STATUS" | grep -o '"waitTime":[0-9]*' | grep -o '[0-9]*')
    
    echo "Applied settings:"
    echo "  Auto-Zero Mode: $AUTO_ZERO_MODE (expected: 1)"
    echo "  Auto-Zero Frequency: $AUTO_ZERO_FREQ (expected: 127)"
    echo "  Wait Time: $WAIT_TIME (expected: 5)"
    
    # Check if values match expectations
    if [ "$AUTO_ZERO_MODE" = "1" ] && [ "$AUTO_ZERO_FREQ" = "127" ] && [ "$WAIT_TIME" = "5" ]; then
        echo "✅ All settings applied correctly!"
    else
        echo "⚠️  Some settings may not have been applied correctly"
    fi
else
    echo "❌ Failed to verify settings"
fi

# Test 5: Test combined settings
echo ""
echo "🔄 Testing combined settings update..."
curl -s -X POST -H "Content-Type: application/json" \
     -d '{"autoZeroMode": 0, "autoZeroFreq": 200, "waitTime": 10}' \
     "$BASE_URL/settings" > /dev/null

if [ $? -eq 0 ]; then
    echo "✅ Combined settings update sent"
    
    sleep 2
    
    # Verify combined settings
    FINAL_STATUS=$(curl -s "$BASE_URL/status")
    FINAL_MODE=$(echo "$FINAL_STATUS" | grep -o '"autoZeroMode":[0-9]*' | grep -o '[0-9]*')
    FINAL_FREQ=$(echo "$FINAL_STATUS" | grep -o '"autoZeroFreq":[0-9]*' | grep -o '[0-9]*')
    FINAL_WAIT=$(echo "$FINAL_STATUS" | grep -o '"waitTime":[0-9]*' | grep -o '[0-9]*')
    
    echo "Final settings:"
    echo "  Auto-Zero Mode: $FINAL_MODE (expected: 0)"
    echo "  Auto-Zero Frequency: $FINAL_FREQ (expected: 200)"
    echo "  Wait Time: $FINAL_WAIT (expected: 10)"
    
    if [ "$FINAL_MODE" = "0" ] && [ "$FINAL_FREQ" = "200" ] && [ "$FINAL_WAIT" = "10" ]; then
        echo "✅ Combined settings applied correctly!"
    else
        echo "⚠️  Combined settings may not have been applied correctly"
    fi
else
    echo "❌ Combined settings update failed"
fi

echo ""
echo "🎯 Quick test completed!"
echo ""
echo "For more comprehensive testing:"
echo "  - Run: python3 test_calibration.py $ESP32_IP"
echo "  - Open: test_web_ui.html in your browser"
echo ""
