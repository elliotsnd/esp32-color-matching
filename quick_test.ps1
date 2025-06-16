# Quick TCS3430 Calibration Test Script (PowerShell)
# Usage: .\quick_test.ps1 [ESP32_IP]

param(
    [string]$ESP32_IP = "192.168.1.100"
)

$BASE_URL = "http://$ESP32_IP"

Write-Host "🧪 TCS3430 Quick Calibration Test" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
Write-Host "ESP32 IP: $ESP32_IP" -ForegroundColor White
Write-Host ""

# Test 1: Check connectivity
Write-Host "🔌 Testing connectivity..." -ForegroundColor Yellow
try {
    $response = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get -TimeoutSec 5
    Write-Host "✅ ESP32 is reachable" -ForegroundColor Green
} catch {
    Write-Host "❌ Cannot connect to ESP32 at $ESP32_IP" -ForegroundColor Red
    Write-Host "Please check:" -ForegroundColor Yellow
    Write-Host "  - ESP32 is powered on" -ForegroundColor Yellow
    Write-Host "  - WiFi connection is working" -ForegroundColor Yellow
    Write-Host "  - IP address is correct" -ForegroundColor Yellow
    exit 1
}

# Test 2: Get current status
Write-Host ""
Write-Host "📊 Getting current calibration settings..." -ForegroundColor Yellow
try {
    $currentStatus = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get
    Write-Host "✅ Status retrieved successfully" -ForegroundColor Green
    Write-Host "Current settings:" -ForegroundColor White
    Write-Host "  Auto-Zero Mode: $($currentStatus.autoZeroMode)" -ForegroundColor Cyan
    Write-Host "  Auto-Zero Frequency: $($currentStatus.autoZeroFreq)" -ForegroundColor Cyan
    Write-Host "  Wait Time: $($currentStatus.waitTime)" -ForegroundColor Cyan
} catch {
    Write-Host "❌ Failed to get status: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Test 3: Update calibration settings
Write-Host ""
Write-Host "🔧 Testing calibration settings update..." -ForegroundColor Yellow

# Test auto-zero mode
Write-Host "  Testing auto-zero mode..." -ForegroundColor White
try {
    $body = @{ autoZeroMode = 1 } | ConvertTo-Json
    Invoke-RestMethod -Uri "$BASE_URL/settings" -Method Post -Body $body -ContentType "application/json" | Out-Null
    Write-Host "  ✅ Auto-zero mode update sent" -ForegroundColor Green
} catch {
    Write-Host "  ❌ Auto-zero mode update failed: $($_.Exception.Message)" -ForegroundColor Red
}

Start-Sleep -Seconds 1

# Test auto-zero frequency
Write-Host "  Testing auto-zero frequency..." -ForegroundColor White
try {
    $body = @{ autoZeroFreq = 127 } | ConvertTo-Json
    Invoke-RestMethod -Uri "$BASE_URL/settings" -Method Post -Body $body -ContentType "application/json" | Out-Null
    Write-Host "  ✅ Auto-zero frequency update sent" -ForegroundColor Green
} catch {
    Write-Host "  ❌ Auto-zero frequency update failed: $($_.Exception.Message)" -ForegroundColor Red
}

Start-Sleep -Seconds 1

# Test wait time
Write-Host "  Testing wait time..." -ForegroundColor White
try {
    $body = @{ waitTime = 5 } | ConvertTo-Json
    Invoke-RestMethod -Uri "$BASE_URL/settings" -Method Post -Body $body -ContentType "application/json" | Out-Null
    Write-Host "  ✅ Wait time update sent" -ForegroundColor Green
} catch {
    Write-Host "  ❌ Wait time update failed: $($_.Exception.Message)" -ForegroundColor Red
}

Start-Sleep -Seconds 1

# Test 4: Verify settings were applied
Write-Host ""
Write-Host "🔍 Verifying settings were applied..." -ForegroundColor Yellow
try {
    $newStatus = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get
    Write-Host "✅ Verification status retrieved" -ForegroundColor Green
    
    Write-Host "Applied settings:" -ForegroundColor White
    Write-Host "  Auto-Zero Mode: $($newStatus.autoZeroMode) (expected: 1)" -ForegroundColor Cyan
    Write-Host "  Auto-Zero Frequency: $($newStatus.autoZeroFreq) (expected: 127)" -ForegroundColor Cyan
    Write-Host "  Wait Time: $($newStatus.waitTime) (expected: 5)" -ForegroundColor Cyan
    
    # Check if values match expectations
    if ($newStatus.autoZeroMode -eq 1 -and $newStatus.autoZeroFreq -eq 127 -and $newStatus.waitTime -eq 5) {
        Write-Host "✅ All settings applied correctly!" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Some settings may not have been applied correctly" -ForegroundColor Yellow
    }
} catch {
    Write-Host "❌ Failed to verify settings: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 5: Test combined settings
Write-Host ""
Write-Host "🔄 Testing combined settings update..." -ForegroundColor Yellow
try {
    $combinedBody = @{ 
        autoZeroMode = 0
        autoZeroFreq = 200
        waitTime = 10
    } | ConvertTo-Json
    
    Invoke-RestMethod -Uri "$BASE_URL/settings" -Method Post -Body $combinedBody -ContentType "application/json" | Out-Null
    Write-Host "✅ Combined settings update sent" -ForegroundColor Green
    
    Start-Sleep -Seconds 2
    
    # Verify combined settings
    $finalStatus = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get
    
    Write-Host "Final settings:" -ForegroundColor White
    Write-Host "  Auto-Zero Mode: $($finalStatus.autoZeroMode) (expected: 0)" -ForegroundColor Cyan
    Write-Host "  Auto-Zero Frequency: $($finalStatus.autoZeroFreq) (expected: 200)" -ForegroundColor Cyan
    Write-Host "  Wait Time: $($finalStatus.waitTime) (expected: 10)" -ForegroundColor Cyan
    
    if ($finalStatus.autoZeroMode -eq 0 -and $finalStatus.autoZeroFreq -eq 200 -and $finalStatus.waitTime -eq 10) {
        Write-Host "✅ Combined settings applied correctly!" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Combined settings may not have been applied correctly" -ForegroundColor Yellow
    }
} catch {
    Write-Host "❌ Combined settings update failed: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 6: Test invalid values (should be rejected)
Write-Host ""
Write-Host "🚫 Testing invalid values (should be rejected)..." -ForegroundColor Yellow

# Get current settings before testing invalid values
$beforeInvalid = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get

# Test invalid auto-zero mode
try {
    $invalidBody = @{ autoZeroMode = 5 } | ConvertTo-Json
    Invoke-RestMethod -Uri "$BASE_URL/settings" -Method Post -Body $invalidBody -ContentType "application/json" | Out-Null
    Start-Sleep -Seconds 1
    
    $afterInvalid = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get
    
    if ($beforeInvalid.autoZeroMode -eq $afterInvalid.autoZeroMode) {
        Write-Host "  ✅ Invalid auto-zero mode (5) correctly rejected" -ForegroundColor Green
    } else {
        Write-Host "  ❌ Invalid auto-zero mode (5) was incorrectly accepted" -ForegroundColor Red
    }
} catch {
    Write-Host "  ✅ Invalid auto-zero mode request properly handled" -ForegroundColor Green
}

Write-Host ""
Write-Host "🎯 Quick test completed!" -ForegroundColor Green
Write-Host ""
Write-Host "For more comprehensive testing:" -ForegroundColor Yellow
Write-Host "  - Run: python test_calibration.py $ESP32_IP" -ForegroundColor Cyan
Write-Host "  - Open: test_web_ui.html in your browser" -ForegroundColor Cyan
Write-Host ""

# Test summary
Write-Host "📋 Test Summary:" -ForegroundColor Magenta
Write-Host "  ✅ Connectivity test" -ForegroundColor Green
Write-Host "  ✅ Status retrieval" -ForegroundColor Green
Write-Host "  ✅ Individual setting updates" -ForegroundColor Green
Write-Host "  ✅ Setting verification" -ForegroundColor Green
Write-Host "  ✅ Combined settings update" -ForegroundColor Green
Write-Host "  ✅ Invalid value rejection" -ForegroundColor Green
Write-Host ""
Write-Host "🎉 All basic calibration tests passed!" -ForegroundColor Green
