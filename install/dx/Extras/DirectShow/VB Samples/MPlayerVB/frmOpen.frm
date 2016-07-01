VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmOpen 
   BorderStyle     =   3  'Fixed Dialog
   ClientHeight    =   2265
   ClientLeft      =   60
   ClientTop       =   330
   ClientWidth     =   5085
   ClipControls    =   0   'False
   Icon            =   "frmOpen.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2265
   ScaleWidth      =   5085
   ShowInTaskbar   =   0   'False
   Visible         =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin MSComDlg.CommonDialog ctrlCommonDialog 
      Left            =   4500
      Top             =   720
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "Browse..."
      Height          =   375
      Left            =   3780
      TabIndex        =   3
      Top             =   1800
      Width           =   1110
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   2580
      TabIndex        =   2
      Top             =   1800
      Width           =   1110
   End
   Begin VB.CommandButton cmdOk 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   375
      Left            =   1380
      TabIndex        =   1
      Top             =   1800
      Width           =   1110
   End
   Begin MSComctlLib.ImageCombo imgcmbOpenURL 
      Height          =   330
      Left            =   780
      TabIndex        =   0
      Top             =   1200
      Width           =   4095
      _ExtentX        =   7223
      _ExtentY        =   582
      _Version        =   393216
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      ImageList       =   "ctrlImageListDropDown"
   End
   Begin MSComctlLib.ImageList ctrlImageList 
      Left            =   0
      Top             =   1680
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
            Picture         =   "frmOpen.frx":0442
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.Toolbar tbMaskedImage 
      Height          =   810
      Left            =   120
      TabIndex        =   4
      Top             =   120
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
   Begin MSComctlLib.ImageList ctrlImageListDropDown 
      Left            =   600
      Top             =   1680
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   16711935
      UseMaskColor    =   0   'False
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   1
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmOpen.frx":0954
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Label lblDescriptor 
      Caption         =   "Type the address of a movie or audio file (on the Internet or your computer) and the player will open it for you."
      Height          =   435
      Left            =   780
      TabIndex        =   6
      Top             =   180
      Width           =   4155
   End
   Begin VB.Label lblOpen 
      Caption         =   "Open"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   1260
      Width           =   555
   End
End
Attribute VB_Name = "frmOpen"
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

Private m_bstrURL As String 'returned string
' **************************************************************************************************************************************
' * FRIEND INTERFACE- FUNCTIONALITY
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: LoadPaths
            ' * procedure description: Load the last used paths from the historical list.
            ' *                                       When loading the last used paths into a dropdown combo; the overall action
            ' *                                       Is going to be slow is you opt to include icons for each item.  This is a simple
            ' *                                       Implementation that stores the last 10 or so paths the user types in.
            ' *                                       This method will then persist the results in the following registry key:
            ' *
            ' *                                       "HKEY_USERS\S-1-5-21-2127521184-1604012920-1887927527-208863
            ' *                                       \Software\VB and VBA Program Settings\VB_MPlayer\8" Where S-1...etc.. is the
            ' *                                       GUID for the current user and \VB_MPlayer\8 is the apps product name and version
            ' *
            ' ******************************************************************************************************************************
            Friend Sub LoadPaths(Optional Limit As Long = 5, Optional Iconic As Boolean = True)
            Dim nCount As Long
            Dim nCount2 As Long
            Dim bstrHistorical As String
            Dim aryHistoricalList() As String
            On Local Error GoTo ErrLine
            
            'center form on the primary monitor;
            'this isn't the best idea for multi monitor systems- but if
            'you get time you can extend on this functionality here..
            imgcmbOpenURL.Locked = True
            Me.MousePointer = vbHourglass: imgcmbOpenURL.Text = "Loading..."
            
            'obtain the last few file paths the user has attempted, and post them to
            'the drop down list for ease of use; you can extend on this implementation
            'if you get some extra free time on your hands..
            bstrHistorical = GetSetting(App.ProductName, App.Major, "recent", vbNullString)
            If InStr(1, bstrHistorical, ";") > 0 Then aryHistoricalList = Split(bstrHistorical, ";")
            If bstrHistorical = vbNullString Or Len(Trim(LCase(bstrHistorical))) <= 2 Then GoTo ErrLine
            
            'enumerate through all the files that were appended to the list; and
            'proceed to append them to the drop down combot box for the user..
            Call ctrlImageListDropDown.ListImages.Clear 'begin by purging all items
            ctrlImageListDropDown.ImageHeight = 16: ctrlImageListDropDown.ImageWidth = 16
            For nCount = UBound(aryHistoricalList) To LBound(aryHistoricalList) Step -1: DoEvents
               If nCount2 = Limit Then Exit For
               If Not aryHistoricalList(nCount) = vbNullString Then
                  If InStr(1, LCase(Trim(aryHistoricalList(nCount))), "://") > 0 Then
                     'append the path as it is a url and cannot be quickly verified
                     'the icon in this case will need to be done manually for each type
                     'of protocol (i.e. http://, ftp://, res://, etc..) in order to be truely accurate
                     nCount2 = (nCount2 + 1)
                     If Iconic Then
                        ctrlImageListDropDown.ListImages.Add , , LoadResPicture(102, vbResIcon)
                        imgcmbOpenURL.ComboItems.Add , , aryHistoricalList(nCount2), nCount2
                     Else: imgcmbOpenURL.ComboItems.Add , , aryHistoricalList(nCount2)
                     End If
                  Else
                     'append the file as it is a local or network unc file path that has been validated
                     'in this case the icon that need be added is the associated icon for the file type
                     nCount2 = (nCount2 + 1)
                     If Iconic Then
                        ctrlImageListDropDown.ListImages.Add , , AppGetFileIcon(aryHistoricalList(nCount))
                        imgcmbOpenURL.ComboItems.Add , , aryHistoricalList(nCount), nCount2
                     Else: imgcmbOpenURL.ComboItems.Add , , aryHistoricalList(nCount)
                     End If
                  End If
               End If
            Next
            
            'reset mouse cursor
            Me.MousePointer = vbDefault
            imgcmbOpenURL.Locked = False
            'assign the last eligible file path as the elected one
            If imgcmbOpenURL.ComboItems.Count > 0 Then _
               imgcmbOpenURL.ComboItems(1).Selected = True
            Exit Sub
            
ErrLine:
            Err.Clear
            Me.MousePointer = vbDefault
            imgcmbOpenURL.Locked = False
            imgcmbOpenURL.Text = vbNullString
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: UpdatePaths
            ' * procedure description:  Updates the historical path list
            ' *
            ' ******************************************************************************************************************************
            Friend Sub UpdatePaths(Optional Purge As Boolean = False)
            Dim bstrHistorical As String
            On Local Error GoTo ErrLine
            
            'consider optional
            'purge of all items
            If Purge Then _
               Call SaveSetting(App.ProductName _
                      , App.Major, "recent", vbNullString)
            
            'append the current file to the list if it exists
            If Not imgcmbOpenURL.Text = vbNullString Then
               If AppVerifyFileExists(LCase(Trim(imgcmbOpenURL.Text))) Then
                  bstrHistorical = GetSetting(App.ProductName, App.Major, "recent", vbNullString)
                  If Not InStr(1, LCase(Trim(bstrHistorical)), LCase(Trim(imgcmbOpenURL.Text))) > 0 Then
                     'this file path has not yet been appended to the list, so we will need to add it
                     bstrHistorical = Trim(LCase(bstrHistorical & imgcmbOpenURL.Text & ";"))
                     Call SaveSetting(App.ProductName, App.Major, "recent", bstrHistorical)
                  Else
                     'the given file has already been appended to the list, but we will need to move it
                     'to the end of the list so that the last opened file appears selected when the dialog is opened..
                     bstrHistorical = Replace(LCase(Trim(bstrHistorical)), LCase(Trim(imgcmbOpenURL.Text & ";")), vbNullString)
                     bstrHistorical = Trim(LCase(bstrHistorical & imgcmbOpenURL.Text & ";"))
                     Call SaveSetting(App.ProductName, App.Major, "recent", bstrHistorical)
                  End If
               End If
            End If
            Exit Sub

ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: CurrentPath
            ' * procedure description:  Obtain the currently elected path entered by the user.
            ' *
            ' ******************************************************************************************************************************
            Friend Property Get CurrentPath() As String
            On Local Error GoTo ErrLine
            
            'return the current path
            CurrentPath = m_bstrURL
            Exit Property

ErrLine:
            Err.Clear
            Exit Property
            End Property


' **************************************************************************************************************************************
' * PRIVATE INTERFACE- FORM EVENTS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: Form_Load
            ' * procedure description:  Occurs when the form becomes visible.
            ' *
            ' ******************************************************************************************************************************
            Private Sub Form_Load()
            On Local Error GoTo ErrLine
            
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
            


' **************************************************************************************************************************************
' * PRIVATE INTERFACE- CONTROL EVENTS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: cmdBrowse_Click
            ' * procedure description:
            ' *
            ' ******************************************************************************************************************************
            Private Sub cmdBrowse_Click()
            On Local Error GoTo ErrLine
            Dim bstrFileName As String
            On Local Error GoTo ErrLine
            
            'query the user for a media file
            ctrlCommonDialog.DefaultExt = "AVI"
            ctrlCommonDialog.InitDir = vbNullString
            ctrlCommonDialog.Filter = "Media Files (" & SUPPORTED_MEDIA & "|"
            Call ctrlCommonDialog.ShowOpen: bstrFileName = ctrlCommonDialog.FileName
            
            If bstrFileName = vbNullString Then
               Exit Sub
            Else: imgcmbOpenURL.Text = bstrFileName
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cmdCancel_Click
            ' * procedure description:  Occurs when the 'Cancel' button is depressed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub cmdCancel_Click()
            On Local Error GoTo ErrLine
            
            'ensure form is no longer visible, the form is not being unloaded
            'due to the fact that the caller is handling this, as it's being used
            'like an interfactive dialog for all intensive purposes.
            Me.Move Screen.Width * -8, Screen.Height * -8: Me.Visible = False
            
            'return focus to the main form
            Call frmMain.SetFocus
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
            
            'return path
            m_bstrURL = imgcmbOpenURL.Text
            
            'ensure form is no longer visible, the form is not being unloaded
            'due to the fact that the caller is handling this, as it's being used
            'like an interfactive dialog for all intensive purposes.
            Me.Move Screen.Width * -8, Screen.Height * -8: Me.Visible = False
            
            'return focus to the main form
            Call frmMain.SetFocus
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
