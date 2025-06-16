@echo off
REM ESP32 Color Matcher - Windows Deployment Script
REM This batch file provides a simple way to deploy changes on Windows

echo.
echo ========================================
echo ESP32 Color Matcher - Quick Deploy
echo ========================================
echo.

REM Check if Node.js is available
node --version >nul 2>&1
if errorlevel 1 (
    echo ❌ Node.js not found. Please install Node.js first.
    pause
    exit /b 1
)

REM Check if PlatformIO CLI is available
pio --version >nul 2>&1
if errorlevel 1 (
    echo ❌ PlatformIO CLI not found. Please install PlatformIO first.
    echo    You can install it with: pip install platformio
    pause
    exit /b 1
)

REM Change to project root directory
cd /d "%~dp0\.."

REM Parse command line arguments
set SKIP_UPLOAD=0
set VERBOSE=0
set WATCH_MODE=0

:parse_args
if "%1"=="--skip-upload" set SKIP_UPLOAD=1
if "%1"=="-s" set SKIP_UPLOAD=1
if "%1"=="--verbose" set VERBOSE=1
if "%1"=="-v" set VERBOSE=1
if "%1"=="--watch" set WATCH_MODE=1
if "%1"=="-w" set WATCH_MODE=1
shift
if not "%1"=="" goto parse_args

REM Build command arguments
set NODE_ARGS=
if %SKIP_UPLOAD%==1 set NODE_ARGS=%NODE_ARGS% --skip-upload
if %VERBOSE%==1 set NODE_ARGS=%NODE_ARGS% --verbose
if %WATCH_MODE%==1 set NODE_ARGS=%NODE_ARGS% --watch

REM Execute the deployment script
echo 🚀 Running deployment script...
node scripts/deploy.js %NODE_ARGS%

if errorlevel 1 (
    echo.
    echo ❌ Deployment failed!
    pause
    exit /b 1
)

echo.
echo ✅ Deployment completed successfully!
echo 💡 Open http://192.168.0.152 in your browser to see changes
echo.

if %WATCH_MODE%==0 pause
