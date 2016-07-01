Attribute VB_Name = "modApplicationEntry"
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
' * APPLICATION ENTRY POINT
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: Main
            ' * procedure description:  main application entry point
            ' *
            ' ******************************************************************************************************************************
            Public Sub Main()
            On Local Error GoTo ErrLine
            
            Load frmMain: frmMain.Show
            If Command <> vbNullString Then Call AppAnswerCommandLine(Command)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
