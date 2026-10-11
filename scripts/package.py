#!/usr/bin/env python3
"""Collect the JUCE build products into release-ready platform archives."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import tarfile
import tempfile
import zipfile
from pathlib import Path


FORMATS = {
    "VST3": ".vst3",
    "AU": ".component",
}

THIRD_PARTY_LICENSES = (
    ("JUCE/LICENSE.md", "_deps/juce-src/LICENSE.md"),
    ("JUCE/VST3-SDK/LICENSE.txt", "_deps/juce-src/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt"),
    ("JUCE/AudioUnitSDK/LICENSE.txt", "_deps/juce-src/modules/juce_audio_plugin_client/AU/AudioUnitSDK/LICENSE.txt"),
    ("JUCE/zlib/LICENSE", "_deps/juce-src/modules/juce_core/zip/zlib/LICENSE"),
    ("JUCE/pnglib/LICENSE", "_deps/juce-src/modules/juce_graphics/image_formats/pnglib/LICENSE"),
    ("JUCE/SheenBidi/LICENSE", "_deps/juce-src/modules/juce_graphics/unicode/sheenbidi/LICENSE"),
    ("JUCE/HarfBuzz/COPYING", "_deps/juce-src/modules/juce_graphics/fonts/harfbuzz/COPYING"),
    ("JUCE/FLAC-Licence.txt", "_deps/juce-src/modules/juce_audio_formats/codecs/flac/Flac Licence.txt"),
    ("JUCE/Ogg-Vorbis-Licence.txt", "_deps/juce-src/modules/juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt"),
    ("JUCE/JPEG-README", "_deps/juce-src/modules/juce_graphics/image_formats/jpglib/README"),
)


def find_plugin(build_dir: Path, plugin_format: str) -> Path:
    extension = FORMATS[plugin_format]
    candidates = [
        path
        for path in build_dir.rglob("*")
        if path.name.lower().endswith(extension)
        and (path.is_dir() or path.is_file())
        and "CMakeFiles" not in path.parts
    ]
    if not candidates:
        raise FileNotFoundError(f"Could not find {plugin_format} product ({extension}) under {build_dir}")
    candidates.sort(key=lambda path: ("Release" not in path.parts, len(path.parts), str(path)))
    return candidates[0]


def copy_product(source: Path, destination: Path) -> None:
    if source.is_dir():
        shutil.copytree(source, destination, symlinks=True)
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--platform", required=True)
    args = parser.parse_args()

    build_dir = args.build_dir.resolve()
    output_dir = args.output_dir.resolve()
    if not build_dir.is_dir():
        parser.error(f"Build directory does not exist: {build_dir}")

    formats = ["VST3"]
    if args.platform.startswith("macos-"):
        formats.append("AU")

    output_dir.mkdir(parents=True, exist_ok=True)
    archive_base = f"ReaperianiBass-{args.platform}"
    use_zip = args.platform.startswith("windows-")
    archive = output_dir / (archive_base + (".zip" if use_zip else ".tar.gz"))

    with tempfile.TemporaryDirectory(prefix="reaperianibass-") as temporary:
        staging = Path(temporary) / archive_base
        staging.mkdir()
        project_root = Path(__file__).resolve().parents[1]
        for plugin_format in formats:
            source = find_plugin(build_dir, plugin_format)
            destination = staging / plugin_format / source.name
            copy_product(source, destination)
            print(f"Packaged {plugin_format}: {source}")

        for document in ("README.md", "LICENSE", "THIRD_PARTY_NOTICES.md"):
            shutil.copy2(project_root / document, staging / document)

        for archive_path, build_path in THIRD_PARTY_LICENSES:
            source = build_dir / build_path
            if not source.is_file():
                raise FileNotFoundError(f"Missing third-party license: {source}")
            destination = staging / "third_party" / archive_path
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)

        if use_zip:
            with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=8) as bundle:
                for item in staging.rglob("*"):
                    if item.is_file():
                        bundle.write(item, item.relative_to(staging.parent))
        else:
            with tarfile.open(archive, "w:gz", format=tarfile.PAX_FORMAT) as bundle:
                bundle.add(staging, arcname=staging.name, recursive=True)

    checksum_file = output_dir / (archive.name + ".sha256")
    checksum_file.write_text(f"{sha256(archive)}  {archive.name}\n", encoding="utf-8")
    print(f"Created {archive}")
    print(f"Created {checksum_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
