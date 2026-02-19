#!/usr/bin/env python3
"""
Shared utilities for Monster Engine build scripts.

Provides cross-platform helpers: platform detection, colored output,
subprocess execution, tool checking, MSVC detection, and file downloads.
"""

import os
import sys
import glob
import shutil
import platform
import subprocess
import tempfile
import urllib.request
from pathlib import Path
from typing import Optional


# ── Color Support ────────────────────────────────────────────────

# ANSI color codes (supported on Windows 10+ with VT100, macOS, Linux)
_COLORS = {
    "reset":   "\033[0m",
    "bold":    "\033[1m",
    "red":     "\033[91m",
    "green":   "\033[92m",
    "yellow":  "\033[93m",
    "blue":    "\033[94m",
    "magenta": "\033[95m",
    "cyan":    "\033[96m",
}


def _enable_windows_ansi() -> None:
    """Enable ANSI escape sequences on Windows 10+."""
    if sys.platform != "win32":
        return
    try:
        import ctypes
        kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
        # STD_OUTPUT_HANDLE = -11
        handle = kernel32.GetStdHandle(-11)
        mode = ctypes.c_ulong()
        kernel32.GetConsoleMode(handle, ctypes.byref(mode))
        # ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
        kernel32.SetConsoleMode(handle, mode.value | 0x0004)
    except Exception:
        pass


_enable_windows_ansi()


def colored(text: str, color: str) -> str:
    """Return *text* wrapped in ANSI color codes.

    Args:
        text: The string to colorize.
        color: One of 'red', 'green', 'yellow', 'blue', 'magenta', 'cyan', 'bold'.
    """
    code = _COLORS.get(color, "")
    reset = _COLORS["reset"]
    return f"{code}{text}{reset}" if code else text


def info(msg: str) -> None:
    """Print an informational message in cyan."""
    print(colored(msg, "cyan"))


def success(msg: str) -> None:
    """Print a success message in green."""
    print(colored(msg, "green"))


def warn(msg: str) -> None:
    """Print a warning message in yellow."""
    print(colored(f"[WARN] {msg}", "yellow"))


def error(msg: str) -> None:
    """Print an error message in red."""
    print(colored(f"[ERROR] {msg}", "red"))


def fatal(msg: str, code: int = 1) -> None:
    """Print an error message and exit."""
    error(msg)
    sys.exit(code)


# ── Platform Detection ───────────────────────────────────────────

def detect_platform() -> str:
    """Return the current platform: 'windows', 'linux', or 'darwin'."""
    s = sys.platform
    if s.startswith("win"):
        return "windows"
    if s.startswith("linux"):
        return "linux"
    if s == "darwin":
        return "darwin"
    return s


def detect_arch() -> str:
    """Return the CPU architecture: 'x86_64' or 'arm64'."""
    machine = platform.machine().lower()
    if machine in ("x86_64", "amd64"):
        return "x86_64"
    if machine in ("arm64", "aarch64"):
        return "arm64"
    return machine


def is_windows() -> bool:
    return detect_platform() == "windows"


def is_linux() -> bool:
    return detect_platform() == "linux"


def is_macos() -> bool:
    return detect_platform() == "darwin"


# ── Path Helpers ─────────────────────────────────────────────────

def get_project_root(from_path: Optional[str] = None) -> Path:
    """Resolve the Monster Engine project root.

    When *from_path* is ``None`` the root is determined relative to
    *this* file (``scripts/script_utils.py`` → parent is ``scripts/``).
    """
    if from_path:
        return Path(from_path).resolve()
    return Path(__file__).resolve().parent.parent


def get_build_dir(project_root: Optional[Path] = None) -> Path:
    """Return the ``build/`` directory path."""
    root = project_root or get_project_root()
    return root / "build"


# ── Tool Checking ────────────────────────────────────────────────

def check_tool(name: str) -> Optional[str]:
    """Return the full path to *name* if it is found on PATH, else ``None``."""
    return shutil.which(name)


def require_tool(name: str, hint: str = "") -> str:
    """Like :func:`check_tool` but exits with an error if the tool is missing."""
    path = check_tool(name)
    if path is None:
        msg = f"{name} not found in PATH."
        if hint:
            msg += f" {hint}"
        fatal(msg)
    return path  # type: ignore[return-value]


# ── Subprocess Helpers ───────────────────────────────────────────

def run_cmd(
    args: list[str],
    *,
    cwd: Optional[str | Path] = None,
    env: Optional[dict[str, str]] = None,
    check: bool = True,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    """Run a command, streaming output by default.

    Args:
        args: Command and its arguments.
        cwd: Working directory.
        env: Environment variables (merged with ``os.environ``).
        check: If True, raise / exit on non-zero return code.
        capture: If True, capture stdout/stderr instead of streaming.

    Returns:
        The ``CompletedProcess`` instance.
    """
    merged_env = None
    if env:
        merged_env = {**os.environ, **env}

    try:
        result = subprocess.run(
            args,
            cwd=cwd,
            env=merged_env,
            check=False,
            text=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.PIPE if capture else None,
        )
    except FileNotFoundError:
        fatal(f"Command not found: {args[0]}")

    if check and result.returncode != 0:
        fatal(f"Command failed (exit {result.returncode}): {' '.join(args)}")

    return result


# ── MSVC / Visual Studio Helpers (Windows only) ─────────────────

_VS_EDITIONS = ["Community", "Professional", "Enterprise", "BuildTools"]


def find_vcvars() -> Optional[Path]:
    """Locate ``vcvars64.bat`` from a Visual Studio 2022 installation."""
    if not is_windows():
        return None

    program_dirs = [
        os.environ.get("ProgramFiles", r"C:\Program Files"),
        os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
    ]
    for prog in program_dirs:
        for edition in _VS_EDITIONS:
            candidate = Path(prog) / "Microsoft Visual Studio" / "2022" / edition / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
            if candidate.exists():
                return candidate
    return None


def find_latest_msvc_toolset() -> Optional[str]:
    """Return the version string of the newest MSVC toolset installed, or ``None``."""
    if not is_windows():
        return None

    prog = os.environ.get("ProgramFiles", r"C:\Program Files")
    latest: Optional[str] = None
    for edition in _VS_EDITIONS:
        msvc_root = Path(prog) / "Microsoft Visual Studio" / "2022" / edition / "VC" / "Tools" / "MSVC"
        if msvc_root.is_dir():
            for ver_dir in sorted(msvc_root.iterdir()):
                if ver_dir.is_dir():
                    latest = ver_dir.name
    return latest


def cmake_generator() -> str:
    """Return the default CMake generator for the current platform."""
    if is_windows():
        return "Visual Studio 17 2022"
    return "Ninja"


def cmake_platform_flags(build_type: str = "RelWithDebInfo") -> list[str]:
    """Return platform-specific CMake flags."""
    flags: list[str] = []
    if is_windows():
        toolset = find_latest_msvc_toolset()
        if toolset:
            flags.append(f"-T version={toolset}")
    elif is_macos():
        flags.append(f"-DCMAKE_BUILD_TYPE={build_type}")
    else:
        # Linux — clang-17 with libc++
        flags.extend([
            f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DCMAKE_C_COMPILER=clang-17",
            "-DCMAKE_CXX_COMPILER=clang++-17",
            "-DCMAKE_CXX_FLAGS=-stdlib=libc++",
        ])
    return flags


def sccache_flags() -> list[str]:
    """Return CMake flags to enable sccache if it is available."""
    if check_tool("sccache"):
        print("sccache detected: enabling compiler launcher.")
        return [
            "-DCMAKE_C_COMPILER_LAUNCHER=sccache",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=sccache",
        ]
    return []


# ── Download Helpers ─────────────────────────────────────────────

def download_file(url: str, dest: str | Path) -> None:
    """Download *url* to *dest* using curl (preferred) or urllib.

    Falls back to Python's ``urllib`` if ``curl`` is unavailable.
    """
    dest = Path(dest)
    dest.parent.mkdir(parents=True, exist_ok=True)

    if check_tool("curl"):
        run_cmd(["curl", "-L", "--fail", "--progress-bar", "-o", str(dest), url])
    else:
        print(f"  Downloading (urllib): {url}")
        try:
            urllib.request.urlretrieve(url, str(dest))
        except Exception as exc:
            fatal(f"Download failed: {exc}")


# ── File-system Helpers ──────────────────────────────────────────

def find_files(
    root: str | Path,
    extensions: list[str],
    exclude_dirs: Optional[list[str]] = None,
) -> list[Path]:
    """Recursively find files matching *extensions* under *root*.

    Args:
        root: Directory to search.
        extensions: List of extensions (with leading dot, e.g. ``['.cpp', '.h']``).
        exclude_dirs: Directory basenames to skip (e.g. ``['build', 'third_party']``).
    """
    exclude = set(exclude_dirs or [])
    results: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        # Prune excluded directories in-place so os.walk skips them
        dirnames[:] = [d for d in dirnames if d not in exclude]
        for fname in filenames:
            if any(fname.endswith(ext) for ext in extensions):
                results.append(Path(dirpath) / fname)
    return results
