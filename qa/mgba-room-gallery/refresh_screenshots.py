#!/usr/bin/env python3
"""Prepare and finalize a full mGBA screenshot refresh for the Zork trilogy.

Run ``prepare`` from a terminal, load the rebuilt ROM in mGBA, and execute the
printed Lua command in mGBA's Scripting window. After it reports completion,
run ``finalize`` to validate the capture report and rebuild the gallery.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
from datetime import datetime, timezone

ROOT = pathlib.Path(__file__).resolve().parents[2]
GALLERY = ROOT / 'qa' / 'mgba-room-gallery'
BUILD = ROOT / 'build' / 'mgba-room-captures'
ROM = ROOT / 'zork-trilogy.gba'
PLAN_BUILDER = GALLERY / 'build_plan.py'
FINALIZER = GALLERY / 'finalize_gallery.py'
CONTEXT = BUILD / 'refresh-context.json'
LUA_RUNNER = GALLERY / 'refresh-gallery.lua'


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def run(command: list[str]) -> None:
    subprocess.run(command, cwd=ROOT, check=True)


def prepare() -> int:
    BUILD.mkdir(parents=True, exist_ok=True)
    run(['make', 'zork-trilogy.gba'])
    run([sys.executable, str(PLAN_BUILDER)])

    with (GALLERY / 'manifest.csv').open(newline='') as stream:
        # Every record, including route replays, runs in the one full pass.
        expected_records = sum(1 for _ in csv.DictReader(stream))

    run_id = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    report_path = GALLERY / f'capture-results-refresh-{run_id}.csv'
    context = {
        'rom_sha256': sha256(ROM),
        'prepared_utc': datetime.now(timezone.utc).isoformat(),
        'expected_records': expected_records,
        'report_path': str(report_path),
    }
    CONTEXT.write_text(json.dumps(context, indent=2) + '\n')

    print('\nRefresh prepared.')
    print(f'ROM: {ROM}')
    print(f'SHA-256: {context["rom_sha256"]}')
    print(f'Capture records: {expected_records}')
    print('\nIn mGBA:')
    print(f'  1. Load this ROM: {ROM}')
    print('  2. Open the Scripting window and run:')
    print(f"     galleryCaptureRoot='{ROOT}'; galleryCaptureReportPath='{report_path}'; dofile('{LUA_RUNNER}')")
    print('  3. Wait for “GALLERY RUN COMPLETE” in the console.')
    print(f'  4. Run: python3 {pathlib.Path(__file__).relative_to(ROOT)} finalize')
    return 0


def finalize() -> int:
    if not CONTEXT.is_file():
        print('No prepared refresh found; run this script with “prepare” first.', file=sys.stderr)
        return 2
    context = json.loads(CONTEXT.read_text())
    report = pathlib.Path(context['report_path'])
    if not report.is_file():
        print('mGBA capture report is missing; run the generated Lua file in mGBA.', file=sys.stderr)
        return 2

    if sha256(ROM) != context['rom_sha256']:
        print('The ROM changed after preparation; prepare again before finalizing.', file=sys.stderr)
        return 2
    if report.stat().st_mtime_ns <= CONTEXT.stat().st_mtime_ns:
        print('The report predates this refresh; run the generated Lua file in mGBA.', file=sys.stderr)
        return 2

    with report.open(newline='') as stream:
        results = list(csv.DictReader(stream))
    if len(results) != context['expected_records']:
        print(
            f'Capture is incomplete: found {len(results)} of '
            f'{context["expected_records"]} records. Wait for mGBA to finish, then retry.',
            file=sys.stderr,
        )
        return 2

    shutil.copyfile(report, GALLERY / 'capture-results.csv')
    run([sys.executable, str(FINALIZER)])
    captured = sum(result['status'] == 'captured' for result in results)
    failed = len(results) - captured
    print(f'Gallery refreshed: {captured}/{len(results)} screenshots captured; {failed} failed.')
    return 0 if failed == 0 else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=('prepare', 'finalize'))
    args = parser.parse_args()
    return prepare() if args.phase == 'prepare' else finalize()


if __name__ == '__main__':
    raise SystemExit(main())
