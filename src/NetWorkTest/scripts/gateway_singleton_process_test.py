"""Exercise the unmodified production binaries: concurrent reuse, replacement, crash and restart."""
import argparse
import concurrent.futures
import json
from pathlib import Path
import re
import shutil
import socket
import subprocess
import threading
import time
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from client_background_process_test import Windows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    location = parser.add_mutually_exclusive_group(required=True)
    location.add_argument('--sdk', type=Path)
    location.add_argument('--bin', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    windows = Windows()
    windows.require_free_client()
    windows.require_free_client('NetServerManager')
    windows.require_free_client('NetClientManagerSupervisor')
    output = args.output.resolve()
    binaries = args.bin.resolve() if args.bin else args.sdk.resolve() / 'bin'
    output.mkdir(parents=True, exist_ok=False)
    # The no-argument server uses its public default port. Refuse to disturb an existing listener.
    check = socket.socket()
    check.bind(('0.0.0.0', 5203))
    check.close()
    results = []
    for role in ['NetServerManager', 'NetClientManager']:
        folder = output / role
        folder.mkdir()
        exe = folder / (role + '1.1.exe')
        shutil.copy2(binaries / exe.name, exe)
        guard = folder / 'NetClientManagerGuard1.1.exe'
        if role == 'NetClientManager':
            shutil.copy2(binaries / guard.name, guard)
        children = []
        def launch(name, arguments):
            log = (folder / (name + '.log')).open('wb')
            child = subprocess.Popen([str(exe)] + arguments, cwd=folder, stdout=log, stderr=subprocess.STDOUT,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            children.append((child, log))
            return child
        def ready(name, child):
            deadline = time.monotonic() + 12
            while time.monotonic() < deadline:
                text = (folder / (name + '.log')).read_text(errors='replace')
                if re.search(r'GATEWAY_READY .*pid=' + str(child.pid), text): return
                if child.poll() is not None: raise RuntimeError(name + ' exited before ready: ' + text)
                time.sleep(0.02)
            raise RuntimeError(name + ' readiness timeout')
        options = ['--run-for-ms=25000', '--stop-file=' + str(folder / 'old.stop')]
        if role == 'NetServerManager':
            options += ['--protocol=3', '--listen=127.0.0.1', '--advertise=127.0.0.1', '--port=0',
                        '--identity-directory=' + str(folder / 'old-identity')]
        else: options += ['--protocol=3', '--address=127.0.0.1', '--port=5203']
        try:
            old = launch('old', options)
            ready('old', old)
            def reuse(index):
                name = 'reuse-' + str(index)
                child = launch(name, ['127.0.0.1'] if role == 'NetClientManager' else ['--port=5204'])
                assert child.wait(timeout=4) == 0
                assert role + '_REUSED' in (folder / (name + '.log')).read_text()
            with concurrent.futures.ThreadPoolExecutor(8) as pool: list(pool.map(reuse, range(8)))
            assert old.poll() is None
            if role == 'NetClientManager':
                plain = launch('no-argument', [])
                assert plain.wait(timeout=4) == 0
                assert 'NetClientManager_REUSED' in (folder / 'no-argument.log').read_text()
                assert old.poll() is None
            newer = launch('replacement', ['--replace'] + options if role == 'NetClientManager' else [])
            assert old.wait(timeout=12) == 0
            ready('replacement', newer)
            assert old.pid != newer.pid
            newer.kill()  # Owned crash fixture: prove the kernel lease cannot remain stuck.
            newer.wait(timeout=4)
            if role == 'NetClientManager':
                # Ordinary launches defer to the existing guard during its backoff.
                reused = launch('restart', options)
                assert reused.wait(timeout=4) == 0
                assert role + '_REUSED' in (folder / 'restart.log').read_text()
                deadline = time.monotonic() + 8
                restored = []
                while time.monotonic() < deadline:
                    restored = [pid for pid in windows.windows(exe) if windows.ready(pid)]
                    if restored:
                        break
                    time.sleep(0.02)
                assert len(restored) == 1 and restored[0] not in (newer.pid, reused.pid)
                (folder / 'old.stop').write_text('stop')
                deadline = time.monotonic() + 5
                while (windows.windows(exe) or windows.windows(guard)) and time.monotonic() < deadline:
                    time.sleep(0.02)
                assert not windows.windows(exe) and not windows.windows(guard)
            else:
                restarted = launch('restart', options)
                ready('restart', restarted)
                (folder / 'old.stop').write_text('stop')
                assert restarted.wait(timeout=5) == 0
            results.append({'role': role, 'parallelReuses': 8, 'noArgumentReplacement': role == 'NetServerManager',
                            'noArgumentReuse': role == 'NetClientManager',
                            'crashReleasesLease': True, 'normalStop': True})
        finally:
            for path in (exe, guard):
                for window in windows.windows(path).values():
                    windows.message(window, 0x0010)
            for child, log in children:
                if child.poll() is None: child.kill()
                child.wait(timeout=5)
                log.close()
    (output / 'result.json').write_text(json.dumps({'pass': True, 'tests': results}, indent=2), encoding='utf-8')
    print('PASS production singleton reuse/replacement/crash for both gateways')


if __name__ == '__main__':
    main()