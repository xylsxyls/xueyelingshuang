#!python3
# -*- coding:utf-8 -*-
import os
import sys
import ast
import subprocess

if sys.version_info < (3, 8):
    sys.stderr.write('Build scripts require Python >= 3.8\n')
    sys.exit(2)

if os.name != 'nt':
    sys.stderr.write('This rebuild entry is Windows-only; use scripts/CMakeLists.txt on other platforms.\n')
    sys.exit(2)

CLOUD_REBUILD_WASM_ENTRY = True

def StopBuildProcess(process):
    """Reap this build and its descendants; POSIX callers start a private process group."""
    if process.poll() is not None:
        return
    try:
        if os.name == 'nt':
            command = os.path.join(os.environ.get('SystemRoot', r'C:\Windows'), 'System32', 'taskkill.exe')
            subprocess.run([command, '/PID', str(process.pid), '/T', '/F'], stdout=subprocess.DEVNULL,
                           stderr=subprocess.STDOUT, timeout=15, check=False)
        else:
            import signal
            os.killpg(process.pid, signal.SIGKILL)
    except (OSError, subprocess.TimeoutExpired):
        process.kill()
    finally:
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=10)

# Windows Wasm settings; other platforms declare their own CMake target.
WASM_SDK_SETTINGS = {
    "WASM_SDK_KIND": [
        "static"
    ],
    "WASM_SDK_LINKAGES": [
        "lib",
        "dll"
    ]
}

# Wasm tool/SDK helpers. Importing this script only exposes checks, never a build.
wasm_arguments = len(sys.argv) > 1 and (sys.argv[1].lower().startswith('wasm') or (len(sys.argv) > 2 and sys.argv[2].lower() == 'wasm'))
if __name__ != '__main__' or wasm_arguments:
    import argparse
    import hashlib
    import json
    from pathlib import Path
    import re
    import shutil
    import subprocess
    import tempfile
    sys.dont_write_bytecode = True

    def managed_home(root):
        configured = os.environ.get('CLOUD_WASM_HOME')
        if configured: return Path(configured).expanduser().resolve()
        parent = Path(root.anchor) if os.name == 'nt' else Path.home()
        return parent / 'BuildTools' / 'WebAssembly'


    def locate_sdk(root):
        explicit = os.environ.get('EMSDK')
        if explicit:
            candidate = Path(explicit).expanduser().resolve()
            if not (candidate / 'upstream/emscripten/emcc.py').is_file():
                raise ValueError('Set EMSDK to the SDK root, not its emscripten subdirectory: ' + str(candidate))
            return candidate
        candidates = [managed_home(root) / 'emsdk-3.1.74']
        for name in ('emcc', 'emcc.bat'):
            found = shutil.which(name)
            if found: candidates.append(Path(found).resolve().parents[2])
        for candidate in candidates:
            if (candidate / 'upstream/emscripten/emcc.py').is_file(): return candidate
        raise ValueError('Set EMSDK to an activated Emscripten 3.1.74 SDK, or prepare it under ' +
                         str(candidates[0]) + '. Install and activate the SDK before compiling.')


    def signatures(root, library, exports=None):
        markers = sdk_setting(library, 'WASM_BUILD_EXPORT_MARKERS') or []
        if exports is None and not markers: return []
        source = root / 'src' / library / library / 'src'
        files = [file for file in source.iterdir() if file.suffix in ('.h', '.hpp', '.cpp', '.cc', '.cxx', '.c')]
        for header in sdk_setting(library, 'WASM_BUILD_ABI_HEADERS') or []:
            name, relative = header.split('/', 1)
            files.append(root / 'src' / name / name / 'src' / relative)
        # Read referenced source declarations, including their ABI typedefs. Do not
        # guess types from function names or read potentially stale installed headers.
        pending, visited = list(files), set()
        while pending:
            file = pending.pop().resolve()
            if file in visited: continue
            visited.add(file)
            content = file.read_text(encoding='utf-8-sig')
            for include in re.findall(r'^\s*#include\s*[<"]([^>"]+)[>"]', content, re.M):
                candidates = [file.parent / include]
                if '/' in include:
                    owner, relative = include.split('/', 1)
                    candidates.append(root / 'src' / owner / owner / 'src' / relative)
                for candidate in candidates:
                    if candidate.is_file():
                        pending.append(candidate)
                        break
        text = '\n'.join(file.read_text(encoding='utf-8-sig') for file in sorted(visited))
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
        # Function aliases are declaration data, independent of project or function names.
        aliases = {}
        for original, target in re.findall(r'^\s*#define\s+(\w+)\s+(\w+)\s*$', text, re.M):
            aliases.setdefault(original, set()).add(target)

        def function_name(name):
            seen = set()
            while name in aliases:
                if name in seen: raise ValueError('Circular C ABI alias: ' + name)
                # Conditional headers also declare export/class macros. Only aliases
                # reached from an actual C function participate in its ABI identity.
                if len(aliases[name]) != 1: raise ValueError('Conflicting C ABI alias: ' + name)
                seen.add(name)
                name = next(iter(aliases[name]))
            return name

        text = re.sub(r'^\s*#.*$', '', text, flags=re.M)
        pointers = set(re.findall(r'\btypedef\s+(?:struct\s+)?\w+\s*\*\s*(\w+)\s*;', text))
        pointers.update(re.findall(r'\btypedef\s+[^;{}]*?\(\s*\*\s*(\w+)\s*\)\s*\([^;{}]*\)\s*;', text))
        pointers.add('size_t')
        scalars = {'void', 'bool', '_Bool', 'char', 'signed char', 'unsigned char',
                   'short', 'short int', 'unsigned short', 'unsigned short int',
                   'int', 'unsigned', 'unsigned int', 'long', 'long int', 'unsigned long',
                   'unsigned long int', 'long long', 'long long int', 'unsigned long long',
                   'unsigned long long int', 'float', 'double', 'intptr_t', 'uintptr_t'}
        scalars.update(sign + 'int' + str(width) + '_t' for sign in ('', 'u') for width in (8, 16, 32, 64))

        def conversion(declaration, argument):
            value = declaration.strip()
            if argument: value = re.sub(r'\b\w+\s*$', '', value).strip()
            value = re.sub(r'\b(?:const|volatile|restrict)\b', '', value).strip()
            if '*' in value or value in pointers: return 'p'
            if value in scalars: return '_'
            raise ValueError('Unclassified declared C ABI type: ' + declaration)

        # The compiler reads source marks; this table preserves the existing binding's pointer ABI.
        declarations = {}
        if exports is None:
            pattern = (r'\b(?:' + '|'.join(map(re.escape, markers)) + r'|EMSCRIPTEN_KEEPALIVE)\s+'
                       r'(?:[A-Za-z_]\w*API\s+)?'
                       r'([\w\s*]+?)\s+(\w+)\s*\(([^()]*?)\)')
            for returned, name, parameters in re.findall(pattern, text):
                declarations.setdefault(function_name(name), []).append((returned.strip(), parameters))
            names = set(re.findall(r'\b(?:' + '|'.join(map(re.escape, markers)) +
                                   r'|EMSCRIPTEN_KEEPALIVE)\s+[\w\s*]+?\s+(\w+)\s*\(', text))
            names = set(function_name(name) for name in names)
            if names - declarations.keys():
                raise ValueError('Explicit simple C ABI declaration required: ' + ', '.join(sorted(names - declarations.keys())))
        else:
            for name in exports:
                pattern = r'\b(void|int32_t|uint32_t|const\s+(?:char|uint8_t)\s*\*)\s*\b' + re.escape(name) + r'\s*\(([^()]*)\)'
                declarations[name] = re.findall(pattern, text)
                if not declarations[name]: raise ValueError('No simple C ABI declaration for ' + name)
        result = []
        for name, matches in sorted(declarations.items()):
            found = set()
            for returned, parameters in matches:
                signature = conversion(returned, False)
                for parameter in parameters.split(','):
                    if parameter.strip() not in ('', 'void'): signature += conversion(parameter, True)
                found.add(signature)
            if len(found) != 1: raise ValueError('Conflicting C ABI declarations: ' + name)
            value = found.pop()
            if 'p' in value: result.append(name + ':' + value)
        return result


    ROOT = Path(__file__).resolve().parents[3]
    SDK_ROOT = ROOT
    BUILD_LINKAGES = {}
    MARKER = '\n// SDK identity: '

    def archive_bits(payload):
        """Check every relocatable Wasm object, including its declared Memory64 ABI."""
        if not payload.startswith(b'!<arch>\n'):
            raise ValueError('Expected a Wasm static archive')
        offset, targets = 8, set()
        while offset < len(payload):
            header = payload[offset:offset + 60]
            if len(header) != 60 or header[58:] != b'`\n': raise ValueError('Invalid archive member header')
            try: size = int(header[48:58].strip())
            except ValueError: raise ValueError('Invalid archive member size')
            offset += 60
            if size < 0 or offset + size > len(payload): raise ValueError('Truncated archive member')
            body = payload[offset:offset + size]
            offset += size + size % 2
            if offset > len(payload): raise ValueError('Truncated archive padding')
            name = header[:16].strip()
            if name in (b'/', b'//', b'/SYM64/'): continue
            if name.startswith(b'#1/'):
                length = int(name[3:])
                if length > len(body): raise ValueError('Invalid archive long name')
                body = body[length:]
            if body[:8] != b'\x00asm\x01\x00\x00\x00':
                raise ValueError('Archive member is not a relocatable Wasm object')
            cursor, features = [8], None
            def number(end):
                value, shift = 0, 0
                while cursor[0] < end and shift < 35:
                    byte = body[cursor[0]]
                    cursor[0] += 1
                    value |= (byte & 127) << shift
                    if not byte & 128: return value
                    shift += 7
                raise ValueError('Invalid Wasm object section length')
            def string(end):
                length = number(end)
                if cursor[0] + length > end: raise ValueError('Truncated Wasm object string')
                value = body[cursor[0]:cursor[0] + length]
                cursor[0] += length
                return value
            while cursor[0] < len(body):
                section = body[cursor[0]]
                cursor[0] += 1
                end = number(len(body)) + cursor[0]
                if end > len(body): raise ValueError('Truncated Wasm object section')
                if section == 0 and string(end) == b'target_features':
                    if features is not None: raise ValueError('Duplicate Wasm target features')
                    features = set()
                    for _ in range(number(end)):
                        if cursor[0] >= end or body[cursor[0]] not in (43, 45): raise ValueError('Invalid Wasm target feature')
                        enabled = body[cursor[0]] == 43
                        cursor[0] += 1
                        feature = string(end)
                        if enabled: features.add(feature)
                    if cursor[0] != end: raise ValueError('Invalid Wasm target features length')
                cursor[0] = end
            if features is None: raise ValueError('Missing Wasm object target features')
            targets.add(64 if b'memory64' in features else 32)
        if len(targets) != 1: raise ValueError('Empty or mixed-ABI Wasm archive')
        return targets.pop()


    def sdk_kind(library, linkage=None):
        linkage = linkage or BUILD_LINKAGES.get(library)
        if linkage is not None:
            if linkage not in ('lib', 'dll'): raise ValueError('Invalid Wasm linkage: ' + linkage)
            return 'static' if linkage == 'lib' else 'module'
        kind = sdk_setting(library, 'WASM_SDK_KIND') or ['module']
        if kind not in (['module'], ['static']): raise ValueError('Invalid WASM_SDK_KIND for ' + library)
        return kind[0]


    def artifact_digest(data):
        return data['archiveSha256'] if data.get('kind') == 'static' else data['wasmSha256']


    def validate_vendor_identity(data):
        for relative, expected in data.get('vendorSha256', {}).items():
            source = (ROOT / relative).resolve()
            if ROOT / 'tools' not in source.parents:
                raise ValueError('Vendor identity outside tools: ' + relative)
            prepared = SDK_ROOT / 'lib' / source.name
            if (hashlib.sha256(source.read_bytes()).hexdigest() != expected or
                    hashlib.sha256(prepared.read_bytes()).hexdigest() != expected):
                raise ValueError('Rebuild SDK: vendor identity changed: ' + relative)


    def wasm_bits(payload):
        if payload[:8] != b'\x00asm\x01\x00\x00\x00':
            raise ValueError('Expected a Wasm v1 module')
        cursor = [8]
        memories = []
        def number(end):
            result, shift = 0, 0
            while cursor[0] < end and shift < 70:
                value = payload[cursor[0]]
                cursor[0] += 1
                result |= (value & 127) << shift
                if not value & 128: return result
                shift += 7
            raise ValueError('Invalid Wasm section length')
        while cursor[0] < len(payload):
            section = payload[cursor[0]]
            cursor[0] += 1
            size = number(len(payload))
            end = cursor[0] + size
            if end > len(payload): raise ValueError('Truncated Wasm section')
            if section == 5:
                count = number(end)
                if count != 1: raise ValueError('SDK requires exactly one defined linear memory')
                flags = number(end)
                if flags & 2: raise ValueError('Shared Wasm memory is not enabled for this SDK')
                number(end)
                if flags & 1: number(end)
                memories.append(64 if flags & 4 else 32)
            cursor[0] = end
        if len(memories) != 1: raise ValueError('SDK must define its own linear memory')
        return memories[0]


    def identity(js):
        text = js.read_text(encoding='utf-8')
        if MARKER not in text:
            raise ValueError('Missing SDK identity: ' + str(js))
        lines = text.rsplit(MARKER, 1)[1].splitlines()
        if not lines: raise ValueError('Empty SDK identity: ' + str(js))
        data = json.loads(lines[0])
        if not isinstance(data, dict) or not {'library', 'bits', 'configuration', 'wasm', 'wasmSha256', 'dependencies'}.issubset(data):
            raise ValueError('Invalid SDK identity: ' + str(js))
        return data


    def validate(library, bit, mode, active=None, linkage=None):
        if not library.isidentifier(): raise ValueError('Invalid SDK library name')
        active = set() if active is None else active
        if library in active: raise ValueError('Circular SDK identity dependency')
        active.add(library)
        js = SDK_ROOT / 'include' / library / (library + 'API.js')
        metadata = js.with_name(library + 'WasmSDK.json')
        if sdk_kind(library, linkage) == 'static':
            data = json.loads(metadata.read_text(encoding='utf-8'))
            expected = library + ('d' if mode == 'debug' else '') + '.a'
            required = {'kind', 'library', 'bits', 'configuration', 'archive', 'archiveSha256', 'dependencies'}
            if not isinstance(data, dict) or not required.issubset(data) or data['kind'] != 'static':
                raise ValueError('Invalid static SDK identity: ' + str(metadata))
            if (data['library'] != library or data['bits'] != bit or data['configuration'] != mode or
                data['archive'] != expected or not isinstance(data['dependencies'], dict)):
                raise ValueError('Static SDK target/configuration mismatch: ' + str(metadata))
            archive = SDK_ROOT / 'lib' / expected
            payload = archive.read_bytes()
            if archive_bits(payload) != bit or hashlib.sha256(payload).hexdigest() != data['archiveSha256']:
                raise ValueError('Static SDK archive identity mismatch: ' + str(archive))
            validate_vendor_identity(data)
            for dep, digest in data['dependencies'].items():
                if artifact_digest(validate(dep, bit, mode, active, data.get('dependencyLinkages', {}).get(dep))) != digest:
                    raise ValueError('Rebuild ' + library + ': dependency changed: ' + dep)
            active.remove(library)
            return data
        data = identity(js)
        expected = library + ('d' if mode == 'debug' else '') + '.wasm'
        if data['wasm'] != expected or not isinstance(data['dependencies'], dict):
            raise ValueError('Invalid SDK dependency/name identity: ' + str(js))
        wasm = SDK_ROOT / 'lib' / expected
        if data['library'] != library or data['bits'] != bit or data['configuration'] != mode:
            raise ValueError('SDK target/configuration mismatch: ' + str(js))
        payload = wasm.read_bytes()
        if wasm_bits(payload) != bit or hashlib.sha256(payload).hexdigest() != data['wasmSha256']:
            raise ValueError('JS/Wasm identity mismatch: ' + str(wasm))
        validate_vendor_identity(data)
        alias = data.get('entryAlias')
        if alias is not None:
            expected_alias = sdk_setting(library, 'WASM_SDK_ENTRY_ALIAS') or []
            if expected_alias != [alias] or not alias.isidentifier() or alias == library:
                raise ValueError('Invalid public SDK entry alias: ' + str(alias))
            entry = SDK_ROOT / 'include' / alias / (alias + 'API.js')
            if not entry.is_file() or hashlib.sha256(entry.read_bytes()).hexdigest() != data.get('entryAliasSha256'):
                raise ValueError('SDK entry alias identity mismatch: ' + str(entry))
        for name, digest in data.get('adapters', {}).items():
            if Path(name).name != name or not name.endswith('.js') or hashlib.sha256(
                    (js.parent / name).read_bytes()).hexdigest() != digest:
                raise ValueError('SDK adapter identity mismatch: ' + name)
        for dep, digest in data['dependencies'].items():
            current = validate(dep, bit, mode, active, 'dll')
            if current.get('kind') == 'static' or current['wasmSha256'] != digest:
                raise ValueError('Rebuild ' + library + ': dependency changed: ' + dep)
        active.remove(library)
        return data


    def sdk_setting(library, name):
        """Read Windows entry settings without loading another entry or CMake."""
        entry = ROOT / 'src' / library / 'scripts' / ('rebuild_' + library + '.py')
        if not entry.is_file(): return None
        tree = ast.parse(entry.read_text(encoding='utf-8-sig'), filename=str(entry))
        declarations = [node.value for node in tree.body if isinstance(node, ast.Assign)
                        and any(isinstance(target, ast.Name) and target.id == 'WASM_SDK_SETTINGS' for target in node.targets)]
        if not declarations: return None
        if len(declarations) != 1: raise ValueError('Duplicate Windows SDK settings: ' + str(entry))
        values = ast.literal_eval(declarations[0]).get(name)
        if values is not None and (not isinstance(values, list) or not all(isinstance(value, str) for value in values)):
            raise ValueError('Expected literal Windows SDK settings for ' + name)
        return values


    def protocol_constants(library):
        output = sdk_setting(library, 'WASM_SDK_PROTOCOL_OUTPUT')
        if output is None: return {}
        if len(output) != 1 or output[0] not in (library + 'Protocol.js', library + 'GatewayProtocol.js'):
            raise ValueError('Invalid SDK protocol output for ' + library)
        prefix = sdk_setting(library, 'WASM_SDK_PROTOCOL_PREFIX')
        headers = sdk_setting(library, 'WASM_SDK_PROTOCOL_HEADERS')
        if not prefix or len(prefix) != 1 or not prefix[0].isidentifier() or not headers:
            raise ValueError('Incomplete SDK protocol declarations for ' + library)
        values = {}
        for header in headers:
            parts = Path(header).parts
            if len(parts) < 2 or not parts[0].isidentifier() or '..' in parts:
                raise ValueError('Invalid SDK protocol header: ' + header)
            path = ROOT / 'src' / parts[0] / parts[0] / 'src' / Path(*parts[1:])
            path.resolve().relative_to((ROOT / 'src').resolve())
            pattern = r'\b(' + re.escape(prefix[0]) + r'[A-Z0-9_]+)\s*=\s*(0x[0-9a-fA-F]+|[0-9]+)[Uu]?\b'
            for name, value in re.findall(pattern, path.read_text(encoding='utf-8-sig')):
                number = int(value, 16 if value.lower().startswith('0x') else 10)
                if name in values and values[name] != number: raise ValueError('Conflicting SDK constant: ' + name)
                values[name] = number
        required = set(sdk_setting(library, 'WASM_SDK_PROTOCOL_REQUIRED') or [])
        if not values or not required.issubset(values):
            raise ValueError('Incomplete ' + library + ' protocol constants: ' + ', '.join(sorted(required - values.keys())))
        body = "'use strict';\n// Generated from SDK CMake declarations and C/C++ headers.\nmodule.exports = Object.freeze({\n"
        body += ',\n'.join('    ' + name + ': ' + str(value) for name, value in values.items()) + '\n});'
        return {ROOT / 'include' / library / output[0]: body.encode('utf-8')}


    def prepared_inputs(library):
        """Read actual before-script results, or the last published SDK receipt for only."""
        session = os.environ.get('CLOUD_WASM_SESSION')
        if session:
            state = json.loads(Path(session).read_text(encoding='utf-8'))
            node = state['nodes'].get(library)
            if node is not None: return node['inputs']
        public = SDK_ROOT / 'include' / library
        metadata = public / (library + 'WasmSDK.json')
        js = public / (library + 'API.js')
        linkage = BUILD_LINKAGES.get(library)
        if linkage == 'lib' or linkage is None and metadata.is_file():
            data = json.loads(metadata.read_text(encoding='utf-8'))
        elif js.is_file():
            data = identity(js)
        else:
            raise ValueError('No prepared dependency receipt for ' + library + '; run same or librely first')
        if 'preparedInputs' not in data:
            raise ValueError('Old dependency receipt for ' + library + '; run same or librely once to migrate')
        return data['preparedInputs']


    def dependencies(library):
        return list(prepared_inputs(library)['dependencies'])


    def runtime_dependencies(library):
        return [name for name, kind in prepared_inputs(library)['dependencies'].items() if kind == 'dll']


    def vendor_libraries(library, bit, mode='release'):
        result = {}
        for name in prepared_inputs(library)['vendors']:
            path = (ROOT / name).resolve()
            if ROOT / 'tools' not in path.parents or 'wasm' not in path.parts:
                raise ValueError('Wasm vendor must be a prepared tools archive: ' + name)
            payload = path.read_bytes()
            if archive_bits(payload) != bit:
                raise ValueError('Expected wasm' + str(bit) + ' vendor archive: ' + name)
            result[name] = hashlib.sha256(payload).hexdigest()
        return result


    def dependency_linkages(library, linkage=None, scope=None):
        return dict(prepared_inputs(library)['dependencies'])


    def plan(library, linkage, scope, active=None):
        """Follow recorded before calls. Never infer dependencies from CMake or shell text."""
        if not library.isidentifier(): raise ValueError('Invalid library name: ' + library)
        active = set() if active is None else active
        if library in active: raise ValueError('Circular Wasm dependency: ' + library)
        if library in BUILD_LINKAGES:
            if BUILD_LINKAGES[library] != linkage: raise ValueError('Conflicting Wasm linkage for ' + library)
            return
        supported = sdk_setting(library, 'WASM_SDK_LINKAGES') or []
        if linkage not in supported: raise ValueError('No Wasm ' + linkage + ' entry for ' + library)
        active.add(library)
        children = dependency_linkages(library)
        for dep, kind in children.items(): plan(dep, kind, scope, active)
        active.remove(library)
        BUILD_LINKAGES[library] = linkage


    def session_node(session, library, linkage, bit, mode, scope, parent=None):
        """Register one actual before invocation; return True for an already completed node."""
        state = json.loads(session.read_text(encoding='utf-8'))
        if Path(state['sdk']).resolve() != SDK_ROOT or state['bits'] != bit or state['mode'] != mode:
            raise ValueError('Invalid inherited Wasm SDK session')
        if not Path(state['lock']).is_file(): raise ValueError('Expired Wasm SDK session')
        if parent:
            if not state['active'] or state['active'][-1] != parent: raise ValueError('Invalid before parent: ' + parent)
        if library in state['active']: raise ValueError('Circular before dependency: ' + ' -> '.join(state['active'] + [library]))
        node = state['nodes'].get(library)
        if node is not None:
            if node['linkage'] != linkage: raise ValueError('Conflicting before linkage for ' + library)
            if node['done']:
                session.write_text(json.dumps(state), encoding='utf-8')
                validate(library, bit, mode, linkage=linkage)
                return True
        inputs = dict(dependencies={}, vendors=[], headers=[])
        state['nodes'][library] = dict(linkage=linkage, inputs=inputs, done=False)
        state['active'].append(library)
        session.write_text(json.dumps(state), encoding='utf-8')
        return False


    def before_receipt(session, library, records):
        """Use only explicit records returned by CloudRebuild's stdout pipe."""
        state = json.loads(session.read_text(encoding='utf-8'))
        inputs = dict(dependencies={}, vendors=[], headers=[])
        for line in records.splitlines():
            fields = line.split('\t')
            if len(fields) < 2 or fields[0] != 'CLOUD_REBUILD_LIBRARIESRELY': raise ValueError('Unexpected CloudRebuild stdout')
            kind = fields[1]
            if kind == 'begin':
                if fields[2:] != [library]: raise ValueError('Invalid before owner')
            elif kind == 'project':
                if len(fields) != 4 or not fields[2].isidentifier() or fields[3] not in ('lib', 'dll'):
                    raise ValueError('Invalid before project declaration')
                previous = inputs['dependencies'].get(fields[2])
                if previous is not None and previous != fields[3]: raise ValueError('Conflicting dependency declaration')
                inputs['dependencies'][fields[2]] = fields[3]
            elif kind == 'librariesrely':
                if len(fields) != 3: raise ValueError('Invalid CloudRebuild link list')
                inputs['librariesrely'] = fields[2]
            elif kind in ('vendor', 'header'):
                if len(fields) != 3: raise ValueError('Invalid before input')
                key = 'vendors' if kind == 'vendor' else 'headers'
                if fields[2] not in inputs[key]: inputs[key].append(fields[2])
            else:
                raise ValueError('Unsupported Wasm before record: ' + kind)
        state['nodes'][library]['inputs'] = inputs
        session.write_text(json.dumps(state), encoding='utf-8')


    def wasm_color_enabled():
        """Detect terminal color once per caller; inherited choice keeps nested pipe output consistent."""
        if 'NO_COLOR' in os.environ: return False
        configured = os.environ.get('CLOUD_WASM_COLOR')
        if configured is not None: return configured == '1'
        if os.environ.get('TERM') == 'dumb': return False
        if not sys.stderr.isatty():
            # Git Bash mintty exposes a pipe to native Python; file redirection must remain plain.
            import stat
            return bool(os.name == 'nt' and os.environ.get('MSYSTEM') and os.environ.get('TERM') and
                        stat.S_ISFIFO(os.fstat(sys.stderr.fileno()).st_mode))
        if os.name == 'nt':
            import ctypes
            import msvcrt
            console = ctypes.windll.kernel32
            handle = ctypes.c_void_p(msvcrt.get_osfhandle(sys.stderr.fileno()))
            mode = ctypes.c_ulong()
            if not console.GetConsoleMode(handle, ctypes.byref(mode)): return False
            if not console.SetConsoleMode(handle, mode.value | 4): return False
        return True


    def wasm_message(message, color='32'):
        """Write one flushed status line; reset color before the compiler can emit diagnostics."""
        if wasm_color_enabled(): message = '\x1b[' + color + 'm' + message + '\x1b[0m'
        sys.stderr.write(message + '\n')
        sys.stderr.flush()


    def before_output(command, environment):
        """Forward dependency diagnostics live; keep stdout protocol separate and retain a bounded error tail."""
        with tempfile.TemporaryFile(dir=Path(environment['CLOUD_REBUILD_PRIVATE_INCLUDE']).parent) as records:
            tail = ''
            with subprocess.Popen(command, env=environment, stdout=records, stderr=subprocess.PIPE,
                                  text=True, encoding='utf-8', errors='replace', bufsize=1,
                                  start_new_session=os.name != 'nt') as process:
                try:
                    for line in process.stderr:
                        sys.stderr.write(line)
                        sys.stderr.flush()
                        tail = (tail + line)[-8192:]
                    status = process.wait()
                except BaseException:
                    StopBuildProcess(process)
                    raise
            if status:
                raise ValueError('before ' + command[2] + ' failed (' + str(status) + '):\n' + tail.strip())
            records.seek(0)
            return records.read().decode('utf-8')


    def prepare_before(session, library, target, linkage, mode, scope, environment):
        """CloudRebuild runs the shell and brokers explicit inputs; only still prepares vendors."""
        cloud = environment.get('CLOUD_REBUILD') or str(ROOT / 'lib' / ('CloudRebuild1.2.exe' if os.name == 'nt' else 'CloudRebuild1.2'))
        environment = dict(environment, CLOUD_REBUILD=cloud, CLOUD_WASM_COLOR='1' if wasm_color_enabled() else '0')
        command = [cloud, '--before', library, target, 'lib' if scope == 'librely' else linkage,
                   mode, 'only' if scope == 'only' else 'same']
        records = before_output(command, environment)
        before_receipt(session, library, records)
        active = set()
        visited = set()
        def collect(name):
            if name in active or len(active) >= 128 or len(visited) >= 1024:
                raise ValueError('Circular or excessive dependency graph: ' + name)
            if name in visited: return
            active.add(name)
            state = json.loads(session.read_text(encoding='utf-8'))
            for child, kind in state['nodes'][name]['inputs']['dependencies'].items():
                # Completed nested builds already carry their own explicit before reports.
                current = json.loads(session.read_text(encoding='utf-8'))
                node = current['nodes'].get(child)
                if node is not None and node['linkage'] != kind: raise ValueError('Conflicting dependency: ' + child)
                if kind == 'dll':
                    # An existing runtime module keeps its own sealed build receipt.
                    # The caller's dll choice must not reinterpret its embedded static inputs.
                    if node is not None and not node['done']:
                        raise ValueError('Circular or unfinished Wasm module dependency: ' + child)
                    if node is None:
                        data = validate(child, int(target[4:]), mode, linkage='dll')
                        if 'preparedInputs' not in data:
                            raise ValueError('Old dependency receipt for ' + child + '; rebuild the module first')
                        current['nodes'][child] = dict(linkage=kind, done=True, inputs=data['preparedInputs'])
                        session.write_text(json.dumps(current), encoding='utf-8')
                    continue
                if node is None:
                    current['nodes'][child] = dict(linkage=kind, done=True, inputs={})
                    session.write_text(json.dumps(current), encoding='utf-8')
                    report = before_output([cloud, '--before', child, target, kind, mode, 'only'], environment)
                    before_receipt(session, child, report)
                collect(child)
            active.remove(name)
            visited.add(name)
        collect(library)


    def check_librariesrely(value, inputs, origin=None):
        """An optional caller-supplied prepared closure must match the actual before declarations."""
        if value is None: return
        expected = [Path(path).resolve() for path in inputs['archives'] + inputs['vendor_paths']]
        supplied = []
        for item in value.split(';') if value else []:
            if not item or any(char in item for char in '\r\n\t'):
                raise ValueError('Invalid --librariesrely item')
            path = Path(item)
            path = path.resolve() if path.is_absolute() else (SDK_ROOT / 'lib' / path).resolve()
            # Public SDK paths identify the same declared inputs after transactional staging.
            # Rebase only that SDK's lib subtree; other paths still require an exact match.
            if origin is not None and Path(item).is_absolute():
                try:
                    relative = path.relative_to(Path(origin).resolve() / 'lib')
                except ValueError:
                    pass
                else:
                    path = (SDK_ROOT / 'lib' / relative).resolve()
            if not path.is_file(): raise ValueError('Missing --librariesrely input: ' + item)
            if path not in supplied: supplied.append(path)
        if set(supplied) != set(expected):
            raise ValueError('--librariesrely must match the prepared static dependency closure from before')


    def complete_node(session, library):
        state = json.loads(session.read_text(encoding='utf-8'))
        if state['nodes'][library]['done'] and not state['active']: return
        if not state['active'] or state['active'].pop() != library: raise ValueError('Invalid before completion order')
        state['nodes'][library]['done'] = True
        session.write_text(json.dumps(state), encoding='utf-8')


    def publish(library, build, bit, mode, compiler, sdk, vendors):
        if sdk_kind(library) == 'static':
            publish_static(library, build, bit, mode, compiler, sdk, vendors)
            return
        stem = library + ('d' if mode == 'debug' else '')
        js = build / (stem + '.js')
        wasm = build / (stem + '.wasm')
        if wasm_bits(wasm.read_bytes()) != bit:
            raise ValueError('Compiler produced the wrong Wasm memory target: ' + str(wasm))
        data = dict(library=library, bits=bit, configuration=mode, compiler=compiler, vendorSha256=vendors, preparedInputs=prepared_inputs(library),
                    wasm=wasm.name, wasmSha256=hashlib.sha256(wasm.read_bytes()).hexdigest(),
                    dependencies={dep: validate(dep, bit, mode, linkage='dll')['wasmSha256']
                                  for dep in dependencies(library) if BUILD_LINKAGES[dep] == 'dll'},
                    buildDependencies={dep: artifact_digest(validate(dep, bit, mode)) for dep in dependencies(library)},
                    buildDependencyLinkages={dep: BUILD_LINKAGES[dep] for dep in dependencies(library)})
        alias = sdk_setting(library, 'WASM_SDK_ENTRY_ALIAS') or []
        alias_content = None
        if alias:
            if len(alias) != 1 or not alias[0].isidentifier() or alias[0] == library:
                raise ValueError('Expected one distinct public SDK entry alias')
            alias_content = ("'use strict';\nmodule.exports = require('../" + library + '/' + library + "API.js');").encode('utf-8')
            data.update(entryAlias=alias[0], entryAliasSha256=hashlib.sha256(alias_content).hexdigest())
        body = js.read_text(encoding='utf-8').rstrip()
        body += MARKER + json.dumps(data, sort_keys=True, separators=(',', ':'))
        body += '\nmodule.exports.sdk = Object.freeze(' + json.dumps(data, sort_keys=True) + ');'
        target = SDK_ROOT / 'include' / library
        target.mkdir(parents=True, exist_ok=True)
        # Validate first, then publish with rollback. Identity checks reject an interrupted pair.
        updates = {SDK_ROOT / 'lib' / wasm.name: wasm.read_bytes(), target / (library + 'API.js'): body.encode('utf-8')}
        if alias_content is not None:
            updates[SDK_ROOT / 'include' / alias[0] / (alias[0] + 'API.js')] = alias_content
        previous_js = target / (library + 'API.js')
        if previous_js.is_file():
            for name in identity(previous_js).get('adapters', {}):
                if Path(name).name != name or not name.endswith('.js'): raise ValueError('Invalid previous SDK adapter')
                updates[target / name] = None
        # Clean up the notice generated by older builds; the application owns its notice list.
        updates[target / (library + 'WasmLICENSE.txt')] = None
        updates.update({target / path.name: content for path, content in protocol_constants(library).items()})
        commit_sdk(library, updates, bit, mode)
        wasm_message('[wasm] PASS ' + library + ' ' + str(bit) + ' ' + mode + ': ' + str(wasm.stat().st_size) + ' bytes')


    def commit_sdk(library, updates, bit, mode):
        previous = {path: path.read_bytes() if path.exists() else None for path in updates}
        try:
            for path, content in updates.items():
                path.parent.mkdir(parents=True, exist_ok=True)
                if content is None:
                    if path.exists(): path.unlink()
                    continue
                staged = path.with_name(path.name + '.wasm-publishing')
                staged.write_bytes(content)
                os.replace(str(staged), str(path))
            validate(library, bit, mode)
        except BaseException:
            for path, content in previous.items():
                if content is None:
                    if path.exists(): path.unlink()
                else:
                    path.write_bytes(content)
            raise
        finally:
            for path in updates:
                staged = path.with_name(path.name + '.wasm-publishing')
                if staged.exists(): staged.unlink()


    def publish_static(library, build, bit, mode, compiler, sdk, vendors):
        archive = build / (library + ('d' if mode == 'debug' else '') + '.a')
        payload = archive.read_bytes()
        if archive_bits(payload) != bit: raise ValueError('Compiler produced the wrong static Wasm target')
        data = dict(kind='static', library=library, bits=bit, configuration=mode, compiler=compiler,
                    archive=archive.name, archiveSha256=hashlib.sha256(payload).hexdigest(), vendorSha256=vendors, preparedInputs=prepared_inputs(library),
                    dependencies={dep: artifact_digest(validate(dep, bit, mode)) for dep in dependencies(library)},
                    dependencyLinkages={dep: BUILD_LINKAGES[dep] for dep in dependencies(library)})
        target = SDK_ROOT / 'include' / library
        updates = {SDK_ROOT / 'lib' / archive.name: payload,
                   target / (library + 'WasmSDK.json'): json.dumps(data, indent=2, sort_keys=True).encode('utf-8')}
        updates[target / (library + 'WasmLICENSE.txt')] = None
        commit_sdk(library, updates, bit, mode)
        wasm_message('[wasm] PASS ' + library + ' ' + str(bit) + ' ' + mode + ': C++ static archive ' + str(len(payload)) + ' bytes')


    def vendor_headers(vendor):
        """Select Wasm-specific headers, or headers shared by all targets."""
        root = ROOT / 'tools' / vendor / 'include'
        specific = root / (vendor + 'wasm')
        if specific.is_dir():
            return specific
        if root.is_dir() and any(path.is_file() and path.suffix.lower() in ('.h', '.hpp')
                                 for path in root.iterdir()):
            return root
        raise ValueError('Missing Wasm or platform-independent vendor headers: ' + str(root))


    def build_inputs(library, bit, mode, linkage, scope, private):
        """Describe declared source/header/link inputs; never invoke a compiler."""
        import shutil
        BUILD_LINKAGES.clear()
        plan(library, linkage, scope)
        names = list(BUILD_LINKAGES)
        # A runtime module owns its static closure. Validate its identity above, but
        # never link that closure into its caller (or cross-module imports collide).
        linked = set()
        def visit_static(name):
            if name in linked: return
            linked.add(name)
            for child, kind in dependency_linkages(name).items():
                if kind == 'lib': visit_static(child)
        visit_static(library)
        vendors = {}
        for name in names:
            if name in linked: vendors.update(vendor_libraries(name, bit, mode))
        vendor_paths = []
        for relative, digest in vendors.items():
            source = ROOT / relative
            prepared = SDK_ROOT / 'lib' / source.name
            if not prepared.is_file() or hashlib.sha256(prepared.read_bytes()).hexdigest() != digest:
                raise ValueError('Missing or mismatched prepared Wasm vendor: ' + str(prepared))
            vendor_paths.append(prepared)
            vendor = source.relative_to(ROOT / 'tools').parts[0]
            headers = vendor_headers(vendor)
            for header in headers.rglob('*'):
                if header.is_file():
                    target = SDK_ROOT / 'include' / vendor / header.relative_to(headers)
                    if not target.is_file() or target.read_bytes() != header.read_bytes():
                        raise ValueError('Missing or mismatched Wasm header: ' + str(target))
        if scope == 'only':
            for dep in names:
                if dep != library: validate(dep, bit, mode)
        header_names = set(names)
        for name in names:
            header_names.update(prepared_inputs(name)['headers'])
        for name in header_names:
            if not name.isidentifier(): raise ValueError('Invalid private header project: ' + name)
            source = ROOT / 'src' / name / name / 'src'
            for header in source.rglob('*'):
                if header.is_file() and header.suffix.lower() in ('.h', '.hpp', '.inl'):
                    target = private / 'include' / name / header.relative_to(source)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(header, target)
        source = ROOT / 'src' / library / library / 'src'
        sources = sorted(file for file in source.iterdir() if file.suffix in ('.cpp', '.cc', '.cxx', '.c'))
        if not sources: raise ValueError('No Wasm sources: ' + library)
        includes = [private / 'include', source]
        for vendor in sorted({(ROOT / name).relative_to(ROOT / 'tools').parts[0] for name in vendors}):
            if vendor != 'zlib': includes.append(SDK_ROOT / 'include' / vendor)
        # Upstream include names (google/..., mbedtls/...) must select the target SDK
        # before any legacy native copies still present at the shared include root.
        includes.append(SDK_ROOT / 'include')
        definitions = ['STATIC_LIB'] + (sdk_setting(library, 'WASM_BUILD_DEFINITIONS') or [])
        if runtime_dependencies(library):
            definitions += sdk_setting(library, 'WASM_BUILD_RUNTIME_DEFINITIONS') or []
        config = sdk_setting(library, 'WASM_BUILD_CONFIG_HEADER') or []
        if config: definitions.append('MBEDTLS_CONFIG_FILE="' + config[0] + '"')
        archives = [SDK_ROOT / 'lib' / (name + ('d' if mode == 'debug' else '') + '.a')
                    for name in reversed(names) if name != library and name in linked and BUILD_LINKAGES[name] == 'lib']
        conversions = []
        if bit == 64:
            # Compatibility data for the library's existing C binding, not an export list.
            conversions = signatures(ROOT, library, None)
        return dict(sources=sources, includes=includes, definitions=definitions, archives=archives,
                    vendors=vendors, vendor_paths=vendor_paths, conversions=conversions,
                    runtime=[name for name in dependencies(library) if BUILD_LINKAGES[name] == 'dll'])


    def stage_sdk(destination, libraries=None):
        """Snapshot existing Wasm receipts/inputs; link selection comes from actual before results."""
        import shutil
        baseline = {}
        selected = set(libraries or [])
        vendors = set()
        include = SDK_ROOT / 'include'
        if include.is_dir():
            for public in include.iterdir():
                if not public.is_dir(): continue
                metadata = public / (public.name + 'WasmSDK.json')
                js = public / (public.name + 'API.js')
                data = None
                if metadata.is_file(): data = json.loads(metadata.read_text(encoding='utf-8'))
                elif js.is_file() and MARKER in js.read_text(encoding='utf-8'): data = identity(js)
                if data is not None:
                    selected.add(public.name)
                    vendors.update(data.get('vendorSha256', {}))
        def copy(source):
            relative = source.relative_to(SDK_ROOT)
            baseline[relative.as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest() if source.is_file() else None
            if source.is_file():
                target = destination / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
        for name in selected:
            # The alias is part of this provider's transaction, not a second Wasm module.
            for alias in sdk_setting(name, 'WASM_SDK_ENTRY_ALIAS') or []:
                if not alias.isidentifier() or alias == name: raise ValueError('Invalid SDK entry alias')
                copy(SDK_ROOT / 'include' / alias / (alias + 'API.js'))
            source = SDK_ROOT / 'include' / name
            for file in source.rglob('*'):
                if file.is_file() and (file.suffix == '.js' or file.name.endswith('WasmSDK.json')): copy(file)
            for suffix in ('.a', 'd.a', '.wasm', 'd.wasm'): copy(SDK_ROOT / 'lib' / (name + suffix))
        for relative in vendors:
            source = ROOT / relative
            copy(SDK_ROOT / 'lib' / source.name)
            vendor = source.relative_to(ROOT / 'tools').parts[0]
            headers = vendor_headers(vendor)
            for header in headers.rglob('*'):
                if header.is_file(): copy(SDK_ROOT / 'include' / vendor / header.relative_to(headers))
        destination.mkdir(parents=True, exist_ok=True)
        (destination / '.wasm-baseline.json').write_text(json.dumps(baseline), encoding='utf-8')


    def validate_staged_origin(destination):
        baseline = json.loads((SDK_ROOT / '.wasm-baseline.json').read_text(encoding='utf-8'))
        for relative, expected in baseline.items():
            current = destination / relative
            actual = hashlib.sha256(current.read_bytes()).hexdigest() if current.is_file() else None
            if actual != expected:
                raise ValueError('Prepared SDK changed during CMake only build: ' + relative)


    def publish_tree(destination, libraries, bit, mode):
        """Commit one complete SDK batch, restoring every replaced file on failure."""
        updates = {}
        for name in libraries: validate(name, bit, mode)
        files = set()
        for name in libraries:
            data = validate(name, bit, mode)
            files.add(SDK_ROOT / 'lib' / data.get('archive', data.get('wasm')))
            public = SDK_ROOT / 'include' / name
            if data.get('entryAlias'):
                alias = data['entryAlias']
                files.add(SDK_ROOT / 'include' / alias / (alias + 'API.js'))
            if data.get('kind') == 'static':
                files.add(public / (name + 'WasmSDK.json'))
            else:
                files.add(public / (name + 'API.js'))
                files.update(public / adapter for adapter in data.get('adapters', {}))
                files.update(public / path.name for path in protocol_constants(name))
            for relative in data.get('vendorSha256', {}):
                source = ROOT / relative
                files.add(SDK_ROOT / 'lib' / source.name)
                vendor = source.relative_to(ROOT / 'tools').parts[0]
                headers = vendor_headers(vendor)
                files.update(SDK_ROOT / 'include' / vendor / header.relative_to(headers)
                             for header in headers.rglob('*') if header.is_file())
        for file in files:
            updates[destination / file.relative_to(SDK_ROOT)] = file.read_bytes()
        for name in libraries:
            updates[destination / 'include' / name / (name + 'WasmLICENSE.txt')] = None
            previous_js = destination / 'include' / name / (name + 'API.js')
            if previous_js.is_file():
                current = validate(name, bit, mode)
                for adapter in identity(previous_js).get('adapters', {}):
                    if Path(adapter).name != adapter or not adapter.endswith('.js'): raise ValueError('Invalid previous SDK adapter')
                    if adapter not in current.get('adapters', {}): updates[previous_js.parent / adapter] = None
        previous = {path: path.read_bytes() if path.is_file() else None for path in updates}
        try:
            for path, content in updates.items():
                path.parent.mkdir(parents=True, exist_ok=True)
                if content is None:
                    if path.exists(): path.unlink()
                else:
                    temporary = path.with_name(path.name + '.wasm-publishing')
                    temporary.write_bytes(content)
                    os.replace(temporary, path)
            # Keep a configured CMake build's snapshot current after its own successful publication.
            baseline_file = SDK_ROOT / '.wasm-baseline.json'
            if baseline_file.is_file():
                baseline = json.loads(baseline_file.read_text(encoding='utf-8'))
                baseline.update({path.relative_to(destination).as_posix(): None for path in updates})
                for relative in baseline:
                    file = destination / relative
                    baseline[relative] = hashlib.sha256(file.read_bytes()).hexdigest() if file.is_file() else None
                baseline_file.write_text(json.dumps(baseline), encoding='utf-8')
        except BaseException:
            for path, content in previous.items():
                if content is None:
                    if path.exists(): path.unlink()
                else: path.write_bytes(content)
            raise
        finally:
            for path in updates:
                temporary = path.with_name(path.name + '.wasm-publishing')
                if temporary.exists(): temporary.unlink()

# Wasm: prepare dependencies, compile, then publish.
if __name__ == '__main__' and wasm_arguments:
    library = 'LevelDBManager'
    root = ROOT

    parser = argparse.ArgumentParser(description='Compile ' + library + ' directly with Emscripten')
    parser.add_argument('target', type=str.lower, choices=['wasm32', 'wasm64'])
    parser.add_argument('linkage', type=str.lower, choices=['lib', 'dll'])
    parser.add_argument('mode', type=str.lower, choices=['debug', 'release'])
    parser.add_argument('scope', type=str.lower, choices=['same', 'only', 'librely'], nargs='?', default='only')
    parser.add_argument('--output-root', type=Path)
    parser.add_argument('--librariesrely', help='Semicolon-separated prepared static link inputs')
    # CMake compiles through its own entry and uses these data/publication steps only.
    phase = parser.add_mutually_exclusive_group()
    phase.add_argument('--prepare-only', type=Path, help=argparse.SUPPRESS)
    phase.add_argument('--publish-only', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--session', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--commit-to', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--emscripten-root', type=Path, help=argparse.SUPPRESS)
    options = parser.parse_args()
    bits = int(options.target[4:])
    SDK_ROOT = (options.output_root or Path(os.environ.get('CLOUD_REBUILD_SDK_ROOT') or str(root))).resolve()
    lock = None
    lockfile = None

    def Run(command, environment):
        command = [str(part) for part in command]
        wasm_message(subprocess.list2cmdline(command))
        with subprocess.Popen(command, env=environment, start_new_session=os.name != 'nt') as process:
            try:
                status = process.wait()
                if status:
                    raise subprocess.CalledProcessError(status, command)
            except BaseException:
                StopBuildProcess(process)
                raise

    def Compiler():
        installation = options.emscripten_root.resolve().parents[1] if options.emscripten_root else locate_sdk(root)
        compiler = installation / 'upstream/emscripten'
        for name in ('emcc.py', 'em++.py', 'emar.py'):
            if not (compiler / name).is_file(): raise ValueError('Missing Emscripten tool: ' + str(compiler / name))
        config = Path(os.environ.get('EM_CONFIG') or str(installation / '.emscripten')).resolve()
        if not config.is_file(): raise ValueError('Activate the selected Emscripten SDK: ' + str(config))
        environment = dict(os.environ, EMSDK=str(installation), EM_CONFIG=str(config), EMSDK_PYTHON=sys.executable, PYTHONUTF8='1')
        environment['PATH'] = os.pathsep.join([str(Path(sys.executable).parent), str(compiler), environment.get('PATH', '')])
        output = subprocess.check_output([sys.executable, str(compiler / 'emcc.py'), '--version'], env=environment, stderr=subprocess.STDOUT).decode('utf-8', errors='replace')
        version = next((line for line in output.splitlines() if line.startswith('emcc ')), '')
        if not re.search(r'\b3\.1\.74(?:-git)?(?:\s|$)', version): raise ValueError('Prepared vendors require Emscripten 3.1.74: ' + version)
        print('[wasm] Python ' + sys.executable + '\n[wasm] Emscripten ' + str(compiler) + '\n[wasm] ' + version, flush=True)
        return compiler, environment, version

    try:
        compiler, environment, version = Compiler()
        inherited = options.session or os.environ.get('CLOUD_WASM_SESSION')
        destination = options.commit_to or (None if inherited else SDK_ROOT)
        if options.publish_only and not inherited:
            raise ValueError('Missing prepared Wasm build session')
        directory = options.prepare_only or options.publish_only
        if directory is None:
            scratch = Path(os.environ.get('WASM_BUILD_ROOT') or str(root / 'src/temp' / library)).resolve()
            scratch.relative_to((root / 'src/temp' / library).resolve())
            scratch = scratch / options.target / options.linkage / options.mode
            scratch.mkdir(parents=True, exist_ok=True)
            directory = Path(tempfile.mkdtemp(dir=str(scratch)))
        directory = directory.resolve()
        directory.mkdir(parents=True, exist_ok=True)
        if inherited:
            session = Path(inherited).resolve()
            state = json.loads(session.read_text(encoding='utf-8'))
            SDK_ROOT = Path(state['sdk']).resolve()
        else:
            lockfile = destination / 'lib/WasmBuild.lock'
            lockfile.parent.mkdir(parents=True, exist_ok=True)
            lock = os.open(str(lockfile), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            stage_sdk(directory / 'sdk')
            SDK_ROOT = directory / 'sdk'
            marker = lockfile
            if options.prepare_only:
                # Configure exits before CMake builds. Publication locks and checks the snapshot again.
                os.close(lock)
                lockfile.unlink()
                lock = None
                marker = directory / 'batch.lock'
                marker.write_text(library, encoding='utf-8')
            session = directory / 'session.json'
            state = dict(sdk=str(SDK_ROOT), origin=str(destination.resolve()), bits=bits, mode=options.mode, nodes={}, active=[], lock=str(marker))
            session.write_text(json.dumps(state), encoding='utf-8')
        os.environ['CLOUD_WASM_SESSION'] = str(session)
        environment.update(CLOUD_WASM_SESSION=str(session), CLOUD_WASM_PARENT=library,
                           CLOUD_REBUILD_PYTHON=sys.executable, CLOUD_REBUILD_SDK_ROOT=SDK_ROOT.as_posix(),
                           XUEYELINGSHUANG=root.as_posix() + '/')
        # Dependencies use their own src/temp/<library> directory.
        environment.pop('WASM_BUILD_ROOT', None)
        if not options.publish_only:
            if session_node(session, library, options.linkage, bits, options.mode, options.scope,
                                os.environ.get('CLOUD_WASM_PARENT')):
                sys.exit(0)
            environment['CLOUD_REBUILD_PRIVATE_INCLUDE'] = (directory / 'include').as_posix()
            prepare_before(session, library, options.target, options.linkage, options.mode, options.scope, environment)
        inputs = build_inputs(library, bits, options.mode, options.linkage, options.scope, directory)
        check_librariesrely(prepared_inputs(library)['librariesrely'], inputs)
        check_librariesrely(options.librariesrely, inputs, state.get('origin', str(SDK_ROOT)))
        planned = dict(BUILD_LINKAGES)
        conversions = directory / (library + '-conversions.json')
        conversions.write_text(json.dumps(inputs['conversions']), encoding='utf-8')
        if options.prepare_only:
            # One CMake call prepares before dependencies and writes all validated inputs.
            values = dict(sources=inputs['sources'], includes=inputs['includes'], definitions=inputs['definitions'],
                          archives=inputs['archives'], vendors=inputs['vendor_paths'], runtime=inputs['runtime'],
                          session=[session], conversions=[conversions])
            text = ''
            for key, items in values.items():
                text += 'set(wasm_' + key + '\n' + ''.join('  [==[' + str(item).replace('\\', '/') + ']==]\n' for item in items) + ')\n'
            (directory / (library + '-inputs.cmake')).write_text(text, encoding='utf-8')
            sys.exit(0)
        if not options.publish_only:
            common = ['-fexceptions', '-sMEMORY64=' + str(int(bits == 64))]
            common += ['-O0', '-g1'] if options.mode == 'debug' else ['-O2']
            flags = common + ['-fPIC']
            flags += ['-D' + value for value in inputs['definitions']]
            # Local quoted headers must not shadow platform headers such as <semaphore.h>.
            local_source = ROOT / 'src' / library / library / 'src'
            for include in inputs['includes']:
                flags += ['-iquote' if include == local_source else '-I', include]
            objects = []
            for index, source in enumerate(inputs['sources']):
                obj = directory / (str(index) + '-' + source.name + '.o')
                c = source.suffix == '.c'
                wasm_message('[wasm] [' + str(index + 1) + '/' + str(len(inputs['sources']) + 1) + '] Compiling ' + source.name)
                Run([sys.executable, compiler / ('emcc.py' if c else 'em++.py'), '-std=c11' if c else '-std=c++11',
                     '-c', source, '-o', obj] + flags, environment)
                objects.append(obj)
            wasm_message('[wasm] [' + str(len(objects) + 1) + '/' + str(len(objects) + 1) + '] ' +
                         ('Archiving ' if options.linkage == 'lib' else 'Linking ') + library)
            stem = library + ('d' if options.mode == 'debug' else '')
            if options.linkage == 'lib':
                Run([sys.executable, compiler / 'emar.py', 'rcs', directory / (stem + '.a')] + objects, environment)
            else:
                pre = directory / (library + '-environment.js')
                pre.write_text("var WebAssembly = Module['wasmApi'] || globalThis.WebAssembly;\nvar performance = Module['performance'] || globalThis.performance;\n", encoding='utf-8')
                link = common + ['--no-entry', '-sEXPORT_KEEPALIVE=1',
                        '-sEXPORTED_FUNCTIONS=["_malloc","_free"]', '--pre-js', pre,
                        '-sASSERTIONS=0', '-sMODULARIZE=1', '-sEXPORT_NAME=create' + library, '-sENVIRONMENT=web', '-sFILESYSTEM=0',
                        '-sALLOW_MEMORY_GROWTH=1',
                        '-sINITIAL_MEMORY=' + (sdk_setting(library, 'WASM_BUILD_INITIAL_MEMORY') or ['16777216'])[0],
                        '-sMAXIMUM_MEMORY=' + (sdk_setting(library, 'WASM_BUILD_MAXIMUM_MEMORY') or ['134217728'])[0],
                        '-sSTACK_SIZE=1048576', '-sDISABLE_EXCEPTION_CATCHING=0', '-sALLOW_TABLE_GROWTH=1',
                        '-sEXPORTED_RUNTIME_METHODS=["addFunction","removeFunction"]']
                if sdk_setting(library, 'WASM_BUILD_BIGINT'): link += ['-sWASM_BIGINT=1']
                if inputs['conversions']:
                    link += ['-sSIGNATURE_CONVERSIONS=@' + str(conversions)]
                if inputs['runtime']:
                    for value in sdk_setting(library, 'WASM_BUILD_RUNTIME_JS_LIBRARY') or []:
                        link += ['--js-library', root / 'src' / library / value]
                # Project archives are retained for source-marked exports; vendor members follow real references.
                link += ['-Wl,--whole-archive'] + inputs['archives'] + ['-Wl,--no-whole-archive', '-Wl,--start-group']
                link += inputs['vendor_paths'] + ['-Wl,--end-group']
                Run([sys.executable, compiler / 'em++.py'] + objects + link + ['-o', directory / (stem + '.js')], environment)
            # Recheck prepared inputs after compilation, before touching the public SDK.
            build_inputs(library, bits, options.mode, options.linkage, options.scope, directory)
        publish(library, directory, bits, options.mode, version, compiler, inputs['vendors'])
        complete_node(session, library)
        if destination:
            if options.publish_only:
                destination = destination.resolve()
                lockfile = destination / 'lib/WasmBuild.lock'
                lockfile.parent.mkdir(parents=True, exist_ok=True)
                lock = os.open(str(lockfile), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
                validate_staged_origin(destination)
            publish_tree(destination, planned, bits, options.mode)
        sys.exit(0)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        wasm_message('[wasm] ' + library + ' FAILED: ' + str(error), '31')
        sys.exit(1)
    finally:
        if lock is not None:
            os.close(lock)
            lockfile.unlink()

import time
import ctypes
import shutil
import subprocess
import argparse
import tempfile
import re
from pathlib import Path
import winreg

BuildType = 'Release'
IsRebuild = True
Build = 'Rebuild'
Update = False
Copy = False
CleanAll = False
BuildTimeout = 30*60
Bit = 'Win32'
Dlllib = 'dll'
MSBuild = None
IncrediBuild = None
UseMSBuild = True # 由入口参数选择，默认MSBuild；incredibuild选择可选工具

# 模板生成器负责替换工程名；依赖仍由before/version准备
SlnFile = '../LevelDBManager.sln' #相对于本py脚本路径的相对路径
UpdateDir = [] #相对于本py脚本路径的相对路径，填空不更新
ExecBatList = [] #相对于本py脚本路径的相对路径，编译前调用的脚本，可填空，执行bat会先cd到bat目录再执行

class ConsoleColor():
    '''This class defines the values of color for printing on console window'''
    Black = 0
    DarkBlue = 1
    DarkGreen = 2
    DarkCyan = 3
    DarkRed = 4
    DarkMagenta = 5
    DarkYellow = 6
    Gray = 7
    DarkGray = 8
    Blue = 9
    Green = 10
    Cyan = 11
    Red = 12
    Magenta = 13
    Yellow = 14
    White = 15

class Win32API():
    """Emit color as part of the log stream; nested builds never share console cursor state."""
    ColorEnabled = None
    Colors = [30, 34, 32, 36, 31, 35, 33, 37, 90, 94, 92, 96, 91, 95, 93, 97]

    @staticmethod
    def SupportsColor():
        if Win32API.ColorEnabled is not None:
            return Win32API.ColorEnabled
        enabled = False
        configured = os.environ.get('CLOUD_REBUILD_COLOR')
        if 'NO_COLOR' not in os.environ and os.environ.get('TERM') != 'dumb':
            if configured is not None:
                enabled = configured == '1'
            elif sys.stdout.isatty():
                import msvcrt
                mode = ctypes.c_ulong()
                handle = ctypes.c_void_p(msvcrt.get_osfhandle(sys.stdout.fileno()))
                kernel = ctypes.windll.kernel32
                enabled = bool(kernel.GetConsoleMode(handle, ctypes.byref(mode)) and
                               kernel.SetConsoleMode(handle, mode.value | 4))
        Win32API.ColorEnabled = enabled
        return enabled

    @staticmethod
    def SetConsoleColor(color):
        if Win32API.SupportsColor() and 0 <= color < len(Win32API.Colors):
            sys.stdout.write('\x1b[' + str(Win32API.Colors[color]) + 'm')

    @staticmethod
    def ResetConsoleColor():
        if Win32API.SupportsColor():
            sys.stdout.write('\x1b[0m')
        sys.stdout.flush()

class Logger():
    LogFile = '@AutomationLog.txt'
    LineSep = '\n'
    @staticmethod
    def Write(log, consoleColor = -1, writeToFile = True, printToStdout = True):
        '''
        consoleColor: value in class ConsoleColor, such as ConsoleColor.DarkGreen
        if consoleColor == -1, use default color
        '''
        if printToStdout:
            isValidColor = (consoleColor >= ConsoleColor.Black and consoleColor <= ConsoleColor.White)
            if isValidColor:
                Win32API.SetConsoleColor(consoleColor)
            try:
                sys.stdout.write(log)
            except UnicodeError as e:
                Win32API.SetConsoleColor(ConsoleColor.Red)
                isValidColor = True
                sys.stdout.write(str(type(e)) + ' can\'t print the log!\n')
            if isValidColor:
                Win32API.ResetConsoleColor()
            else:
                sys.stdout.flush()
        if not writeToFile:
            return
        logFile = open(Logger.LogFile, 'a+', encoding = 'utf-8')
        try:
            logFile.write(log)
        except Exception as ex:
            sys.stdout.write('can not write log with exception: {0} {1}'.format(type(ex), ex))
        finally:
            logFile.close()

    @staticmethod
    def WriteLine(log, consoleColor = -1, writeToFile = True, printToStdout = True):
        '''
        consoleColor: value in class ConsoleColor, such as ConsoleColor.DarkGreen
        if consoleColor == -1, use default color
        '''
        Logger.Write(log + Logger.LineSep, consoleColor, writeToFile, printToStdout)

    @staticmethod
    def Log(log, consoleColor = -1, writeToFile = True, printToStdout = True):
        '''
        consoleColor: value in class ConsoleColor, such as ConsoleColor.DarkGreen
        if consoleColor == -1, use default color
        '''
        t = time.localtime()
        log = '{0}-{1:02}-{2:02} {3:02}:{4:02}:{5:02} - {6}{7}'.format(t.tm_year, t.tm_mon, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec, log, Logger.LineSep)
        Logger.Write(log, consoleColor, writeToFile, printToStdout)

def GetMSBuildPath():
    """Locate VS2013 MSBuild without creating a shared batch file or changing the caller environment."""
    explicit = os.environ.get('MSBUILD_EXE_PATH')
    if explicit:
        return os.path.abspath(explicit) if os.path.isfile(explicit) else None
    roots = [os.environ.get('ProgramFiles(x86)'), os.environ.get('ProgramFiles')]
    for root in roots:
        if not root:
            continue
        folder = os.path.join(root, 'MSBuild', '12.0', 'Bin')
        candidate = os.path.join(folder, 'amd64', 'MSBuild.exe') if Bit == 'x64' else os.path.join(folder, 'MSBuild.exe')
        if os.path.isfile(candidate):
            return candidate
    for view in (winreg.KEY_WOW64_32KEY, winreg.KEY_WOW64_64KEY):
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Microsoft\MSBuild\ToolsVersions\12.0', 0, winreg.KEY_READ | view) as key:
                folder = winreg.QueryValueEx(key, 'MSBuildToolsPath')[0]
            candidate = os.path.join(folder, 'amd64', 'MSBuild.exe') if Bit == 'x64' else os.path.join(folder, 'MSBuild.exe')
            if os.path.isfile(candidate):
                return candidate
        except OSError:
            continue
    return None

def GetIncrediBuildPath():
    """Read the optional IncrediBuild installation and always release the registry handle."""
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Classes\IncrediBuild.MonitorFile\shell\open\command') as key:
            value = winreg.QueryValueEx(key, '')[0]
        if value and value.startswith('"') and '"' in value[1:]:
            path = value.split('"', 2)[1]
            candidate = os.path.join(os.path.dirname(path), 'BuildConsole.exe')
            return candidate if os.path.isfile(candidate) else None
    except OSError:
        pass
    return None

def RunNativeCommand(command, cwd=None, environment=None):
    """Run a selected command with a bounded wait; timeout and interruption cannot leave this build running."""
    process = subprocess.Popen(command, cwd=cwd, env=environment)
    try:
        return process.wait(timeout=BuildTimeout)
    except subprocess.TimeoutExpired:
        StopBuildProcess(process)
        Logger.Log('build timed out after {0}s'.format(BuildTimeout), ConsoleColor.Red)
        return 1
    except BaseException:
        StopBuildProcess(process)
        raise

def UpdateCode():
    git = shutil.which('git.exe')
    if not git:
        Logger.Log('git.exe is missing; update failed', ConsoleColor.Red)
        return False
    for directory in UpdateDir:
        path = os.path.abspath(os.path.join(os.path.dirname(__file__), directory))
        if RunNativeCommand([git, 'pull'], cwd=path) != 0:
            return False
    return True

def BuildProject(command):
    """Build once; deterministic compiler errors are not retried six times or masked by stale logs."""
    Logger.WriteLine(subprocess.list2cmdline(command), ConsoleColor.Cyan)
    if not UseMSBuild and os.path.isfile('IncrediBuild.log'):
        os.remove('IncrediBuild.log')
    start = time.monotonic()
    code = RunNativeCommand(command)
    success = code == 0
    if not UseMSBuild and '/clean' not in command:
        success = False
        if code == 0 and os.path.isfile('IncrediBuild.log'):
            raw = Path('IncrediBuild.log').read_bytes()
            try:
                text = raw.decode('utf-8-sig')
            except UnicodeDecodeError:
                text = raw.decode('mbcs', errors='replace')
            # A successful summary is required; absent/malformed summaries must not report success.
            matches = re.findall(r'(?:失败|failed)\s*[:：]?\s*(\d+)|(\d+)\s*failed', text, re.I)
            success = bool(matches) and all(int(left or right) == 0 for left, right in matches)
            if not success:
                Logger.WriteLine(text[-8192:], ConsoleColor.Red)
    Logger.WriteLine('build cost time: {0:.1f}s\n'.format(time.monotonic() - start), ConsoleColor.Green if success else ConsoleColor.Red)
    return success

def BuildAllProjects():
    commands = []
    if UseMSBuild:
        configurations = ['Debug', 'Release'] if CleanAll else [BuildType]
        if IsRebuild:
            for configuration in configurations:
                commands.append([MSBuild, SlnFile, '/t:Clean', '/p:Configuration=' + configuration + ';Platform=' + Bit,
                                 '/nologo', '/maxcpucount', '/nr:false', '/filelogger', '/consoleloggerparameters:ErrorsOnly'])
        # One solution build honors its normal project graph; a separate target build is redundant.
        commands.append([MSBuild, SlnFile, '/p:Configuration=' + BuildType + ';Platform=' + Bit,
                         '/nologo', '/maxcpucount', '/nr:false', '/filelogger', '/consoleloggerparameters:ErrorsOnly'])
    else:
        if IsRebuild:
            for configuration in (['Debug', 'Release'] if CleanAll else [BuildType]):
                commands.append([IncrediBuild, SlnFile, '/clean', '/cfg=' + configuration + '|' + Bit,
                                 '/nologo', '/out=IncrediBuild.log'])
        commands.append([IncrediBuild, SlnFile, '/build', '/cfg=' + BuildType + '|' + Bit,
                         '/nologo', '/out=IncrediBuild.log'])
    return all(BuildProject(command) for command in commands)

def main(arguments=None):
    global Bit, Dlllib, BuildType, IsRebuild, Build, Update, Copy, CleanAll, UseMSBuild, SlnFile, MSBuild, IncrediBuild
    parser = argparse.ArgumentParser(description='Build LevelDBManager with VS2013; dependencies are prepared by before/version')
    parser.add_argument('bit', nargs='?', default='32', choices=['32', '64'])
    parser.add_argument('linkage', nargs='?', default='dll', type=str.lower, choices=['dll', 'lib'])
    parser.add_argument('mode', nargs='?', default='release', type=str.lower, choices=['debug', 'release'])
    parser.add_argument('actions', nargs='*', type=str.lower)
    options = parser.parse_args(arguments)
    allowed = {'build', 'update', 'copy', 'clean', 'incredibuild'}
    if set(options.actions) - allowed:
        parser.error('Unknown native action: ' + ', '.join(sorted(set(options.actions) - allowed)))
    if len(options.actions) != len(set(options.actions)) or 'build' in options.actions and 'clean' in options.actions:
        parser.error('Duplicate or conflicting native actions')
    Bit = 'Win32' if options.bit == '32' else 'x64'
    Dlllib = options.linkage
    BuildType = options.mode.title()
    IsRebuild = 'build' not in options.actions
    Build = 'Rebuild' if IsRebuild else 'Build'
    Update, Copy, CleanAll = (name in options.actions for name in ('update', 'copy', 'clean'))
    UseMSBuild = 'incredibuild' not in options.actions
    script = Path(__file__).resolve()
    SlnFile = str(script.parent.parent / ('LevelDBManager_lib.sln' if Dlllib == 'lib' else 'LevelDBManager.sln'))
    MSBuild = GetMSBuildPath() if UseMSBuild else None
    IncrediBuild = None if UseMSBuild else GetIncrediBuildPath()
    if not (MSBuild if UseMSBuild else IncrediBuild):
        sys.stderr.write('Cannot find the selected VS2013 MSBuild or IncrediBuild executable\n')
        return 1
    if not os.path.isfile(SlnFile):
        sys.stderr.write('Missing solution: ' + SlnFile + '\n')
        return 1
    # All temporary logs are per-invocation, including calls launched from a different project directory.
    root = script.parents[3] / 'src/temp/LevelDBManager/native' / options.bit / Dlllib / options.mode
    root.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='build-', dir=str(root)))
    Logger.LogFile = str(directory / '@AutomationLog.txt')
    previous = os.getcwd()
    start = time.monotonic()
    try:
        os.chdir(directory)
        Logger.Log('run with argv ' + str(sys.argv), ConsoleColor.Green)
        Logger.WriteLine('build dir is: {0}, {1}: {2}'.format(directory, Build, BuildType))
        if Update and not UpdateCode():
            return 1
        if Copy:
            for value in ExecBatList:
                path = (script.parent / value).resolve()
                if not path.is_file():
                    raise OSError('Missing prebuild script: ' + str(path))
                environment = dict(os.environ, CLOUD_REBUILD_PREBUILD_SCRIPT=str(path))
                command = subprocess.list2cmdline([os.environ.get('COMSPEC', 'cmd.exe')]) + ' /d /s /c ""%CLOUD_REBUILD_PREBUILD_SCRIPT%""'
                code = RunNativeCommand(command, cwd=str(path.parent), environment=environment)
                if code != 0:
                    Logger.Log('prebuild script failed: ' + str(path), ConsoleColor.Red)
                    return 1
        success = BuildAllProjects()
        Logger.Log('build succeed' if success else 'build failed', ConsoleColor.Green if success else ConsoleColor.Red)
        return 0 if success else 1
    except (OSError, subprocess.SubprocessError) as error:
        sys.stderr.write('Native build failed: ' + str(error) + '\n')
        return 1
    finally:
        os.chdir(previous)
        Logger.WriteLine('all build cost time: {0:.2f} seconds'.format(time.monotonic() - start), ConsoleColor.Green)

if __name__ == '__main__':
    sys.exit(main())