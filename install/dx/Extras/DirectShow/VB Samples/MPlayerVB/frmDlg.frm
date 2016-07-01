VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmDlg 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "About DirectX Media Player VB Sample"
   ClientHeight    =   1455
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   3975
   ControlBox      =   0   'False
   Icon            =   "frmDlg.frx":0000
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1455
   ScaleWidth      =   3975
   Begin VB.CommandButton cmdOk 
      Cancel          =   -1  'True
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   330
      Left            =   2880
      TabIndex        =   3
      Top             =   940
      Width           =   975
   End
   Begin MSComctlLib.ImageList ctrlImageList 
      Left            =   0
      Top             =   840
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   48
      ImageHeight     =   48
      MaskColor       =   16711935
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   1
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmDlg.frx":0442
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.Toolbar tbMaskedImage 
      Height          =   810
      Left            =   120
      TabIndex        =   4
      Top             =   150
      Width           =   570
      _ExtentX        =   1005
      _ExtentY        =   1429
      ButtonWidth     =   2831
      ButtonHeight    =   1429
      ToolTips        =   0   'False
      AllowCustomize  =   0   'False
      Wrappable       =   0   'False
      Style           =   1
      TextAlignment   =   1
      ImageList       =   "ctrlImageList"
      _Version        =   393216
      BeginProperty Buttons {66833FE8-8583-11D1-B16A-00C0F0283628} 
         NumButtons      =   1
         BeginProperty Button1 {66833FEA-8583-11D1-B16A-00C0F0283628} 
            ImageIndex      =   1
         EndProperty
      EndProperty
      Enabled         =   0   'False
   End
   Begin VB.Label lblVersion 
      Caption         =   "Version:    8.0.0"
      Height          =   195
      Left            =   840
      TabIndex        =   2
      Top             =   780
      Width           =   1755
   End
   Begin VB.Label lblCopyright 
      Caption         =   "Copyright (C) Microsoft Corporation.  All rights reserved."
      Height          =   195
      Left            =   840
      TabIndex        =   1
      Top             =   480
      Width           =   2955
   End
   Begin VB.Label lblTitle 
      Caption         =   "DirectX Media Player VB Sample"
      Height          =   195
      Left            =   840
      TabIndex        =   0
      Top             =   165
      Width           =   2955
   End
End
Attribute VB_Name = "frmDlg"
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
            
            'assign the version info
            Me.lblVersion.Caption = "Version:    " & _
            App.Major & "." & App.Minor & "." & App.Revision
            
            'center form on the primary monitor;
            'this isn't the best idea for multi monitor systems- but if
            'you get time you can extend on this functionality here..
            Me.Move (Screen.Width - Me.Width) / 2, (Screen.Height - Me.Height) / 2
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cmdOk_Click
            ' * procedure description:  Occurs when the 'OK' button is depressed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub cmdOk_Click()
            On Local Error GoTo ErrLine
            
            'ensure form is no longer visible, and proceed to unload it
            Me.Move Screen.Width * -8, Screen.Height * -8: Me.Visible = False: Unload Me
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
