#!/usr/bin/env bash
set -e

# ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
# ┃               Monster Engine — Setup Script               ┃
# ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
#
# Installs system dependencies and downloads the Filament SDK.
# Supports macOS (Homebrew) and Linux (apt).
#
# Usage:
#   ./scripts/setup.sh              # uses default Filament version
#   FILAMENT_VERSION=v1.69.2 ./scripts/setup.sh  # override version

FILAMENT_VERSION="${FILAMENT_VERSION:-v1.69.2}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FILAMENT_DIR="$PROJECT_ROOT/engine/third_party/filament"

# ─── Platform Detection ──────────────────────────────────────
OS="$(uname -s)"
ARCH="$(uname -m)"

echo "=== Monster Engine Setup ==="
echo "  Platform: $OS ($ARCH)"
echo "  Filament: $FILAMENT_VERSION"
echo ""

# ─── Install System Dependencies ─────────────────────────────

install_macos_deps() {
    echo ">>> Installing macOS dependencies via Homebrew..."

    if ! command -v brew &> /dev/null; then
        echo "ERROR: Homebrew not found. Install it from https://brew.sh"
        exit 1
    fi

    # cmake + ninja (build tools)
    brew install cmake ninja

    # Todas as outras dependências (GLFW, Bullet, etc.) são compiladas do source
    # O macOS já vem com Metal, Cocoa, IOKit, CoreVideo frameworks

    echo "macOS dependencies installed."
}

install_linux_deps() {
    echo ">>> Installing Linux dependencies via apt..."

    sudo apt update

    # Build tools
    sudo apt install -y \
        pkg-config \
        cmake \
        ninja-build

    # Clang 17 + libc++ (para C++23 sem bugs do libstdc++)
    sudo apt install -y \
        clang-17 \
        libc++-17-dev \
        libc++abi-17-dev

    # X11 core + extensions (necessário para GLFW)
    sudo apt install -y \
        libx11-dev \
        libxrandr-dev \
        libxinerama-dev \
        libxcursor-dev \
        libxi-dev \
        libxext-dev \
        libxkbcommon-dev

    # Wayland (necessário para GLFW Wayland backend)
    sudo apt install -y \
        libwayland-dev \
        wayland-protocols

    # OpenGL / Mesa (necessário para GLAD/ImGui)
    sudo apt install -y \
        mesa-common-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        freeglut3-dev \
        mesa-utils

    # Vulkan SDK (Filament backend no Linux)
    sudo apt install -y \
        libvulkan-dev \
        vulkan-tools \
        vulkan-validationlayers

    # zlib (necessário para Assimp)
    sudo apt install -y \
        zlib1g-dev

    echo "Linux dependencies installed."
}

case "$OS" in
    Darwin) install_macos_deps ;;
    Linux)  install_linux_deps ;;
    *)
        echo "ERROR: Unsupported platform: $OS"
        echo "This engine supports macOS and Linux."
        exit 1
        ;;
esac

# ─── Download Filament SDK ────────────────────────────────────

is_filament_ready() {
    [ -f "$FILAMENT_DIR/include/filament/Engine.h" ] || return 1

    if [ -f "$FILAMENT_DIR/lib/x86_64/libfilament.a" ] || [ -f "$FILAMENT_DIR/lib/arm64/libfilament.a" ]; then
        return 0
    fi

    return 1
}

download_filament() {
    # Determina o asset correto baseado na plataforma
    case "$OS" in
        Darwin) FILAMENT_ASSET="filament-${FILAMENT_VERSION}-mac.tgz" ;;
        Linux)  FILAMENT_ASSET="filament-${FILAMENT_VERSION}-linux.tgz" ;;
    esac

    FILAMENT_URL="https://github.com/google/filament/releases/download/${FILAMENT_VERSION}/${FILAMENT_ASSET}"

    echo ""
    echo ">>> Downloading Filament SDK ${FILAMENT_VERSION}..."
    echo "    URL: $FILAMENT_URL"

    # Cria diretório temporário para download
    TEMP_DIR=$(mktemp -d)
    trap "rm -rf $TEMP_DIR" EXIT

    # Download
    if command -v curl &> /dev/null; then
        curl -L --progress-bar -o "$TEMP_DIR/$FILAMENT_ASSET" "$FILAMENT_URL"
    elif command -v wget &> /dev/null; then
        wget --show-progress -O "$TEMP_DIR/$FILAMENT_ASSET" "$FILAMENT_URL"
    else
        echo "ERROR: Neither curl nor wget found. Install one of them."
        exit 1
    fi

    # Remove diretório existente (se houver)
    if [ -d "$FILAMENT_DIR" ]; then
        echo "    Removing existing Filament SDK..."
        rm -rf "$FILAMENT_DIR"
    fi

    # Extrai o SDK
    echo "    Extracting to $FILAMENT_DIR..."
    mkdir -p "$FILAMENT_DIR"
    tar -xzf "$TEMP_DIR/$FILAMENT_ASSET" -C "$FILAMENT_DIR" --strip-components=1

    echo "    Filament SDK ${FILAMENT_VERSION} installed successfully."
}

# Verifica se o Filament já está instalado
if is_filament_ready; then
    echo ""
    echo ">>> Filament SDK already present at $FILAMENT_DIR"
    read -p "    Re-download? [y/N] " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        download_filament
    else
        echo "    Keeping existing Filament SDK."
    fi
else
    download_filament
fi

# ─── Verify Installation ──────────────────────────────────────

echo ""
echo "=== Setup Complete ==="
echo ""

# Verifica se o Filament tem as libs para a arquitetura correta
if [ "$OS" = "Darwin" ]; then
    if [ "$ARCH" = "arm64" ] && [ -d "$FILAMENT_DIR/lib/arm64" ]; then
        echo "  ✓ Filament SDK (arm64)"
    elif [ "$ARCH" = "x86_64" ] && [ -d "$FILAMENT_DIR/lib/x86_64" ]; then
        echo "  ✓ Filament SDK (x86_64)"
    else
        echo "  ⚠ Filament SDK present but architecture mismatch (expected $ARCH)"
    fi
else
    if [ -d "$FILAMENT_DIR/lib/x86_64" ]; then
        echo "  ✓ Filament SDK (x86_64)"
    elif [ -d "$FILAMENT_DIR/lib/arm64" ]; then
        echo "  ✓ Filament SDK (arm64)"
    else
        echo "  ⚠ Filament SDK: no library directory found"
    fi
fi

echo ""
echo "To build and run:"
echo "  ./scripts/run.sh"
echo ""
