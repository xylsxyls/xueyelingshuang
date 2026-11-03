"""Audit or publish separate Windows x64 v3 gateway/business runtime directories."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import os
import ast
import re
import lzma
import tarfile

ROOT = Path(__file__).resolve().parents[3]
COOK_VERSION = (ROOT / 'src/CookServer/version.txt').read_text(encoding='utf-8').strip()
if not re.fullmatch(r'[0-9]+(?:\.[0-9]+)+', COOK_VERSION):
    raise RuntimeError('Invalid shared CookServer release version')
COOK_SERVER_EXE = 'CookServer' + COOK_VERSION + '.exe'
COOK_HTTP_EXE = 'CookHttpServer' + COOK_VERSION + '.exe'
FFMPEG_DLLS = ['avcodec-60.dll', 'avformat-60.dll', 'avutil-58.dll', 'swresample-4.dll', 'swscale-7.dll']
COOK_SOURCE_MATERIALS = {
    'ffmpeg': ['ffmpeg-6.1.6-source.tar.xz', 'LICENSE'],
    'openh264': ['openh264-2.6.0.tar.gz', 'LICENSE'],
    'mbedtls': ['mbedtls-3.6.7-source.tar.xz', 'LICENSE']}
SYSTEM_DLLS = set(('kernel32 user32 advapi32 shell32 shlwapi ole32 oleaut32 ws2_32 mswsock bcrypt crypt32 '
    'secur32 ntdll version winmm gdiplus dbghelp psapi rpcrt4 iphlpapi dnsapi normaliz comdlg32 '
    'comctl32 gdi32 winspool winhttp setupapi imm32 uxtheme d3d11 dxgi d3d9 dwmapi mpr netapi32 '
    'wtsapi32 powrprof propsys wininet wintrust cfgmgr32 userenv usp10 msimg32 opengl32 glu32 mfplat mfreadwrite').split())

def publish_cook_sources(target, report):
    staged = report / 'third-party-sources'
    staged.mkdir(parents=True, exist_ok=False)
    for library, names in COOK_SOURCE_MATERIALS.items():
        destination = staged / 'tools' / library
        destination.mkdir(parents=True)
        for name in names:
            shutil.copy2(ROOT / 'tools' / library / name, destination / name)
    pending = report / 'third-party-sources.tar.xz'
    expected = {p.relative_to(staged) for p in staged.rglob('*') if p.is_file()}
    with tarfile.open(pending, 'w:xz', preset=9 | lzma.PRESET_EXTREME) as bundle:
        for relative in sorted(expected):
            bundle.add(staged / relative, arcname=relative.as_posix(), recursive=False)
    with tarfile.open(pending, 'r:xz') as bundle:
        members = bundle.getmembers()
        if {Path(member.name) for member in members} != expected or any(not member.isfile() for member in members):
            raise RuntimeError('Source archive file list mismatch')
        for member in members:
            if hashlib.sha256(bundle.extractfile(member).read()).hexdigest().upper() != sha(staged / member.name):
                raise RuntimeError('Source archive roundtrip mismatch: ' + member.name)
    archive = target / pending.name
    if archive.exists():
        shutil.copy2(archive, report / ('previous-' + archive.name))
    os.replace(str(pending), str(archive))
    return {'path': str(archive.relative_to(ROOT)), 'sha256': sha(archive), 'files': len(expected)}


def pe_details(path, require_x64=True):
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 60)[0]
    machine = struct.unpack_from("<H", data, pe + 4)[0]
    if data[pe:pe + 4] != b"PE\0\0" or machine not in (0x14C, 0x8664) or (require_x64 and machine != 0x8664):
        raise RuntimeError("Not a Windows x64 module: " + str(path))
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    magic = struct.unpack_from("<H", data, optional)[0]
    if magic != (0x20B if machine == 0x8664 else 0x10B):
        raise RuntimeError("PE optional-header architecture mismatch: " + str(path))
    directories = optional + (112 if magic == 0x20B else 96)
    sections = []
    for index in range(count):
        size, rva, raw_size, raw = struct.unpack_from("<IIII", data, optional + optional_size + index * 40 + 8)
        sections.append((rva, max(size, raw_size), raw))
    def offset(rva):
        for start, size, raw in sections:
            if start <= rva < start + size:
                return raw + rva - start
        raise RuntimeError("Unmapped PE RVA in " + str(path))
    def string(rva):
        start = offset(rva)
        return data[start:data.index(b"\0", start)].decode("ascii")
    imports = []
    for directory, width, name_offset in ((1, 20, 12), (13, 32, 4)):
        rva, size = struct.unpack_from("<II", data, directories + directory * 8)
        if not rva:
            continue
        cursor = offset(rva)
        end = cursor + size
        while cursor + width <= end and any(data[cursor:cursor + width]):
            name = struct.unpack_from("<I", data, cursor + name_offset)[0]
            if directory == 13 and not (struct.unpack_from("<I", data, cursor)[0] & 1):
                raise RuntimeError("Unsupported absolute delay-import descriptor")
            imports.append(string(name))
            cursor += width
    identity = None
    rva, size = struct.unpack_from("<II", data, directories + 6 * 8)
    if rva:
        cursor = offset(rva)
        for index in range(size // 28):
            kind, length, pointer = (struct.unpack_from("<I", data, cursor + index * 28 + item)[0] for item in (12, 16, 24))
            if kind == 2 and length >= 24 and data[pointer:pointer + 4] == b"RSDS":
                identity = data[pointer + 4:pointer + 24]
    return imports, identity


def pdb_identity(path):
    data = path.read_bytes()
    if not data.startswith(b"Microsoft C/C++ MSF 7.00"):
        raise RuntimeError("Unsupported PDB: " + str(path))
    block, directory_size, block_map = (struct.unpack_from("<I", data, index)[0] for index in (32, 44, 52))
    page_count = (directory_size + block - 1) // block
    pages = struct.unpack_from("<" + "I" * page_count, data, block_map * block)
    directory = b"".join(data[page * block:(page + 1) * block] for page in pages)[:directory_size]
    streams = struct.unpack_from("<I", directory)[0]
    sizes = struct.unpack_from("<" + "I" * streams, directory, 4)
    cursor = 4 + 4 * streams
    for index, size in enumerate(sizes):
        count = 0 if size == 0xFFFFFFFF else (size + block - 1) // block
        pages = struct.unpack_from("<" + "I" * count, directory, cursor)
        cursor += 4 * count
        if index == 1:
            info = b"".join(data[page * block:(page + 1) * block] for page in pages)[:size]
            return info[12:28] + info[8:12]
    raise RuntimeError("Missing PDB identity")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def prepare_symbol_bundle(product, target, sources, report, tag):
    """Match both updated and retained modules; never substitute a same-name PDB."""
    deployed = {p.name: p for p in target.iterdir()
                if p.is_file() and p.suffix.lower() in ('.exe', '.dll')} if target.is_dir() else {}
    deployed.update(sources)
    staged = report / 'symbols' / product
    staged.mkdir(parents=True, exist_ok=False)
    expected = {}
    matched = {}
    missing = []
    for name, module in sorted(deployed.items()):
        _, identity = pe_details(module, require_x64=False)
        if identity is None:
            missing.append({'module': name, 'reason': 'Module has no RSDS identity'})
            continue
        expected[name] = identity
        pdb = module.with_suffix('.pdb')
        if pdb.is_file() and pdb_identity(pdb) == identity:
            destination = staged / Path(name).suffix[1:] / Path(name).with_suffix('.pdb').name
            destination.parent.mkdir(exist_ok=True)
            shutil.copy2(pdb, destination)
            matched[name] = destination
    # Read members without extracting archive-supplied paths. Old SDK/compiler PDBs
    # are not runtime symbols and must not replace a module's GUID/Age pair.
    wanted_names = {Path(name).with_suffix('.pdb').name.lower() for name in expected if name not in matched}
    scratch = report / ('symbol-candidate-' + product + '.pdb')
    try:
        for archive in sorted(target.glob('symbols-*.tar.xz')):
            if not wanted_names:
                break
            with tarfile.open(archive, 'r:xz') as bundle:
                for member in bundle:
                    if not member.isfile() or Path(member.name).name.lower() not in wanted_names:
                        continue
                    if member.size > 512 * 1024 * 1024:
                        raise RuntimeError('Oversize symbol archive member: ' + member.name)
                    scratch.write_bytes(bundle.extractfile(member).read())
                    identity = pdb_identity(scratch)
                    for name in expected:
                        if name not in matched and expected[name] == identity:
                            destination = staged / Path(name).suffix[1:] / Path(name).with_suffix('.pdb').name
                            destination.parent.mkdir(exist_ok=True)
                            shutil.copy2(scratch, destination)
                            matched[name] = destination
                    wanted_names = {Path(name).with_suffix('.pdb').name.lower() for name in expected if name not in matched}
    finally:
        if scratch.exists():
            scratch.unlink()
    for name in expected:
        if name not in matched:
            missing.append({'module': name, 'reason': 'Matching PDB unavailable in current module directory or previous symbols'})
    archive = report / ('symbols-' + product + '-' + tag + '.tar.xz')
    members = {}
    with tarfile.open(archive, 'w:xz', preset=9 | lzma.PRESET_EXTREME) as bundle:
        for name, pdb in sorted(matched.items()):
            # Separate kinds so foo.exe and foo.dll can retain their own symbols.
            relative = pdb.relative_to(staged).as_posix()
            members[relative] = {'module': name, 'sha256': sha(pdb), 'identity': expected[name].hex()}
            bundle.add(pdb, arcname=relative, recursive=False)
    with tarfile.open(archive, 'r:xz') as bundle:
        entries = bundle.getmembers()
        if len(entries) != len(members) or {x.name for x in entries} != set(members):
            raise RuntimeError('Symbol archive file list mismatch')
        for entry in entries:
            if not entry.isfile() or hashlib.sha256(bundle.extractfile(entry).read()).hexdigest().upper() != members[entry.name]['sha256']:
                raise RuntimeError('Symbol archive roundtrip mismatch')
    return {'archive': archive, 'sha256': sha(archive), 'members': members,
            'missingSymbols': missing, 'complete': not missing}


def deploy_files(changes, obsolete, report):
    """Back up the whole write set before replacing anything; roll back on failure."""
    backups = {}
    touched = []
    destinations = [destination for _, destination in changes] + list(obsolete)
    if len(set(destinations)) != len(destinations):
        raise RuntimeError('Duplicate release destination')
    for destination in destinations:
        if not destination.resolve().is_relative_to((ROOT / 'common').resolve()) or destination.is_symlink():
            raise RuntimeError('Release destination escaped common: ' + str(destination))
        backup = None
        if destination.exists():
            backup = report / 'previous' / destination.relative_to(ROOT / 'common')
            backup.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(destination, backup)
            if sha(destination) != sha(backup):
                raise RuntimeError('Release backup mismatch')
        backups[destination] = backup
    try:
        for source, destination in changes:
            destination.parent.mkdir(parents=True, exist_ok=True)
            pending = destination.with_name(destination.name + '.release-pending')
            if pending.exists():
                raise RuntimeError('Existing pending release: ' + str(pending))
            try:
                shutil.copy2(source, pending)
                if sha(source) != sha(pending):
                    raise RuntimeError('Staged bytes differ: ' + str(destination))
                os.replace(pending, destination)
                touched.append(destination)
                if sha(source) != sha(destination):
                    raise RuntimeError('Published bytes differ: ' + str(destination))
            finally:
                if pending.exists():
                    pending.unlink()
        for destination in obsolete:
            if destination.exists():
                destination.unlink()
                touched.append(destination)
    except Exception:
        rollback_errors = []
        for destination in reversed(touched):
            try:
                backup = backups[destination]
                if backup is None:
                    destination.unlink()
                else:
                    rollback = destination.with_name(destination.name + '.release-rollback')
                    shutil.copy2(backup, rollback)
                    os.replace(rollback, destination)
            except OSError as failure:
                rollback_errors.append({'path': str(destination), 'error': str(failure)})
        (report / 'rollback.json').write_text(json.dumps({'complete': not rollback_errors,
            'errors': rollback_errors}, indent=2), encoding='utf-8')
        raise


def prepare_cook_mysql_launchers(report):
    """Embed the maintained PowerShell tool; deployed CMDs need no Python/source tree."""
    payload = (ROOT / 'src/CookServer/CookServer/mysql_transfer.ps1').read_text(encoding='utf-8-sig')
    payload.encode('ascii')
    destination = report / 'cook-mysql-launchers'
    destination.mkdir(parents=True, exist_ok=False)
    files = []
    for action, name in [('Export', '\u5bfc\u51faMySQL.cmd'), ('Import', '\u6e05\u7a7a\u5e76\u5bfc\u5165MySQL.cmd')]:
        launcher = '''@echo off
setlocal DisableDelayedExpansion
set "MYSQL_TRANSFER_SCRIPT=%~f0"
set "MYSQL_TRANSFER_DIRECTORY=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "$text=[IO.File]::ReadAllText($env:MYSQL_TRANSFER_SCRIPT); $marker='#<MYSQL_TRANSFER_PAYLOAD>'; $payload=$text.Substring($text.LastIndexOf($marker)+$marker.Length); & ([scriptblock]::Create($payload)) -Action ''' + action + ''' -Directory $env:MYSQL_TRANSFER_DIRECTORY"
set "MYSQL_TRANSFER_EXIT=%ERRORLEVEL%"
if not "%MYSQL_TRANSFER_NO_PAUSE%"=="1" pause
exit /b %MYSQL_TRANSFER_EXIT%
#<MYSQL_TRANSFER_PAYLOAD>
'''
        file = destination / name
        file.write_bytes((launcher + payload).replace('\r\n', '\n').rstrip().replace('\n', '\r\n').encode('ascii'))
        files.append(file)
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--core-bin', type=Path, required=True)
    parser.add_argument('--luma-bin', type=Path)
    parser.add_argument('--luma-bridges', type=Path, help='Update only LumaPlayer C ABI bridges; preserve application EXEs and deploy NetClientManager separately')
    parser.add_argument('--luma-network-only', action='store_true', help='With --luma-bridges, update only NetSender.dll; client gateway belongs to its own directory')
    parser.add_argument('--luma-sender-only', action='store_true', help='With --luma-bridges, update only NetSender.dll; deploy the shared client gateway separately')
    parser.add_argument('--client-runtime', type=Path, help='Standard x64 DLL SDK; deploy the actual NetSender.dll runtime dependency closure')
    parser.add_argument('--client-products', nargs='+', choices=('LumaPlayer', 'PdfReader', 'PdfReaderServer', 'SplitViewer'))
    parser.add_argument('--report', type=Path, required=True, help='New audit/staging directory outside common; summarize results in product test documentation')
    parser.add_argument('--wasm-module', type=Path, help='Exact tested public module for the signing-key audit')
    parser.add_argument('--symbol-format', choices=('tar.xz',), default='tar.xz', help='Archive format for matching PDBs')
    parser.add_argument('--publish', action='store_true', help='Write audited artifacts into separate common directories')
    parser.add_argument('--products', nargs='+', choices=('NetServerManager', 'NetClientManager', 'NetWorkTest', 'AccountServer', 'CookServer', 'LumaPlayerServer'), help='Audit/publish only changed product closures')
    parser.add_argument('--symbol-tag', default='20261015-gateway-v3', help='Fresh symbol archive identity for this release')
    args = parser.parse_args()
    if bool(args.client_runtime) != bool(args.client_products):
        parser.error('--client-runtime and --client-products must be used together')
    if args.client_runtime and (args.luma_bin or args.luma_bridges):
        parser.error('--client-runtime cannot be combined with the Luma-specific inputs')
    if args.luma_bin and args.luma_bridges:
        parser.error('--luma-bin and --luma-bridges are mutually exclusive')
    if args.luma_network_only and not args.luma_bridges:
        parser.error('--luma-network-only requires --luma-bridges')
    if args.luma_sender_only and (not args.luma_bridges or args.luma_network_only):
        parser.error('--luma-sender-only requires --luma-bridges and excludes --luma-network-only')
    if not re.fullmatch(r'[A-Za-z0-9-]{1,80}', args.symbol_tag):
        parser.error('Invalid symbol tag')
    binaries = args.core_bin.resolve()
    report = args.report.resolve()
    if report.is_relative_to((ROOT / 'common').resolve()):
        parser.error('--report must be outside common; it contains temporary PDBs and previous runtime backups')
    report.mkdir(parents=True, exist_ok=False)
    modules = {
        'NetServerManager': ['NetServerManager1.1.exe'],
        'NetClientManager': ['NetClientManager1.1.exe', 'NetClientManagerGuard1.1.exe'],
        'NetWorkTest': ['NetWorkTest1.0.exe'],
        'AccountServer': ['AccountServer.exe', 'AccountServerTest.exe', 'MysqlCpp.dll', 'libmysql.dll'],
        'CookServer': [COOK_SERVER_EXE, COOK_HTTP_EXE, 'CookServerTest1.0.exe', 'HiRedis.dll', 'MysqlCpp.dll', 'libmysql.dll'] + FFMPEG_DLLS,
        'LumaPlayerServer': ['LumaPlayerServer.exe']}
    if args.products:
        modules = {product: names for product, names in modules.items() if product in args.products}
    sources = {(product, name): binaries / name for product, names in modules.items() for name in names}
    dynamic_products = set(args.client_products or [])
    if args.client_runtime:
        runtime = args.client_runtime.resolve()
        available = {path.name.lower(): path for path in runtime.glob('*.dll')}
        closure = {}
        def collect_client_runtime(name):
            key = name.lower()
            if key in closure:
                return
            source = available.get(key)
            if source is None:
                raise RuntimeError('Missing network runtime DLL: ' + name)
            imports, _ = pe_details(source)
            closure[key] = source
            for dependency in imports:
                if dependency.lower().removesuffix('.dll') not in SYSTEM_DLLS and not dependency.lower().startswith(('api-ms-win-', 'ext-ms-win-')):
                    collect_client_runtime(dependency)
        collect_client_runtime('NetSender.dll')
        for product in args.client_products:
            modules[product] = sorted(path.name for path in closure.values())
            for source in closure.values():
                sources[product, source.name] = source
    if args.luma_bin:
        modules['LumaPlayer'] = ['LumaPlayer1.0.exe', 'LumaPlayerTest1.0.exe', 'Account.dll', 'NetSender.dll']
        for name in modules['LumaPlayer']:
            sources['LumaPlayer', name] = binaries / name if name == 'NetClientManager1.1.exe' else args.luma_bin.resolve() / name
    elif args.luma_bridges:
        modules['LumaPlayer'] = (['NetSender.dll'] if args.luma_sender_only or args.luma_network_only else
            ['Account.dll', 'NetSender.dll'])
        for name in modules['LumaPlayer']:
            sources['LumaPlayer', name] = binaries / name if name == 'NetClientManager1.1.exe' else args.luma_bridges.resolve() / name
    notice = binaries / 'network-third-party.txt'
    if not notice.is_file(): raise RuntimeError('Missing network license notices')
    mysql = binaries / 'libmysql.dll'
    mysql_required = any('libmysql.dll' in names for names in modules.values())
    if mysql_required and sha(mysql) != 'EF10128A947DBC3942D49C19192FC64340033F131C95A065195559128ED42ADD':
        raise RuntimeError('Original libmysql.dll identity changed')
    signing_source = (ROOT / 'src/NetServerManager/NetServerManager/NetGatewayAuthority.cpp').read_text(encoding='utf-8-sig')
    signing_block = signing_source.split('g_gatewaySigningKey[] =', 1)[1].split(';', 1)[0]
    signing_key = ''.join(ast.literal_eval(part) for part in re.findall(r'"(?:[^"\\]|\\.)*"', signing_block)).encode('ascii')
    if len(signing_key) < 100: raise RuntimeError('Signing-key audit input unavailable')
    records = []
    symbols = {}
    for product, names in modules.items():
        symbols[product] = []
        for name in names:
            source = sources[product, name]
            imports, identity = pe_details(source)
            external = [value for value in imports if value.lower().removesuffix('.dll') not in SYSTEM_DLLS and
                not value.lower().startswith(('api-ms-win-', 'ext-ms-win-'))]
            allowed_external = set(value.lower() for value in FFMPEG_DLLS) if product == 'CookServer' else set()
            if product == 'AccountServer':
                # The connector's internal zlib is isolated in MysqlCpp.dll (network norm v4.6).
                allowed_external = {'mysqlcpp.dll'} if name.endswith('.exe') else {'libmysql.dll'} if name == 'MysqlCpp.dll' else set()
                if name.endswith('.exe') and set(value.lower() for value in external) != allowed_external:
                    raise RuntimeError('AccountServer runtime closure mismatch: ' + name + ': ' + str(external))
            if product in dynamic_products:
                allowed_external = set(value.lower() for value in names)
            if name == COOK_SERVER_EXE:
                allowed_external.update(('hiredis.dll', 'mysqlcpp.dll'))
            if product == 'CookServer' and name == 'MysqlCpp.dll':
                allowed_external.add('libmysql.dll')
            if product != 'LumaPlayer' and any(value.lower() not in allowed_external for value in external):
                if name != 'libmysql.dll': raise RuntimeError('Non-static runtime imports in ' + name + ': ' + str(external))
            if product not in ('LumaPlayer', 'CookServer', 'AccountServer') and product not in dynamic_products and name not in (COOK_SERVER_EXE, COOK_HTTP_EXE) and external:
                if name != 'libmysql.dll': raise RuntimeError('Unexpected third-party DLL dependency: ' + name)
            if name == COOK_SERVER_EXE and 'hiredis.dll' not in [value.lower() for value in external]:
                raise RuntimeError('CookServer must retain its HiRedis DLL import')
            if name == COOK_HTTP_EXE and external:
                raise RuntimeError('CookHttpServer must not depend on database or other third-party DLLs: ' + str(external))
            if name != 'NetServerManager1.1.exe' and signing_key in source.read_bytes():
                raise RuntimeError('Server-only CA signing key escaped into ' + name)
            pdb = source.with_suffix('.pdb')
            omit_vendor_pdb = product == 'CookServer' and name in FFMPEG_DLLS
            require_pdb = name != 'libmysql.dll' and not omit_vendor_pdb
            if require_pdb:
                if identity is None or not pdb.is_file() or pdb_identity(pdb) != identity:
                    raise RuntimeError('Missing or mismatched PDB: ' + str(source))
                symbols[product].append(pdb)
            records.append({'product': product, 'name': name, 'sha256': sha(source), 'bytes': source.stat().st_size,
                'imports': imports, 'pdbMatched': require_pdb, 'pdbOmittedByPolicy': omit_vendor_pdb})
    wasm_module = args.wasm_module.resolve() if args.wasm_module else ROOT / 'lib/NetPeer.wasm'
    if signing_key in wasm_module.read_bytes():
        raise RuntimeError('Server-only signing key found in public Wasm')
    symbol_archives = {}
    changes = []
    obsolete = []
    cook_mysql_launchers = prepare_cook_mysql_launchers(report) if 'CookServer' in modules else []
    for product, names in modules.items():
        target = ROOT / 'common' / product
        current_sources = {name: sources[product, name] for name in names}
        prepared = prepare_symbol_bundle(product, target, current_sources, report, args.symbol_tag)
        archive = target / prepared['archive'].name
        if archive.exists():
            raise RuntimeError('Use a fresh symbol archive identity: ' + str(archive))
        changes.extend((source, target / name) for name, source in current_sources.items())
        changes.append((prepared['archive'], archive))
        symbol_archives[product] = {key: value for key, value in prepared.items() if key != 'archive'}
        symbol_archives[product]['path'] = str(archive.relative_to(ROOT))
        obsolete.extend(path for path in target.glob('symbols-*') if path.is_file() and path != archive)
        if product != 'CookServer':
            changes.append((notice, target / notice.name))
        else:
            changes.extend((file, target / file.name) for file in cook_mysql_launchers)
            document = target / '\u5f00\u53d1\u6587\u6863.txt'
            text = document.read_text(encoding='utf-8-sig')
            marker = '\n\n[THIRD-PARTY-NOTICES]\n'
            text = text.split(marker, 1)[0] + marker + notice.read_text(encoding='utf-8-sig')
            staged_document = report / 'cook-development.txt'
            staged_document.write_bytes(b'\xef\xbb\xbf' + text.replace('\r\n', '\n').rstrip('\n').replace('\n', '\r\n').encode('utf-8'))
            changes.append((staged_document, document))
            obsolete.extend(path for path in [target / 'third-party-sources.tar.xz', target / 'third-party-sources.7z',
                target / 'network-third-party.txt'] if path.is_file())
        if product == 'LumaPlayer' and args.luma_bin:
            for name in ('setup.sh', 'LumaPlayer.iss'):
                changes.append((ROOT / 'src/LumaPlayer/LumaPlayer' / name, target / name))
    if args.publish:
        deploy_files(changes, obsolete, report)
    result = {'mode': 'published' if args.publish else 'audit-only', 'platform': 'Windows x64 Release v120',
        'modules': records, 'symbols': symbol_archives, 'originalMysqlPreserved': mysql_required, 'mysqlInScope': mysql_required,
        'auditedWasm': {'path': str(wasm_module), 'sha256': sha(wasm_module)},
        'clientSigningKeyAbsent': True, 'voiceEngine': 'Windows SAPI; optional system Kangkang',
        'missingVendorPdb': ['libmysql.dll (unchanged vendor original)'] if mysql_required else [],
        'omittedVendorPdb': FFMPEG_DLLS[:] if 'CookServer' in modules else [],
        'cookMysqlLaunchers': [{'name': file.name, 'sha256': sha(file), 'bytes': file.stat().st_size}
            for file in cook_mysql_launchers],
        'data': 'No database, INI, identity, cache or media copied or modified'}
    (report / 'release.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(('COMMON_RELEASE_OK ' if args.publish else 'RELEASE_AUDIT_OK ') + str(report), flush=True)


if __name__ == '__main__':
    main()