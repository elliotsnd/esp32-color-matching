#!/usr/bin/env node

/**
 * ESP32 Color Matcher - Automated Deployment Script
 * 
 * This script automates the build → copy → upload workflow for the ESP32 web interface.
 * It eliminates the manual steps required to deploy React changes to the ESP32 device.
 */

import { execSync, spawn } from 'child_process';
import { copyFileSync, mkdirSync, existsSync, readdirSync, statSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = join(__dirname, '..');

// Configuration
const config = {
  distDir: join(projectRoot, 'dist'),
  dataDir: join(projectRoot, 'data'),
  buildCommand: 'npm run build',
  uploadCommand: 'pio run --target uploadfs',
  verbose: process.argv.includes('--verbose') || process.argv.includes('-v'),
  skipUpload: process.argv.includes('--skip-upload'),
  watchMode: process.argv.includes('--watch') || process.argv.includes('-w')
};

// Logging utilities
const log = {
  info: (msg) => console.log(`ℹ️  ${msg}`),
  success: (msg) => console.log(`✅ ${msg}`),
  error: (msg) => console.error(`❌ ${msg}`),
  warn: (msg) => console.warn(`⚠️  ${msg}`),
  verbose: (msg) => config.verbose && console.log(`🔍 ${msg}`)
};

/**
 * Execute a command and handle errors
 */
function executeCommand(command, description) {
  log.info(`${description}...`);
  log.verbose(`Executing: ${command}`);
  
  try {
    const output = execSync(command, { 
      cwd: projectRoot, 
      stdio: config.verbose ? 'inherit' : 'pipe',
      encoding: 'utf8'
    });
    log.success(`${description} completed`);
    return output;
  } catch (error) {
    log.error(`${description} failed: ${error.message}`);
    if (error.stdout) log.verbose(`stdout: ${error.stdout}`);
    if (error.stderr) log.verbose(`stderr: ${error.stderr}`);
    throw error;
  }
}

/**
 * Recursively copy directory contents
 */
function copyDirectory(src, dest) {
  log.verbose(`Copying ${src} → ${dest}`);
  
  if (!existsSync(dest)) {
    mkdirSync(dest, { recursive: true });
  }

  const items = readdirSync(src);
  let copiedFiles = 0;

  for (const item of items) {
    const srcPath = join(src, item);
    const destPath = join(dest, item);
    const stat = statSync(srcPath);

    if (stat.isDirectory()) {
      copiedFiles += copyDirectory(srcPath, destPath);
    } else {
      copyFileSync(srcPath, destPath);
      copiedFiles++;
      log.verbose(`Copied: ${item}`);
    }
  }

  return copiedFiles;
}

/**
 * Main deployment function
 */
async function deploy() {
  const startTime = Date.now();
  log.info('🚀 Starting ESP32 web interface deployment...');

  try {
    // Step 1: Build React application
    executeCommand(config.buildCommand, 'Building React application');

    // Step 2: Verify build output exists
    if (!existsSync(config.distDir)) {
      throw new Error(`Build output directory not found: ${config.distDir}`);
    }

    // Step 3: Copy files from dist to data directory
    log.info('Copying built files to ESP32 data directory...');
    const copiedFiles = copyDirectory(config.distDir, config.dataDir);
    log.success(`Copied ${copiedFiles} files to data directory`);

    // Step 4: Upload filesystem to ESP32 (unless skipped)
    if (config.skipUpload) {
      log.warn('Skipping ESP32 filesystem upload (--skip-upload flag)');
    } else {
      executeCommand(config.uploadCommand, 'Uploading filesystem to ESP32');
    }

    // Success summary
    const duration = ((Date.now() - startTime) / 1000).toFixed(1);
    log.success(`🎉 Deployment completed successfully in ${duration}s`);
    log.info('💡 Refresh your browser at http://192.168.0.152 to see changes');

  } catch (error) {
    log.error(`Deployment failed: ${error.message}`);
    process.exit(1);
  }
}

/**
 * Watch mode for development
 */
async function startWatchMode() {
  log.info('👀 Starting watch mode for development...');
  log.info('Press Ctrl+C to stop watching');

  // Use chokidar for better file watching if available, otherwise use basic fs.watch
  try {
    const chokidar = await import('chokidar');
    const watcher = chokidar.watch(['components/**/*', 'src/**/*', '*.tsx', '*.ts'], {
      ignored: /node_modules/,
      persistent: true
    });

    watcher.on('change', (path) => {
      log.info(`📝 File changed: ${path}`);
      deploy().catch(() => {}); // Don't exit on deployment errors in watch mode
    });

  } catch (e) {
    log.warn('chokidar not available, using basic file watching');
    // Fallback to basic watching - just watch the components directory
    const fs = await import('fs');
    fs.watch(join(projectRoot, 'components'), { recursive: true }, (eventType, filename) => {
      if (filename) {
        log.info(`📝 File changed: ${filename}`);
        deploy().catch(() => {});
      }
    });
  }
}

// Main execution
if (config.watchMode) {
  startWatchMode().catch(error => {
    log.error(`Watch mode failed: ${error.message}`);
    process.exit(1);
  });
} else {
  deploy();
}
