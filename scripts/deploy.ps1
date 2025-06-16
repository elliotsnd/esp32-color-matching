# ESP32 Color Matcher - PowerShell Deployment Script
# This script provides a PowerShell-based deployment solution

param(
    [switch]$SkipUpload,
    [switch]$Verbose,
    [switch]$Watch,
    [switch]$Help
)

# Show help if requested
if ($Help) {
    Write-Host @"
ESP32 Color Matcher - Deployment Script

USAGE:
    .\scripts\deploy.ps1 [OPTIONS]

OPTIONS:
    -SkipUpload    Skip uploading filesystem to ESP32 device
    -Verbose       Show detailed output during deployment
    -Watch         Start file watching mode for development
    -Help          Show this help message

EXAMPLES:
    .\scripts\deploy.ps1                    # Full deployment
    .\scripts\deploy.ps1 -SkipUpload        # Build and copy only
    .\scripts\deploy.ps1 -Watch             # Development mode with file watching
    .\scripts\deploy.ps1 -Verbose           # Detailed output

"@
    exit 0
}

# Function to write colored output
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Type = "Info"
    )
    
    switch ($Type) {
        "Success" { Write-Host "✅ $Message" -ForegroundColor Green }
        "Error"   { Write-Host "❌ $Message" -ForegroundColor Red }
        "Warning" { Write-Host "⚠️  $Message" -ForegroundColor Yellow }
        "Info"    { Write-Host "ℹ️  $Message" -ForegroundColor Cyan }
        "Verbose" { if ($Verbose) { Write-Host "🔍 $Message" -ForegroundColor Gray } }
        default   { Write-Host $Message }
    }
}

# Check prerequisites
function Test-Prerequisites {
    Write-ColorOutput "Checking prerequisites..." "Info"
    
    # Check Node.js
    try {
        $nodeVersion = node --version 2>$null
        Write-ColorOutput "Node.js version: $nodeVersion" "Verbose"
    } catch {
        Write-ColorOutput "Node.js not found. Please install Node.js first." "Error"
        exit 1
    }
    
    # Check PlatformIO CLI
    if (-not $SkipUpload) {
        try {
            $pioVersion = pio --version 2>$null
            Write-ColorOutput "PlatformIO version: $pioVersion" "Verbose"
        } catch {
            Write-ColorOutput "PlatformIO CLI not found. Install with: pip install platformio" "Error"
            exit 1
        }
    }
    
    Write-ColorOutput "Prerequisites check passed" "Success"
}

# Main deployment function
function Start-Deployment {
    $startTime = Get-Date
    Write-ColorOutput "🚀 Starting ESP32 web interface deployment..." "Info"
    
    try {
        # Change to project root
        $scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
        $projectRoot = Split-Path -Parent $scriptPath
        Set-Location $projectRoot
        
        # Build Node.js arguments
        $nodeArgs = @("scripts/deploy.js")
        if ($SkipUpload) { $nodeArgs += "--skip-upload" }
        if ($Verbose) { $nodeArgs += "--verbose" }
        if ($Watch) { $nodeArgs += "--watch" }
        
        # Execute deployment
        Write-ColorOutput "Executing: node $($nodeArgs -join ' ')" "Verbose"
        & node @nodeArgs
        
        if ($LASTEXITCODE -ne 0) {
            throw "Deployment script failed with exit code $LASTEXITCODE"
        }
        
        $duration = [math]::Round(((Get-Date) - $startTime).TotalSeconds, 1)
        Write-ColorOutput "🎉 Deployment completed successfully in ${duration}s" "Success"
        
        if (-not $Watch) {
            Write-ColorOutput "💡 Open http://192.168.0.152 in your browser to see changes" "Info"
        }
        
    } catch {
        Write-ColorOutput "Deployment failed: $($_.Exception.Message)" "Error"
        exit 1
    }
}

# Main execution
Write-Host ""
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "ESP32 Color Matcher - PowerShell Deploy" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
Write-Host ""

Test-Prerequisites
Start-Deployment
