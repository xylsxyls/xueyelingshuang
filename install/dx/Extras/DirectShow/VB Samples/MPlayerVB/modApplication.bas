Attribute VB_Name = "modApplication"
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

'state of the application
Public Enum AppState
AppStateClosed = -1
AppStatePaused = 1
AppStatePlaying = 2
AppStateStopped = 0
End Enum

'the following media formats will be supported by this player given the typical filters available in Microsoft Windows.
Public Const SUPPORTED_MEDIA As String = "*.ac3;*.asf;*.asx;*.au;*.avi;*.bmp;*.dat;*.jpg;*.jpeg;*.mid;*.mov;*.mpg;*.mpeg;*.wav;*.aif;*.mp2;*.mp3;*.qt;*.wma;*.wmv;*.mpv"
' **************************************************************************************************************************************
' * PUBLIC INTERFACE- APPLICATION METHODS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: AppAnswerCommandLine
            ' * procedure description:  executes the given command line for the given application instance
            ' *
            ' ******************************************************************************************************************************
            Public Sub AppAnswerCommandLine(bstrCommandLine As String)
            Dim nStart As Long
            Dim nLength As Long
            Dim bstrFilePath As String
            Dim bstrFileName As String
            Dim boolOpenFile As Boolean
            Dim boolCloseOnComplete As Boolean
            On Local Error GoTo ErrLine
            
            'handle bstrCommandLine line
            If bstrCommandLine <> vbNullString Then
                  'do a preliminary parse of the command line
                  bstrCommandLine = Trim(bstrCommandLine)
                  bstrCommandLine = Replace(LCase(bstrCommandLine), LCase(App.EXEName), vbNullString)
                  'check the bstrCommandLine line argument(s) for open optional
                  If InStr(1, LCase(bstrCommandLine), Space(1) & "/open") > 0 Then
                     boolOpenFile = True
                  End If
                  'check the bstrCommandLine line argument(s) for close when finished optional
                  If InStr(1, LCase(bstrCommandLine), Space(1) & "/close") > 0 Then
                     boolCloseOnComplete = True
                  End If
                  'conduct a secondary parse of the command line
                  bstrCommandLine = Replace(LCase(bstrCommandLine), "/open", vbNullString)
                  bstrCommandLine = Replace(LCase(bstrCommandLine), "/close", vbNullString)
                  
                  'check the command line argument(s) for a valid media filename with double quotes
                  If InStr(1, LCase(bstrCommandLine), Chr(34)) > 0 Then
                     nStart = InStr(1, LCase(bstrCommandLine), Chr(34))
                     nLength = InStr(nStart + 1, bstrCommandLine, Chr(34))
                     If nLength - nStart > Len(bstrCommandLine) Then
                        bstrFilePath = Mid(bstrCommandLine, nStart, nLength)
                     End If
                     bstrFilePath = Trim(LCase(bstrCommandLine))
                     bstrFilePath = Replace(bstrFilePath, Chr(34), vbNullString)
                     bstrFilePath = Trim(bstrFilePath)
                     
                  'check the command line argument(s) for a valid media filename with single quotes
                  ElseIf InStr(1, LCase(bstrCommandLine), Chr(39)) > 0 Then
                     nStart = InStr(1, LCase(bstrCommandLine), Chr(39))
                     nLength = InStr(nStart + 1, bstrCommandLine, Chr(39))
                     If nLength - nStart > Len(bstrCommandLine) Then
                        bstrFilePath = Mid(bstrCommandLine, nStart, nLength)
                     End If
                     bstrFilePath = Trim(LCase(bstrCommandLine))
                     bstrFilePath = Replace(bstrFilePath, Chr(39), vbNullString)
                     bstrFilePath = Trim(bstrFilePath)
                         
                  'check the command line argument(s) for a valid media filename with no quotes
                  Else
                         bstrFilePath = Trim(LCase(bstrCommandLine))
                         bstrFilePath = Trim(bstrFilePath)
                  End If
            End If
                  
               
            'proceed to load the media file into the application and render it
            If bstrFilePath <> vbNullString Then
               If boolCloseOnComplete Then
                  Call frmMain.AppPlayFile(bstrFilePath, True)
                  Do Until frmMain.AppGetState = 0: DoEvents
                  Loop: Unload frmMain: Exit Sub
               Else: Call frmMain.AppPlayFile(bstrFilePath, True)
               End If
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Resume Next
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppGetFileIcon
            ' * procedure description:  Returns the icon associated with the given file, otherwise a blank picture
            ' *
            ' ******************************************************************************************************************************
            Public Function AppGetFileIcon(bstrFilePath As String) As StdPicture
            Dim nhandle As Long
            Dim Info As SHFILEINFO
            Dim objImageList As Object
            Dim objStdPicture As IPictureDisp
            On Local Error GoTo ErrLine
            
            'instantiate new imagelist
            Set objImageList = CreateObject("MSComctlLib.ImageListCtrl.2")
            'append an image to the list for replacement
            Call objImageList.ListImages.Add(, , LoadResPicture(101, vbResIcon))
            'get a handle to the icon for this filetype
            nhandle = SHGetFileInfo(bstrFilePath, 0, Info, Len(Info), SHGFI_ICON + SHGFI_SMALLICON)
            'replace the listimage we just added with the correct one
            nhandle = ImageList_ReplaceIcon(objImageList.hImageList, 0, ByVal Info.hIcon)
            'obtain the new listimage back as a standardpicture
            Set objStdPicture = objImageList.ListImages(1).ExtractIcon
            'return the icon to the user as a pointer to the OLE Automation picture
            If Not objStdPicture Is Nothing Then Set AppGetFileIcon = objStdPicture
            Exit Function
            
ErrLine:

            Err.Clear
            Set AppGetFileIcon = _
                  New StdPicture
            Exit Function
            End Function


            ' ******************************************************************************************************************************
            ' * procedure name: AppVerifyFileExists
            ' * procedure description:  Returns true if the specified file does in fact exist.
            ' *
            ' ******************************************************************************************************************************
            Public Function AppVerifyFileExists(ByVal bstrFileName As String) As Boolean
            Dim WFD As WIN32_FIND_DATA, hFile As Long
            On Local Error GoTo ErrLine
            
            WFD.cFileName = bstrFileName & vbNullChar
            hFile = FindFirstFile(bstrFileName, WFD)
            AppVerifyFileExists = hFile <> INVALID_HANDLE_VALUE
            Call FindClose(hFile)
            Exit Function
            
ErrLine:
            Err.Clear
            Exit Function
            End Function
            
            
' **************************************************************************************************************************************
' * PUBLIC INTERFACE- GENERAL PROCEDURES
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: AppParseStringBuffer
            ' * procedure description:   Parse's a fixed length string buffer of all vbNullCharacters AND vbNullStrings.
            ' *                                        Argument bstrBuffer evaluates to either an ANSII or Unicode BSTR string buffer.
            ' *                                        (bstrBuffer is almost always the output from a windows api call which needs parsed)
            ' *
            ' ******************************************************************************************************************************
            Public Function AppParseStringBuffer(bstrBuffer As String) As String
            Dim iCount As Long, bstrChar As String, bstrReturn As String
            On Local Error GoTo ErrLine
            
            For iCount = 1 To Len(bstrBuffer) 'set up a loop to remove the vbNullChar's from the buffer.
                  bstrChar = Strings.Mid(bstrBuffer, iCount, 1)
                  If bstrChar <> vbNullChar And bstrChar <> vbNullString Then bstrReturn = (bstrReturn + bstrChar)
            Next
            AppParseStringBuffer = bstrReturn
            Exit Function
            
ErrLine:
            Err.Clear
            Exit Function
            End Function
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppGetSystemTempDirectory
            ' * procedure description:  Returns a bstr String representing the fully qualified path to the system's temp directory
            ' *
            ' ******************************************************************************************************************************
            Public Function AppGetSystemTempDirectory() As String
            Dim bstrBuffer As String * MAX_PATH
            On Local Error GoTo ErrLine
            
            'call the win32api
            Call GetTempPath(MAX_PATH, bstrBuffer)
            'parse & return the value to the client
            AppGetSystemTempDirectory = AppParseStringBuffer(bstrBuffer)
            Exit Function
            
ErrLine:
            Err.Clear
            Exit Function
            End Function
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppDeleteFile
            ' * procedure description:  This will delete a File. Pass any of the specified optionals to invoke those particular features.
            ' *
            ' ******************************************************************************************************************************
            Public Function AppDeleteFile(bstrFileName As String, Optional SendToRecycleBin As Boolean = True, Optional Confirm As Boolean = True, Optional ShowProgress As Boolean = True) As Long
            Dim fileop As SHFILEOPSTRUCT
            Dim WFD As WIN32_FIND_DATA, hFile As Long
            On Local Error GoTo ErrLine
            
            'check argument
            If Right(bstrFileName, 1) = "\" Then bstrFileName = Left(bstrFileName, (Len(bstrFileName) - 1))
            'ensure the file exists
            WFD.cFileName = bstrFileName & vbNullChar
            hFile = FindFirstFile(bstrFileName, WFD)
            If hFile = INVALID_HANDLE_VALUE Then
               Call FindClose(hFile)
               Exit Function
            Else: Call FindClose(hFile)
            End If
            'set the error mode
            Call SetErrorMode(SEM_NOOPENFILEERRORBOX + SEM_FAILCRITICALERRORS)
            'set up the file operation by the specified optionals
            With fileop
                .hWnd = 0: .wFunc = FO_DELETE
                .pFrom = UCase(bstrFileName) & vbNullChar & vbNullChar
                If SendToRecycleBin Then   'goes to recycle bin
                   .fFlags = FOF_ALLOWUNDO
                   If Confirm = False Then .fFlags = .fFlags + FOF_NOCONFIRMATION  'do not confirm
                   If ShowProgress = False Then .fFlags = .fFlags + FOF_SILENT  'do not show progress
                Else 'just delete the file
                   If Confirm = False Then .fFlags = .fFlags + FOF_NOCONFIRMATION  'do not confirm
                   If ShowProgress = False Then .fFlags = .fFlags + FOF_SILENT  'do not show progress
                End If
            End With
            'execute the file operation, return any errors..
            AppDeleteFile = SHFileOperation(fileop)
            Exit Function
            
ErrLine:
            AppDeleteFile = _
                  Err.Number
            Err.Clear
            Exit Function
            End Function



