#!/usr/bin/env python3
"""
Monster Engine — Cross-platform build script.

Usage:
    python scripts/build.py [config] [target]

Examples:
    python scripts/build.py
    python scripts/build.py Debug sandbox
    python scripts/build.py RelWithDebInfo animation_test
    python scripts/build.py RelWithDebInfo demo_test
    python scripts/build.py RelWithDebInfo all

Environment overrides:
    BUILD_TYPE=Debug python scripts/build.py
    BUILD_TARGET=animation_test python scripts/build.py
"""

import os
import sys
from pathlib import Path

# Ensure the scripts directory is importable
sys.path.insert(0, str(Path(__file__).resolve().parent))
import script_utils as u

# Target aliases
_TARGET_ALIASES = {
    "animation": "animation_test",
    "anim_test": "animation_test",
}


def _resolve_cmake_app_flags(target: str) -> list[str]:
    """Return CMake -D flags to enable/disable app targets."""
    target_lower = target.lower()

    flag_map = {
        "sandbox":        ["-DSE_BUILD_APP_SANDBOX=ON",  "-DSE_BUILD_APP_ANIMATION_TEST=OFF"],
        "animation_test": ["-DSE_BUILD_APP_SANDBOX=OFF", "-DSE_BUILD_APP_ANIMATION_TEST=ON"],
        "demo_test":      ["-DSE_BUILD_APP_DEMOTEST=ON", "-DSE_BUILD_APP_ANIMATION_TEST=OFF"],
        "all":            ["-DSE_BUILD_APP_SANDBOX=ON",  "-DSE_BUILD_APP_ANIMATION_TEST=ON"],
    }

    return flag_map.get(target_lower, ["-DSE_BUILD_APP_SANDBOX=ON", "-DSE_BUILD_APP_ANIMATION_TEST=ON"])


def _print_usage() -> None:
    print(__doc__)


def main() -> None:
    # ── Parse arguments ──────────────────────────────────────────
    args = sys.argv[1:]
    if any(a in ("-h", "--help", "/?") for a in args):
        _print_usage()
        sys.exit(0)

    build_type = os.environ.get("BUILD_TYPE", "RelWithDebInfo")
    if len(args) >= 1:
        build_type = args[0]

    build_target = os.environ.get("BUILD_TARGET", "sandbox")
    if len(args) >= 2:
        build_target = args[1]

    # Resolve aliases
    build_target = _TARGET_ALIASES.get(build_target.lower(), build_target)

    project_root = u.get_project_root()
    build_dir = u.get_build_dir(project_root)
    filament_dir = project_root / "engine" / "third_party" / "filament"

    cmake_app_flags = _resolve_cmake_app_flags(build_target)

    # ── Banner ───────────────────────────────────────────────────
    u.info("=== Monster Engine Build ===")
    print(f"  Generator: {u.cmake_generator()}")
    print(f"  Config:    {build_type}")
    print(f"  Target:    {build_target}")
    print(f"  AppFlags:  {' '.join(cmake_app_flags)}")
    print()

    # ── Pre-flight checks ────────────────────────────────────────
    u.require_tool("cmake", "Install CMake and run this script again.")

    if not (filament_dir / "include").is_dir():
        u.fatal(
            f"Filament SDK headers not found at \"{filament_dir / 'include'}\".\n"
            "       Run: python scripts/setup.py"
        )

    if not (filament_dir / "lib").is_dir():
        u.fatal(
            f"Filament SDK libraries not found at \"{filament_dir / 'lib'}\".\n"
            "       Run: python scripts/setup.py"
        )

    # ── CMake configure ──────────────────────────────────────────
    cmake_cmd = [
        "cmake",
        "-S", str(project_root),
        "-B", str(build_dir),
        "-G", u.cmake_generator(),
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        *cmake_app_flags,
        *u.cmake_platform_flags(build_type),
        *u.sccache_flags(),
    ]

    u.run_cmd(cmake_cmd)

    # ── Build ────────────────────────────────────────────────────
    build_cmd = [
        "cmake", "--build", str(build_dir),
        "--config", build_type,
        "--parallel",
    ]
    if build_target.lower() != "all":
        build_cmd.extend(["--target", build_target])

    u.run_cmd(build_cmd)

    # ── Report ───────────────────────────────────────────────────
    print()
    u.success("Build completed successfully.")

    if build_target.lower() != "all":
        if u.is_windows():
            exe = build_dir / "apps" / build_target / build_type / f"{build_target}.exe"
        else:
            exe = build_dir / "apps" / build_target / build_target
        if exe.exists():
            print(f"  Executable: \"{exe}\"")
        else:
            u.warn(f"Executable not found at expected location:\n       \"{exe}\"")
    else:
        print("  Built target set: all")
    print()


if __name__ == "__main__":
    main()
