#!/usr/bin/env python3
"""
Black Calibration Test Suite
Tests the improved black calibration functionality with validation
"""

import requests
import time
import json
from typing import Dict, Any, Tuple

ESP32_IP = "192.168.0.152"
BASE_URL = f"http://{ESP32_IP}"
TIMEOUT = 30

class BlackCalibrationTester:
    def __init__(self):
        self.results = []
        
    def log_result(self, test_name: str, success: bool, message: str = ""):
        """Log test result with timestamp"""
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status} {test_name}: {message}")
        self.results.append({
            "test": test_name,
            "success": success,
            "message": message,
            "timestamp": time.time()
        })

    def get_calibration_status(self) -> Tuple[bool, Dict[str, Any]]:
        """Get current calibration status"""
        try:
            response = requests.get(f"{BASE_URL}/calibrate/standard/status", timeout=TIMEOUT)
            if response.status_code == 200:
                return True, response.json()
            return False, {"error": f"HTTP {response.status_code}"}
        except Exception as e:
            return False, {"error": str(e)}

    def perform_white_calibration(self) -> bool:
        """Perform white calibration"""
        print("\n🔆 Performing White Calibration")
        print("Please ensure:")
        print("1. White reference card is clean")
        print("2. Sensor is properly positioned")
        print("3. Stable lighting conditions")
        input("Press Enter when ready...")

        try:
            response = requests.post(f"{BASE_URL}/calibrate/standard/white", timeout=TIMEOUT)
            if response.status_code == 200:
                data = response.json()
                if data.get('success'):
                    self.log_result("White Calibration", True, 
                                  f"X={data['calibration']['x']}, Y={data['calibration']['y']}, Z={data['calibration']['z']}")
                    return True
            self.log_result("White Calibration", False, "Failed to get valid response")
            return False
        except Exception as e:
            self.log_result("White Calibration", False, str(e))
            return False

    def perform_black_calibration(self) -> bool:
        """Perform black calibration with improved validation"""
        print("\n⚫ Performing Black Calibration")
        print("Please ensure:")
        print("1. Black reference card is properly positioned")
        print("2. No ambient light contamination")
        print("3. Sensor and card are completely covered")
        print("4. Wait for 5 seconds after covering to let conditions stabilize")
        input("Press Enter when ready...")

        try:
            response = requests.post(f"{BASE_URL}/calibrate/standard/black", timeout=TIMEOUT)            if response.status_code == 200:
                data = response.json()
                if data.get('success'):
                    black_data = data.get('blackData', {})
                    self.log_result("Black Calibration", True,
                                  f"X={black_data.get('x')}, Y={black_data.get('y')}, Z={black_data.get('z')}")
                    return True
                else:
                    self.log_result("Black Calibration", False, data.get('error', 'Unknown error'))
            else:
                self.log_result("Black Calibration", False, f"HTTP {response.status_code}")
            return False
        except Exception as e:
            self.log_result("Black Calibration", False, str(e))
            return False

    def validate_black_levels(self) -> bool:
        """Validate black calibration levels are appropriate"""
        success, status = self.get_calibration_status()
        if not success:
            self.log_result("Black Level Validation", False, "Could not get calibration status")
            return False

        try:
            black_cal = status['blackCalibration']
            white_cal = status['whiteCalibration']

            # Check black levels are significantly lower than white
            for channel in ['x', 'y', 'z']:
                black_value = black_cal.get(channel, 0)
                white_value = white_cal.get(channel, 65535)
                ratio = black_value / white_value if white_value > 0 else 1
                
                if ratio > 0.15:  # Black should be less than 15% of white
                    self.log_result("Black Level Validation", False, 
                                  f"{channel.upper()} channel too high: {ratio*100:.1f}% of white")
                    return False

            self.log_result("Black Level Validation", True, "All channels within acceptable ranges")
            return True

        except KeyError as e:
            self.log_result("Black Level Validation", False, f"Missing data: {e}")
            return False
        except Exception as e:
            self.log_result("Black Level Validation", False, str(e))
            return False

    def test_ambient_light_rejection(self) -> bool:
        """Test if calibration rejects high ambient light conditions"""
        print("\n💡 Testing Ambient Light Rejection")
        print("Please:")
        print("1. Position the black reference")
        print("2. Deliberately leave some ambient light")
        print("3. Do NOT fully cover the sensor")
        input("Press Enter to test ambient light rejection...")

        try:
            response = requests.post(f"{BASE_URL}/calibrate/standard/black", timeout=TIMEOUT)
            if response.status_code == 500:  # Expected failure
                data = response.json()
                if 'light contamination' in data.get('error', '').lower():
                    self.log_result("Ambient Light Rejection", True, "Correctly rejected high ambient light")
                    return True
                
            self.log_result("Ambient Light Rejection", False, 
                          "Calibration should have rejected high ambient light")
            return False
        except Exception as e:
            self.log_result("Ambient Light Rejection", False, str(e))
            return False

    def run_all_tests(self):
        """Run complete test sequence"""
        print("🧪 Black Calibration Test Suite")
        print("==============================")

        # Step 1: Initial white calibration
        if not self.perform_white_calibration():
            print("\n❌ Stopping tests: White calibration required for black calibration validation")
            return

        time.sleep(2)  # Brief pause between calibrations

        # Step 2: Proper black calibration
        if not self.perform_black_calibration():
            print("\n❌ Black calibration failed, continuing with remaining tests...")

        time.sleep(2)

        # Step 3: Validate black levels
        self.validate_black_levels()

        # Step 4: Test ambient light rejection
        self.test_ambient_light_rejection()

        # Print summary
        print("\n📊 Test Summary")
        print("==============")
        total_tests = len(self.results)
        passed_tests = sum(1 for r in self.results if r['success'])
        print(f"Tests Passed: {passed_tests}/{total_tests}")
        
        if passed_tests == total_tests:
            print("✅ All black calibration tests passed!")
        else:
            print("⚠️ Some tests failed. Please review the results above.")
            print("Common issues to check:")
            print("1. Light leakage around sensor")
            print("2. Black reference card quality")
            print("3. Sensor positioning")
            print("4. Ambient light conditions")

if __name__ == "__main__":
    tester = BlackCalibrationTester()
    tester.run_all_tests()
