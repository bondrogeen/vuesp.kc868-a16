# Vuesp Development Guide

Complete guide for developing and contributing to the Vuesp project.

## Prerequisites

- Node.js >= 20 and npm >= 10
- Python 3.8+
- PlatformIO Core
- Git

## Project Structure

```
vuesp/
├── src/              # C++ firmware source
│   ├── const/        # Constants and struct definitions
│   ├── include/      # Header files
│   └── *.cpp         # Implementation files
├── vue/              # Vue 3 web frontend
│   ├── src/
│   │   ├── pages/    # Page components
│   │   ├── components/
│   │   ├── stores/   # Pinia state management
│   │   ├── composables/
│   │   └── assets/
│   └── public/       # Static assets (default.json with struct)
├── data/
│   └── www/          # Built web files (LittleFS)
├── firmware/         # Compiled firmware binaries
├── platformio.ini    # PlatformIO configuration
└── vite.config.ts    # Vite build configuration
```

## Frontend Development

### Setup

```bash
cd vue
npm install
```

### Environment Configuration

Create `.env` file:

```bash
cp .env.example .env
```

Edit `.env` to set your device IP:

```env
VITE_IP=192.168.1.100
```

### Development Server

```bash
npm run dev
```

Access at http://localhost:5173/

### Code Quality

**TypeScript checking**:
```bash
npm run typecheck
```

**ESLint**:
```bash
npm run lint        # Show violations
npm run lint:fix    # Fix automatically
```

**Prettier formatting**:
```bash
npm run format
```

**Run all checks**:
```bash
npm run check
```

### Building

**Web only**:
```bash
npm run build-web
```

**Full build with firmware**:
```bash
npm run build
```

### Screenshots

Screenshots for the README live in `github/images/screen`. Regenerate them
without a device:

```bash
npm run screen:install   # once: downloads the Playwright Chromium build
npm run screen           # all screens, dark theme, English UI
```

The command starts a mock device (binary WebSocket protocol + `/fs` endpoints),
starts Vite in `screen` mode and captures the main pages:

```bash
npm run screen -- dashboard gpio         # only selected screens
npm run screen -- --theme=light          # light theme
npm run screen -- --locale=ru            # Russian UI
npm run screen -- --width=1440 --height=900 --scale=2
npm run screen -- --out=/tmp/screens     # custom output directory
```

To capture the real device UI instead of the mock:

```bash
npm run screen -- --base-url=http://192.168.1.100
```

The mock data is generated from `public/default.json`, so run
`npm run struct-and-json` first if the firmware struct changed.

The same tool can capture screenshots for a device project. Run it from the
device's `vue/` directory and point `VUESP_SCREEN_ROOT` at that directory:

```bash
cd ../vuesp-devices/vuesp.cam/vue
VUESP_SCREEN_ROOT="$PWD" \
  node ../../../vuesp/vue/scripts/screen/capture.mjs --out=../github/images/screen
```

The mock reads the firmware version from the project's `platformio.ini`, so the
captured status panel matches the device version. For BMS projects it also
fills realistic voltage, cell, temperature and capacity values.

The mock can also be started standalone for UI work:

```bash
npm run screen:mock   # only the mock device
npm run dev:mock      # mock device + Vite dev server, no hardware needed
```

### Public demo

The site demo is a static build of this UI with a browser-side device mock:

```bash
npm run demo
```

The command regenerates `public/default.json` from `src/const/struct.h`, builds
with `--mode demo`, and copies the result to
`../vuesp.ru/public/demo-app`. It reads the dashboard, scripts and firmware
version from the current repository, so rebuild the demo after changing the
firmware struct or dashboard.

## Backend Development

### PlatformIO Setup

```bash
pio project init --ide vscode
pio run -e esp32 -t build
```

### Supported Platforms

- `esp32` - ESP32
- `esp32-c3-devkitm-1` - ESP32-C3-DevKitM-1
- `esp8266` - ESP8266
- `pico32` - Raspberry Pi Pico W (RP2040)

### Building Firmware

```bash
# Build for specific platform
pio run -e esp32

# Build and upload to device
pio run -e esp32 -t upload

# Clean build
pio run -e esp32 -t clean
```

### Debugging

**Serial monitor**:
```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

## WebSocket Protocol

Binary protocol over WebSocket at `/esp`:

- First byte: command/task ID
- Remaining bytes: struct data (defined in `src/const/struct.h`)

### Message Flow

1. Client connects to `ws://device:80/esp`
2. Server sends device info (task ID = 1)
3. Client sends commands (struct-encoded binary)
4. Server responds with updated state

### Authentication

Optional HTTP Basic Auth:
- Control via `settings.authMode` in device
- Required for `/update`, `/fs`, `/recovery` endpoints if enabled

## Contributing

### Code Style

**Frontend**:
- Use TypeScript with strict mode
- Vue 3 Composition API
- Tailwind CSS for styling
- Prettier for formatting
- ESLint for linting

**Backend**:
- Use `strncpy()` instead of `strcpy()`
- Validate all inputs
- Use error codes in return values
- Avoid blocking I/O in main loop

### Before Submitting PR

1. Run `npm run check` in frontend
2. Verify TypeScript types: `npm run typecheck`
3. Test on actual device
4. Update documentation if needed

### Commit Message Convention

```
type(scope): description

[optional body]
[optional footer]
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Example:
```
feat(webserver): add authentication to update endpoint

- Implement isAuthenticated() helper
- Apply to /update, /fs, /recovery endpoints
- Respect settings.authMode flag
```

## Testing

### Frontend

```bash
# TypeScript check (no build)
npm run typecheck

# Build test
npm run build-web
```

### Backend

```bash
# Compile for all platforms
for platform in esp32 esp32-c3-devkitm-1 esp8266 pico32; do
  pio run -e $platform -t build
done
```

## CI/CD Pipeline

GitHub Actions automatically:
- Type checks and lints frontend on every push
- Builds firmware for all platforms
- Validates documentation files

See `.github/workflows/ci.yml` for configuration.

## Performance Tips

**Frontend**:
- Use computed properties instead of watchers when possible
- Lazy load pages with Vue Router
- Minimize WebSocket message frequency
- Use `@vueuse/core` composables for common patterns

**Backend**:
- Batch EEPROM commits to reduce writes
- Use async HTTP server for non-blocking I/O
- Limit WebSocket message sizes
- Implement proper task scheduling

## Troubleshooting

### WebSocket Won't Connect

1. Check `VITE_IP` is set to correct device IP
2. Verify device is on network: `ping 192.168.1.100`
3. Check browser console for specific error
4. Ensure device firmware is running and WiFi enabled

### Build Failures

```bash
# Clear build artifacts
rm -rf build data/www

# Reinstall dependencies
cd vue && rm -rf node_modules && npm install

# Verify PlatformIO
pio --version
pio system info
```

### TypeScript Errors

```bash
npm run typecheck
```

Check `tsconfig.app.json` for compiler options.

## Resources

- [Vue 3 Documentation](https://vuejs.org/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [Vite Documentation](https://vitejs.dev/)

## Getting Help

- Check existing GitHub issues
- Review code comments and documentation
- Check device serial output for firmware errors
- Run TypeScript checker for frontend issues
