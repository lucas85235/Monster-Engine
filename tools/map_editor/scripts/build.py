#!/usr/bin/env python3
"""
Map Editor — Cross-platform build script.

Usage:
    python tools/map_editor/scripts/build.py [config]
"""

import os
import sys
from pathlib import Path

SCRIPTS_DIR = Path(__file__).resolve().parent.parent.parent.parent / "scripts"
sys.path.insert(0, str(SCRIPTS_DIR))
import script_utils as u

TOOL_NAME = "map_editor"


def main() -> None:
    config = sys.argv[1] if len(sys.argv) > 1 else "Debug"
    project_root = Path(__file__).resolve().parent.parent.parent.parent
    build_dir = u.get_build_dir(project_root)

    u.info("========================================")
    u.info(f"  Map Editor - Build Script ({u.detect_platform()})")
    u.info("========================================")
    print()

    u.require_tool("cmake")
    print(f"[1/3] Project root: {project_root}")
    print()

    # ── CMake configure ──────────────────────────────────────────
    if not build_dir.is_dir():
        print("[2/3] Creating build directory and generating project files...")
    else:
        print("[2/3] Regenerating CMake project files...")

    cmake_cmd = [
        "cmake",
        "-S", str(project_root),
        "-B", str(build_dir),
        "-G", u.cmake_generator(),
        *u.cmake_platform_flags(config),
    ]
    if u.is_windows():
        cmake_cmd.extend(["-A", "x64"])

    u.run_cmd(cmake_cmd)
    print()

    # ── Build ────────────────────────────────────────────────────
    print(f"[3/3] Building {TOOL_NAME} ({config})...")
    u.run_cmd([
        "cmake", "--build", str(build_dir),
        "--target", TOOL_NAME,
        "--config", config,
        "--parallel",
    ])

    print()
    u.info("========================================")
    u.success("  Build completed successfully!")
    u.info("========================================")
    print()

    if u.is_windows():
        print(f"Executable: {build_dir / 'tools' / TOOL_NAME / config / f'{TOOL_NAME}.exe'}")
    else:
        print(f"Executable: {build_dir / 'tools' / TOOL_NAME / TOOL_NAME}")
    print()


if __name__ == "__main__":
    main()
