# Flutter Integration Test

Test application that verifies the Flutter Embedder API integration with Monster Engine.

## Prerequisites

1. **Flutter engine library** — download via:
   ```bash
   ./scripts/setup_flutter.sh
   ```

2. **Flutter SDK** — needed to build the Dart test app:
   ```bash
   # Install Flutter: https://docs.flutter.dev/get-started/install
   flutter --version  # verify installation
   ```

## Build the Dart App

```bash
cd apps/flutter_test/flutter_app
flutter pub get
flutter build bundle  # produces build/flutter_assets/
```

## Build the C++ Test App

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSE_ENABLE_FLUTTER=ON \
  -DSE_BUILD_APP_FLUTTER_TEST=ON

cmake --build build --target flutter_test
```

## Run

```bash
./build/apps/flutter_test/flutter_test
```

## What It Does

- Launches a Monster Engine window with Flutter overlay
- The Dart UI shows a **test dashboard** with:
  - Connection status (ping/pong)
  - Engine info (name, renderer)
  - Tick counter (C++ → Dart messages every 2s)
  - FPS readout
  - Activity log
- Platform channel `monster/test` is registered for bidirectional messaging
- `FlutterTestLayer` sends periodic tick messages from C++ to Dart
- Dart UI can ping the engine and receive engine info

## Expected Output (Logs)

```
[INFO] FlutterOverlayRenderer initialized (Filament overlay, layer 5)
[INFO] FlutterEmbedder initialized (software rendering, Dart project: '...')
[INFO] FlutterTestLayer: Registering 'monster/test' platform channel
[INFO] FlutterTestLayer attached — Flutter integration active
[INFO] === Flutter Integration Test ===
[INFO]   Flutter embedder: ACTIVE
```
