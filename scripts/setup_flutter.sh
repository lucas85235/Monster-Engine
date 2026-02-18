#!/usr/bin/env bash
set -e

# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃   Monster Engine — Flutter Engine Setup (from local SDK)  ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#
#  Uses the Flutter SDK installed on the host machine to copy
#  the engine framework and ICU data into engine/third_party/flutter_engine/.
#
#  Prerequisites: Flutter SDK installed (brew install --cask flutter)
#
#  Usage:  ./scripts/setup_flutter.sh

DEST_DIR="engine/third_party/flutter_engine"

echo ""
echo "=== Flutter Engine Setup (from local SDK) ==="
echo ""

# ── Locate Flutter SDK ──────────────────────────────────────
FLUTTER_BIN=$(which flutter 2>/dev/null || true)
if [ -z "$FLUTTER_BIN" ]; then
    echo "ERROR: 'flutter' not found in PATH."
    echo "  Install it:  brew install --cask flutter"
    exit 1
fi

# Resolve symlinks (Homebrew uses symlinks)
FLUTTER_BIN_REAL=$(readlink -f "$FLUTTER_BIN" 2>/dev/null || python3 -c "import os; print(os.path.realpath('$FLUTTER_BIN'))")
FLUTTER_ROOT=$(dirname "$(dirname "$FLUTTER_BIN_REAL")")

echo "  Flutter SDK : $FLUTTER_ROOT"

# Verify this is actually a Flutter SDK
if [ ! -f "$FLUTTER_ROOT/bin/internal/engine.version" ]; then
    echo "ERROR: Could not find engine.version at $FLUTTER_ROOT/bin/internal/"
    echo "  Ensure Flutter SDK is properly installed."
    exit 1
fi

ENGINE_VERSION=$(cat "$FLUTTER_ROOT/bin/internal/engine.version")
echo "  Engine hash : $ENGINE_VERSION"

# ── Detect platform ────────────────────────────────────────
OS=$(uname -s)
ARCH=$(uname -m)

case "$OS" in
    Darwin)
        # macOS uses FlutterMacOS.xcframework (universal arm64+x86_64)
        ENGINE_CACHE="$FLUTTER_ROOT/bin/cache/artifacts/engine/darwin-x64"
        FRAMEWORK_DIR="$ENGINE_CACHE/FlutterMacOS.xcframework/macos-arm64_x86_64"
        FRAMEWORK_NAME="FlutterMacOS.framework"
        ;;
    Linux)
        ENGINE_CACHE="$FLUTTER_ROOT/bin/cache/artifacts/engine/linux-x64"
        FRAMEWORK_DIR=""  # Linux uses libflutter_engine.so directly
        FRAMEWORK_NAME=""
        ;;
    *)
        echo "ERROR: Unsupported platform: $OS"
        exit 1
        ;;
esac

echo "  Platform    : $OS ($ARCH)"
echo "  Engine cache: $ENGINE_CACHE"

# ── Ensure engine artifacts are cached ─────────────────────
if [ ! -d "$ENGINE_CACHE" ]; then
    echo ""
    echo ">>> Engine artifacts not cached. Running 'flutter precache --macos'..."
    flutter precache --macos
fi

# ── Verify framework exists ────────────────────────────────
if [ "$OS" = "Darwin" ]; then
    if [ ! -d "$FRAMEWORK_DIR/$FRAMEWORK_NAME" ]; then
        echo "ERROR: Framework not found at: $FRAMEWORK_DIR/$FRAMEWORK_NAME"
        echo "  Try running: flutter precache --macos"
        exit 1
    fi
fi

# ── Copy to third_party ────────────────────────────────────
echo ""
echo ">>> Copying engine artifacts to $DEST_DIR..."
rm -rf "$DEST_DIR"
mkdir -p "$DEST_DIR"

if [ "$OS" = "Darwin" ]; then
    # Copy the framework
    cp -R "$FRAMEWORK_DIR/$FRAMEWORK_NAME" "$DEST_DIR/"

    # Copy ICU data
    if [ -f "$ENGINE_CACHE/icudtl.dat" ]; then
        cp "$ENGINE_CACHE/icudtl.dat" "$DEST_DIR/"
    fi

    echo "  ✓ $FRAMEWORK_NAME copied"
    echo "  ✓ icudtl.dat copied"
elif [ "$OS" = "Linux" ]; then
    # Linux: copy .so and headers
    cp "$ENGINE_CACHE/libflutter_engine.so" "$DEST_DIR/" 2>/dev/null || true
    cp "$ENGINE_CACHE/flutter_embedder.h" "$DEST_DIR/" 2>/dev/null || true
    cp "$ENGINE_CACHE/icudtl.dat" "$DEST_DIR/" 2>/dev/null || true
fi

# ── Write version stamp ───────────────────────────────────
echo "$ENGINE_VERSION" > "$DEST_DIR/engine.version"

echo ""
echo "=== Flutter Engine Setup Complete ==="
echo "  Destination : $DEST_DIR"
echo "  Version     : $ENGINE_VERSION"
du -sh "$DEST_DIR" | awk '{print "  Size        : " $1}'
echo ""
ls -1 "$DEST_DIR/"
echo ""
