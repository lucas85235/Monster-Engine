#!/usr/bin/env python3
"""
Monster Engine — Cross-platform setup script.

Downloads the Filament SDK and installs system-level dependencies.
Replaces both ``setup.bat`` (Windows) and ``setup.sh`` (Linux/macOS).

Usage:
    python scripts/setup.py
    FILAMENT_VERSION=v1.69.2 python scripts/setup.py
"""

import os
import sys
import shutil
import tarfile
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import script_utils as u

FILAMENT_VERSION = os.environ.get("FILAMENT_VERSION", "v1.69.2")


def _filament_dir() -> Path:
    return u.get_project_root() / "engine" / "third_party" / "filament"


# ── System Dependencies ──────────────────────────────────────────

def _install_macos_deps() -> None:
    print(">>> Installing macOS dependencies via Homebrew...")
    u.require_tool("brew", "Install Homebrew from https://brew.sh")
    u.run_cmd(["brew", "install", "cmake", "ninja"])
    u.success("macOS dependencies installed.")


def _install_linux_deps() -> None:
    print(">>> Installing Linux dependencies via apt...")
    u.run_cmd(["sudo", "apt", "update"])

    packages = [
        # Build tools
        "pkg-config", "cmake", "ninja-build",
        # Clang 17 + libc++
        "clang-17", "libc++-17-dev", "libc++abi-17-dev",
        # X11
        "libx11-dev", "libxrandr-dev", "libxinerama-dev",
        "libxcursor-dev", "libxi-dev", "libxext-dev", "libxkbcommon-dev",
        # Wayland
        "libwayland-dev", "wayland-protocols",
        # OpenGL / Mesa
        "mesa-common-dev", "libgl1-mesa-dev", "libglu1-mesa-dev",
        "freeglut3-dev", "mesa-utils",
        # Vulkan
        "libvulkan-dev", "vulkan-tools", "vulkan-validationlayers",
        # zlib (Assimp)
        "zlib1g-dev",
    ]
    u.run_cmd(["sudo", "apt", "install", "-y"] + packages)
    u.success("Linux dependencies installed.")


def _install_windows_deps() -> None:
    """Verify Windows prerequisites (no auto-install)."""
    u.require_tool("cmake", "Install CMake and ensure it is on PATH.")
    u.require_tool("tar", "Use Windows 10/11 tar.exe or install bsdtar.")

    vcvars = u.find_vcvars()
    if vcvars:
        u.success(f"Found Visual Studio build environment: {vcvars}")
    else:
        u.warn(
            "Visual Studio 2022 Build Tools not detected.\n"
            "       Build may fail until MSVC tools are installed."
        )


def install_system_deps() -> None:
    if u.is_macos():
        _install_macos_deps()
    elif u.is_linux():
        _install_linux_deps()
    else:
        _install_windows_deps()


# ── Filament SDK ──────────────────────────────────────────────────

def _filament_asset_name() -> str:
    plat = u.detect_platform()
    platform_map = {
        "windows": "windows",
        "darwin":  "mac",
        "linux":   "linux",
    }
    return f"filament-{FILAMENT_VERSION}-{platform_map[plat]}.tgz"


def _filament_url() -> str:
    asset = _filament_asset_name()
    return f"https://github.com/google/filament/releases/download/{FILAMENT_VERSION}/{asset}"


def _is_filament_ready() -> bool:
    fdir = _filament_dir()
    if not (fdir / "include" / "filament" / "Engine.h").is_file():
        return False

    if u.is_windows():
        return (fdir / "lib" / "x86_64" / "filament.lib").is_file()
    else:
        return (
            (fdir / "lib" / "x86_64" / "libfilament.a").is_file()
            or (fdir / "lib" / "arm64" / "libfilament.a").is_file()
        )


def _reorganize_windows_layout(fdir: Path) -> None:
    """Move the flat Windows SDK layout into include/ + lib/ structure."""
    x86_dir = fdir / "x86_64"
    if not x86_dir.is_dir():
        return

    print("  Reorganizing Windows SDK layout...")

    include_dir = fdir / "include"
    include_dir.mkdir(exist_ok=True)

    header_dirs = [
        "backend", "camutils", "filamat", "filament",
        "filament-generatePrefilterMipmap", "filament-iblprefilter",
        "filament-matp", "filameshio", "geometry", "gltfio",
        "ibl", "image", "imageio-lite", "ktxreader", "math",
        "mathio", "mikktspace", "tsl", "uberz", "utils", "viewer",
    ]

    for hdir in header_dirs:
        src = fdir / hdir
        if src.is_dir():
            dst = include_dir / hdir
            if dst.exists():
                shutil.rmtree(dst)
            shutil.move(str(src), str(dst))

    # Libraries
    lib_dir = fdir / "lib" / "x86_64"
    lib_dir.mkdir(parents=True, exist_ok=True)

    md_dir = x86_dir / "md"
    if md_dir.is_dir():
        for lib_file in md_dir.glob("*.lib"):
            shutil.copy2(str(lib_file), str(lib_dir / lib_file.name))

    mdd_dir = x86_dir / "mdd"
    if mdd_dir.is_dir():
        debug_lib_dir = fdir / "lib" / "x86_64_debug"
        debug_lib_dir.mkdir(parents=True, exist_ok=True)
        for lib_file in mdd_dir.glob("*.lib"):
            shutil.copy2(str(lib_file), str(debug_lib_dir / lib_file.name))

    print("  Windows SDK reorganized into include/ + lib/ layout.")


def download_filament() -> None:
    url = _filament_url()
    fdir = _filament_dir()

    print()
    print(f">>> Downloading Filament SDK {FILAMENT_VERSION}...")
    print(f"    URL: {url}")

    with tempfile.TemporaryDirectory(prefix="monster-engine-setup-") as tmpdir:
        archive_path = Path(tmpdir) / _filament_asset_name()
        u.download_file(url, archive_path)

        # Remove existing SDK
        if fdir.is_dir():
            print("  Removing existing Filament SDK...")
            shutil.rmtree(fdir)

        fdir.mkdir(parents=True, exist_ok=True)

        # Extract
        print("  Extracting SDK...")
        with tarfile.open(archive_path) as tar:
            # --strip-components=1 equivalent
            members = tar.getmembers()
            for member in members:
                parts = member.name.split("/", 1)
                if len(parts) > 1:
                    member.name = parts[1]
                else:
                    continue
                tar.extract(member, fdir)

    # Reorganize on Windows
    if u.is_windows():
        _reorganize_windows_layout(fdir)

    u.success(f"  Filament SDK {FILAMENT_VERSION} installed successfully.")


# ── Main ──────────────────────────────────────────────────────────

def main() -> None:
    arch = u.detect_arch()
    plat = u.detect_platform()

    u.info("=== Monster Engine Setup ===")
    print(f"  Platform: {plat} ({arch})")
    print(f"  Filament: {FILAMENT_VERSION}")
    print()

    # Install system dependencies
    install_system_deps()

    # Filament SDK
    is_ci = os.environ.get("CI", "").lower() in ("true", "1") or "--ci" in sys.argv

    if _is_filament_ready():
        print()
        print(f">>> Filament SDK already present at {_filament_dir()}")
        if is_ci:
            print("    CI mode: keeping existing Filament SDK.")
        else:
            answer = input("    Re-download? [y/N] ").strip().lower()
            if answer == "y":
                download_filament()
            else:
                print("    Keeping existing Filament SDK.")
    else:
        download_filament()

    # Verify
    fdir = _filament_dir()
    print()
    u.info("=== Setup Complete ===")

    if u.is_windows():
        if (fdir / "lib" / "x86_64" / "filament.lib").exists():
            u.success("  [OK] Filament SDK (x86_64)")
        else:
            u.warn("Filament SDK present, but no expected library directory was found.")
    else:
        if (fdir / "lib" / arch).is_dir():
            u.success(f"  [OK] Filament SDK ({arch})")
        elif (fdir / "lib" / "x86_64").is_dir():
            u.success("  [OK] Filament SDK (x86_64)")
        else:
            u.warn("Filament SDK: no library directory found.")

    print()
    print("To build:")
    print("  python scripts/build.py")
    print()


if __name__ == "__main__":
    main()
