#!/usr/bin/env python3
"""
Monster Engine — Cross-platform source formatter.

Formats all C/C++ source files using clang-format, skipping
``build/`` and ``third_party/`` directories.

Usage:
    python scripts/format.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import script_utils as u

_EXTENSIONS = [".cpp", ".hpp", ".h", ".c", ".cc", ".cxx"]
_EXCLUDE_DIRS = ["build", "third_party"]

# Fallback clang-format locations on Windows
_WINDOWS_CLANG_CANDIDATES = [
    r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\clang-format.exe",
    r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\Llvm\bin\clang-format.exe",
    r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\bin\clang-format.exe",
]


def _find_clang_format() -> str:
    """Locate clang-format on PATH or in known VS locations."""
    path = u.check_tool("clang-format")
    if path:
        return path

    if u.is_windows():
        u.warn("clang-format not found in PATH. Trying Visual Studio locations...")
        for candidate in _WINDOWS_CLANG_CANDIDATES:
            if Path(candidate).exists():
                return candidate

    u.fatal("clang-format not found. Install LLVM or use Visual Studio's clang-format.")
    return ""  # unreachable


def main() -> None:
    project_root = u.get_project_root()

    u.info("========================================")
    u.info("   FORMATTING ALL SOURCE FILES")
    u.info("========================================")
    print()

    clang_format = _find_clang_format()
    print(f"Using: {clang_format}")
    print()

    files = u.find_files(project_root, _EXTENSIONS, _EXCLUDE_DIRS)

    for f in files:
        u.success(f"[FORMAT] {f}")
        u.run_cmd([clang_format, "-i", str(f)])

    print()
    u.info("========================================")
    u.info(f"   DONE! Formatted {len(files)} files")
    u.info("========================================")
    print()


if __name__ == "__main__":
    main()
