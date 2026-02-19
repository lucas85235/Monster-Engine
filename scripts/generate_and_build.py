#!/usr/bin/env python3
"""
Monster Engine — Generate project files, build, and (optionally) run.

Replaces ``generate_visual_studio_files_and_build.bat``.
On Windows uses Visual Studio / MSBuild; on Linux/macOS uses Ninja.

Usage:
    python scripts/generate_and_build.py
"""

import os
import sys
import shutil
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import script_utils as u

BUILD_TYPE = os.environ.get("BUILD_TYPE", "Debug")
APP_NAME = os.environ.get("APP_NAME", "animation_test")
GAME_NAME = os.environ.get("GAME_NAME", "AnimationTest")


def _find_executable(build_dir: Path) -> Path | None:
    """Locate the built executable."""
    if u.is_windows():
        search_dir = build_dir / "apps" / APP_NAME / BUILD_TYPE
        for f in search_dir.glob("*.exe"):
            return f
    else:
        candidate = build_dir / "apps" / APP_NAME / APP_NAME
        if candidate.exists():
            return candidate
    return None


def _create_distribution(exe_path: Path, build_dir: Path, project_root: Path) -> None:
    """Create a distribution folder (Release builds only)."""
    if BUILD_TYPE.lower() != "release":
        print(u.colored("[STEP 5/5]", "blue"), "Skipping distribution (non-Release build)")
        return

    print(u.colored("[STEP 5/5]", "blue"), u.colored("Creating distribution folder...", "yellow"))

    dist_dir = project_root / "dist" / GAME_NAME

    if dist_dir.exists():
        shutil.rmtree(dist_dir)
    dist_dir.mkdir(parents=True)

    # Copy executable
    print("  Copying executable...")
    shutil.copy2(str(exe_path), str(dist_dir))

    # Copy assets
    assets_dir = project_root / "assets"
    if assets_dir.is_dir():
        print("  Copying assets...")
        shutil.copytree(str(assets_dir), str(dist_dir / "assets"))

    # Copy DLLs (Windows)
    if u.is_windows():
        dll_dir = build_dir / "apps" / APP_NAME / BUILD_TYPE
        for dll in dll_dir.glob("*.dll"):
            print(f"  Copying {dll.name}...")
            shutil.copy2(str(dll), str(dist_dir))

    u.success(f"Distribution folder created: {dist_dir}")
    print(u.colored("    Ready to zip and distribute!", "yellow"))


def main() -> None:
    project_root = u.get_project_root()
    build_dir = u.get_build_dir(project_root)

    u.info("========================================")
    u.info("   MONSTER ENGINE - BUILD SCRIPT")
    u.info("========================================")
    print()

    # ── Step 1: CMake Generate ───────────────────────────────────
    print(u.colored("[STEP 1/5]", "blue"), u.colored("Generating CMake project...", "yellow"))

    cmake_cmd = [
        "cmake",
        "-S", str(project_root),
        "-B", str(build_dir),
        "-G", u.cmake_generator(),
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        *u.cmake_platform_flags(BUILD_TYPE),
        *u.sccache_flags(),
    ]
    u.run_cmd(cmake_cmd)
    u.success("[OK] CMake project generated successfully")
    print()

    # ── Step 2: VS Environment (Windows) ─────────────────────────
    print(u.colored("[STEP 2/5]", "blue"), u.colored("Setting up build environment...", "yellow"))
    if u.is_windows():
        vcvars = u.find_vcvars()
        if vcvars:
            u.success(f"[OK] Found: {vcvars}")
        else:
            u.warn("Visual Studio 2022 not found. MSBuild may not be available.")
    else:
        u.success("[OK] Using system compilers")
    print()

    # ── Step 3: Build ────────────────────────────────────────────
    print(u.colored("[STEP 3/5]", "blue"), u.colored(f"Building project ({BUILD_TYPE})...", "yellow"))
    print()

    u.run_cmd([
        "cmake", "--build", str(build_dir),
        "--config", BUILD_TYPE,
        "--parallel",
    ])

    print()
    u.success("[OK] Build completed successfully")
    print()

    # ── Step 4: Find Executable ──────────────────────────────────
    print(u.colored("[STEP 4/5]", "blue"), u.colored("Looking for executable...", "yellow"))

    exe_path = _find_executable(build_dir)
    if exe_path is None:
        u.fatal("Executable not found in build folder!")

    u.success(f"[OK] Found: {exe_path}")
    print()

    # ── Step 5: Distribution ─────────────────────────────────────
    _create_distribution(exe_path, build_dir, project_root)
    print()

    # ── Launch ───────────────────────────────────────────────────
    u.info("========================================")
    u.info(f"   LAUNCHING {GAME_NAME}")
    u.info("========================================")
    print()

    u.run_cmd([str(exe_path)], cwd=str(project_root), check=False)
    u.success("[SUCCESS] Program finished.")


if __name__ == "__main__":
    main()
