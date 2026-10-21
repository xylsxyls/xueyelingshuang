#!python3
# -*- coding:utf-8 -*-
import os
import sys

CLOUD_REBUILD_WASM_ENTRY = True

# Wasm: prepare dependencies, compile, then publish. Native Windows builds follow below.
if __name__ == '__main__' and len(sys.argv) > 1 and (sys.argv[1].lower().startswith('wasm') or (len(sys.argv) > 2 and sys.argv[2].lower() == 'wasm')):
    if sys.version_info < (3, 8):
        sys.stderr.write('Wasm builds require Python >= 3.8\n')
        sys.exit(1)
    import argparse
    import json
    from pathlib import Path
    import shutil
    import subprocess
    import tempfile
    import re
    sys.dont_write_bytecode = True
    library = 'NetPeer'
    root = Path(__file__).resolve().parents[3]
    sys.path.insert(0, str(root / 'tools/wasm'))
    import wasm_sdk as sdk

    parser = argparse.ArgumentParser(description='Compile ' + library + ' directly with Emscripten')
    parser.add_argument('target', type=str.lower, choices=['wasm32', 'wasm64'])
    parser.add_argument('linkage', type=str.lower, choices=['lib', 'dll'])
    parser.add_argument('mode', type=str.lower, choices=['debug', 'release'])
    parser.add_argument('scope', type=str.lower, choices=['same', 'only', 'librely'], nargs='?', default='same')
    parser.add_argument('--output-root', type=Path)
    # CMake compiles through its own entry and uses these data/publication steps only.
    phase = parser.add_mutually_exclusive_group()
    phase.add_argument('--prepare-only', type=Path, help=argparse.SUPPRESS)
    phase.add_argument('--publish-only', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--session', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--commit-to', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--emscripten-root', type=Path, help=argparse.SUPPRESS)
    options = parser.parse_args()
    bits = int(options.target[4:])
    sdk.SDK_ROOT = (options.output_root or Path(os.environ.get('CLOUD_REBUILD_SDK_ROOT') or str(root))).resolve()
    lock = None
    lockfile = None

    def Run(command, environment):
        command = [str(part) for part in command]
        print(subprocess.list2cmdline(command))
        subprocess.check_call(command, env=environment)

    def Compiler():
        from wasm_toolchain import locate_sdk
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
        print('[wasm] Python ' + sys.executable + '\n[wasm] Emscripten ' + str(compiler) + '\n[wasm] ' + version)
        return compiler, environment, version

    try:
        compiler, environment, version = Compiler()
        inherited = options.session or os.environ.get('CLOUD_WASM_SESSION')
        destination = options.commit_to or (None if inherited else sdk.SDK_ROOT)
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
            sdk.SDK_ROOT = Path(json.loads(session.read_text(encoding='utf-8'))['sdk']).resolve()
        else:
            lockfile = destination / 'lib/WasmBuild.lock'
            lockfile.parent.mkdir(parents=True, exist_ok=True)
            lock = os.open(str(lockfile), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            sdk.stage_sdk(directory / 'sdk')
            sdk.SDK_ROOT = directory / 'sdk'
            marker = lockfile
            if options.prepare_only:
                # Configure exits before CMake builds. Publication locks and checks the snapshot again.
                os.close(lock)
                lockfile.unlink()
                lock = None
                marker = directory / 'batch.lock'
                marker.write_text(library, encoding='utf-8')
            session = directory / 'session.json'
            state = dict(sdk=str(sdk.SDK_ROOT), bits=bits, mode=options.mode, nodes={}, active=[], lock=str(marker))
            session.write_text(json.dumps(state), encoding='utf-8')
        os.environ['CLOUD_WASM_SESSION'] = str(session)
        environment.update(CLOUD_WASM_SESSION=str(session), CLOUD_WASM_PARENT=library,
                           CLOUD_REBUILD_PYTHON=sys.executable, CLOUD_REBUILD_SDK_ROOT=sdk.SDK_ROOT.as_posix(),
                           XUEYELINGSHUANG=root.as_posix() + '/')
        if not options.publish_only:
            if sdk.session_node(session, library, options.linkage, bits, options.mode, options.scope,
                                os.environ.get('CLOUD_WASM_PARENT')):
                sys.exit(0)
            if options.scope != 'only':
                environment.setdefault('CLOUD_REBUILD', (root / 'lib' / ('CloudRebuild1.2.exe' if os.name == 'nt' else 'CloudRebuild1.2')).as_posix())
                receipt = directory / 'before-inputs.txt'
                receipt.write_text('', encoding='utf-8')
                environment['CLOUD_REBUILD_INPUTS'] = receipt.as_posix()
                environment['CLOUD_REBUILD_PRIVATE_INCLUDE'] = (directory / 'include').as_posix()
                before = root / 'src' / library / 'scripts' / ('before_' + library + '.sh')
                bash = shutil.which('bash') or str(Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Git/bin/bash.exe')
                Run([bash, before.as_posix(), options.target, 'lib' if options.scope == 'librely' else options.linkage,
                     options.mode, 'same'], environment)
                sdk.before_receipt(session, library, receipt)
        inputs = sdk.build_inputs(library, bits, options.mode, options.linkage, options.scope, directory)
        planned = dict(sdk.BUILD_LINKAGES)
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
            for include in inputs['includes']: flags += ['-I', include]
            objects = []
            for index, source in enumerate(inputs['sources']):
                obj = directory / (str(index) + '-' + source.name + '.o')
                c = source.suffix == '.c'
                Run([sys.executable, compiler / ('emcc.py' if c else 'em++.py'), '-std=c11' if c else '-std=c++11',
                     '-c', source, '-o', obj] + flags, environment)
                objects.append(obj)
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
                        '-sINITIAL_MEMORY=' + (sdk.sdk_setting(library, 'WASM_BUILD_INITIAL_MEMORY') or ['16777216'])[0],
                        '-sMAXIMUM_MEMORY=' + (sdk.sdk_setting(library, 'WASM_BUILD_MAXIMUM_MEMORY') or ['134217728'])[0],
                        '-sSTACK_SIZE=1048576', '-sDISABLE_EXCEPTION_CATCHING=0', '-sALLOW_TABLE_GROWTH=1',
                        '-sEXPORTED_RUNTIME_METHODS=["addFunction","removeFunction"]']
                if sdk.sdk_setting(library, 'WASM_BUILD_BIGINT'): link += ['-sWASM_BIGINT=1']
                if inputs['conversions']:
                    link += ['-sSIGNATURE_CONVERSIONS=@' + str(conversions)]
                if inputs['runtime']:
                    for value in sdk.sdk_setting(library, 'WASM_BUILD_RUNTIME_JS_LIBRARY') or []:
                        link += ['--js-library', root / 'src' / library / value]
                # Project archives are retained for source-marked exports; vendor members follow real references.
                link += ['-Wl,--whole-archive'] + inputs['archives'] + ['-Wl,--no-whole-archive', '-Wl,--start-group']
                link += inputs['vendor_paths'] + ['-Wl,--end-group']
                Run([sys.executable, compiler / 'em++.py'] + objects + link + ['-o', directory / (stem + '.js')], environment)
            # Recheck prepared inputs after compilation, before touching the public SDK.
            sdk.build_inputs(library, bits, options.mode, options.linkage, options.scope, directory)
        sdk.publish(library, directory, bits, options.mode, version, compiler, inputs['vendors'])
        sdk.complete_node(session, library)
        if destination:
            if options.publish_only:
                destination = destination.resolve()
                lockfile = destination / 'lib/WasmBuild.lock'
                lockfile.parent.mkdir(parents=True, exist_ok=True)
                lock = os.open(str(lockfile), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
                sdk.validate_staged_origin(destination)
            sdk.publish_tree(destination, planned, bits, options.mode)
        sys.exit(0)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        sys.stderr.write('[wasm] ' + library + ' FAILED: ' + str(error) + '\n')
        sys.exit(1)
    finally:
        if lock is not None:
            os.close(lock)
            lockfile.unlink()

# Native Windows build.
import time
import ctypes
import shutil
import subprocess
IsPy3 = sys.version_info[0] >= 3
if IsPy3:
    import winreg
else:
    import codecs
    import _winreg as winreg

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
UseMSBuild = True #默认用MSBuild编译，如果为False则用IncrediBuild编译

#不同项目只需修改下面5个变量
SlnFile = '../NetPeer.sln' #相对于本py脚本路径的相对路径
UpdateDir = [] #相对于本py脚本路径的相对路径，填空不更新
ExecBatList = [] #相对于本py脚本路径的相对路径，编译前调用的脚本，可填空，执行bat会先cd到bat目录再执行
MSBuildFirstProjects = [r'NetPeer'] #使用MSBuild需要工程文件在解决方案sln中的路径
     # MSBuild首先编译的项目，填空不指定顺序
IncrediBuildFirstProjects = ['NetPeer'] #使用IncrediBuild只需工程名字
     #IncrediBuild首先编译的项目，填空不指定顺序

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

class Coord(ctypes.Structure):
    _fields_ = [('X', ctypes.c_short), ('Y', ctypes.c_short)]

class SmallRect(ctypes.Structure):
    _fields_ = [('Left', ctypes.c_short),
               ('Top', ctypes.c_short),
               ('Right', ctypes.c_short),
               ('Bottom', ctypes.c_short),
              ]

class ConsoleScreenBufferInfo(ctypes.Structure):
    _fields_ = [('dwSize', Coord),
               ('dwCursorPosition', Coord),
               ('wAttributes', ctypes.c_uint),
               ('srWindow', SmallRect),
               ('dwMaximumWindowSize', Coord),
              ]

class Win32API():
    '''Some native methods for python calling'''
    StdOutputHandle = -11
    ConsoleOutputHandle = None
    DefaultColor = None

    @staticmethod
    def SetConsoleColor(color):
        '''Change the text color on console window'''
        if not Win32API.DefaultColor:
            if not Win32API.ConsoleOutputHandle:
                Win32API.ConsoleOutputHandle = ctypes.windll.kernel32.GetStdHandle(Win32API.StdOutputHandle)
            bufferInfo = ConsoleScreenBufferInfo()
            ctypes.windll.kernel32.GetConsoleScreenBufferInfo(Win32API.ConsoleOutputHandle, ctypes.byref(bufferInfo))
            Win32API.DefaultColor = int(bufferInfo.wAttributes & 0xFF)
        if IsPy3:
            sys.stdout.flush() # need flush stdout in python 3
        ctypes.windll.kernel32.SetConsoleTextAttribute(Win32API.ConsoleOutputHandle, color)

    @staticmethod
    def ResetConsoleColor():
        '''Reset the default text color on console window'''
        if IsPy3:
            sys.stdout.flush() # need flush stdout in python 3
        ctypes.windll.kernel32.SetConsoleTextAttribute(Win32API.ConsoleOutputHandle, Win32API.DefaultColor)

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
        if not writeToFile:
            return
        if IsPy3:
            logFile = open(Logger.LogFile, 'a+', encoding = 'utf-8')
        else:
            logFile = codecs.open(Logger.LogFile, 'a+', 'utf-8')
        try:
            logFile.write(log)
            # logFile.flush() # need flush in python 3, otherwise log won't be saved
        except Exception as ex:
            logFile.close()
            sys.stdout.write('can not write log with exception: {0} {1}'.format(type(ex), ex))

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

    @staticmethod
    def DeleteLog():
        if os.path.exists(Logger.LogFile):
            os.remove(Logger.LogFile)


def GetMSBuildPath():
    if Bit == 'Win32':
        cmd = 'call "%VS120COMNTOOLS%..\\..\\VC\\vcvarsall.bat" x86\nwhere msbuild'
    elif Bit == 'x64':
        cmd = 'call "%VS120COMNTOOLS%..\\..\\VC\\vcvarsall.bat" amd64\nwhere msbuild'
    ftemp = open('GetMSBuildPath.bat', 'wt')
    ftemp.write(cmd)
    ftemp.close()
    p = subprocess.Popen('GetMSBuildPath.bat', stdout = subprocess.PIPE)
    p.wait()
    lines = p.stdout.read().decode().splitlines()
    os.remove('GetMSBuildPath.bat')
    for line in lines:
        if 'MSBuild.exe' in line:
            return line

def GetIncrediBuildPath():
    try:
        key=winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Classes\IncrediBuild.MonitorFile\shell\open\command')
        value, typeId = winreg.QueryValueEx(key, '')
        if value:
            start = value.find('"')
            end = value.find('"', start + 1)
            path = value[start+1:end]
            buildConsole = os.path.join(os.path.dirname(path), 'BuildConsole.exe')
            return buildConsole
    except FileNotFoundError as e:
        Logger.WriteLine('can not find IncrediBuild', ConsoleColor.Red)

def UpdateCode():
    # put git to path first
    if not shutil.which('git.exe'):
        Logger.Log('找不到git.exe. 请确认安装git时将git\bin目录路径加入到环境变量path中!!!\n, 跳过更新代码!!!', ConsoleColor.Yellow)
        return false
    oldDir = os.getcwd()
    for dir in UpdateDir:
        os.chdir(dir)
        ret = os.system('git pull')
        os.chdir(oldDir)
        if ret != 0:
            Logger.Log('update {0} failed'.format(dir), ConsoleColor.Yellow)
            return false
    return True

def BuildProject(cmd):
    for i in range(6):
        Logger.WriteLine(cmd, ConsoleColor.Cyan)
        buildFailed = True
        startTime = time.time()
        p = subprocess.Popen(cmd) #IncrediBuild不能使用stdout=subprocess.PIPE，否则会导致p.wait()不返回，可能是IncrediBuild的bug
        if IsPy3:
            try:
                buildFailed = p.wait(BuildTimeout)
            except subprocess.TimeoutExpired as e:
                Logger.Log('{0}'.format(e), ConsoleColor.Yellow)
                p.kill()
        else:
            buildFailed = p.wait()
        if not UseMSBuild:
            #IncrediBuild的返回值不能说明编译是否成功，需要提取输出判断
            fin = open('IncrediBuild.log')
            for line in fin:
                if line.startswith('=========='):
                    Logger.Write(line, ConsoleColor.Cyan, writeToFile = True if IsPy3 else False)
                    if IsPy3:
                        start = line.find('失败') + 3  #========== 生成: 成功 1 个，失败 0 个，最新 0 个，跳过 0 个 ==========
                    else:#为了兼容py2做的特殊处理，很恶心
                        start = 0
                        n2 = 0
                        while 1:
                            if line[start].isdigit():
                                n2 += 1
                                if n2 == 2:
                                    break
                                start = line.find(' ', start)
                            start += 1
                    end = line.find(' ', start)
                    failCount = int(line[start:end])
                    buildFailed = failCount > 0
                else:
                    Logger.Write(line, ConsoleColor.Red, writeToFile = True if IsPy3 else False, printToStdout = True if ' error ' in line else False)
            fin.close()
        costTime = time.time() - startTime
        Logger.WriteLine('build cost time: {0:.1f}s\n'.format(costTime), ConsoleColor.Green)
        if not buildFailed:
            return True
    return False

def BuildAllProjects():
    buildSuccess = False
    cmds = []
    if UseMSBuild:
        if IsRebuild:
            if CleanAll:
                cmds.append('{0} {1} /t:Clean /p:Configuration={2} /nologo /maxcpucount /filelogger /consoleloggerparameters:ErrorsOnly'.format(MSBuild, SlnFile, 'Debug'))
                cmds.append('{0} {1} /t:Clean /p:Configuration={2} /nologo /maxcpucount /filelogger /consoleloggerparameters:ErrorsOnly'.format(MSBuild, SlnFile, 'Release'))
            else:
                cmds.append('{0} {1} /t:Clean /p:Configuration={2} /nologo /maxcpucount /filelogger /consoleloggerparameters:ErrorsOnly'.format(MSBuild, SlnFile, BuildType))
        for project in MSBuildFirstProjects:
            cmds.append('{0} {1} /t:{2} /p:Configuration={3};platform={4} /nologo /maxcpucount /filelogger /consoleloggerparameters:ErrorsOnly'.format(MSBuild, SlnFile,  project, BuildType, Bit))
        cmds.append('{0} {1} /p:Configuration={2};platform={3} /nologo /maxcpucount /filelogger /consoleloggerparameters:ErrorsOnly'.format(MSBuild, SlnFile, BuildType, Bit))
    else: #IncrediBuild
        if IsRebuild:
            if CleanAll:
                cmds.append('"{0}" {1} /clean /cfg="{2}|{3}" /nologo /out=IncrediBuild.log'.format(IncrediBuild, SlnFile, 'Debug', Bit))
                cmds.append('"{0}" {1} /clean /cfg="{2}|{3}" /nologo /out=IncrediBuild.log'.format(IncrediBuild, SlnFile, 'Release', Bit))
            else:
                cmds.append('"{0}" {1} /clean /cfg="{2}|{3}" /nologo /out=IncrediBuild.log'.format(IncrediBuild, SlnFile, BuildType, Bit))
        for project in IncrediBuildFirstProjects:
            cmds.append('"{0}" {1} /build /prj={2} /cfg="{3}|{4}" /nologo /out=IncrediBuild.log'.format(IncrediBuild, SlnFile, project, BuildType, Bit))
        cmds.append('"{0}" {1} /build /cfg="{2}|{3}" /nologo /out=IncrediBuild.log'.format(IncrediBuild, SlnFile, BuildType, Bit))
    for cmd in cmds:
        buildSuccess = BuildProject(cmd)
        if not buildSuccess:
            break
    return buildSuccess

def main():
    if UseMSBuild:
        if not os.path.exists(MSBuild):
            Logger.Log('can not find msbuild.exe', ConsoleColor.Red)
            return 1
    else:
        if not os.path.exists(IncrediBuild):
            Logger.Log('can not find msbuild.exe', ConsoleColor.Red)
            return 1
    dir = os.path.dirname(__file__)
    if dir:
        oldDir = os.getcwd()
        os.chdir(dir)
    if Update:
        if not UpdateCode():
            return 1
        Logger.Log('git update succeed', ConsoleColor.Green)
    if Copy:
        for bat in ExecBatList:
            oldBatDir = os.getcwd()
            batDir = os.path.dirname(bat)
            batName = os.path.basename(bat)
            if batDir:
                os.chdir(batDir)
            start = time.clock()
            os.system(batName)
            Logger.Log('run "{}" cost {:.1f} seconds'.format(batName, time.clock() - start), ConsoleColor.Green)
            if batDir:
                os.chdir(oldBatDir)
    buildSuccess = BuildAllProjects()
    if buildSuccess:
        Logger.Log('build succeed', ConsoleColor.Green)
    else:
        Logger.Log('build failed', ConsoleColor.Red)
    if dir:
        os.chdir(oldDir)
    return 0 if buildSuccess else 1

if __name__ == '__main__':
    Logger.Log('run with argv ' + str(sys.argv), ConsoleColor.Green)
    sys.argv = [x.lower() for x in sys.argv]
    start_time = time.time()
    if 'debug' in sys.argv:
        BuildType = 'Debug'
    if 'lib' in sys.argv:
        Dlllib = 'lib'
        SlnFile = '../NetPeer_lib.sln'
        MSBuildFirstProjects = [r'NetPeer_lib']
        IncrediBuildFirstProjects = ['NetPeer_lib']
    if '64' in sys.argv:
        Bit = 'x64'
    if 'build' in sys.argv:
        IsRebuild = False
        Build = 'Build'
    if 'update' in sys.argv:
        Update = True
    if 'copy' in sys.argv:
        Copy = True
    if 'clean' in sys.argv:
        CleanAll = True
    if 'incredibuild' in sys.argv:
        UseMSBuild = False
    if UseMSBuild:
        MSBuild = GetMSBuildPath()
        if not MSBuild:
            Logger.Log('can not find MSBuild.exe', ConsoleColor.Red)
            exit(1)
    else:
        IncrediBuild = GetIncrediBuildPath()
        if not IncrediBuild:
            Logger.Log('can not find BuildConsole.exe', ConsoleColor.Red)
            exit(1)
    cwd = os.getcwd()
    Logger.WriteLine('current dir is: {0}, {1}: {2}'.format(cwd, Build, BuildType))
    ret = main()
    end_time = time.time()
    cost_time = end_time-start_time
    Logger.WriteLine('all build cost time: {0:.2f} seconds'.format(cost_time), ConsoleColor.Green)
    exit(ret)