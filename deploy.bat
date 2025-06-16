@echo off
echo 馃搮 ESP32 Color Matcher Deployment Script
echo ========================================

REM Check if PlatformIO CLI is available
where platformio >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo 鉂? Error: PlatformIO CLI not found
    echo Please install PlatformIO Core CLI first:
    echo https://docs.platformio.org/en/latest/core/installation.html
    exit /b 1
)

echo 馃搫 Building firmware...
platformio run
if %ERRORLEVEL% neq 0 (
    echo 鉂? Build failed! Please check the errors above.
    exit /b 1
)

echo.
echo 馃搧 Building filesystem...
platformio run --target buildfs
if %ERRORLEVEL% neq 0 (
    echo 鉂? Filesystem build failed! Please check the errors above.
    exit /b 1
)

echo.
echo 馃搶 Uploading firmware...
platformio run --target upload
if %ERRORLEVEL% neq 0 (
    echo 鉂? Firmware upload failed! Please check:
    echo 1. ESP32 is connected and powered on
    echo 2. Correct COM port is selected
    echo 3. No other program is using the COM port
    exit /b 1
)

echo.
echo 馃搨 Uploading filesystem...
platformio run --target uploadfs
if %ERRORLEVEL% neq 0 (
    echo 鉂? Filesystem upload failed!
    exit /b 1
)

echo.
echo 鉁? Deployment completed successfully!
echo.
echo Next steps:
echo 1. Check the serial monitor for boot messages
echo 2. Verify the web interface is accessible
echo 3. Run calibration tests
echo.

REM Ask if user wants to open serial monitor
set /p MONITOR="Would you like to open the serial monitor? (Y/N) "
if /i "%MONITOR%"=="Y" (
    echo Opening serial monitor...
    platformio device monitor
)
