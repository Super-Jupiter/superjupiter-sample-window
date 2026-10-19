#!/usr/bin/env python3
"""Validate version identity and exported package contents; never publish."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def package_files(path):
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            return {x.filename: archive.read(x) for x in archive.infolist() if not x.is_dir()}
    with tarfile.open(path) as archive:
        result = {}
        for entry in archive.getmembers():
            if entry.issym() or entry.islnk():
                raise ValueError("Package contains a link")
            if entry.isfile():
                result[entry.name] = archive.extractfile(entry).read()
        return result


def normalised_files(files):
    result = {}
    for name, data in files.items():
        name = name.removeprefix('./')
        p = PurePosixPath(name)
        if p.is_absolute() or '..' in p.parts or '\\' in name or ':' in name:
            raise ValueError("Unsafe archive path")
        result[name] = data
    if 'library.json' not in result:
        roots = {PurePosixPath(name).parts[0] for name in result}
        if len(roots) != 1:
            raise ValueError("Package root is ambiguous")
        result = {name.split('/', 1)[1]: data for name, data in result.items() if '/' in name}
    return result



def validate_review_contract():
    import re
    manifest=json.loads((ROOT/'library.json').read_text())
    header=(ROOT/'src'/manifest['headers'][0]).read_text()
    versions=re.findall(r'^#define\s+SUPERJUPITER_\w+_VERSION\s+"([^"]+)"',header,re.M)
    assert versions==[manifest['version']], 'Public version macro mismatch'
    assert all(a['name']=='SuperJupiter' for a in manifest['authors'])
    assert manifest['license']=='MIT'
    assert 'hardware validation pending' in (ROOT/'README.md').read_text().lower()
    assert (ROOT/'docs/INTEROPERABILITY.md').is_file()
    assert (ROOT/'tests/review_regressions.cpp').is_file()

def main():
    validate_review_contract()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag')
    parser.add_argument('--require-clean', action='store_true')
    parser.add_argument('--archive', type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'library.json').read_text())
    version = manifest['version']
    assert re.fullmatch(r'\d+\.\d+\.\d+', version), 'Invalid semantic version'
    properties = dict(line.split('=', 1) for line in (ROOT / 'library.properties').read_text().splitlines() if '=' in line)
    assert manifest['name'] == properties['name'] == 'SuperJupiterSampleWindow'
    assert properties['version'] == version
    assert properties['author'] == 'SuperJupiter'
    assert 'hardware validation pending' in manifest['description'].lower()
    assert 'hardware validation pending' in properties['paragraph'].lower()
    assert 'VERSION ' + version + ' LANGUAGES' in (ROOT / 'CMakeLists.txt').read_text()
    assert '#define SUPERJUPITER_SAMPLE_WINDOW_VERSION "' + version + '"' in (ROOT / 'src/SuperJupiterSampleWindow.h').read_text()
    assert '## ' + version + ' - ' in (ROOT / 'CHANGELOG.md').read_text()
    assert '## Release ' + version in (ROOT / 'docs/VALIDATION.md').read_text()
    assert '@' + version in (ROOT / 'README.md').read_text()
    if args.tag:
        assert args.tag == 'v' + version, 'Tag does not match package version'
    if args.require_clean:
        assert not subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT, text=True).strip(), 'Working tree is not clean'
    if args.archive:
        files = normalised_files(package_files(args.archive))
        required = ['tests/review_regressions.cpp','docs/INTEROPERABILITY.md','library.json', 'library.properties', 'src/SuperJupiterSampleWindow.h', 'LICENSE',
                    'README.md', 'CHANGELOG.md', 'docs/API.md', 'docs/MIGRATION.md', 'docs/CONVENTIONS.md',
                    'docs/VALIDATION.md', 'CMakeLists.txt', 'cmake/SuperJupiterSampleWindowConfig.cmake.in',
                    'tests/test_sample_window.cpp', 'tests/namespace_isolation.cpp', 'tests/consumer/main.cpp',
                    'tests/consumer/CMakeLists.txt', 'tools/check_release.py', 'tools/verify_consumers.py',
                    'tests/reference_fixture.h', 'tools/generate_reference.py', 'tools/library_manager_mode.py',
                    'tools/verify_platformio.py', 'tools/package_zip.py', 'extras/WithSensorQuality/WithSensorQuality.ino',
                    'examples/MinimalWindow/MinimalWindow.ino', 'examples/SyntheticTemperatureWindow/SyntheticTemperatureWindow.ino']
        for name in required:
            assert name in files, 'Package missing ' + name
            assert files[name].replace(b'\r\n', b'\n') == (ROOT / name).read_bytes().replace(b'\r\n', b'\n'), 'Packaged file differs: ' + name
        assert not any(name.startswith(('build/', '.git/', '.pio/')) for name in files), 'Private/build state in package'
        print('Archive SHA-256: ' + hashlib.sha256(args.archive.read_bytes()).hexdigest())
    print('PASS: SuperJupiterSampleWindow ' + version + ' version/identity/package checks')


if __name__ == '__main__':
    main()
