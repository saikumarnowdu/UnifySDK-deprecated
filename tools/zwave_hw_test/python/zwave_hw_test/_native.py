"""ctypes loader for libzwave_python_harness."""

from __future__ import annotations

import os
import sys
from ctypes import CDLL
from pathlib import Path


def _candidate_paths() -> list[Path]:
    env = os.environ.get("ZWAVE_PYTHON_HARNESS_LIB")
    if env:
        return [Path(env)]

    here = Path(__file__).resolve()
    roots = [
        here.parents[2],  # tools/zwave_hw_test
        here.parents[3],  # tools
        here.parents[4],  # repo root
    ]
    names = [
        "libzwave_python_harness.so",
        "zwave_python_harness.so",
    ]
    paths: list[Path] = []
    for root in roots:
        for name in names:
            paths.append(root / "build" / name)
            paths.append(root / name)
    return paths


def load_library() -> CDLL:
    for path in _candidate_paths():
        if path.is_file():
            return CDLL(str(path))
    tried = "\n".join(str(p) for p in _candidate_paths())
    raise OSError(
        "libzwave_python_harness not found. Build with:\n"
        "  cmake -B tools/zwave_hw_test/build tools/zwave_hw_test && "
        "cmake --build tools/zwave_hw_test/build\n"
        f"Tried:\n{tried}"
    )


_lib: CDLL | None = None


def lib() -> CDLL:
    global _lib
    if _lib is None:
        _lib = load_library()
    return _lib
