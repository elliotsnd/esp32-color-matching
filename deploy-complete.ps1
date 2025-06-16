#!/usr/bin/env pwsh
# ESP32 Color Matcher Complete Deployment Script
# Automates the build and upload process for both firmware and web interface

param(
    [switch]$FirmwareOnly,
    [switch]$WebOnly,
    [switch]$SkipBuild,
    [switch]$Monitor,
    [switch]$Verbose,
    [string]$Port = "COM6"
)

# Colors for output
$Red = "`e[31m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Cyan = "`e[36m"
$Magenta = "`e[35m"
$Reset = "`e[0m"

function Write-Status {
    param($Message, $Color = $Blue)
    Write-Host "${Color}[DEPLOY]${Reset} $Message"
}

function Write-Success {
    param($Message)
    Write-Host "${Green}[SUCCESS]${Reset} $Message"
}

function Write-Error {
    param($Message)
    Write-Host "${Red}[ERROR]${Reset} $Message"
}

function Write-Warning {
    param($Message)
    Write-Host "${Yellow}[WARNING]${Reset} $Message"
}

function Write-Info {
    param($Message)
    Write-Host "${Cyan}[INFO]${Reset} $Message"
}

function Write-Header {
    param($Message)
    Write-Host "`n${Magenta}=== $Message ===${Reset}" -ForegroundColor Magenta
}

# Check if we're in the right directory
if (-not (Test-Path "platformio.ini")) {
    Write-Error "platformio.ini not found. Please run this script from the project root directory."
    exit 1
}

Write-Header "ESP32 Color Matcher Complete Deployment"
Write-Status "Deployment Options: FirmwareOnly=$FirmwareOnly WebOnly=$WebOnly SkipBuild=$SkipBuild Monitor=$Monitor"
Write-Status "Target Port: $Port"

$startTime = Get-Date

# Check dependencies
Write-Header "Checking Dependencies"

# Check PlatformIO
if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    Write-Error "PlatformIO CLI not found. Please install PlatformIO Core."
    exit 1
}
Write-Success "PlatformIO CLI found"

# Check Node.js for React build (unless firmware only)
if (-not $FirmwareOnly) {
    if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
        Write-Error "npm not found. Please install Node.js for React build."
        exit 1
    }
    Write-Success "Node.js/npm found"
}

# Step 1: Build and deploy web interface (unless FirmwareOnly)
if (-not $FirmwareOnly) {
    Write-Header "Building React Web Interface"
    
    if (-not $SkipBuild) {
        try {
            Write-Status "Running npm run build..."
            $buildResult = npm run build 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Error "React build failed"
                Write-Host $buildResult
                exit 1
            }
            Write-Success "React build completed"
        }
        catch {
            Write-Error "Failed to build React interface: $_"
            exit 1
        }
    } else {
        Write-Warning "Skipping React build (using existing dist/)"
    }
    
    # Copy built files to ESP32 data directory
    Write-Status "Copying built files to ESP32 data directory..."
    try {
        if (Test-Path "data") {
            Remove-Item "data\*" -Recurse -Force -ErrorAction SilentlyContinue
        } else {
            New-Item -ItemType Directory -Path "data" -Force | Out-Null
        }
        
        if (Test-Path "dist") {
            Copy-Item -Path "dist\*" -Destination "data\" -Recurse -Force
            Write-Success "Files copied to data directory"
        } else {
            Write-Warning "dist directory not found - skipping file copy"
        }
    }
    catch {
        Write-Error "Failed to copy files: $_"
        exit 1
    }
    
    # Upload filesystem to ESP32
    if (-not $WebOnly) {
        Write-Status "Uploading filesystem to ESP32..."
        try {
            $uploadResult = pio run --target uploadfs --upload-port $Port 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Filesystem upload had warnings but may have succeeded"
                Write-Host $uploadResult
            } else {
                Write-Success "Filesystem uploaded successfully"
            }
        }
        catch {
            Write-Error "Failed to upload filesystem: $_"
            exit 1
        }
    }
}

# Step 2: Build and upload firmware (unless WebOnly)
if (-not $WebOnly) {
    Write-Header "Building ESP32 Firmware"
    
    try {
        Write-Status "Compiling firmware..."
        $compileResult = pio run 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Firmware compilation failed"
            Write-Host $compileResult
            exit 1
        }
        Write-Success "Firmware compiled successfully"
    }
    catch {
        Write-Error "Failed to compile firmware: $_"
        exit 1
    }
    
    Write-Status "Uploading firmware to ESP32..."
    try {
        $uploadResult = pio run --target upload --upload-port $Port 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Firmware upload failed"
            Write-Host $uploadResult
            exit 1
        }
        Write-Success "Firmware uploaded successfully"
    }
    catch {
        Write-Error "Failed to upload firmware: $_"
        exit 1
    }
}

$endTime = Get-Date
$duration = $endTime - $startTime

Write-Header "Deployment Summary"
Write-Success "Deployment completed successfully!"
Write-Status "Total time: $($duration.TotalSeconds.ToString('F1')) seconds"

# Optional: Start serial monitor
if ($Monitor) {
    Write-Status "Starting serial monitor..."
    Write-Info "Press Ctrl+C to exit monitor"
    Start-Sleep -Seconds 2
    pio device monitor --port $Port
}

# Optional: Open web interface
if (-not $FirmwareOnly) {
    Write-Status "Waiting for ESP32 to boot..."
    Start-Sleep -Seconds 5
    
    $deviceUrl = "http://192.168.0.152"
    Write-Status "ESP32 Color Matcher should be available at: $deviceUrl"
    Write-Info "Opening web interface..."
    try {
        Start-Process $deviceUrl
    }
    catch {
        Write-Warning "Could not open browser automatically. Please visit: $deviceUrl"
    }
}

Write-Header "Deployment Complete"
Write-Success "ESP32 Color Matcher is ready for operation!"
