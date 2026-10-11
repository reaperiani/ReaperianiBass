from __future__ import annotations

import subprocess
import sys
import tarfile
import tempfile
import unittest
import zipfile
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
PACKAGER = PROJECT_ROOT / "scripts" / "package.py"
THIRD_PARTY_LICENSES = (
    "_deps/juce-src/LICENSE.md",
    "_deps/juce-src/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt",
    "_deps/juce-src/modules/juce_audio_plugin_client/AU/AudioUnitSDK/LICENSE.txt",
    "_deps/juce-src/modules/juce_core/zip/zlib/LICENSE",
    "_deps/juce-src/modules/juce_graphics/image_formats/pnglib/LICENSE",
    "_deps/juce-src/modules/juce_graphics/unicode/sheenbidi/LICENSE",
    "_deps/juce-src/modules/juce_graphics/fonts/harfbuzz/COPYING",
    "_deps/juce-src/modules/juce_audio_formats/codecs/flac/Flac Licence.txt",
    "_deps/juce-src/modules/juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt",
    "_deps/juce-src/modules/juce_graphics/image_formats/jpglib/README",
)


class PackageSmokeTests(unittest.TestCase):
    def create_fake_build(self, root: Path, include_au: bool = False) -> Path:
        build = root / "build"
        release = build / "ReaperianiBass_artefacts" / "Release"
        vst3 = release / "VST3" / "ReaperianiBass.vst3" / "Contents"
        vst3.mkdir(parents=True)
        (vst3 / "manifest.txt").write_text("vst3 placeholder", encoding="utf-8")
        if include_au:
            au = release / "AU" / "ReaperianiBass.component" / "Contents"
            au.mkdir(parents=True)
            (au / "manifest.txt").write_text("au placeholder", encoding="utf-8")
        for license_path in THIRD_PARTY_LICENSES:
            license_file = build / license_path
            license_file.parent.mkdir(parents=True, exist_ok=True)
            license_file.write_text("third-party license", encoding="utf-8")
        return build

    def run_packager(self, build: Path, output: Path, platform: str) -> None:
        subprocess.run(
            [sys.executable, str(PACKAGER), "--build-dir", str(build),
             "--output-dir", str(output), "--platform", platform],
            check=True,
            capture_output=True,
            text=True,
        )

    def test_windows_archive_contains_vst3_and_checksum(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = self.create_fake_build(root)
            output = root / "dist"
            self.run_packager(build, output, "windows-x86_64")

            archive = output / "ReaperianiBass-windows-x86_64.zip"
            with zipfile.ZipFile(archive) as bundle:
                names = set(bundle.namelist())
            self.assertIn("ReaperianiBass-windows-x86_64/VST3/ReaperianiBass.vst3/Contents/manifest.txt", names)
            self.assertIn("ReaperianiBass-windows-x86_64/LICENSE", names)
            self.assertIn("ReaperianiBass-windows-x86_64/third_party/JUCE/VST3-SDK/LICENSE.txt", names)
            self.assertTrue((output / (archive.name + ".sha256")).is_file())

    def test_macos_archive_includes_audio_unit_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = self.create_fake_build(root, include_au=True)
            output = root / "dist"
            self.run_packager(build, output, "macos-universal")

            archive = output / "ReaperianiBass-macos-universal.tar.gz"
            with tarfile.open(archive, "r:gz") as bundle:
                names = set(bundle.getnames())
            self.assertIn("ReaperianiBass-macos-universal/AU/ReaperianiBass.component/Contents/manifest.txt", names)
            self.assertIn("ReaperianiBass-macos-universal/VST3/ReaperianiBass.vst3/Contents/manifest.txt", names)
            self.assertIn("ReaperianiBass-macos-universal/LICENSE", names)
            self.assertIn("ReaperianiBass-macos-universal/third_party/JUCE/AudioUnitSDK/LICENSE.txt", names)
            self.assertTrue((output / (archive.name + ".sha256")).is_file())


if __name__ == "__main__":
    unittest.main()
