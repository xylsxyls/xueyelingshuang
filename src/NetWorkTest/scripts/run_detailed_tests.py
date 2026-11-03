"""Run bounded native/Wasm/network suites sequentially in an owned report directory."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time


def run_luma_server(server, test, hosts, output, log, deadline):
    """Exercise the existing C++ probe through owned diagnostic gateways only."""
    output.mkdir()
    begin = time.monotonic()
    body_end = begin + deadline - 15
    children = []
    report = {'pass': False, 'checks': [], 'error': '', 'cleanupErrors': []}
    flags = subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0

    def remaining(limit):
        seconds = min(limit, body_end - time.monotonic())
        if seconds <= 0:
            raise RuntimeError('Luma server fixture deadline exceeded')
        return seconds

    def launch(name, executable, arguments, resident=False):
        directory = output / name
        directory.mkdir()
        stop = directory / 'stop'
        command = [str(executable)] + arguments
        if resident:
            command += ['--stop-file=' + str(stop), '--run-for-ms=' + str(deadline * 1000)]
        path = directory / 'process.log'
        stream = path.open('wb')
        try:
            process = subprocess.Popen(command, cwd=directory, stdout=stream, stderr=subprocess.STDOUT,
                                       stdin=subprocess.DEVNULL, creationflags=flags)
        except OSError:
            stream.close()
            raise
        child = {'name': name, 'process': process, 'stop': stop if resident else None,
                 'log': path, 'stream': stream, 'command': command}
        children.append(child)
        return child

    def read_log(child):
        return child['log'].read_text(encoding='utf-8', errors='replace')

    def ready(child, pattern, pid_group):
        end = time.monotonic() + remaining(10)
        while time.monotonic() < end:
            if child['process'].poll() is not None:
                break
            match = re.search(pattern, read_log(child))
            if match and int(match.group(pid_group)) == child['process'].pid:
                return match
            time.sleep(0.02)
        raise RuntimeError('Missing ready identity: ' + str(child['log']))

    def probe(name, arguments, markers):
        child = launch(name, test, arguments)
        code = child['process'].wait(timeout=remaining(15))
        text = read_log(child)
        if code != 0 or any(marker not in text.splitlines() for marker in markers):
            raise RuntimeError(name + ' probe failed or missing assertions: ' + str(child['log']))

    try:
        probe('selection', [], ['Gateway selection 9/9'])
        report['checks'].append({'name': 'selection-boundaries', 'pass': True, 'cases': 9})
        gateway = launch('gateway', hosts / 'NetWorkTestNode.exe', ['--protocol=3', '--listen=127.0.0.1',
            '--advertise=127.0.0.1', '--port=0', '--seeds=127.0.0.1:1',
            '--identity-directory=' + str(output / 'gateway' / 'identity')], True)
        gateway_ready = ready(gateway, r'GATEWAY_READY port=(\d+) pid=(\d+)', 2)
        port = gateway_ready.group(1)
        service = launch('service', server, ['--gateway-pid=' + str(gateway['process'].pid)], True)
        ready(service, r'LUMA_SERVICE_READY endpoint=\d+ product=101 pid=(\d+)', 1)
        client = launch('client', hosts / 'NetWorkTestClient.exe', ['--protocol=3', '--address=127.0.0.1',
            '--port=' + port, '--seeds=127.0.0.1:' + port,
            '--cache-file=' + str(output / 'client' / 'nodes.cache')], True)
        ready(client, r'CLIENT_GATEWAY_READY pid=(\d+)', 1)
        probe('rpc', ['--gateway-pid=' + str(client['process'].pid)], ['Gateway selection 9/9',
            'PASS desktop IPC -> gateway -> LumaPlayerServer: failures/cancel=3/3'])
        if any(child['process'].poll() is not None for child in (gateway, service, client)):
            raise RuntimeError('Owned service/gateway exited during RPC/cancel')
        report['checks'].append({'name': 'explicit-pid-desktop-rpc-and-cancel', 'pass': True, 'terminals': 3})
        service['stop'].write_bytes(b'stop')
        if service['process'].wait(timeout=remaining(5)) != 0 or 'LUMA_SERVICE_STOPPED' not in read_log(service).splitlines():
            raise RuntimeError('Service did not report orderly stop')
        report['checks'].append({'name': 'service-orderly-stop', 'pass': True})
        report['pass'] = True
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as failure:
        report['error'] = str(failure)
    finally:
        # Signal all owned residents before waiting; never find/stop production processes by name.
        for child in reversed(children):
            if child['process'].poll() is None and child['stop'] is not None:
                try:
                    child['stop'].write_bytes(b'stop')
                except OSError as failure:
                    report['cleanupErrors'].append(child['name'] + ': ' + str(failure))
        cleanup_end = time.monotonic() + 10
        for child in reversed(children):
            process = child['process']
            try:
                process.wait(timeout=max(0.01, min(5, cleanup_end - time.monotonic())))
            except subprocess.TimeoutExpired:
                report['cleanupErrors'].append(child['name'] + ': owned child required termination')
                process.kill()
                process.wait(timeout=2)
            if process.returncode != 0:
                report['cleanupErrors'].append(child['name'] + ': exit=' + str(process.returncode))
            child['stream'].close()
        report['pass'] = report['pass'] and not report['cleanupErrors']
        if report['pass']:
            report['checks'].append({'name': 'diagnostic-hosts-orderly-stop', 'pass': True})
        report['children'] = [{'name': child['name'], 'pid': child['process'].pid,
            'command': child['command'], 'exitCode': child['process'].returncode, 'log': str(child['log'])}
            for child in children]
        report['seconds'] = round(time.monotonic() - begin, 3)
        payload = json.dumps(report, indent=2)
        (output / 'result.json').write_text(payload, encoding='utf-8')
        log.write(payload.encode('utf-8'))
    return 0 if report['pass'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin', type=Path, required=True)
    parser.add_argument('--module', type=Path, required=True)
    parser.add_argument('--account-module', type=Path, help='Independent production Account.js for account integration')
    parser.add_argument('--test-hosts', type=Path, help='build_gateway_test_hosts.py output for isolated gateway fixtures')
    parser.add_argument('--node', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--only', help='Comma-separated suite names; omitted runs all suites')
    parser.add_argument('--wasm-tests', type=Path, help='Built --tests --sanitizers Wasm module')
    parser.add_argument('--wx-cli', type=Path, help='Official WeChat developer tools cli.js')
    parser.add_argument('--wx-helper', type=Path, help='Defaults to CookWxTest bounded automation helper')
    parser.add_argument('--mysql-config', type=Path, help='Explicit local test MySQL credentials; random schema is used')
    parser.add_argument('--gui-test', type=Path, help='LumaPlayerTest executable; requires --qt')
    parser.add_argument('--luma-server', type=Path, help='LumaPlayerServer executable for the luma-server suite')
    parser.add_argument('--luma-server-test', type=Path, help='LumaPlayerServerTest executable for the luma-server suite')
    parser.add_argument('--luma-server-fixture', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--qt', type=Path, help='Matching Qt runtime root')
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        parser.error('Use a fresh report directory')
    if args.luma_server_fixture:
        if not args.luma_server or not args.luma_server_test or not args.test_hosts:
            parser.error('Internal fixture requires Luma executables and diagnostic hosts')
        return run_luma_server(args.luma_server.resolve(), args.luma_server_test.resolve(),
                               args.test_hosts.resolve(), output, sys.stdout.buffer, 55)
    root = Path(__file__).resolve().parents[3]
    scripts = Path(__file__).resolve().parent
    binaries = args.bin.resolve()
    module = args.module.resolve()
    node = str(args.node.resolve())
    server = str(args.test_hosts.resolve() / 'NetWorkTestNode.exe' if args.test_hosts else binaries / 'NetServerManager1.1.exe')
    client = str(args.test_hosts.resolve() / 'NetWorkTestClient.exe' if args.test_hosts else binaries / 'NetClientManager1.1.exe')
    test = str(binaries / 'NetWorkTest1.0.exe')
    # Full native suite includes bounded 35-second recovery plus IPC/process cases.
    commands = [('native', [test, '--all', '--report=' + str(output / 'native.csv')], 300)]
    # These old hosts directly call the removed NetInterface C ABI or old product adapters.
    # Keep the scenario sources for migration, but never report them as current SDK passes.
    missing = {name: 'Historical direct NetInterface C host; use current NetSender suites or migrate the product adapter'
        for name in ('tls', 'legacy-gateway', 'link', 'service', 'internal_service', 'managed_service',
            'compression', 'mesh', 'slow_channel', 'service_capacity', 'managed_cancel', 'channel_capacity',
            'window_rtt', 'recovery-entry', 'recovery-service', 'recovery-budget', 'account-recovery')}
    for name, filename in [('singleton', 'gateway_singleton_process_test.py'), ('replacement', 'gateway_replacement_process_test.py'),
                           ('client-background', 'client_background_process_test.py')]:
        extra = ['--server=' + str(args.test_hosts.resolve() / 'NetWorkTestNode.exe')] if name == 'client-background' and args.test_hosts else []
        commands.append((name, [sys.executable, str(scripts / filename), '--bin=' + str(binaries),
            '--output=' + str(output / name)] + extra, 70))
        if name == 'client-background' and not args.test_hosts:
            missing[name] = 'Requires --test-hosts to avoid touching a production server lease'

    def script(name, filename, options, deadline=80):
        commands.append((name, [node, str(scripts / filename)] + options, deadline))

    script('schedule', 'wx_schedule_test.js', [])
    script('recovery-callback', 'wx_recovery_callback_test.js', [])
    script('tls', 'tls_interop_test.js', ['--module=' + str(module), '--report=' + str(output / 'tls.json'), '--instant'])
    script('legacy-gateway', 'gateway_process_test.js', ['--module=' + str(module), '--executable=' + server,
           '--report=' + str(output / 'legacy-gateway.json'), '--instant'])
    script('identity', 'gateway_identity_process_test.js', ['--server=' + server, '--output=' + str(output / 'identity')])
    for name in ['link', 'service', 'internal_service', 'managed_service', 'compression', 'mesh', 'slow_channel']:
        filename = name + ('_process_test.js' if name in ['slow_channel'] else '_gateway_process_test.js')
        options = ['--server=' + server, '--module=' + str(module), '--output=' + str(output / name)]
        if name == 'service':
            options += ['--client=' + client, '--test=' + test]
        script(name, filename, options)
    for name in ['service_capacity', 'managed_cancel', 'channel_capacity', 'window_rtt']:
        script(name, name + '_process_test.js', ['--server=' + server, '--module=' + str(module),
               '--output=' + str(output / name)])
    for name, filename in [('local', 'local_link_gateway_process_test.js'), ('sender-facade', 'sender_c_api_process_test.js')]:
        script(name, filename, ['--server=' + server, '--client=' + client, '--test=' + test, '--output=' + str(output / name)])
    # This suite targets the native Peer and desktop callers; generated Peer JS recovery is covered by balanced-failover.
    for fault in ['entry', 'service']:
        script('recovery-' + fault, 'client_recovery_process_test.js', ['--server=' + server, '--client=' + client,
               '--test=' + test, '--module=' + str(module), '--output=' + str(output / ('recovery-' + fault)), '--fault=' + fault])
    script('file-host', 'wasm_file_flow_host_test.js', ['--module=' + str(module), '--report=' + str(output / 'file-host.json')])
    script('peer-contract', 'run_wasm_test.js', ['--module=' + str(module), '--peer-contract', '--report=' + str(output / 'peer-contract.json')])
    script('sender-delivery', 'run_wasm_test.js', ['--module=' + str(module), '--server=' + server, '--sender-delivery', '--report=' + str(output / 'sender-delivery/result.json')], 60)
    script('account-bridge', 'run_wasm_test.js', ['--module=' + str(module), '--account-module=' + str(args.account_module.resolve() if args.account_module else ''), '--account-bridge', '--report=' + str(output / 'account-bridge.json')])
    if not args.account_module:
        missing['account-bridge'] = 'Requires the matching independent --account-module'
    script('balanced-failover', 'balanced_failover_process_test.js', ['--server=' + server, '--module=' + str(module), '--output=' + str(output / 'balanced-failover')], 110)
    if not args.test_hosts:
        missing['balanced-failover'] = 'Requires diagnostic --test-hosts; never fault a production process'

    script('recovery-budget', 'recovery_budget_process_test.js', ['--server=' + server, '--test=' + test,
           '--module=' + str(module), '--output=' + str(output / 'recovery-budget')])
    for name, extra in [('legacy-desktop', []), ('legacy-file', ['--files'])]:
        script(name, 'desktop_gateway_process_test.js', ['--gateway=' + server, '--client-manager=' + client,
               '--probe=' + test, '--report=' + str(output / (name + '.json')), '--instant'] + extra)
    script('cluster', 'cluster_stress_test.js', ['--run', '--runtime=network', '--server=' + server, '--module=' + str(module),
           '--output=' + str(output / 'cluster')], 150)
    commands.append(('runner', [sys.executable, str(scripts / 'runner_selfcheck.py'), '--test=' + test,
                              '--output=' + str(output / 'runner')], 60))
    script('directory-cache', 'directory_cache_wasm_test.js', ['--server=' + server, '--module=' + str(module),
           '--output=' + str(output / 'directory-cache.json')], 100)
    script('wasm', 'run_wasm_test.js', ['--module=' + str(args.wasm_tests.resolve() if args.wasm_tests else ''),
           '--report=' + str(output / 'wasm.csv'), '--all'], 100)
    if not args.wasm_tests:
        missing['wasm'] = 'Requires --wasm-tests built with --tests --sanitizers'
    helper = (args.wx_helper or root / 'src/CookWxTest/lib/automationConnection.js').resolve()
    script('wx', 'wx_smoke_test.js', ['--module=' + str(module), '--output=' + str(output / 'wx'),
           '--cli=' + str(args.wx_cli.resolve() if args.wx_cli else ''), '--helper=' + str(helper)], 140)
    if not args.wx_cli:
        missing['wx'] = 'Requires --wx-cli and installed CookWxTest automation dependencies'
    luma = ['--bin=' + str(binaries), '--module=' + str(module), '--output=' + str(output / 'luma-account'),
            '--mysql-config=' + str(args.mysql_config.resolve() if args.mysql_config else '')]
    if args.account_module:
        luma += ['--account-module=' + str(args.account_module.resolve())]
    if args.gui_test:
        if not args.qt:
            parser.error('--gui-test requires --qt')
        luma += ['--gui-test=' + str(args.gui_test.resolve()), '--qt=' + str(args.qt.resolve())]
    script('luma-account', 'luma_account_process_test.js', luma, 100)
    if not args.mysql_config:
        missing['luma-account'] = 'Requires --mysql-config for an isolated local test instance'
    if not args.account_module:
        missing['luma-account'] = 'Requires independent --account-module and isolated --mysql-config'
    commands.append(('luma-server', [sys.executable, str(Path(__file__).resolve()), '--luma-server-fixture',
        '--bin=' + str(binaries), '--module=' + str(module), '--node=' + node,
        '--luma-server=' + str(args.luma_server.resolve() if args.luma_server else ''),
        '--luma-server-test=' + str(args.luma_server_test.resolve() if args.luma_server_test else ''),
        '--test-hosts=' + str(args.test_hosts.resolve() if args.test_hosts else ''),
        '--output=' + str(output / 'luma-server')], 60))
    if not args.luma_server or not args.luma_server_test or not args.test_hosts:
        missing['luma-server'] = 'Requires --luma-server, --luma-server-test and diagnostic --test-hosts'
    else:
        required = [args.luma_server.resolve(), args.luma_server_test.resolve(),
            args.test_hosts.resolve() / 'NetWorkTestNode.exe', args.test_hosts.resolve() / 'NetWorkTestClient.exe']
        absent = [str(path) for path in required if not path.is_file()]
        if absent:
            missing['luma-server'] = 'Missing fixture executable(s): ' + ', '.join(absent)
    if not args.test_hosts:
        for name in ['identity', 'mesh', 'slow_channel', 'recovery-entry', 'recovery-service', 'cluster', 'directory-cache']:
            missing[name] = 'Requires --test-hosts; production gateways are machine singletons'
    if args.only is not None and (not args.only or any(not item.strip() for item in args.only.split(','))):
        parser.error('--only must contain nonempty suite names')
    selected = {item.strip() for item in args.only.split(',')} if args.only is not None else {name for name, _, _ in commands}
    if selected - {name for name, _, _ in commands}:
        parser.error('Unknown suite name')
    output.mkdir(parents=True)
    identities = {}
    inputs = list(binaries.glob('*.exe')) + [module, module.with_suffix('.wasm')]
    # Evidence must identify the assertions/adapters as well as the executables.
    inputs += sorted(scripts.glob('*.py')) + sorted(scripts.glob('*.js'))
    inputs += sorted((root / 'src/NetWorkTest/scripts/host').glob('*.js'))
    inputs += sorted((root / 'src/NetWorkTest/testdata/tls').glob('*'))
    if args.test_hosts:
        inputs += list(args.test_hosts.resolve().glob('*.exe'))
    if args.account_module:
        inputs += [args.account_module.resolve(), args.account_module.resolve().with_suffix('.wasm')]
    if args.wasm_tests:
        inputs += [args.wasm_tests.resolve(), args.wasm_tests.resolve().with_suffix('.wasm')]
    if args.gui_test:
        inputs.append(args.gui_test.resolve())
    for executable in (args.luma_server, args.luma_server_test):
        if executable:
            inputs.append(executable.resolve())
    for file in inputs:
        if file.is_file():
            identities[str(file)] = hashlib.sha256(file.read_bytes()).hexdigest()
    (output / 'inputs.json').write_text(json.dumps({'sha256': identities, 'selected': sorted(selected),
        'guiRequested': bool(args.gui_test), 'python': sys.version}, indent=2), encoding='utf-8')
    results = []

    def save():
        complete = len(results) == len(selected) and all(r['state'] not in ('NOT_RUN', 'RUNNING') for r in results)
        (output / 'result.json').write_text(json.dumps({'pass': complete and all(r['pass'] for r in results),
            'complete': complete, 'selected': sorted(selected), 'results': results}, indent=2), encoding='utf-8')

    save()
    for name, command, deadline in commands:
        if name not in selected:
            continue
        if name in missing:
            results.append({'suite': name, 'state': 'NOT_RUN', 'pass': False, 'reason': missing[name]})
            save()
            print('NOT_RUN ' + name + ': ' + missing[name], flush=True)
            continue
        print('START ' + name, flush=True)
        begin = time.monotonic()
        result = {'suite': name, 'state': 'RUNNING', 'pass': False, 'command': command,
                  'deadlineSeconds': deadline}
        results.append(result)
        save()
        timed_out = False
        error = ''
        code = None
        with (output / (name + '.log')).open('wb') as log:
            try:
                process = subprocess.Popen(command, cwd=root, stdout=log, stderr=subprocess.STDOUT)
                code = process.wait(timeout=deadline)
            except subprocess.TimeoutExpired:
                timed_out = True
                # Only this runner's freshly spawned child and its descendants.
                if os.name == 'nt':
                    subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], stdout=log, stderr=log, timeout=15)
                else:
                    process.kill()
                code = process.wait(timeout=15)
            except OSError as failure:
                error = str(failure)
                log.write(error.encode('utf-8'))
        result.update({'pass': code == 0 and not timed_out, 'exitCode': code,
                  'deadlineExceeded': timed_out, 'error': error, 'seconds': round(time.monotonic() - begin, 3)})
        result['state'] = 'PASS' if result['pass'] else 'FAIL'
        save()
        print(('PASS ' if result['pass'] else 'FAIL ') + name + ' ' + str(result['seconds']) + 's', flush=True)
    return 0 if all(result['pass'] for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())