#!/usr/bin/env python3
"""Install or remove the Windows VST3 bundle in the current user's standard VST3 folder."""

from __future__ import annotations

import argparse
import os
import shutil
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("install", "remove"))
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    args = parser.parse_args()

    local_app_data = os.environ.get("LOCALAPPDATA")
    if not local_app_data:
        parser.error("LOCALAPPDATA is not set; this helper is for Windows user installs")

    plugin_root = Path(local_app_data) / "Programs" / "Common" / "VST3"
    destination = plugin_root / "ReaperianiBass.vst3"

    if args.action == "remove":
        if destination.exists():
            shutil.rmtree(destination)
            print(f"Removed {destination}")
        else:
            print(f"Not installed: {destination}")
        return 0

    build_dir = args.build_dir.resolve()
    candidates = [path for path in build_dir.rglob("ReaperianiBass.vst3") if path.is_dir()]
    candidates.sort(key=lambda path: ("Release" not in path.parts, len(path.parts), str(path)))
    if not candidates:
        parser.error(f"No built ReaperianiBass.vst3 bundle found under {build_dir}")
    if destination.exists():
        parser.error(f"Destination already exists; remove it explicitly first: {destination}")

    plugin_root.mkdir(parents=True, exist_ok=True)
    shutil.copytree(candidates[0], destination, symlinks=True)
    print(f"Installed {candidates[0]} -> {destination}")
    print("Rescan VST3 plug-ins or restart REAPER to load the new plug-in.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
