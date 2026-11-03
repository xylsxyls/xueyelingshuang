"""Check the test runner using independent selection and failure expectations."""
import argparse
import csv
import json
import re
from pathlib import Path
import subprocess
import sys


def sender_public_contracts():
    """Reject drift in the complete public NetSender declaration set."""
    root = Path(__file__).resolve().parents[2]
    contracts = {}
    for owner in ('NetSender', 'NetPeer', 'NetWork'):
        text = (root / owner / owner / 'src/NetSender.h').read_text(encoding='utf-8-sig')
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
        text = text.split('public:', 1)[1].split('private:', 1)[0]
        text = re.sub(r'\b(?:NetPeerNetSender|NetWorkNetSender)\b', 'NetSender', text)
        declarations = [' '.join(item.split()) for item in text.split(';') if item.strip()]
        if len(declarations) != 27:
            raise RuntimeError(owner + ' must expose exactly 27 declared entries')
        contracts[owner] = declarations
    if contracts['NetSender'] != contracts['NetPeer'] or contracts['NetSender'] != contracts['NetWork']:
        raise RuntimeError('NetSender public names, parameter types/names/defaults or return qualifiers differ')
    return contracts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--test', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        parser.error('Use a fresh report directory')
    output.mkdir(parents=True)
    results = []
    try:
        contracts = sender_public_contracts()
        (output / 'sender-contracts.json').write_text(json.dumps(contracts, indent=2), encoding='utf8')
        results.append({'id': 900, 'name': 'identical complete public NetSender declarations', 'state': 'PASS'})
    except Exception as error:
        results.append({'id': 900, 'name': 'identical complete public NetSender declarations', 'state': 'FAIL', 'error': str(error)})

    def check(case_id, name, arguments, expected_code, expected_rows=None, listed=False):
        report = output / (str(case_id) + '.csv')
        command = [str(args.test.resolve())] + arguments
        if expected_rows is not None:
            command.append('--report=' + str(report))
        try:
            run = subprocess.run(command, cwd=output, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
            (output / (str(case_id) + '.log')).write_bytes(run.stdout)
            assert run.returncode == expected_code, 'exit {} != {}'.format(run.returncode, expected_code)
            if expected_rows is not None:
                with report.open(encoding='utf-8', newline='') as stream:
                    rows = [r for r in csv.DictReader(stream) if r['id'] != 'TOTAL']
                actual = [(int(r['id']), r['state']) for r in rows]
                assert actual == expected_rows, '{} != {}'.format(actual, expected_rows)
            if listed:
                ids = [int(line.split()[0]) for line in run.stdout.decode('utf-8').splitlines()]
                assert ids and ids == sorted(set(ids)), 'List must be unique and ascending'
            results.append({'id': case_id, 'name': name, 'state': 'PASS'})
        except Exception as error:
            results.append({'id': case_id, 'name': name, 'state': 'FAIL', 'error': str(error)})

    check(901, 'unique sorted list', ['--list'], 0, listed=True)
    check(902, 'deduplicated ascending selection', ['--cases=63,1,63'], 0, [(1, 'PASS'), (63, 'PASS')])
    invalid = [[], ['--cases='], ['--cases=1,'], ['--cases=1,,2'], ['--cases=3-1'],
               ['--cases=-1'], ['--cases=0'], ['--cases=9999'], ['--cases=1x'], ['--cases=1-1000000000'],
               ['--all', '--instant'], ['--cases=1', '--cases=2'], ['--list', '--all'],
               ['--runner-probe=cancel', '--all'], ['--unknown'], ['--cases=1', '--report='],
               ['--cases=1', '--report=x', '--report=y'], ['--runner-probe=fail', '--runner-probe=cancel']]
    for index, arguments in enumerate(invalid):
        check(910 + index, 'reject ' + repr(arguments), arguments, 2)
    check(940, 'injected failure is failure', ['--runner-probe=fail'], 1, [(9001, 'FAIL')])
    check(941, 'deadline is failure', ['--runner-probe=timeout'], 1, [(9001, 'FAIL')])
    check(942, 'cancellation is distinct', ['--runner-probe=cancel'], 1, [(9001, 'CANCELLED')])
    check(943, 'report open failure', ['--cases=1', '--report=' + str(output / 'absent/report.csv')], 3)
    # Do not start any product process: use an absent bin and only invalid/missing-dependency selections.
    batch = Path(__file__).resolve().with_name('run_detailed_tests.py')
    for case_id, selection, expected_code, expected_state in [
            (950, '', 2, None), (951, ',', 2, None), (952, 'unknown', 2, None),
            (953, 'wasm', 1, 'NOT_RUN'), (954, 'native', 1, 'FAIL'),
            (955, 'luma-server', 1, 'NOT_RUN')]:
        destination = output / ('batch-' + str(case_id))
        try:
            run = subprocess.run([sys.executable, str(batch), '--bin=' + str(output / 'absent-bin'),
                '--module=' + str(output / 'absent.js'), '--node=' + sys.executable,
                '--output=' + str(destination), '--only=' + selection], stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, timeout=15)
            (output / (str(case_id) + '.log')).write_bytes(run.stdout)
            assert run.returncode == expected_code, 'Incorrect batch exit code'
            if expected_state is None:
                assert not destination.exists(), 'Invalid selection created a report directory'
            else:
                result = json.loads((destination / 'result.json').read_text(encoding='utf-8'))
                assert result['pass'] is False and len(result['results']) == 1
                assert result['results'][0]['state'] == expected_state
                assert result['complete'] == (expected_state != 'NOT_RUN')
            results.append({'id': case_id, 'name': 'batch selection/dependency ' + repr(selection), 'state': 'PASS'})
        except Exception as error:
            results.append({'id': case_id, 'name': 'batch selection/dependency ' + repr(selection),
                            'state': 'FAIL', 'error': str(error)})
    passed = all(result['state'] == 'PASS' for result in results)
    (output / 'result.json').write_text(json.dumps({'pass': passed, 'complete': True, 'results': results}, indent=2), encoding='utf-8')
    for result in results:
        print('{} {} {} {}'.format(result['id'], result['state'], result['name'], result.get('error', '')))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())