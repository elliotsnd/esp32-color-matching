# Google Apps Script Deployment Guide

## 🚨 Current Issue
The Google Apps Script is returning a JSON parsing error:
```
SyntaxError: Unexpected non-whitespace character after JSON at position 5106
```

This prevents the ESP32 from getting proper color names from the Dulux database.

## 🔧 Solution: Deploy Fixed Script

### Step 1: Access Google Apps Script
1. Go to [script.google.com](https://script.google.com)
2. Sign in with your Google account
3. Either:
   - **Option A**: Open the existing project (if you have access)
   - **Option B**: Create a new project

### Step 2: Replace the Code
1. Delete all existing code in the editor
2. Copy the entire contents of `google_apps_script_fixed.js`
3. Paste it into the Google Apps Script editor
4. Save the project (Ctrl+S or File > Save)

### Step 3: Deploy as Web App
1. Click **Deploy** > **New deployment**
2. Click the gear icon ⚙️ next to "Type"
3. Select **Web app**
4. Configure deployment:
   - **Description**: "ESP32 Color Matcher - Fixed JSON parsing"
   - **Execute as**: Me (your email)
   - **Who has access**: Anyone
5. Click **Deploy**
6. **IMPORTANT**: Copy the Web app URL that appears

### Step 4: Test the New Deployment
Test the new URL in your browser:
```
https://script.google.com/macros/s/YOUR_NEW_SCRIPT_ID/exec?r=255&g=255&b=255
```

You should get a JSON response like:
```json
{
  "success": true,
  "match": {
    "name": "Vivid White",
    "code": "W01A1",
    "lrv": 90.00,
    "r": 255,
    "g": 255,
    "b": 255,
    "distance": "0.00"
  }
}
```

### Step 5: Update ESP32 Configuration
If the URL changed, update the ESP32 config:

1. Open `src/config.h`
2. Update line 34:
```cpp
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/YOUR_NEW_SCRIPT_ID/exec"
```
3. Rebuild and upload the ESP32 firmware

### Step 6: Verify the Fix
Run the test script to verify everything works:
```powershell
powershell -ExecutionPolicy Bypass -File "simple_test.ps1"
```

## 🎯 Expected Results After Fix

### Before (Broken):
- Samples saved as "Unknown"
- Google Apps Script returns HTML error page
- Color matching completely broken

### After (Fixed):
- Samples get proper Dulux paint names
- Google Apps Script returns proper JSON
- Complete color matching workflow works

## 🔍 Troubleshooting

### If you get permission errors:
1. Make sure "Who has access" is set to "Anyone"
2. Try re-deploying with a new deployment

### If you get "Script not found":
1. Make sure the URL is correct
2. Check that the deployment is active

### If colors still show as "Unknown":
1. Verify the Google Drive file ID is correct: `1o3u8emfZMNvdDdfC7IquTabZ_t47ff6L`
2. Make sure the dulux.json file is accessible
3. Check the Google Apps Script logs for errors

## 📋 Current URLs to Replace

**Current broken URL in config.h:**
```
https://script.google.com/macros/s/AKfycbzeRoYmT_W3pNI2G2H0YJ0aHlHyBZEIH8WnDem5x4-nVzLRXU3B8D3wTHN2X0COAakA/exec
```

**Replace with your new deployment URL after following the steps above.**
