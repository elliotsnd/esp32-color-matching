#!/usr/bin/env pwsh
# ESP32 Color Matcher Deployment Script

Write-Host "馃搮 ESP32 Color Matcher Deployment Script" -ForegroundColor Blue
Write-Host "========================================" -ForegroundColor Blue

# Check if PlatformIO CLI is available
if (!(Get-Command platformio -ErrorAction SilentlyContinue)) {
    Write-Host "鉂? Error: PlatformIO CLI not found" -ForegroundColor Red
    Write-Host "Please install PlatformIO Core CLI first:"
    Write-Host "https://docs.platformio.org/en/latest/core/installation.html"
    exit 1
}

# Build firmware
Write-Host "`n馃搫 Building firmware..." -ForegroundColor Yellow
platformio run
if ($LASTEXITCODE -ne 0) {
    Write-Host "鉂? Build failed! Please check the errors above." -ForegroundColor Red
    exit 1
}

# Build filesystem
Write-Host "`n馃搧 Building filesystem..." -ForegroundColor Yellow
platformio run --target buildfs
if ($LASTEXITCODE -ne 0) {
    Write-Host "鉂? Filesystem build failed! Please check the errors above." -ForegroundColor Red
    exit 1
}

# Upload firmware
Write-Host "`n馃搶 Uploading firmware..." -ForegroundColor Yellow
platformio run --target upload
if ($LASTEXITCODE -ne 0) {
    Write-Host "鉂? Firmware upload failed! Please check:" -ForegroundColor Red
    Write-Host "1. ESP32 is connected and powered on"
    Write-Host "2. Correct COM port is selected"
    Write-Host "3. No other program is using the COM port"
    exit 1
}

# Upload filesystem
Write-Host "`n馃搨 Uploading filesystem..." -ForegroundColor Yellow
platformio run --target uploadfs
if ($LASTEXITCODE -ne 0) {
    Write-Host "鉂? Filesystem upload failed!" -ForegroundColor Red
    exit 1
}

Write-Host "`n鉁? Deployment completed successfully!" -ForegroundColor Green
Write-Host "`nNext steps:" -ForegroundColor Cyan
Write-Host "1. Check the serial monitor for boot messages"
Write-Host "2. Verify the web interface is accessible"
Write-Host "3. Run calibration tests"
Write-Host ""

# Ask if user wants to open serial monitor
$monitor = Read-Host "Would you like to open the serial monitor? (Y/N)"
if ($monitor -eq "Y" -or $monitor -eq "y") {
    Write-Host "Opening serial monitor..."
    platformio device monitor
}
