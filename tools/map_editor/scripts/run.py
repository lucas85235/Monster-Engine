#!/usr/bin/env python3
"""
Map Editor — Cross-platform run script.

Usage:
    python tools/map_editor/scripts/run.py
"""

import sys
from pathlib import Path

SCRIPTS_DIR = Path(__file__).resolve().parent.parent.parent.parent / "scripts"
sys.path.insert(0, str(SCRIPTS_DIR))
import script_utils as u

TOOL_NAME = "map_editor"


def main() -> None:
    project_root = Path(__file__).resolve().parent.parent.parent.parent
    build_dir = u.get_build_dir(project_root)

    u.info("========================================")
    u.info(f"  Map Editor - Run Script ({u.detect_platform()})")
    u.info("========================================")
    print()

    if u.is_windows():
        exe = build_dir / "tools" / TOOL_NAME / "Debug" / f"{TOOL_NAME}.exe"
    else:
        exe = build_dir / "tools" / TOOL_NAME / TOOL_NAME

    if not exe.exists():
        u.fatal(
            f"{TOOL_NAME} not found at {exe}\n"
            "       Please run: python tools/map_editor/scripts/build.py"
        )

    print(f"Starting Map Editor from: {project_root}")
    print()

    u.run_cmd([str(exe)], cwd=str(project_root), check=False)

    print()
    print("Map Editor closed.")


if __name__ == "__main__":
    main()
