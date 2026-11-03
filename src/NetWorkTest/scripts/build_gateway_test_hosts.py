"""Build isolated multi-node test hosts; production gateway singleton policy remains enabled."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--msbuild', default='C:/Program Files (x86)/MSBuild/12.0/Bin/amd64/MSBuild.exe')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    sdk, output = root, args.output.resolve()
    if args.sdk and args.sdk.resolve() != root:
        parser.error('Test hosts consume the repository shared include/lib')
    for library in ('NetPeer', 'NetSender'):
        if not (sdk / 'lib' / (library + '.lib')).is_file():
            raise RuntimeError('Prepare the shared x64 Release libraries through version first')
    identity = {'version': 1, 'platform': 'x64', 'configuration': 'Release', 'runtime': 'MT'}
    output.mkdir(parents=True, exist_ok=True)
    for project, target in [('NetServerManager', 'NetWorkTestNode'), ('NetClientManager', 'NetWorkTestClient')]:
        values = {'Configuration': 'Release', 'Platform': 'x64', 'OutDir': str(output) + '/',
            'IntDir': str(output / 'obj' / target) + '/', 'TargetName': target, 'BuildProjectReferences': 'false',
            'NetGatewayTestInstance': 'true'}
        command = [args.msbuild, str(root / 'src' / project / project / (project + '.vcxproj')),
                   '/t:Build', '/m:1', '/nologo', '/v:minimal']
        command += ['/p:' + key + '=' + value for key, value in values.items()]
        with (output / (target + '.log')).open('wb') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, cwd=root)
        if result.returncode:
            raise RuntimeError('Build failed: ' + str(output / (target + '.log')))
        print(target + ' TEST HOST PASS', flush=True)
    paths = [path for owner in ('NetServerManager', 'NetClientManager')
             for path in (root / 'src' / owner / owner).glob('*') if path.suffix in ('.cpp', '.h')]
    (output / 'test-hosts.json').write_text(json.dumps({'sdk': str(sdk), 'sdkIdentity': identity,
        'sources': {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths},
        'binaries': {name: hashlib.sha256((output / name).read_bytes()).hexdigest()
                     for name in ('NetWorkTestNode.exe', 'NetWorkTestClient.exe')}}, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
