VERSION 5.00
Begin VB.Form frmHelp 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Windows Media Player Error"
   ClientHeight    =   2445
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5865
   ClipControls    =   0   'False
   ControlBox      =   0   'False
   Icon            =   "frmHelp.frx":0000
   LinkTopic       =   "frmIDEMessageBoxMpErr"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2445
   ScaleWidth      =   5865
   ShowInTaskbar   =   0   'False
   StartUpPosition =   2  'CenterScreen
   Visible         =   0   'False
   Begin VB.TextBox txtAdvanced 
      BackColor       =   &H8000000F&
      Height          =   1815
      Left            =   60
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   3
      Top             =   2550
      Width           =   5715
   End
   Begin VB.CommandButton cmdAdvanced 
      Caption         =   "Advanced >>"
      Height          =   375
      Left            =   4500
      TabIndex        =   1
      Top             =   720
      Width           =   1110
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Default         =   -1  'True
      Height          =   375
      Left            =   4500
      TabIndex        =   0
      Top             =   240
      Width           =   1110
   End
   Begin VB.PictureBox picIconic 
      Appearance      =   0  'Flat
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   615
      Left            =   180
      Picture         =   "frmHelp.frx":0442
      ScaleHeight     =   615
      ScaleWidth      =   615
      TabIndex        =   5
      TabStop         =   0   'False
      Top             =   300
      Width           =   615
   End
   Begin VB.Frame fraTip 
      Height          =   1035
      Left            =   60
      TabIndex        =   4
      Top             =   1320
      Width           =   5715
      Begin VB.CommandButton cmdHelp 
         Caption         =   "More Help..."
         Height          =   375
         Left            =   4440
         TabIndex        =   2
         Top             =   360
         Width           =   1110
      End
      Begin VB.PictureBox picIconicHelp 
         Appearance      =   0  'Flat
         BorderStyle     =   0  'None
         ForeColor       =   &H80000008&
         Height          =   495
         Left            =   300
         Picture         =   "frmHelp.frx":08AA
         ScaleHeight     =   495
         ScaleWidth      =   315
         TabIndex        =   6
         TabStop         =   0   'False
         Top             =   300
         Width           =   315
      End
      Begin VB.Label lblGenericHelpText 
         Caption         =   "Click on ""More Help"" to launch your browser and get help on this problem from the Microsoft web site."
         Height          =   615
         Left            =   900
         TabIndex        =   8
         Top             =   240
         Width           =   3195
      End
   End
   Begin VB.Label lblErrorDescription 
      Caption         =   "Cannot open.  Please verify that the path and filename are correct and try again."
      Height          =   795
      Left            =   1020
      TabIndex        =   7
      Top             =   480
      Width           =   3015
   End
End
Attribute VB_Name = "frmHelp"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
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

Private m_boolOfferOnlineHelp As Boolean 'offer to navigate to ms web site
Private m_bstrSuspectedNavigationURL As String 'navigation url to ms web site
' **************************************************************************************************************************************
' * FRIEND INTERFACE- FORM FUNCTIONALITY
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: ShowError
            ' * procedure description:  A friend method called by the application.  This allows it to carry over the error info.
            ' *                                        That is to be displayed after it is loaded by the caller.
            ' ******************************************************************************************************************************
            Friend Sub ShowError(Number As Long, bstrErrorDescription As String, Optional bstrAdvancedDescription As String, Optional OfferOnlineHelp As Boolean = True)
            On Local Error GoTo ErrLine
            
            'map error to dialog
            Me.lblErrorDescription.Caption = bstrErrorDescription
            If Not bstrAdvancedDescription = vbNullString Then
               frmHelp.txtAdvanced.Text = bstrAdvancedDescription & Space(1) & "(Error=" & Number & ")"
            Else: frmHelp.txtAdvanced.Text = bstrErrorDescription & Space(1) & "(Error=" & Number & ")"
            End If
            
            'offer extended online help services given passed optional
            If OfferOnlineHelp Then m_boolOfferOnlineHelp = True
            m_bstrSuspectedNavigationURL = "http://www.microsoft.com/windows/windowsmedia/support/mperr.asp?prd=Windows&spb=MediaPlayer&pver=6.4.9.1109&os=WinNT&over=5.0.2195&olcid=0x409&clcid=0x409&id=" & Number
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub



' **************************************************************************************************************************************
' * PRIVATE INTERFACE- FORM / CONTROL EVENTS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: Form_Load
            ' * procedure description:  Occurs when the form becomes visible.
            ' *
            ' ******************************************************************************************************************************
            Private Sub Form_Load()
            On Local Error GoTo ErrLine
            
            'default enabled state of
            'textbox to clear up any tabstop issue(s0
            Me.txtAdvanced.Enabled = False
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: Form_QueryUnload
            ' * procedure description:  Occurs before a form or application closes.
            ' *
            ' ******************************************************************************************************************************
            Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
            On Local Error GoTo ErrLine
            
            Select Case UnloadMode
                Case vbFormControlMenu
                         '0 The user chose the Close command from the Control menu on the form.
                         Cancel = 1: Me.Visible = False
                Case vbFormCode
                         '1 The Unload statement is invoked from code.
                         Exit Sub
                Case vbAppWindows
                         '2 The current Microsoft Windows operating environment session is ending.
                         Cancel = 1: Me.Visible = False
                Case vbAppTaskManager
                         '3 The Microsoft Windows Task Manager is closing the application.
                         Cancel = 1: Me.Visible = False
                Case vbFormMDIForm
                         '4 An MDI child form is closing because the MDI form is closing.
                         Cancel = 1: Me.Visible = False
                Case vbFormOwner
                         '5 A form is closing because its owner is closing
                         Cancel = 1: Me.Visible = False
            End Select
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cmdAdvanced_Click
            ' * procedure description:  Occurs when the 'Advanced >>' button is depressed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub cmdAdvanced_Click()
            Static boolDepressed As Boolean
            On Local Error GoTo ErrLine
            
            'show extended error information;
            'by resizing the form to a preset value
            If Not boolDepressed Then
               boolDepressed = True
               Me.Height = 4830
               Me.txtAdvanced.Enabled = True
               cmdAdvanced.Caption = "<< Advanced"
            Else
               boolDepressed = False
               Me.Height = 2850
               Me.txtAdvanced.Enabled = False
               cmdAdvanced.Caption = "Advanced >>"
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cmdClose_Click
            ' * procedure description:  Occurs when the 'Close' button is depressed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub cmdClose_Click()
            On Local Error GoTo ErrLine
            
            'ensure form is no longer visible, the form is not being unloaded
            'due to the fact that the caller is handling this, as it's being used
            'like an interfactive dialog for all intensive purposes. no unloading here!
            Me.Move Screen.Width * -8, Screen.Height * -8: Me.Visible = False
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cmdHelp_Click
            ' * procedure description:  Occurs when the 'Help' button is depressed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub cmdHelp_Click()
            Dim objExplorer As Object
            On Local Error GoTo ErrLine
            
            'create an instance of internet explorer;
            'and attempt to navigate to the suspected ms help site like wmp
            If m_boolOfferOnlineHelp Then
               Set objExplorer = CreateObject("InternetExplorer.Application")
               If Not objExplorer Is Nothing Then
                  objExplorer.Visible = True
                  Call objExplorer.Navigate(m_bstrSuspectedNavigationURL)
               End If
            End If
            
            'clean-up & dereference
            If Not objExplorer Is Nothing Then Set objExplorer = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
