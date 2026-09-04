#!/usr/bin/env python3
from pathlib import Path
import sys

roots = [Path('include'), Path('src')]
banned = {
    'jni.h': 'JNI belongs in the Android adapter, not the portable frontend',
    'android/': 'Android framework/NDK platform headers are forbidden in the core',
    'vulkan/': 'renderer APIs are forbidden in the compiler core',
    'emscripten': 'web bindings are adapters, not compiler-core dependencies',
    'windows.h': 'platform headers are forbidden in the core',
    'sys/mman.h': 'page mapping belongs outside the compiler core',
    'PAGE_SIZE': 'the compiler core must not assume an Android/Linux memory page size',
    'std::filesystem': 'filesystem authority belongs to the host capability layer',
    'system(': 'process execution is forbidden in the compiler core',
    'popen(': 'process execution is forbidden in the compiler core',
}

failures = []
for root in roots:
    for path in sorted(root.rglob('*')):
        if not path.is_file() or path.suffix not in {'.hpp', '.h', '.cpp', '.cc'}:
            continue
        text = path.read_text(encoding='utf-8')
        for needle, reason in banned.items():
            if needle in text:
                failures.append(f'{path}: contains {needle!r}: {reason}')

if failures:
    print('\n'.join(failures), file=sys.stderr)
    raise SystemExit(1)
print('portable core boundary clean')
