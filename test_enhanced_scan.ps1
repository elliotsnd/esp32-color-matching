# Enhanced Scan Test Script
Write-Host "Enhanced 5-Second Continuous Scan Test" -ForegroundColor Cyan
Write-Host "=======================================" -ForegroundColor Cyan

$ESP32_IP = "192.168.0.152"
$BASE_URL = "http://$ESP32_IP"

Write-Host ""
Write-Host "Testing Enhanced Scanning Functionality..." -ForegroundColor Yellow
Write-Host "This will test the new 5-second continuous scanning feature" -ForegroundColor White
Write-Host ""

# Test 1: Single Enhanced Scan
Write-Host "1. Testing Single Enhanced Scan (5 seconds)..." -ForegroundColor Yellow
try {
    Write-Host "   Starting scan - this will take 5+ seconds..." -ForegroundColor White
    $startTime = Get-Date
    
    $scanResult = Invoke-RestMethod -Uri "$BASE_URL/scan" -Method Post -TimeoutSec 30
    
    $endTime = Get-Date
    $duration = ($endTime - $startTime).TotalSeconds
    
    Write-Host "SUCCESS: Enhanced scan completed in $([math]::Round($duration, 1)) seconds" -ForegroundColor Green
    Write-Host "   RGB: ($($scanResult.r), $($scanResult.g), $($scanResult.b))" -ForegroundColor White
    Write-Host "   XYZ: ($($scanResult.x), $($scanResult.y), $($scanResult.z))" -ForegroundColor White
    Write-Host "   IR: $($scanResult.ir)" -ForegroundColor White
    
    # Store results for comparison
    $scan1_r = $scanResult.r
    $scan1_g = $scanResult.g
    $scan1_b = $scanResult.b
    
} catch {
    Write-Host "ERROR: Enhanced scan failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "2. Testing Scan Consistency (3 consecutive enhanced scans)..." -ForegroundColor Yellow

$scans = @()
for ($i = 1; $i -le 3; $i++) {
    try {
        Write-Host "   Enhanced Scan $i/3..." -ForegroundColor White
        $startTime = Get-Date
        
        $scanResult = Invoke-RestMethod -Uri "$BASE_URL/scan" -Method Post -TimeoutSec 30
        
        $endTime = Get-Date
        $duration = ($endTime - $startTime).TotalSeconds
        
        $scans += @{
            r = $scanResult.r
            g = $scanResult.g
            b = $scanResult.b
            x = $scanResult.x
            y = $scanResult.y
            z = $scanResult.z
            ir = $scanResult.ir
            duration = $duration
        }
        
        Write-Host "   SUCCESS: Scan $i completed in $([math]::Round($duration, 1))s - RGB($($scanResult.r), $($scanResult.g), $($scanResult.b))" -ForegroundColor Green
        
        # Small delay between scans
        Start-Sleep -Seconds 2
        
    } catch {
        Write-Host "   ERROR: Scan $i failed: $($_.Exception.Message)" -ForegroundColor Red
    }
}

# Analyze consistency
if ($scans.Count -eq 3) {
    Write-Host ""
    Write-Host "3. Consistency Analysis..." -ForegroundColor Yellow
    
    # Calculate RGB variations
    $r_values = $scans | ForEach-Object { $_.r }
    $g_values = $scans | ForEach-Object { $_.g }
    $b_values = $scans | ForEach-Object { $_.b }
    
    $r_min = ($r_values | Measure-Object -Minimum).Minimum
    $r_max = ($r_values | Measure-Object -Maximum).Maximum
    $r_avg = ($r_values | Measure-Object -Average).Average
    
    $g_min = ($g_values | Measure-Object -Minimum).Minimum
    $g_max = ($g_values | Measure-Object -Maximum).Maximum
    $g_avg = ($g_values | Measure-Object -Average).Average
    
    $b_min = ($b_values | Measure-Object -Minimum).Minimum
    $b_max = ($b_values | Measure-Object -Maximum).Maximum
    $b_avg = ($b_values | Measure-Object -Average).Average
    
    $r_variation = if ($r_avg -gt 0) { (($r_max - $r_min) / $r_avg) * 100 } else { 0 }
    $g_variation = if ($g_avg -gt 0) { (($g_max - $g_min) / $g_avg) * 100 } else { 0 }
    $b_variation = if ($b_avg -gt 0) { (($b_max - $b_min) / $b_avg) * 100 } else { 0 }
    
    Write-Host "   RGB Consistency Results:" -ForegroundColor White
    Write-Host "     Red   - Min: $r_min, Max: $r_max, Avg: $([math]::Round($r_avg, 1)), Variation: $([math]::Round($r_variation, 1))%" -ForegroundColor Cyan
    Write-Host "     Green - Min: $g_min, Max: $g_max, Avg: $([math]::Round($g_avg, 1)), Variation: $([math]::Round($g_variation, 1))%" -ForegroundColor Cyan
    Write-Host "     Blue  - Min: $b_min, Max: $b_max, Avg: $([math]::Round($b_avg, 1)), Variation: $([math]::Round($b_variation, 1))%" -ForegroundColor Cyan
    
    # Overall consistency rating
    $max_variation = [math]::Max([math]::Max($r_variation, $g_variation), $b_variation)
    if ($max_variation -lt 5) {
        Write-Host "   EXCELLENT consistency (< 5% variation)" -ForegroundColor Green
    } elseif ($max_variation -lt 10) {
        Write-Host "   GOOD consistency (< 10% variation)" -ForegroundColor Yellow
    } elseif ($max_variation -lt 20) {
        Write-Host "   MODERATE consistency (< 20% variation)" -ForegroundColor Yellow
    } else {
        Write-Host "   POOR consistency (> 20% variation)" -ForegroundColor Red
    }
    
    # Average scan duration
    $avg_duration = ($scans | ForEach-Object { $_.duration } | Measure-Object -Average).Average
    Write-Host "   Average scan duration: $([math]::Round($avg_duration, 1)) seconds" -ForegroundColor White
}

Write-Host ""
Write-Host "4. Testing Save with Enhanced Scan..." -ForegroundColor Yellow
try {
    $saveBody = @{
        r = $scan1_r
        g = $scan1_g
        b = $scan1_b
    } | ConvertTo-Json
    
    $saveResult = Invoke-RestMethod -Uri "$BASE_URL/save" -Method Post -Body $saveBody -ContentType "application/json" -TimeoutSec 15
    Write-Host "SUCCESS: Enhanced scan sample saved" -ForegroundColor Green
    Write-Host "   Response: $saveResult" -ForegroundColor White
} catch {
    Write-Host "ERROR: Save failed: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host ""
Write-Host "Enhanced Scan Test Complete!" -ForegroundColor Green
Write-Host "=======================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Key Improvements:" -ForegroundColor Yellow
Write-Host "- 5-second continuous scanning for maximum accuracy" -ForegroundColor White
Write-Host "- Up to 200 readings per scan (vs. 3 previously)" -ForegroundColor White
Write-Host "- Real-time consistency analysis" -ForegroundColor White
Write-Host "- Detailed variation metrics" -ForegroundColor White
Write-Host "- Optimized reading frequency (~40 readings/second)" -ForegroundColor White
