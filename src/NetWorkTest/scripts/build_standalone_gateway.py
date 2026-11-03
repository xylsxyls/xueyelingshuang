"""通过既有 CloudRebuild 流程准备网关/服务依赖，共用仓库 include/lib。"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
ZLIB_HEADERS = ROOT / 'tools/zlib/include'
if not (ZLIB_HEADERS / 'zlib.h').is_file():
    ZLIB_HEADERS = ZLIB_HEADERS / 'zlibwindows'
CORE = ['CStringManager', 'CSystem', 'AtomicMath', 'Semaphore', 'SharedMemory', 'ReadWriteMutex',
    'CTaskThreadManager', 'Compress', 'CorrespondParam', 'CRandom', 'ProcessWork', 'CDump',
    'Variant', 'ProtoMessage', 'NetChannel', 'NetInterface', 'LibuvTcp', 'NetPeer', 'NetWork', 'NetSender']
COOK = ['Cini', 'IntDateTime', 'LogManager', 'RapidJson', 'HttpService', 'MysqlCpp', 'FFmpegCpp']
FFMPEG_COMPONENTS = {'avcodec': '60', 'avformat': '60', 'avutil': '58', 'swresample': '4', 'swscale': '7'}



def copy_header_tree(source, target):
    """Overlay vendor headers on Python 3.6+, without copying Git metadata."""
    for directory, subdirectories, filenames in os.walk(str(source)):
        subdirectories[:] = [name for name in subdirectories if name != '.git']
        current = Path(directory)
        destination = target / current.relative_to(source)
        destination.mkdir(parents=True, exist_ok=True)
        for name in filenames:
            if name not in ('.gitignore', '.gitattributes'):
                shutil.copy2(str(current / name), str(destination / name))


def write_network_notices(output, ffmpeg_root=None):
    """保存静态第三方的现有许可正文；Mbed TLS明确选择Apache-2.0。"""
    sections = [('Protocol Buffers 3.3.0 (BSD)', ROOT / 'tools/protobuf/LICENSE'),
        ('Mbed TLS 3.6.7 (Apache-2.0 selected; upstream dual-license text follows)',
            ROOT / 'tools/mbedtls/LICENSE')]
    text = 'Static network dependencies: third-party notices\n\n'
    for name, source in sections:
        text += name + '\n' + source.read_text(encoding='utf-8') + '\n\n'
    for name, source in [('libuv (MIT)', ROOT / 'tools/libuv/include/uv.h'),
                         ('Compress API: prepared static zlib 1.3.2 (zlib license)',
                          ZLIB_HEADERS / 'zlib.h')]:
        text += name + '\n' + source.read_text(encoding='utf-8').split('*/', 1)[0] + '*/\n\n'
    if ffmpeg_root:
        for source in (ROOT / 'tools/openh264/LICENSE', ffmpeg_root / 'LICENSE'):
            text += '\n' + source.parent.name + ': ' + source.name + '\n' + source.read_text(encoding='utf-8') + '\n'
    (output / 'lib/network-third-party.txt').write_text(text.rstrip(), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--configuration', choices=('Release', 'Debug'), default='Release')
    parser.add_argument('--platform', choices=('x64', 'Win32'), default='x64')
    parser.add_argument('--scope', choices=('network', 'account', 'cook', 'all'), default='all')
    parser.add_argument('--dependencies-only', action='store_true')
    parser.add_argument('--applications-only', action='store_true')
    parser.add_argument('--ffmpeg-root', type=Path, default=ROOT / 'tools/ffmpeg',
        help='Prepared shared FFmpeg SDK with OpenH264/AAC; see the matching source archive')
    parser.add_argument('--libraries', nargs='+', choices=CORE + COOK + ['Account'],
        help='Selected libraries in dependency order (MysqlCpp uses its DLL); requires --dependencies-only')
    parser.add_argument('--projects', nargs='+', choices=('NetServerManager', 'NetClientManager', 'NetClientManagerGuard', 'NetWorkTest', 'AccountServer', 'AccountServerTest', 'CookHttpServer', 'CookServer', 'CookServerTest', 'LumaPlayerServer', 'SplitViewerServer', 'PdfReaderServer'))
    parser.add_argument('--msbuild', default='C:/Program Files (x86)/MSBuild/12.0/Bin/amd64/MSBuild.exe')
    args = parser.parse_args()
    if args.libraries and (not args.dependencies_only or args.applications_only):
        parser.error('--libraries requires --dependencies-only')
    if os.name != 'nt':
        parser.error('This entry builds Windows MSVC targets')
    output = ROOT
    if args.output and args.output.resolve() != ROOT:
        parser.error('Builds use the repository shared include/lib; --output must be the repository root')
    headers = output / 'include'
    libraries = output / 'lib'
    logs = ROOT / 'src/temp/SharedNetwork' / args.platform / args.configuration / 'logs'
    for path in (headers, libraries, logs):
        path.mkdir(parents=True, exist_ok=True)
    # Each existing before/rebuild/after chain prepares its own public headers.
    bits = '64' if args.platform == 'x64' else '32'
    arch = 'x64' if bits == '64' else 'x86'
    ffmpeg_libraries = args.ffmpeg_root.resolve() / 'lib' / arch
    if args.configuration == 'Debug':
        ffmpeg_libraries = ffmpeg_libraries / 'MD'
    if args.scope in ('cook', 'all'):
        for name, version in FFMPEG_COMPONENTS.items():
            for suffix in ('.lib', '-' + version + '.dll'):
                source = ffmpeg_libraries / (name + suffix)
                if not source.is_file():
                    parser.error('Missing shared FFmpeg SDK file: ' + str(source) + '; restore the matching prepared third-party SDK')
    # SDK identity is data. Build settings stay in the existing shared project
    # configuration, rather than generating another property sheet per SDK.
    (logs / 'network-sdk.json').write_text(json.dumps({'version': 1, 'platform': args.platform,
        'configuration': args.configuration, 'runtime': 'MT' if args.configuration == 'Release' else 'MDd'},
        ensure_ascii=False, indent=2), encoding='utf-8')
    def build(name, project, application=False):
        log = logs / (name + '.log')
        if not application or name in ('HiRedisDLL', 'MysqlCppDLL'):
            library = name[:-3] if name.endswith('DLL') else name
            linkage = 'dll' if application else 'lib'
            cloud = os.environ.get('CLOUD_REBUILD') or str(ROOT / 'common/CloudRebuild/CloudRebuild1.2.exe')
            command = [cloud, library, bits, linkage, args.configuration.lower(), 'only' if application else 'same']
        else:
            command = [args.msbuild, str(project), '/t:Build', '/m:1', '/nologo', '/v:minimal',
                '/p:Configuration=' + args.configuration, '/p:Platform=' + args.platform,
                '/p:BuildProjectReferences=false']
        with log.open('wb') as stream:
            result = subprocess.run(command, cwd=str(ROOT), stdout=stream, stderr=subprocess.STDOUT)
        print(name + (' DLL' if name.endswith('DLL') else ' BUILD') + (' PASS' if result.returncode == 0 else ' FAIL'), flush=True)
        if result.returncode:
            print(log.read_text(encoding='utf-8', errors='replace')[-7000:], flush=True)
            raise SystemExit(result.returncode)

    dependencies = CORE[:]
    if args.scope in ('account', 'all'):
        dependencies += ['Cini', 'MysqlCpp', 'Account']
    if args.scope in ('cook', 'all'):
        for name, version in FFMPEG_COMPONENTS.items():
            shutil.copy2(str(ffmpeg_libraries / (name + '.lib')), str(libraries / (name + '.lib')))
            runtime_name = name + '-' + version + '.dll'
            shutil.copy2(str(ffmpeg_libraries / runtime_name), str(output / 'lib' / runtime_name))
        dependencies += [name for name in COOK if name not in dependencies]
    if args.libraries:
        if not set(args.libraries).issubset(dependencies):
            parser.error('A selected library is outside the requested scope')
        dependencies = [name for name in dependencies if name in args.libraries]
    if not args.applications_only and 'Compress' in dependencies:
        # Copy the prepared vendor SDK; never compile or rename a private zlib.
        vendor = ROOT / 'tools/zlib/lib'
        if args.configuration == 'Debug':
            vendor = vendor / 'MD'
        stem = 'zlibstatic' + bits + ('d' if args.configuration == 'Debug' else '')
        for extension in ('.lib', '.pdb'):
            shutil.copy2(vendor / (stem + extension), libraries / (stem + extension))
        copy_header_tree(ROOT / 'tools/zlib/include/zlibwindows', headers / 'zlib')
    if not args.applications_only:
        suffix = 'd' if args.configuration == 'Debug' else ''
        if 'NetInterface' in dependencies:
            vendor = ROOT / 'tools/mbedtls/lib'
            if args.configuration == 'Debug':
                vendor = vendor / 'MD'
            name = 'mbedtlsstatic' + bits + suffix + '.lib'
            shutil.copy2(vendor / name, libraries / name)
            include = ROOT / 'tools/mbedtls/include/mbedtlswindows'
            copy_header_tree(include, headers / 'mbedtls')
        if 'ProtoMessage' in dependencies or 'NetInterface' in dependencies:
            name = 'libprotobuf-litestatic' + bits + suffix + '.lib'
            shutil.copy2(ROOT / 'tools/protobuf/lib' / name, libraries / name)
            copy_header_tree(ROOT / 'tools/protobuf/include/google', headers / 'protobuf/google')
    for name in ([] if args.applications_only else dependencies):
        if name == 'MysqlCpp':
            # Contain the connector's bundled zlib inside the DLL link unit.
            build('MysqlCppDLL', ROOT / 'src/MysqlCpp/MysqlCpp/vcxproj/MysqlCpp.vcxproj', True)
            shutil.copy2(ROOT / 'tools/mysqlcpp/lib' / arch / 'libmysql.dll', output / 'lib/libmysql.dll')
            continue
        project = ROOT / 'src' / name / name / 'vcxproj' / (name + '_lib.vcxproj')
        build(name, project)
    if args.scope in ('cook', 'all') and not args.applications_only:
        # 原Redis后端仍隔离在DLL内，只把它的仓库依赖静态合入，避免套带CStringManager.dll。
        build('HiRedisDLL', ROOT / 'src/HiRedis/HiRedis/vcxproj/HiRedis.vcxproj', True)
        shutil.copy2(ROOT / 'tools/mysqlcpp/lib' / arch / 'libmysql.dll', output / 'lib/libmysql.dll')
    if args.scope in ('account', 'all'):
        shutil.copy2(ROOT / 'tools/mysqlcpp/lib' / arch / 'libmysql.dll', output / 'lib/libmysql.dll')
    write_network_notices(output, args.ffmpeg_root if args.scope in ('cook', 'all') else None)
    if args.dependencies_only:
        return
    projects = args.projects or {
        'network': ['NetServerManager', 'NetClientManager'],
        'account': ['AccountServer', 'AccountServerTest'],
        'cook': ['CookHttpServer', 'CookServer', 'CookServerTest'],
        'all': ['NetServerManager', 'NetClientManager', 'AccountServer', 'AccountServerTest', 'CookHttpServer', 'CookServer', 'CookServerTest', 'LumaPlayerServer']
    }[args.scope]
    if 'CookServer' in projects and 'CookHttpServer' not in projects:
        projects = ['CookHttpServer'] + projects
    if 'NetClientManager' in projects and 'NetClientManagerGuard' not in projects:
        projects = ['NetClientManagerGuard'] + projects
    for name in projects:
        owner = 'NetClientManager' if name == 'NetClientManagerGuard' else name
        project = ROOT / 'src/CookServer/CookServer/CookHttpServer.vcxproj' if name == 'CookHttpServer' else ROOT / 'src' / owner / owner / (name + '.vcxproj')
        build(name, project, True)


if __name__ == '__main__':
    main()
