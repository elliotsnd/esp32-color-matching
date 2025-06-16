# ESP32 Color Matcher Test Script
Write-Host "ESP32 Color Matcher Complete Workflow Test" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

$ESP32_IP = "192.168.0.152"
$BASE_URL = "http://$ESP32_IP"

# Test 1: Check Status
Write-Host "`n1. Testing ESP32 Status..." -ForegroundColor Yellow
try {
    $status = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get -TimeoutSec 10
    Write-Host "SUCCESS: ESP32 Status Retrieved" -ForegroundColor Green
    Write-Host "   Device IP: $($status.esp32IP)" -ForegroundColor White
    Write-Host "   Calibrated: $($status.isCalibrated)" -ForegroundColor White
    Write-Host "   Sample Count: $($status.sampleCount)" -ForegroundColor White
    Write-Host "   Current RGB: ($($status.currentR), $($status.currentG), $($status.currentB))" -ForegroundColor White
} catch {
    Write-Host "ERROR: Failed to get status: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Test 2: Perform Color Scan
Write-Host "`n2. Testing Color Scan..." -ForegroundColor Yellow
try {
    $scanResult = Invoke-RestMethod -Uri "$BASE_URL/scan" -Method Post -TimeoutSec 15
    Write-Host "SUCCESS: Color Scan Successful" -ForegroundColor Green
    Write-Host "   RGB: ($($scanResult.r), $($scanResult.g), $($scanResult.b))" -ForegroundColor White
    Write-Host "   XYZ: ($($scanResult.x), $($scanResult.y), $($scanResult.z))" -ForegroundColor White
    Write-Host "   IR: $($scanResult.ir)" -ForegroundColor White

    # Store scan results for saving
    $scannedR = $scanResult.r
    $scannedG = $scanResult.g
    $scannedB = $scanResult.b
} catch {
    Write-Host "ERROR: Color scan failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Test 3: Save Sample
Write-Host "`n3. Testing Sample Save..." -ForegroundColor Yellow
try {
    $saveBody = @{
        r = $scannedR
        g = $scannedG
        b = $scannedB
    } | ConvertTo-Json
    
    $saveResult = Invoke-RestMethod -Uri "$BASE_URL/save" -Method Post -Body $saveBody -ContentType "application/json" -TimeoutSec 15
    Write-Host "✅ Sample Saved Successfully" -ForegroundColor Green
    Write-Host "   Response: $saveResult" -ForegroundColor White
} catch {
    Write-Host "❌ Sample save failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "   This might indicate Google Apps Script integration issues" -ForegroundColor Yellow
}

# Test 4: Retrieve Saved Samples
Write-Host "`n4. Testing Saved Samples Retrieval..." -ForegroundColor Yellow
try {
    $samples = Invoke-RestMethod -Uri "$BASE_URL/samples" -Method Get -TimeoutSec 10
    Write-Host "✅ Samples Retrieved Successfully" -ForegroundColor Green
    Write-Host "   Total Samples: $($samples.samples.Count)" -ForegroundColor White
    
    if ($samples.samples.Count -gt 0) {
        Write-Host "   Recent Samples:" -ForegroundColor White
        for ($i = 0; $i -lt [Math]::Min(3, $samples.samples.Count); $i++) {
            $sample = $samples.samples[$i]
            Write-Host "     Sample $($i+1): RGB($($sample.r), $($sample.g), $($sample.b)) - $($sample.paintName)" -ForegroundColor Cyan
        }
    }
} catch {
    Write-Host "❌ Failed to retrieve samples: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 5: Check Network Connectivity for Google Apps Script
Write-Host "`n5. Testing Google Apps Script Integration..." -ForegroundColor Yellow
try {
    # Check if we can reach Google Apps Script (this is what the ESP32 calls)
    $googleScriptUrl = "https://script.google.com/macros/s/AKfycbzpujFUIP68MjGGTBpwyfkmlWozN2BsPIjxdM7zTqNzBD4Zz62qvioL4QizZ-W_ruB0/exec"
    $testUrl = "$googleScriptUrl" + "?r=255&g=255&b=255"
    
    Write-Host "   Testing Google Apps Script URL..." -ForegroundColor White
    $googleResponse = Invoke-RestMethod -Uri $testUrl -Method Get -TimeoutSec 10
    Write-Host "✅ Google Apps Script Reachable" -ForegroundColor Green
    Write-Host "   Response: $($googleResponse | ConvertTo-Json -Compress)" -ForegroundColor White
} catch {
    Write-Host "❌ Google Apps Script test failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "   This could explain color matching issues" -ForegroundColor Yellow
}

# Test 6: Test Multiple Scans
Write-Host "`n6. Testing Multiple Scans..." -ForegroundColor Yellow
for ($i = 1; $i -le 3; $i++) {
    try {
        Write-Host "   Scan $i..." -ForegroundColor White
        $multiScan = Invoke-RestMethod -Uri "$BASE_URL/scan" -Method Post -TimeoutSec 15
        Write-Host "   ✅ Scan $i: RGB($($multiScan.r), $($multiScan.g), $($multiScan.b))" -ForegroundColor Green
        Start-Sleep -Seconds 2
    } catch {
        Write-Host "   ❌ Scan $i failed: $($_.Exception.Message)" -ForegroundColor Red
    }
}

# Final Status Check
Write-Host "`n7. Final Status Check..." -ForegroundColor Yellow
try {
    $finalStatus = Invoke-RestMethod -Uri "$BASE_URL/status" -Method Get -TimeoutSec 10
    Write-Host "✅ Final Status Retrieved" -ForegroundColor Green
    Write-Host "   Sample Count: $($finalStatus.sampleCount)" -ForegroundColor White
    Write-Host "   Last RGB: ($($finalStatus.currentR), $($finalStatus.currentG), $($finalStatus.currentB))" -ForegroundColor White
} catch {
    Write-Host "❌ Final status check failed: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`n🎯 ESP32 Color Matcher Workflow Test Complete!" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Cyan
