#!/usr/bin/env python3
"""
Monster Engine — Cross-platform build-and-run script.

Configures, builds, and immediately runs the sandbox application.
Replaces ``run.sh``.

Usage:
    python scripts/run.py [target]

Examples:
    python scripts/run.py
    python scripts/run.py sandbox
    python scripts/run.py animation_test
    BUILD_TYPE=Debug python scripts/run.py
"""

import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import script_utils as u


def main() -> None:
    build_type = os.environ.get("BUILD_TYPE", "RelWithDebInfo")
    target = sys.argv[1] if len(sys.argv) > 1 else "sandbox"

    project_root = u.get_project_root()
    build_dir = u.get_build_dir(project_root)

    u.require_tool("cmake")

    # ── Configure ────────────────────────────────────────────────
    cmake_cmd = [
        "cmake",
        "-S", str(project_root),
        "-B", str(build_dir),
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-G", u.cmake_generator(),
        *u.cmake_platform_flags(build_type),
        *u.sccache_flags(),
    ]

    u.run_cmd(cmake_cmd)

    # ── Build ────────────────────────────────────────────────────
    u.run_cmd(["cmake", "--build", str(build_dir), "--config", build_type, "--parallel"])

    # ── Run ──────────────────────────────────────────────────────
    if u.is_windows():
        exe = build_dir / "apps" / target / build_type / f"{target}.exe"
    else:
        exe = build_dir / "apps" / target / target

    if not exe.exists():
        u.fatal(f"Executable not found: {exe}")

    print()
    u.info(f"Launching {target}...")
    u.run_cmd([str(exe)], cwd=str(project_root))


if __name__ == "__main__":
    main()
