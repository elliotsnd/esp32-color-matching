# ESP32 Color Matcher - Makefile
# Cross-platform build and deployment automation

.PHONY: help deploy deploy-skip-upload deploy-watch build upload monitor clean install

# Default target
help:
	@echo "ESP32 Color Matcher - Available Commands:"
	@echo ""
	@echo "  make deploy              - Full deployment (build + copy + upload)"
	@echo "  make deploy-skip-upload  - Build and copy only (skip ESP32 upload)"
	@echo "  make deploy-watch        - Development mode with file watching"
	@echo "  make build               - Build React application only"
	@echo "  make upload              - Upload filesystem to ESP32 only"
	@echo "  make monitor             - Monitor ESP32 serial output"
	@echo "  make install             - Install dependencies"
	@echo "  make clean               - Clean build artifacts"
	@echo ""
	@echo "For more options, see: npm run deploy -- --help"

# Full deployment
deploy:
	npm run deploy

# Build and copy only (skip upload)
deploy-skip-upload:
	npm run deploy:skip-upload

# Development mode with file watching
deploy-watch:
	npm run deploy:watch

# Build React application only
build:
	npm run build

# Upload filesystem to ESP32 only
upload:
	npm run esp32:upload

# Monitor ESP32 serial output
monitor:
	npm run esp32:monitor

# Install dependencies
install:
	npm install

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@if exist dist rmdir /s /q dist 2>nul || rm -rf dist 2>/dev/null || true
	@if exist data\assets rmdir /s /q data\assets 2>nul || rm -rf data/assets 2>/dev/null || true
	@if exist data\index.html del data\index.html 2>nul || rm -f data/index.html 2>/dev/null || true
	@echo "Clean completed"
