#!/usr/bin/env python3
"""
TCS3430 Advanced Calibration Settings Test Suite
Tests the ESP32 color matcher calibration API endpoints
"""

import requests
import json
import time
import sys
from typing import Dict, Any

# Configuration
ESP32_IP = "192.168.0.152"  # ESP32 IP address
BASE_URL = f"http://{ESP32_IP}"
TIMEOUT = 10

class CalibrationTester:
    def __init__(self, base_url: str):
        self.base_url = base_url
        self.test_results = []
        
    def log_test(self, test_name: str, success: bool, message: str = ""):
        """Log test result"""
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status} {test_name}: {message}")
        self.test_results.append({
            "test": test_name,
            "success": success,
            "message": message
        })
    
    def get_status(self) -> Dict[str, Any]:
        """Get current device status"""
        try:
            response = requests.get(f"{self.base_url}/status", timeout=TIMEOUT)
            if response.status_code == 200:
                return response.json()
            else:
                print(f"❌ Status request failed: HTTP {response.status_code}")
                return {}
        except Exception as e:
            print(f"❌ Status request error: {e}")
            return {}
    
    def update_settings(self, settings: Dict[str, Any]) -> bool:
        """Update device settings"""
        try:
            response = requests.post(
                f"{self.base_url}/settings",
                json=settings,
                timeout=TIMEOUT,
                headers={'Content-Type': 'application/json'}
            )
            return response.status_code == 200
        except Exception as e:
            print(f"❌ Settings update error: {e}")
            return False
    
    def test_connectivity(self):
        """Test basic connectivity to ESP32"""
        print("\n🔌 Testing ESP32 Connectivity...")
        try:
            response = requests.get(f"{self.base_url}/status", timeout=5)
            if response.status_code == 200:
                data = response.json()
                self.log_test("ESP32 Connectivity", True, f"Connected to {data.get('esp32IP', 'unknown IP')}")
                return True
            else:
                self.log_test("ESP32 Connectivity", False, f"HTTP {response.status_code}")
                return False
        except Exception as e:
            self.log_test("ESP32 Connectivity", False, str(e))
            return False
    
    def test_status_includes_calibration_settings(self):
        """Test that status endpoint returns calibration settings"""
        print("\n📊 Testing Status Endpoint...")
        status = self.get_status()
        
        required_fields = ['autoZeroMode', 'autoZeroFreq', 'waitTime']
        missing_fields = []
        
        for field in required_fields:
            if field not in status:
                missing_fields.append(field)
        
        if not missing_fields:
            self.log_test("Status Calibration Fields", True, 
                         f"autoZeroMode={status['autoZeroMode']}, autoZeroFreq={status['autoZeroFreq']}, waitTime={status['waitTime']}")
        else:
            self.log_test("Status Calibration Fields", False, f"Missing fields: {missing_fields}")
    
    def test_auto_zero_mode_settings(self):
        """Test auto-zero mode setting (0 and 1)"""
        print("\n🔧 Testing Auto-Zero Mode Settings...")
        
        # Test valid values
        for mode in [0, 1]:
            success = self.update_settings({"autoZeroMode": mode})
            if success:
                time.sleep(0.5)  # Allow setting to apply
                status = self.get_status()
                if status.get('autoZeroMode') == mode:
                    self.log_test(f"Auto-Zero Mode {mode}", True, f"Successfully set to {mode}")
                else:
                    self.log_test(f"Auto-Zero Mode {mode}", False, f"Expected {mode}, got {status.get('autoZeroMode')}")
            else:
                self.log_test(f"Auto-Zero Mode {mode}", False, "Settings update failed")
        
        # Test invalid value
        success = self.update_settings({"autoZeroMode": 2})
        time.sleep(0.5)
        status = self.get_status()
        # Should reject invalid value and keep previous setting
        if status.get('autoZeroMode') in [0, 1]:
            self.log_test("Auto-Zero Mode Invalid", True, "Rejected invalid value 2")
        else:
            self.log_test("Auto-Zero Mode Invalid", False, "Did not reject invalid value")
    
    def test_auto_zero_frequency_settings(self):
        """Test auto-zero frequency setting (0-255)"""
        print("\n📡 Testing Auto-Zero Frequency Settings...")
        
        # Test valid values
        test_values = [0, 127, 255]
        for freq in test_values:
            success = self.update_settings({"autoZeroFreq": freq})
            if success:
                time.sleep(0.5)
                status = self.get_status()
                if status.get('autoZeroFreq') == freq:
                    self.log_test(f"Auto-Zero Freq {freq}", True, f"Successfully set to {freq}")
                else:
                    self.log_test(f"Auto-Zero Freq {freq}", False, f"Expected {freq}, got {status.get('autoZeroFreq')}")
            else:
                self.log_test(f"Auto-Zero Freq {freq}", False, "Settings update failed")
        
        # Test invalid value
        success = self.update_settings({"autoZeroFreq": 300})
        time.sleep(0.5)
        status = self.get_status()
        # Should reject invalid value
        if status.get('autoZeroFreq') <= 255:
            self.log_test("Auto-Zero Freq Invalid", True, "Rejected invalid value 300")
        else:
            self.log_test("Auto-Zero Freq Invalid", False, "Did not reject invalid value")
    
    def test_wait_time_settings(self):
        """Test wait time setting (0-255)"""
        print("\n⏱️ Testing Wait Time Settings...")
        
        # Test valid values
        test_values = [0, 10, 50, 255]
        for wait_time in test_values:
            success = self.update_settings({"waitTime": wait_time})
            if success:
                time.sleep(0.5)
                status = self.get_status()
                if status.get('waitTime') == wait_time:
                    self.log_test(f"Wait Time {wait_time}", True, f"Successfully set to {wait_time}")
                else:
                    self.log_test(f"Wait Time {wait_time}", False, f"Expected {wait_time}, got {status.get('waitTime')}")
            else:
                self.log_test(f"Wait Time {wait_time}", False, "Settings update failed")
        
        # Test invalid value
        success = self.update_settings({"waitTime": 300})
        time.sleep(0.5)
        status = self.get_status()
        # Should reject invalid value
        if status.get('waitTime') <= 255:
            self.log_test("Wait Time Invalid", True, "Rejected invalid value 300")
        else:
            self.log_test("Wait Time Invalid", False, "Did not reject invalid value")
    
    def test_combined_settings(self):
        """Test setting multiple calibration parameters at once"""
        print("\n🔄 Testing Combined Settings...")
        
        combined_settings = {
            "autoZeroMode": 1,
            "autoZeroFreq": 127,
            "waitTime": 5,
            "atime": 56,
            "again": 3
        }
        
        success = self.update_settings(combined_settings)
        if success:
            time.sleep(1)  # Allow all settings to apply
            status = self.get_status()
            
            all_correct = True
            for key, expected_value in combined_settings.items():
                actual_value = status.get(key)
                if actual_value != expected_value:
                    all_correct = False
                    print(f"  ❌ {key}: expected {expected_value}, got {actual_value}")
            
            if all_correct:
                self.log_test("Combined Settings", True, "All settings applied correctly")
            else:
                self.log_test("Combined Settings", False, "Some settings not applied correctly")
        else:
            self.log_test("Combined Settings", False, "Settings update failed")
    
    def test_persistence(self):
        """Test that settings persist (simulated by checking current values)"""
        print("\n💾 Testing Settings Persistence...")
        
        # Set specific values
        test_settings = {
            "autoZeroMode": 0,
            "autoZeroFreq": 200,
            "waitTime": 15
        }
        
        success = self.update_settings(test_settings)
        if success:
            time.sleep(1)
            status1 = self.get_status()
            
            # Wait a bit and check again
            time.sleep(2)
            status2 = self.get_status()
            
            persistent = True
            for key in test_settings:
                if status1.get(key) != status2.get(key):
                    persistent = False
                    break
            
            if persistent:
                self.log_test("Settings Persistence", True, "Settings remained consistent")
            else:
                self.log_test("Settings Persistence", False, "Settings changed unexpectedly")
        else:
            self.log_test("Settings Persistence", False, "Initial settings update failed")
    
    def run_all_tests(self):
        """Run complete test suite"""
        print("🧪 TCS3430 Advanced Calibration Test Suite")
        print("=" * 50)
        
        # Test connectivity first
        if not self.test_connectivity():
            print("\n❌ Cannot connect to ESP32. Please check:")
            print(f"   - ESP32 IP address: {ESP32_IP}")
            print("   - WiFi connection")
            print("   - ESP32 is powered on and running")
            return False
        
        # Run all tests
        self.test_status_includes_calibration_settings()
        self.test_auto_zero_mode_settings()
        self.test_auto_zero_frequency_settings()
        self.test_wait_time_settings()
        self.test_combined_settings()
        self.test_persistence()
        
        # Summary
        print("\n" + "=" * 50)
        print("📋 TEST SUMMARY")
        print("=" * 50)
        
        passed = sum(1 for result in self.test_results if result['success'])
        total = len(self.test_results)
        
        for result in self.test_results:
            status = "✅" if result['success'] else "❌"
            print(f"{status} {result['test']}")
        
        print(f"\n🎯 Results: {passed}/{total} tests passed")
        
        if passed == total:
            print("🎉 ALL TESTS PASSED! Calibration system is working correctly.")
            return True
        else:
            print("⚠️  Some tests failed. Please check the implementation.")
            return False

def main():
    if len(sys.argv) > 1:
        global ESP32_IP
        ESP32_IP = sys.argv[1]
        print(f"Using ESP32 IP: {ESP32_IP}")
    
    tester = CalibrationTester(f"http://{ESP32_IP}")
    success = tester.run_all_tests()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
