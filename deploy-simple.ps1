#!/usr/bin/env pwsh
# ESP32 Color Matcher Simple Deployment Script
# Fast firmware compilation and upload

param(
    [string]$Port = "COM6",
    [switch]$Monitor,
    [switch]$Verbose
)

# Colors for output
$Green = "`e[32m"
$Red = "`e[31m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Reset = "`e[0m"

Write-Host "${Blue}ESP32 Color Matcher - Quick Firmware Deploy${Reset}"
Write-Host "${Blue}===========================================${Reset}"

$startTime = Get-Date

# Check if we're in the right directory
if (-not (Test-Path "platformio.ini")) {
    Write-Host "${Red}Error: platformio.ini not found${Reset}"
    Write-Host "Please run this script from the project root directory."
    exit 1
}

# Check PlatformIO
if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    Write-Host "${Red}Error: PlatformIO CLI not found${Reset}"
    Write-Host "Please install PlatformIO Core CLI first."
    exit 1
}

# Build firmware
Write-Host "`n${Yellow}Building firmware...${Reset}"
$compileResult = pio run 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "${Red}Build failed! Please check the errors above.${Reset}"
    if ($Verbose) {
        Write-Host $compileResult
    }
    exit 1
}
Write-Host "${Green}Firmware compiled successfully${Reset}"

# Upload firmware
Write-Host "`n${Yellow}Uploading firmware to $Port...${Reset}"
$uploadResult = pio run --target upload --upload-port $Port 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "${Red}Upload failed! Please check the connection and port.${Reset}"
    if ($Verbose) {
        Write-Host $uploadResult
    }
    exit 1
}

$endTime = Get-Date
$duration = $endTime - $startTime

Write-Host "${Green}Firmware uploaded successfully!${Reset}"
Write-Host "${Blue}Total time: $($duration.TotalSeconds.ToString('F1')) seconds${Reset}"

# Optional: Start serial monitor
if ($Monitor) {
    Write-Host "`n${Yellow}Starting serial monitor...${Reset}"
    Write-Host "${Blue}Press Ctrl+C to exit monitor${Reset}"
    Start-Sleep -Seconds 2
    pio device monitor --port $Port --baud 115200
} else {
    Write-Host "`n${Blue}Tip: Use -Monitor flag to start serial monitor automatically${Reset}"
    Write-Host "${Blue}Manual monitor: pio device monitor --port $Port --baud 115200${Reset}"
}

Write-Host "`n${Green}ESP32 Color Matcher firmware deployment complete!${Reset}"
Write-Host "${Blue}Device should be available at: http://192.168.0.152${Reset}"
