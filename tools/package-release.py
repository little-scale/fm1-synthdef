#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build an allowlisted release tree, checksum manifest and complete ZIP."""
import argparse
import hashlib
import io
import json
import shutil
import subprocess
import tarfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.1.0-preview'
NAME = 'FM1-SynthDef-' + VERSION
EXCLUDE = {'build', '__pycache__', '.git', '.DS_Store', 'data'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy_tree(source, destination):
    for path in sorted(source.rglob('*')):
        rel = path.relative_to(source)
        if any(part in EXCLUDE or part.endswith('.dSYM') for part in rel.parts):
            continue
        if path.is_symlink():
            raise ValueError('Unexpected source symlink: ' + str(rel))
        if path.is_file() and path.suffix not in ('.pyc', '.wav'):
            target = destination / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)


def snapshot(source, destination, revision):
    if (source / '.git').exists():
        diff = subprocess.check_output(['git', '-C', str(source), 'diff', revision, '--'])
        if diff:
            raise ValueError('Tracked upstream source differs from pin: ' + str(source))
        data = subprocess.check_output(['git', '-C', str(source), 'archive', revision])
        destination.mkdir(parents=True, exist_ok=True)
        with tarfile.open(fileobj=io.BytesIO(data)) as archive:
            archive.extractall(destination, filter='data')
    else:
        copy_tree(source, destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True,
                        help='New destination directory; never overwritten')
    args = parser.parse_args()
    dest = args.output.resolve()
    if dest.exists():
        raise SystemExit('Output already exists: ' + str(dest))
    accepted = json.loads((ROOT / 'evidence/master-effect.json').read_text())
    firmware = ROOT / 'fm1-synth/firmware/build/synthdef.fwsc'
    if not firmware.exists():
        firmware = ROOT / 'firmware' / (NAME + '.fwsc')
    if digest(firmware) != accepted['sha256']['synthdef.fwsc']:
        raise SystemExit('Firmware differs from accepted build; verify it before releasing.')
    for relative, expected in accepted['source_sha256'].items():
        if digest(ROOT / relative) != expected:
            raise SystemExit('Accepted firmware source changed: ' + relative)
    pins = json.loads((ROOT / 'setup/dependencies.json').read_text())
    dest.mkdir(parents=True)
    for filename in ('README.md', 'LICENSE', 'THIRD_PARTY.md', '.gitignore'):
        shutil.copy2(ROOT / filename, dest / filename)
    for dirname in ('fm1-synth', 'synthdef-engine', 'evidence', 'tools'):
        copy_tree(ROOT / dirname, dest / dirname)
    for filename in ('QUICKSTART.md', 'PATCHES.md', 'COMPATIBILITY.md',
                     'DEVELOPMENT.md', 'VERIFICATION.md', 'DEMO.md', 'RELEASE-NOTES.md'):
        target = dest / 'docs' / filename
        target.parent.mkdir(exist_ok=True)
        shutil.copy2(ROOT / 'docs' / filename, target)
    for filename in ('env.sh', 'verify-sdk.py', 'fetch-toolchain.sh',
                     'docker-image.txt', 'dependencies.json',
                     'requirements-lock.txt', 'requirements-midi-lock.txt'):
        target = dest / 'setup' / filename
        target.parent.mkdir(exist_ok=True)
        shutil.copy2(ROOT / 'setup' / filename, target)
    snapshot(ROOT / 'fm1-base', dest / 'fm1-base', pins['fm1-base']['commit'])
    snapshot(ROOT / 'fm1-base/bsp', dest / 'fm1-base/bsp', pins['felucca-bsp']['commit'])
    for rel in ('LICENSE', 'README.md', 'cpu/wl82/tools/uboot.boot',
                'cpu/wl82/tools/cfg_tool.bin', 'cpu/wl82/tools/cfg/eq_cfg_hw.bin'):
        target = dest / 'dependencies/ac79-sdk' / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / 'dependencies/ac79-sdk' / rel, target)
    (dest / 'firmware').mkdir()
    shutil.copy2(firmware, dest / 'firmware' / (NAME + '.fwsc'))
    (dest / 'media').mkdir()
    shutil.copy2(ROOT / 'media/two-op-fm-host-preview.wav', dest / 'media/two-op-fm-host-preview.wav')
    manifest = []
    for path in sorted(dest.rglob('*')):
        if path.is_symlink():
            raise ValueError('Symlink in distribution: ' + str(path))
        if path.is_file():
            manifest.append(digest(path) + '  ' + path.relative_to(dest).as_posix())
    (dest / 'SHA256SUMS').write_text('\n'.join(manifest) + '\n')
    archive_path = dest.with_name(dest.name + '.zip')
    if archive_path.exists():
        raise SystemExit('Archive already exists: ' + str(archive_path))
    with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(dest.rglob('*')):
            if path.is_file():
                archive.write(path, NAME + '/' + path.relative_to(dest).as_posix())
    archive_path.with_suffix('.zip.sha256').write_text(digest(archive_path) + '  ' + archive_path.name + '\n')
    print(json.dumps(dict(directory=str(dest), archive=str(archive_path), files=len(manifest),
                          archive_sha256=digest(archive_path)), indent=2))


if __name__ == '__main__':
    main()
