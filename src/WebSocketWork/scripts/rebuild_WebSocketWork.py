#!python3
# -*- coding:utf-8 -*-
import os
import sys
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
SlnFile = '../WebSocketWork.sln' #相对于本py脚本路径的相对路径
UpdateDir = [] #相对于本py脚本路径的相对路径，填空不更新
ExecBatList = [] #相对于本py脚本路径的相对路径，编译前调用的脚本，可填空，执行bat会先cd到bat目录再执行
MSBuildFirstProjects = [r'WebSocketWork'] #使用MSBuild需要工程文件在解决方案sln中的路径
     # MSBuild首先编译的项目，填空不指定顺序
IncrediBuildFirstProjects = ['WebSocketWork'] #使用IncrediBuild只需工程名字
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
        SlnFile = '../WebSocketWork_lib.sln'
        MSBuildFirstProjects = [r'WebSocketWork_lib']
        IncrediBuildFirstProjects = ['WebSocketWork_lib']
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
