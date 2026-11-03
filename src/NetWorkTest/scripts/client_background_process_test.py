"""Windows GUI startup, resident reuse, crash recovery, session end and reversible login registration."""
import argparse
import concurrent.futures
import ctypes
from ctypes import wintypes as wt
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time
import traceback


def console_probe(pid):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.FreeConsole()
    ctypes.set_last_error(0)
    attached = bool(kernel.AttachConsole(pid))
    error = ctypes.get_last_error()
    if attached:
        kernel.FreeConsole()
    print(json.dumps({'attached': attached, 'error': error}), flush=True)
    return 0 if not attached and error == 6 else 1


class Windows:
    def __init__(self):
        self.kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        self.user = ctypes.WinDLL('user32', use_last_error=True)
        self.kernel.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
        self.kernel.OpenProcess.restype = wt.HANDLE
        self.kernel.OpenEventW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
        self.kernel.OpenEventW.restype = wt.HANDLE
        self.kernel.CloseHandle.argtypes = [wt.HANDLE]
        self.kernel.OpenFileMappingW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
        self.kernel.OpenFileMappingW.restype = wt.HANDLE
        self.kernel.MapViewOfFile.argtypes = [wt.HANDLE, wt.DWORD, wt.DWORD, wt.DWORD, ctypes.c_size_t]
        self.kernel.MapViewOfFile.restype = wt.LPVOID
        self.kernel.UnmapViewOfFile.argtypes = [wt.LPCVOID]
        self.kernel.OpenMutexW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
        self.kernel.OpenMutexW.restype = wt.HANDLE
        self.kernel.ReleaseMutex.argtypes = [wt.HANDLE]
        self.kernel.TerminateProcess.argtypes = [wt.HANDLE, wt.UINT]
        self.kernel.WaitForSingleObject.argtypes = [wt.HANDLE, wt.DWORD]
        self.kernel.QueryFullProcessImageNameW.argtypes = [wt.HANDLE, wt.DWORD, wt.LPWSTR, ctypes.POINTER(wt.DWORD)]
        self.user.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
        self.user.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
        self.user.IsWindowVisible.argtypes = [wt.HWND]
        self.callback_type = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
        self.user.EnumWindows.argtypes = [self.callback_type, wt.LPARAM]
        self.user.SendMessageTimeoutW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM, wt.UINT, wt.UINT,
                                                ctypes.POINTER(ctypes.c_size_t)]
        self.user.SendMessageTimeoutW.restype = ctypes.c_size_t
        self.user.CreateWindowExW.argtypes = [wt.DWORD, wt.LPCWSTR, wt.LPCWSTR, wt.DWORD,
            ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wt.HWND, wt.HMENU, wt.HINSTANCE, wt.LPVOID]
        self.user.CreateWindowExW.restype = wt.HWND
        self.user.DestroyWindow.argtypes = [wt.HWND]
        # The isolated desktop can return FALSE/last-error 0 when its last window closes.
        # Keep an owned invisible host window so enumeration still has a valid desktop
        # entry; retain the strict API-success and gateway-visibility assertions below.
        self.anchor = self.user.CreateWindowExW(0, 'STATIC', 'Xyls gateway test host', 0,
            0, 0, 1, 1, None, None, None, None)
        assert self.anchor and not self.user.IsWindowVisible(self.anchor), 'Cannot create the hidden test host'

    def __del__(self):
        anchor = getattr(self, 'anchor', None)
        if anchor:
            self.user.DestroyWindow(anchor)
            self.anchor = None

    def require_free_client(self, name='NetClientManager'):
        ctypes.set_last_error(0)
        handle = self.kernel.OpenMutexW(0x100001, False, 'Global\\xyls.' + name + '.owner')
        if not handle:
            assert ctypes.get_last_error() == 2, 'Cannot prove the client lease is free'
            return
        try:
            result = self.kernel.WaitForSingleObject(handle, 0)
            if result in (0, 0x80):
                self.kernel.ReleaseMutex(handle)
            assert result in (0, 0x80), 'Existing ' + name + ': refuse to touch its lease or process'
        finally:
            self.kernel.CloseHandle(handle)

    def image(self, pid):
        handle = self.kernel.OpenProcess(0x1000, False, pid)
        if not handle:
            return None
        try:
            text = ctypes.create_unicode_buffer(32768)
            size = wt.DWORD(len(text))
            return Path(text.value).resolve() if self.kernel.QueryFullProcessImageNameW(handle, 0, text, ctypes.byref(size)) else None
        finally:
            self.kernel.CloseHandle(handle)

    def windows(self, executable):
        found = {}
        @self.callback_type
        def callback(window, _):
            name = ctypes.create_unicode_buffer(100)
            self.user.GetClassNameW(window, name, len(name))
            if name.value == 'XylsNetClientManagerLifetime':
                pid = wt.DWORD()
                self.user.GetWindowThreadProcessId(window, ctypes.byref(pid))
                if self.image(pid.value) == executable.resolve():
                    assert not self.user.IsWindowVisible(window), 'Gateway/guard has a visible window'
                    found[pid.value] = window
            return True
        assert self.user.EnumWindows(callback, 0), 'EnumWindows failed: ' + str(ctypes.get_last_error())
        return found

    def ready(self, pid):
        handle = self.kernel.OpenEventW(0x100000, False, 'Global\\xyls.NetClientManager.ready.' + str(pid))
        if not handle:
            return False
        try:
            return self.kernel.WaitForSingleObject(handle, 0) == 0
        finally:
            self.kernel.CloseHandle(handle)

    def process_work_pid(self, name):
        # Read-only white-box check of the current ProcessWork name directory.
        # Production gateway code must use its public IPC/lease APIs, not this layout.
        encoded = name.encode('utf-8')
        assert 0 < len(encoded) < 1024 and b'\0' not in encoded
        digest = 14695981039346656037
        for byte in encoded:
            digest = ((digest ^ byte) * 1099511628211) & ((1 << 64) - 1)
        ctypes.set_last_error(0)
        handle = self.kernel.OpenFileMappingW(4, False, 'P2N' + str(digest))
        if not handle:
            assert ctypes.get_last_error() == 2, 'Cannot inspect ProcessWork name mapping'
            return None
        view = None
        try:
            view = self.kernel.MapViewOfFile(handle, 4, 0, 0, 1040)
            assert view
            pid, length, identity, full_name = struct.unpack('<iiQ1024s', ctypes.string_at(view, 1040))
            assert length == len(encoded) and full_name[:length] == encoded and full_name[length] == 0
            assert pid == ctypes.cast(view, ctypes.POINTER(ctypes.c_int32)).contents.value
            assert pid > 0 and identity != 0
            return pid
        finally:
            if view:
                self.kernel.UnmapViewOfFile(view)
            self.kernel.CloseHandle(handle)

    def message(self, window, message, value=0):
        result = ctypes.c_size_t()
        assert self.user.SendMessageTimeoutW(window, message, value, 0, 2, 5000, ctypes.byref(result)), 'Session handler timed out'
        return result.value

    def crash_owned(self, pid, executable):
        assert self.image(pid) == executable.resolve(), 'Refuse to terminate a different executable'
        handle = self.kernel.OpenProcess(1, False, pid)
        assert handle
        try:
            assert self.kernel.TerminateProcess(handle, 1)
        finally:
            self.kernel.CloseHandle(handle)


def wait_for(predicate, seconds=10):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        result = predicate()
        if result:
            return result
        time.sleep(0.02)
    raise AssertionError('Condition deadline exceeded')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin', type=Path, required=True)
    parser.add_argument('--server', type=Path, required=True, help='Independent NetWorkTestNode.exe, never a production server')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--business-repeats', type=int, default=2)
    parser.add_argument('--skip-login-startup', action='store_true', help='Runtime-only qualification; login entry explicitly NOT_RUN')
    args = parser.parse_args()
    assert 2 <= args.business_repeats <= 256
    assert os.name == 'nt', 'Windows lifecycle requires Windows; do not report a skipped test as PASS'
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    binaries = args.bin.resolve()
    test_server = args.server.resolve()
    assert test_server.name == 'NetWorkTestNode.exe', 'Use the separately compiled isolated server fixture'
    folder = output / 'runtime with spaces'
    folder.mkdir()
    exe = folder / 'NetClientManager1.1.exe'
    shutil.copy2(binaries / exe.name, exe)
    guard_exe = folder / 'NetClientManagerGuard1.1.exe'
    if (binaries / guard_exe.name).is_file():
        shutil.copy2(binaries / guard_exe.name, guard_exe)
    windows = Windows()
    windows.require_free_client()
    windows.require_free_client('NetClientManagerSupervisor')
    children = []
    owned_paths = [exe, guard_exe]
    registry_backup = None
    registry_touched = False
    results = {}

    def launch(binary, options, name, normal=False):
        log = (output / (name + '.log')).open('wb')
        child = subprocess.Popen([str(binary)] + options, cwd=output, stdout=log, stderr=subprocess.STDOUT,
                                 creationflags=0 if normal else subprocess.CREATE_NO_WINDOW)
        children.append((child, log))
        return child

    def text(name):
        return (output / (name + '.log')).read_text(encoding='utf-8', errors='replace')

    def logged(name, expression, seconds=10):
        return wait_for(lambda: re.search(expression, text(name)), seconds)

    def close_owned(executable=exe):
        for window in windows.windows(executable).values():
            try:
                windows.message(window, 0x0010)  # WM_CLOSE to only this test's exact executable path.
            except AssertionError:
                pass
        wait_for(lambda: not windows.windows(executable), 7)

    try:
        data = exe.read_bytes()
        offset = struct.unpack_from('<I', data, 0x3c)[0]
        assert data[offset:offset + 4] == b'PE\0\0'
        assert struct.unpack_from('<H', data, offset + 24 + 68)[0] == 2, 'Windows GUI subsystem required; console baseline must fail'
        results['guiSubsystem'] = True
        server = launch(test_server, ['--protocol=3', '--listen=127.0.0.1', '--port=0',
            '--identity-directory=' + str(output / 'identity'), '--stop-file=' + str(output / 'server.stop'),
            '--run-for-ms=180000'], 'server')
        server_ready = logged('server', r'GATEWAY_READY port=(\d+) pid=(\d+)')
        service = launch(test_server, ['--self-test-service=' + server_ready[2],
            '--stop-file=' + str(output / 'service.stop'), '--run-for-ms=180000'], 'service')
        logged('service', r'SERVICE_PROBE_READY pid=(\d+)')
        client = launch(exe, ['--background', '--protocol=3', '--address=127.0.0.1', '--port=' + server_ready[1],
            '--cache-file=' + str(output / 'client.cache'), '--stop-file=' + str(output / 'client.stop'),
            '--run-for-ms=180000'], 'client', normal=True)
        logged('client', 'CLIENT_GATEWAY_READY pid=' + str(client.pid))
        guard_pid = int(logged('client', r'CLIENT_GATEWAY_WATCHDOG pid=(\d+)')[1])
        owned = windows.windows(exe)
        assert windows.image(guard_pid) == guard_exe.resolve(), 'Watchdog must use its own NetClientManagerGuard1.1.exe image'
        assert set(owned) == {client.pid} and set(windows.windows(guard_exe)) == {guard_pid}
        assert not windows.ready(guard_pid), 'Guard must never be discovered as a ready business gateway'
        assert windows.process_work_pid(exe.stem) == client.pid
        assert windows.process_work_pid(guard_exe.stem) is None, 'Guard must not register a ProcessWork receiver'
        results['processWorkNameTargetsGatewayAndGuardHasNoInbox'] = True
        results['distinctGatewayAndGuardProcessNames'] = True
        probe = launch(Path(sys.executable), [str(Path(__file__).resolve()), '--probe-console=' + str(client.pid)], 'console')
        assert probe.wait(timeout=5) == 0, text('console')
        results['normalCreateHasNoConsoleOrVisibleWindow'] = True

        def reuse(index):
            child = launch(exe, [] if index % 2 == 0 else ['--background'], 'reuse-' + str(index), normal=True)
            assert child.wait(timeout=5) == 0
            assert 'NetClientManager_REUSED' in text('reuse-' + str(index))
        with concurrent.futures.ThreadPoolExecutor(8) as pool:
            list(pool.map(reuse, range(8)))
        assert client.poll() is None and windows.ready(client.pid)
        results['concurrentNoArgumentAndBackgroundReuse'] = 8
        for index in range(args.business_repeats):
            name = 'business-%03d' % index
            business = launch(binaries / 'NetWorkTest1.0.exe', ['--c-service-client=0'], name)
            code = business.wait(timeout=12)
            assert code == 0, 'Business exit code %d: %s' % (code, text(name))
            assert 'CPP_SENDER_CLIENT_PASS' in text(name)
            time.sleep(0.15)
            assert client.poll() is None and windows.ready(client.pid), 'Business exit stopped the resident gateway'
        results['residentAfterAllBusinessExitAndReusable'] = True
        results['completedBusinessProcesses'] = args.business_repeats

        endpoint = launch(binaries / 'NetWorkTest1.0.exe', ['--recovery-desktop=0'], 'recovery')
        first = logged('recovery', r'RECOVERY_ROUND count=1 generation=(\d+)')
        crashed_at = time.monotonic()
        client.kill()
        client.wait(timeout=4)
        # IPC death is sampled within 500 ms. The committed tail then has a
        # separate fixed 5 s drain deadline, so closed is not a 3 s promise.
        logged('recovery', r'RECOVERY_CLOSED generation=\d+ reason=-?\d+', 7)
        notified_ms = round((time.monotonic() - crashed_at) * 1000)
        # The current SDK can return the resident guard during backoff, or the
        # already recovered gateway. Neither PID should be expected to exit.
        during_backoff = launch(binaries / 'NetWorkTest1.0.exe', ['--client-launch-probe'], 'backoff-business')
        assert during_backoff.wait(timeout=5) == 0, text('backoff-business')
        triggered = re.search(r'CLIENT_LAUNCH result=(-?\d+) pid=(\d+)', text('backoff-business'))
        assert triggered and int(triggered[1]) == 0
        reported = int(triggered[2])
        assert reported > 0 and windows.image(reported) in (guard_exe.resolve(), exe.resolve())
        if windows.image(reported) == guard_exe.resolve():
            assert reported == guard_pid, 'SDK selected a different supervisor'
        results['existingSdkStartupDuringBackoffPreservesOriginalConfiguration'] = True
        second = logged('recovery', r'RECOVERY_ROUND count=2 generation=(\d+)', 15)
        assert int(second[1]) > int(first[1])
        assert endpoint.wait(timeout=5) == 0, text('recovery')
        recovered = wait_for(lambda: [pid for pid in windows.windows(exe) if windows.ready(pid)])
        assert len(recovered) == 1 and recovered[0] not in (client.pid, guard_pid)
        assert set(windows.windows(guard_exe)) == {guard_pid}, 'Restart created a guard chain'
        assert set(windows.windows(exe)) == set(recovered)
        assert windows.process_work_pid(exe.stem) == recovered[0]
        assert windows.process_work_pid(guard_exe.stem) is None
        results['crashRestartWithOriginalLocalConfiguration'] = True
        results['closedCallbackObservedMs'] = notified_ms
        results['newGenerationReal65537ByteRoundTrip'] = True

        before = recovered[0]
        windows.crash_owned(before, exe)
        recovered = wait_for(lambda: [pid for pid in windows.windows(exe) if pid != before and windows.ready(pid)], 10)
        assert len(recovered) == 1 and set(windows.windows(guard_exe)) == {guard_pid} and len(windows.windows(exe)) == 1
        again = launch(binaries / 'NetWorkTest1.0.exe', ['--c-service-client=0'], 'business-after-second-crash')
        assert again.wait(timeout=12) == 0, text('business-after-second-crash')
        results['secondCrashRecoversWithSameGuardAndRealRoundTrip'] = True

        window = windows.windows(exe)[recovered[0]]
        assert windows.message(window, 0x0011) == 1  # WM_QUERYENDSESSION
        windows.message(window, 0x0016, 0)  # canceled WM_ENDSESSION
        assert windows.ready(recovered[0])
        windows.message(window, 0x0016, 1)
        wait_for(lambda: not windows.windows(exe), 7)
        wait_for(lambda: not windows.windows(guard_exe), 7)
        time.sleep(1.2)
        assert not windows.windows(exe) and not windows.windows(guard_exe), 'Normal session end restarted the gateway'
        results['canceledSessionRetainedAndConfirmedSessionStoppedWithoutRestart'] = True

        rejected = launch(exe, ['--watch-parent=1'], 'wrong-guard-entry')
        assert rejected.wait(timeout=5) == 2 and not windows.windows(exe)
        for index, arguments in enumerate([[], ['--background'], ['--watch-parent=0'], ['--watch-parent=abc']]):
            rejected = launch(guard_exe, arguments, 'invalid-guard-' + str(index))
            assert rejected.wait(timeout=5) == 2 and not windows.windows(guard_exe)
        results['gatewayRejectsGuardRoleAndGuardRejectsBusinessLaunch'] = True

        layouts = []
        for layout in ['sibling', 'flat-root', 'missing']:
            root = output / ('deployment-' + layout + '-中文 空格')
            base = root / 'Business' if layout == 'sibling' else root
            base.mkdir(parents=True)
            business = base / 'Business.exe'
            shutil.copy2(binaries / 'NetWorkTest1.0.exe', business)
            target = root / 'NetClientManager' / exe.name if layout == 'sibling' else base / exe.name
            if layout != 'missing':
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(exe, target)
                owned_paths.append(target)
                target_guard = target.with_name(guard_exe.name)
                shutil.copy2(guard_exe, target_guard)
                owned_paths.append(target_guard)
                if layout == 'sibling':
                    # A stale private copy must never win over the required sibling directory.
                    (base / exe.name).write_bytes(b'This private copy must not be launched')
            process = launch(business, ['--client-launch-probe'], 'layout-' + layout)
            assert process.wait(timeout=6) == 0, text('layout-' + layout)
            result = re.search(r'CLIENT_LAUNCH result=(-?\d+) pid=(\d+)', text('layout-' + layout))
            assert result
            if layout == 'missing':
                assert int(result[1]) != 0 and int(result[2]) == 0
            else:
                assert int(result[1]) == 0 and int(result[2]) > 0
                pid = int(result[2])
                wait_for(lambda: windows.ready(pid), 6)
                assert windows.image(pid) == target.resolve()
                assert set(windows.windows(target)) == {pid} and len(windows.windows(target_guard)) == 1
                close_owned(target)
                wait_for(lambda: not windows.windows(target_guard), 7)
            layouts.append(layout)
        results['unicodeSpaceDeploymentBusinessExitAndMissingGateway'] = layouts

        incomplete = output / 'missing guard'
        incomplete.mkdir()
        incomplete_exe = incomplete / exe.name
        shutil.copy2(exe, incomplete_exe)
        owned_paths.append(incomplete_exe)
        missing_guard = launch(incomplete_exe, ['--address=127.0.0.1', '--port=' + server_ready[1]], 'missing-guard')
        assert missing_guard.wait(timeout=5) == 1
        assert 'missing NetClientManagerGuard1.1.exe' in text('missing-guard') and not windows.windows(incomplete_exe)
        results['missingGuardFailsWithoutReadyOrSameNameFallback'] = True

        if not args.skip_login_startup:
            # Back up only this tool's value and restore it even if install/remove assertions fail.
            import winreg
            key_path = r'Software\Microsoft\Windows\CurrentVersion\Run'
            with winreg.CreateKey(winreg.HKEY_CURRENT_USER, key_path) as key:
                try:
                    registry_backup = winreg.QueryValueEx(key, 'NetClientManager')
                except FileNotFoundError:
                    registry_backup = None
            registry_touched = True
            install = launch(exe, ['--install-autostart'], 'install')
            assert install.wait(timeout=5) == 0, text('install')
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path) as key:
                value, kind = winreg.QueryValueEx(key, 'NetClientManager')
            assert kind == winreg.REG_SZ and value == '"' + str(exe) + '" --background', value
            for index in range(2):
                remove = launch(exe, ['--uninstall-autostart'], 'remove-' + str(index))
                assert remove.wait(timeout=5) == 0, text('remove-' + str(index))
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path) as key:
                try:
                    winreg.QueryValueEx(key, 'NetClientManager')
                except FileNotFoundError:
                    pass
                else:
                    raise AssertionError('Startup entry was not removed')
            results['quotedLoginEntryInstalledReadBackRemovedIdempotently'] = True
        else:
            results['loginStartupQualification'] = 'NOT_RUN'
        for name in ['service', 'server']:
            (output / (name + '.stop')).write_text('stop')
        assert service.wait(timeout=5) == server.wait(timeout=5) == 0
        (output / 'result.json').write_text(json.dumps({'pass': True, 'tests': results}, indent=2), encoding='utf-8')
        print('PASS GUI resident/reuse/crash/closed callback/recovery/session end/login startup', flush=True)
    except BaseException:
        (output / 'failure.json').write_text(json.dumps({'error': traceback.format_exc(), 'completed': results,
            'children': [{'pid': child.pid, 'exitCode': child.poll(), 'log': log.name} for child, log in children]}, indent=2), encoding='utf-8')
        raise
    finally:
        if registry_touched:
            import winreg
            with winreg.CreateKey(winreg.HKEY_CURRENT_USER, r'Software\Microsoft\Windows\CurrentVersion\Run') as key:
                if registry_backup is None:
                    try:
                        winreg.DeleteValue(key, 'NetClientManager')
                    except FileNotFoundError:
                        pass
                else:
                    winreg.SetValueEx(key, 'NetClientManager', 0, registry_backup[1], registry_backup[0])
        try:
            for path in owned_paths:
                close_owned(path)
        finally:
            for child, log in children:
                if child.poll() is None:
                    child.kill()
                child.wait(timeout=5)
                log.close()


if __name__ == '__main__':
    if len(sys.argv) == 2 and sys.argv[1].startswith('--probe-console='):
        raise SystemExit(console_probe(int(sys.argv[1].split('=', 1)[1])))
    main()