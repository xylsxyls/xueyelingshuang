Attribute VB_Name = "modAPIX86"
'*******************************************************************************
'*       This is a part of the Microsoft DirectX SDK Code Samples.
'*       Copyright (C) Microsoft Corporation.  All rights reserved.
'*       This source code is only intended as a supplement to
'*       Microsoft Development Tools and/or SDK documentation.
'*       See these sources for detailed information regarding the
'*       Microsoft samples programs.
'*******************************************************************************
Option Explicit
Option Base 0
Option Compare Text

' **************************************************************************************************************************************
' * PUBLIC INTERFACE- WIN32 API CONSTANTS
' *
' *
            Public Const FO_COPY = &H2
            Public Const FO_DELETE = &H3
            Public Const FO_MOVE = &H1
            Public Const FO_RENAME = &H4
            Public Const FOF_ALLOWUNDO = &H40
            Public Const FOF_CONFIRMMOUSE = &H2
            Public Const FOF_FILESONLY = &H80      ''"" on *.*, do only files
            Public Const FOF_MULTIDESTFILES = &H1
            Public Const FOF_NOCONFIRMATION = &H10      ''"" Don't prompt the user.
            Public Const FOF_NOCONFIRMMKDIR = &H200     ''"" don't confirm making any needed dirs
            Public Const FOF_NOCOPYSECURITYATTRIBS = &H800     ''"" dont copy NT file Security Attributes
            Public Const FOF_NOERRORUI = &H400     ''"" don't put up error UI
            Public Const FOF_NORECURSION = &H1000    ''"" don't recurse into directories.
            Public Const FOF_NO_CONNECTED_ELEMENTS = &H2000    ''"" don't operate on connected file elements.
            Public Const FOF_RENAMEONCOLLISION = &H8
            Public Const FOF_SILENT = &H4       ''"" don't create progress"report
            Public Const FOF_SIMPLEPROGRESS = &H100     ''"" means don't show names of files
            Public Const FOF_WANTMAPPINGHANDLE = &H20      ''"" Fill in SHFILEOPSTRUCT.hNameMappings
            Public Const MAX_PATH As Long = 255
            Public Const INVALID_HANDLE_VALUE = -1
            Public Const SEM_FAILCRITICALERRORS = &H1
            Public Const SEM_NOOPENFILEERRORBOX = &H8000
            Public Const SHGFI_ICON = &H100                        'get icon
            Public Const SHGFI_SMALLICON = &H1                 'get small icon
            Public Const SWP_NOSIZE  As Long = &H1           'no size
            Public Const SWP_NOMOVE  As Long = &H2         'no move
            Public Const HWND_TOPMOST  As Long = -1        'topmost zorder
            Public Const HWND_NOTOPMOST  As Long = -2   'notopmost zorder
            
' **************************************************************************************************************************************
' * PUBLIC INTERFACE- WIN32 API DATA STRUCTURES
' *
' *
            Public Type FILETIME
                    dwLowDateTime As Long
                    dwHighDateTime As Long
            End Type
            
            Public Type WIN32_FIND_DATA
                    dwFileAttributes As Long
                    ftCreationTime As FILETIME
                    ftLastAccessTime As FILETIME
                    ftLastWriteTime As FILETIME
                    nFileSizeHigh As Long
                    nFileSizeLow As Long
                    dwReserved0 As Long
                    dwReserved1 As Long
                    cFileName As String * MAX_PATH
                    cAlternate As String * 14
            End Type
            
            Public Type SHFILEINFO
                    hIcon As Long                      '  out: icon
                    iIcon As Long          '  out: icon index
                    dwAttributes As Long               '  out: SFGAO_ flags
                    szDisplayName As String * MAX_PATH '  out: display name (or path)
                    szTypeName As String * 80         '  out: type name
            End Type

            Public Type SHFILEOPSTRUCT
                    hWnd As Long
                    wFunc As Long
                    pFrom As String
                    pTo As String
                    fFlags As Integer
                    fAnyOperationsAborted As Long
                    hNameMappings As Long
                    lpszProgressTitle As String '  only used if FOF_SIMPLEPROGRESS
            End Type


' **************************************************************************************************************************************
' * PUBLIC INTERFACE- WIN32 API DECLARATIONS
' *
' *
            'win32 api declarations for obtaining using general path/ui functionality
            Public Declare Function FindClose Lib "kernel32" (ByVal hFindFile As Long) As Long
            Public Declare Function SetErrorMode Lib "kernel32" (ByVal wMode As Long) As Long
            Public Declare Function SHFileOperation Lib "shell32.dll" Alias "SHFileOperationA" (lpFileOp As SHFILEOPSTRUCT) As Long
            Public Declare Function FindFirstFile Lib "kernel32" Alias "FindFirstFileA" (ByVal lpFileName As String, lpFindFileData As WIN32_FIND_DATA) As Long
            Public Declare Function GetTempPath Lib "kernel32" Alias "GetTempPathA" (ByVal nBufferLength As Long, ByVal lpBuffer As String) As Long
            Public Declare Function ImageList_ReplaceIcon Lib "comctl32.dll" (ByVal ImgListHandle As Long, ByVal Index As Long, ByVal IconhWnd As Long) As Long 'replace an icon in a imagelist
            Public Declare Function SHGetFileInfo Lib "shell32.dll" Alias "SHGetFileInfoA" (ByVal pszPath As String, ByVal dwFileAttributes As Long, psfi As SHFILEINFO, ByVal cbFileInfo As Long, ByVal uFlags As Long) As Long
            'win32 api declarations using winmm.dll for obtaining soundcard attached/detached information
            Public Declare Function midiOutGetNumDevs Lib "winmm" () As Integer 'returns the number of midi out devices
            Public Declare Function waveOutGetNumDevs Lib "winmm.dll" () As Long 'returns the number of wave out devices
            'win32 api declaration for using user32.dll for assigning the relative position of a window within the context of a z-plane
            Public Declare Function SetWindowPos Lib "user32" (ByVal hWnd As Long, ByVal hWndInsertAfter As Long, ByVal X As Long, ByVal Y As Long, ByVal cx As Long, ByVal cy As Long, ByVal wFlags As Long) As Long 'set the window position
