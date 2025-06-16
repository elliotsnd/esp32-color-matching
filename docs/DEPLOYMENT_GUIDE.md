# ESP32 Color Matcher - Deployment Guide

## Quick Start

### 🚀 One-Command Deployment
```bash
npm run deploy
```
This single command will:
1. Build the React application
2. Copy files to the ESP32 data directory
3. Upload the filesystem to your ESP32 device
4. Your changes will be live at `http://192.168.0.152`

## Available Commands

### NPM Scripts (Recommended)
```bash
# Full deployment (build → copy → upload)
npm run deploy

# Build and copy only (skip ESP32 upload)
npm run deploy:skip-upload

# Development mode with file watching
npm run deploy:watch

# Verbose output for debugging
npm run deploy:verbose

# ESP32-specific commands
npm run esp32:upload    # Upload filesystem only
npm run esp32:monitor   # Monitor ESP32 serial output
npm run esp32:build     # Build ESP32 firmware
```

### Platform-Specific Scripts

#### Windows Batch File
```cmd
# Quick deployment
scripts\deploy.bat

# With options
scripts\deploy.bat --skip-upload --verbose
```

#### PowerShell (Windows)
```powershell
# Quick deployment
.\scripts\deploy.ps1

# With options
.\scripts\deploy.ps1 -SkipUpload -Verbose -Watch

# Show help
.\scripts\deploy.ps1 -Help
```

#### Direct Node.js
```bash
# Full deployment
node scripts/deploy.js

# With options
node scripts/deploy.js --skip-upload --verbose --watch
```

## Development Workflow

### Making Changes to the React Interface

1. **Edit React Components**: Make changes to files in `components/`, `src/`, or root `.tsx` files
2. **Deploy Changes**: Run `npm run deploy`
3. **View Results**: Open `http://192.168.0.152` in your browser

### Development Mode (Recommended for Active Development)

```bash
npm run deploy:watch
```

This will:
- Watch for file changes in your React components
- Automatically rebuild and deploy when files change
- Keep running until you press Ctrl+C

### Testing Without ESP32 Upload

If you want to test the build process without uploading to the ESP32:

```bash
npm run deploy:skip-upload
```

This is useful for:
- Testing build configurations
- Preparing files for manual upload
- CI/CD pipelines

## Troubleshooting

### Common Issues

#### "PlatformIO CLI not found"
**Solution**: Install PlatformIO CLI
```bash
pip install platformio
```

#### "Node.js not found"
**Solution**: Install Node.js from [nodejs.org](https://nodejs.org)

#### "ESP32 not responding"
**Solutions**:
1. Check ESP32 is connected via USB
2. Verify correct COM port in `platformio.ini`
3. Try resetting the ESP32 device
4. Check if another program is using the serial port

#### "Build fails"
**Solutions**:
1. Run `npm install` to ensure dependencies are installed
2. Check for TypeScript errors: `npm run build`
3. Use verbose mode: `npm run deploy:verbose`

#### "Files not updating on ESP32"
**Solutions**:
1. Clear browser cache (Ctrl+F5)
2. Verify upload completed successfully
3. Check ESP32 serial monitor for errors: `npm run esp32:monitor`

### Debug Mode

For detailed troubleshooting, use verbose mode:
```bash
npm run deploy:verbose
```

This shows:
- Detailed command execution
- File copy operations
- Build output
- Upload progress

## File Structure

```
project/
├── components/          # React components (your edits here)
├── src/                # ESP32 firmware source
├── dist/               # Vite build output (auto-generated)
├── data/               # ESP32 filesystem staging (auto-updated)
├── scripts/            # Deployment automation
│   ├── deploy.js       # Main deployment script
│   ├── deploy.bat      # Windows batch script
│   └── deploy.ps1      # PowerShell script
└── docs/               # Documentation
```

## How It Works

1. **React Build**: Vite builds your React app to `dist/`
2. **File Copy**: Script copies `dist/*` to `data/` directory
3. **ESP32 Upload**: PlatformIO uploads `data/` to ESP32 LittleFS filesystem
4. **ESP32 Serving**: ESP32 serves files from its internal filesystem

## Advanced Usage

### Custom Build Configuration

Edit `vite.config.ts` to customize the build process:
- Add environment variables
- Configure build optimizations
- Set up proxy for development

### Custom Deployment

You can customize the deployment script (`scripts/deploy.js`) to:
- Add pre/post-deployment hooks
- Integrate with CI/CD systems
- Add custom file processing
- Support multiple ESP32 devices

### Integration with IDEs

#### VS Code
Add to `.vscode/tasks.json`:
```json
{
  "label": "Deploy to ESP32",
  "type": "shell",
  "command": "npm run deploy",
  "group": "build",
  "presentation": {
    "echo": true,
    "reveal": "always",
    "focus": false,
    "panel": "shared"
  }
}
```

## Best Practices

1. **Use Watch Mode**: During active development, use `npm run deploy:watch`
2. **Test Locally First**: Use `npm run build` to check for errors before deploying
3. **Version Control**: Commit changes before deploying to track what was deployed
4. **Monitor Serial Output**: Use `npm run esp32:monitor` to watch for runtime errors
5. **Clear Browser Cache**: Use Ctrl+F5 to ensure you see the latest changes
