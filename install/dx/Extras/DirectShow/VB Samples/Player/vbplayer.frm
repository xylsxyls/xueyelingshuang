VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Object = "{38911DA0-E448-11D0-84A3-00DD01104159}#1.1#0"; "COMCT332.OCX"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "VB Player"
   ClientHeight    =   5010
   ClientLeft      =   105
   ClientTop       =   615
   ClientWidth     =   4350
   Icon            =   "vbplayer.frx":0000
   LinkTopic       =   "frmMain"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   OLEDropMode     =   1  'Manual
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5010
   ScaleWidth      =   4350
   Begin VB.Frame fraMain 
      BorderStyle     =   0  'None
      Height          =   4965
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   4320
      Begin VB.Frame fraChild 
         BorderStyle     =   0  'None
         Height          =   4515
         Left            =   0
         TabIndex        =   6
         Top             =   480
         Width           =   4320
         Begin VB.PictureBox picMovieWindow 
            Appearance      =   0  'Flat
            BackColor       =   &H00000000&
            BorderStyle     =   0  'None
            ForeColor       =   &H80000008&
            Height          =   2640
            Left            =   0
            OLEDropMode     =   1  'Manual
            ScaleHeight     =   2640
            ScaleWidth      =   4290
            TabIndex        =   24
            Top             =   0
            Width           =   4290
         End
         Begin VB.Frame fraInfo 
            BackColor       =   &H00000000&
            BorderStyle     =   0  'None
            Caption         =   "Frame1"
            Height          =   1440
            Left            =   0
            TabIndex        =   7
            Top             =   3060
            Width           =   4320
            Begin VB.PictureBox picPrettyWave 
               Appearance      =   0  'Flat
               BackColor       =   &H00000000&
               BorderStyle     =   0  'None
               ForeColor       =   &H80000008&
               Height          =   200
               Left            =   4050
               OLEDropMode     =   1  'Manual
               Picture         =   "vbplayer.frx":0442
               ScaleHeight     =   195
               ScaleWidth      =   195
               TabIndex        =   25
               Top             =   1200
               Width           =   200
            End
            Begin VB.Line lnSeptum2 
               BorderColor     =   &H00808080&
               X1              =   0
               X2              =   4350
               Y1              =   1125
               Y2              =   1125
            End
            Begin VB.Label lblAuthor 
               Alignment       =   1  'Right Justify
               BackColor       =   &H00000000&
               Caption         =   "Author:"
               ForeColor       =   &H00FFFFFF&
               Height          =   255
               Left            =   -375
               OLEDropMode     =   1  'Manual
               TabIndex        =   17
               Top             =   630
               Width           =   1650
            End
            Begin VB.Label lblCopyright 
               Alignment       =   1  'Right Justify
               BackColor       =   &H00000000&
               Caption         =   "Copyright: "
               ForeColor       =   &H00FFFFFF&
               Height          =   255
               Left            =   -300
               OLEDropMode     =   1  'Manual
               TabIndex        =   16
               Top             =   880
               Width           =   1650
            End
            Begin VB.Label lblState 
               Alignment       =   2  'Center
               BackColor       =   &H00000000&
               ForeColor       =   &H00FFFFFF&
               Height          =   255
               Left            =   -225
               OLEDropMode     =   1  'Manual
               TabIndex        =   15
               Top             =   1200
               Width           =   1800
            End
            Begin VB.Label lblClip 
               Alignment       =   1  'Right Justify
               BackColor       =   &H00000000&
               Caption         =   "Clip:"
               ForeColor       =   &H00FFFFFF&
               Height          =   255
               Left            =   -375
               OLEDropMode     =   1  'Manual
               TabIndex        =   14
               Top             =   375
               Width           =   1650
            End
            Begin VB.Label lblClipValue 
               BackColor       =   &H00000000&
               ForeColor       =   &H00FFFFFF&
               Height          =   240
               Left            =   1425
               OLEDropMode     =   1  'Manual
               TabIndex        =   13
               Top             =   375
               Width           =   2850
            End
            Begin VB.Label lblAuthorValue 
               BackColor       =   &H00000000&
               ForeColor       =   &H00FFFFFF&
               Height          =   240
               Left            =   1425
               OLEDropMode     =   1  'Manual
               TabIndex        =   12
               Top             =   630
               Width           =   2850
            End
            Begin VB.Label lblCopyrightValue 
               BackColor       =   &H00000000&
               ForeColor       =   &H00FFFFFF&
               Height          =   240
               Left            =   1425
               OLEDropMode     =   1  'Manual
               TabIndex        =   11
               Top             =   880
               Width           =   2850
            End
            Begin VB.Label lblCurrentTime 
               Alignment       =   1  'Right Justify
               BackColor       =   &H00000000&
               ForeColor       =   &H00FFFFFF&
               Height          =   240
               Left            =   1800
               OLEDropMode     =   1  'Manual
               TabIndex        =   10
               Top             =   1200
               Width           =   2205
            End
            Begin VB.Line lnSeptum 
               BorderColor     =   &H00808080&
               X1              =   0
               X2              =   4350
               Y1              =   300
               Y2              =   300
            End
            Begin VB.Label lblFileName 
               Alignment       =   1  'Right Justify
               BackColor       =   &H00000000&
               Caption         =   "Show:"
               ForeColor       =   &H00FFFFFF&
               Height          =   255
               Left            =   -525
               OLEDropMode     =   1  'Manual
               TabIndex        =   9
               Top             =   75
               Width           =   1800
            End
            Begin VB.Label lblFileNameValue 
               BackColor       =   &H00000000&
               ForeColor       =   &H00FFFFFF&
               Height          =   240
               Left            =   1800
               OLEDropMode     =   1  'Manual
               TabIndex        =   8
               Top             =   75
               Width           =   2400
            End
         End
         Begin ComCtl3.CoolBar cbControlBar 
            Height          =   390
            Left            =   0
            TabIndex        =   18
            Top             =   2660
            Width           =   4320
            _ExtentX        =   7620
            _ExtentY        =   688
            FixedOrder      =   -1  'True
            BandBorders     =   0   'False
            VariantHeight   =   0   'False
            OLEDropMode     =   1
            _CBWidth        =   4320
            _CBHeight       =   390
            _Version        =   "6.7.8862"
            Child1          =   "tbControlBar"
            MinWidth1       =   975
            MinHeight1      =   330
            Width1          =   975
            NewRow1         =   0   'False
            AllowVertical1  =   0   'False
            Child2          =   "fraVolume"
            MinWidth2       =   1200
            MinHeight2      =   330
            Width2          =   1200
            NewRow2         =   0   'False
            Child3          =   "fraBalance"
            MinWidth3       =   1200
            MinHeight3      =   330
            Width3          =   1200
            NewRow3         =   0   'False
            Begin VB.Frame fraBalance 
               BorderStyle     =   0  'None
               Height          =   330
               Left            =   2595
               TabIndex        =   22
               Top             =   30
               Width           =   1635
               Begin MSComctlLib.Slider slBalance 
                  Height          =   270
                  Left            =   375
                  TabIndex        =   23
                  TabStop         =   0   'False
                  ToolTipText     =   "Balance Control"
                  Top             =   0
                  Width           =   1000
                  _ExtentX        =   1773
                  _ExtentY        =   476
                  _Version        =   393216
                  OLEDropMode     =   1
                  LargeChange     =   1000
                  SmallChange     =   100
                  Min             =   -10000
                  Max             =   10000
                  TickFrequency   =   2000
               End
               Begin VB.Image imgBalance 
                  Height          =   240
                  Left            =   45
                  OLEDropMode     =   1  'Manual
                  Picture         =   "vbplayer.frx":051A
                  Top             =   45
                  Width           =   240
               End
            End
            Begin VB.Frame fraVolume 
               BorderStyle     =   0  'None
               Height          =   330
               Left            =   1200
               TabIndex        =   20
               Top             =   30
               Width           =   1200
               Begin MSComctlLib.Slider slVolume 
                  Height          =   270
                  Left            =   260
                  TabIndex        =   21
                  TabStop         =   0   'False
                  ToolTipText     =   "Volume Control"
                  Top             =   0
                  Width           =   1000
                  _ExtentX        =   1773
                  _ExtentY        =   476
                  _Version        =   393216
                  OLEDropMode     =   1
                  LargeChange     =   100
                  SmallChange     =   50
                  Min             =   -5000
                  Max             =   0
                  TickFrequency   =   500
               End
               Begin VB.Image imgVolume 
                  Height          =   240
                  Left            =   0
                  OLEDropMode     =   1  'Manual
                  Picture         =   "vbplayer.frx":099E
                  Top             =   40
                  Width           =   240
               End
            End
            Begin MSComctlLib.Toolbar tbControlBar 
               Height          =   330
               Left            =   30
               TabIndex        =   19
               Top             =   30
               Width           =   975
               _ExtentX        =   1720
               _ExtentY        =   582
               ButtonWidth     =   609
               ButtonHeight    =   582
               Wrappable       =   0   'False
               Style           =   1
               ImageList       =   "ctrlControlList"
               _Version        =   393216
               BeginProperty Buttons {66833FE8-8583-11D1-B16A-00C0F0283628} 
                  NumButtons      =   3
                  BeginProperty Button1 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                     Key             =   "play"
                     Object.ToolTipText     =   "Play"
                     ImageIndex      =   1
                  EndProperty
                  BeginProperty Button2 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                     Key             =   "pause"
                     Object.ToolTipText     =   "Pause"
                     ImageIndex      =   2
                  EndProperty
                  BeginProperty Button3 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                     Key             =   "stop"
                     Object.ToolTipText     =   "Stop"
                     ImageIndex      =   3
                  EndProperty
               EndProperty
               OLEDropMode     =   1
            End
         End
      End
      Begin ComCtl3.CoolBar cbLinkListBar 
         Height          =   450
         Left            =   0
         TabIndex        =   1
         Top             =   0
         Width           =   4320
         _ExtentX        =   7620
         _ExtentY        =   794
         BandCount       =   1
         BandBorders     =   0   'False
         OLEDropMode     =   1
         _CBWidth        =   4320
         _CBHeight       =   450
         _Version        =   "6.7.8862"
         Child1          =   "fraLinkList"
         MinWidth1       =   5145
         MinHeight1      =   390
         Width1          =   5205
         NewRow1         =   0   'False
         BandStyle1      =   1
         Begin VB.Frame fraLinkList 
            BorderStyle     =   0  'None
            Height          =   390
            Left            =   30
            TabIndex        =   2
            Top             =   30
            Width           =   5205
            Begin MSComctlLib.Toolbar tbRadioLinkList 
               Height          =   360
               Left            =   0
               TabIndex        =   3
               Top             =   0
               Width           =   1155
               _ExtentX        =   2037
               _ExtentY        =   635
               ButtonWidth     =   1931
               ButtonHeight    =   635
               ToolTips        =   0   'False
               Wrappable       =   0   'False
               Style           =   1
               ImageList       =   "ctrlRadioLinkList"
               HotImageList    =   "ctrlRadioLinkListHot"
               _Version        =   393216
               BeginProperty Buttons {66833FE8-8583-11D1-B16A-00C0F0283628} 
                  NumButtons      =   3
                  BeginProperty Button1 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                     Object.ToolTipText     =   "Radio"
                     ImageIndex      =   1
                  EndProperty
                  BeginProperty Button2 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                  EndProperty
                  BeginProperty Button3 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                  EndProperty
               EndProperty
               OLEDropMode     =   1
            End
            Begin MSComctlLib.Toolbar tbMusicLinkList 
               Height          =   360
               Left            =   1200
               TabIndex        =   4
               Top             =   0
               Width           =   1260
               _ExtentX        =   2223
               _ExtentY        =   635
               ButtonWidth     =   1984
               ButtonHeight    =   635
               AllowCustomize  =   0   'False
               Wrappable       =   0   'False
               Style           =   1
               ImageList       =   "ctrlMusicLinkList"
               HotImageList    =   "ctrlMusicLinkListHot"
               _Version        =   393216
               BeginProperty Buttons {66833FE8-8583-11D1-B16A-00C0F0283628} 
                  NumButtons      =   3
                  BeginProperty Button1 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                     Object.ToolTipText     =   "Music"
                     ImageIndex      =   1
                  EndProperty
                  BeginProperty Button2 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                  EndProperty
                  BeginProperty Button3 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                  EndProperty
               EndProperty
               OLEDropMode     =   1
            End
            Begin MSComctlLib.Toolbar tbMediaGuideLinkList 
               Height          =   360
               Left            =   2475
               TabIndex        =   5
               Top             =   0
               Width           =   1875
               _ExtentX        =   3307
               _ExtentY        =   635
               ButtonWidth     =   3254
               ButtonHeight    =   635
               AllowCustomize  =   0   'False
               Wrappable       =   0   'False
               Style           =   1
               ImageList       =   "ctrlMediaGuideLinkList"
               HotImageList    =   "ctrlMediaGuideLinkListHot"
               _Version        =   393216
               BeginProperty Buttons {66833FE8-8583-11D1-B16A-00C0F0283628} 
                  NumButtons      =   3
                  BeginProperty Button1 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                     Object.ToolTipText     =   "Media Guide"
                     ImageIndex      =   1
                  EndProperty
                  BeginProperty Button2 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                  EndProperty
                  BeginProperty Button3 {66833FEA-8583-11D1-B16A-00C0F0283628} 
                  EndProperty
               EndProperty
               OLEDropMode     =   1
            End
         End
      End
   End
   Begin MSComctlLib.ImageList ctrlControlList 
      Left            =   0
      Top             =   5325
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   3
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":0A7A
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":0B8C
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":0C9E
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Timer tmrTimer 
      Left            =   600
      Top             =   5475
   End
   Begin MSComctlLib.ImageList ctrlRadioLinkList 
      Left            =   0
      Top             =   5925
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   66
      ImageHeight     =   18
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   2
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":0DB0
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":109C
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMusicLinkList 
      Left            =   600
      Top             =   5925
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   68
      ImageHeight     =   18
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   2
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":13B0
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":1698
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMediaGuideLinkList 
      Left            =   1200
      Top             =   5925
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   116
      ImageHeight     =   18
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   2
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":1984
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":1E20
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlRadioLinkListHot 
      Left            =   0
      Top             =   6525
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   66
      ImageHeight     =   18
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   1
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":22C0
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMusicLinkListHot 
      Left            =   600
      Top             =   6525
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   68
      ImageHeight     =   18
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   1
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":25D4
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMediaGuideLinkListHot 
      Left            =   1200
      Top             =   6525
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   116
      ImageHeight     =   18
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   1
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "vbplayer.frx":28C0
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Menu mnuFile 
      Caption         =   "&File"
      Begin VB.Menu mnuFileOpen 
         Caption         =   "&Open"
         Shortcut        =   ^O
      End
      Begin VB.Menu mnuFileClose 
         Caption         =   "&Close"
      End
      Begin VB.Menu mnuFileSeptum 
         Caption         =   "-"
      End
      Begin VB.Menu mnuFileExit 
         Caption         =   "E&xit"
      End
   End
   Begin VB.Menu mnuView 
      Caption         =   "&View"
      Begin VB.Menu mnuViewLinks 
         Caption         =   "&Links"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewDisplay 
         Caption         =   "&Display"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewControlbar 
         Caption         =   "&Control Bar"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewStatusbar 
         Caption         =   "&Status Bar"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewSeptum 
         Caption         =   "-"
      End
      Begin VB.Menu mnuViewAlwaysOnTop 
         Caption         =   "&Always On Top"
         Checked         =   -1  'True
      End
   End
End
Attribute VB_Name = "frmMain"
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

Private m_boolLoaded As Boolean                    'file loaded
Private m_boolViewLinks As Boolean                'view link bar
Private m_boolViewDisplay As Boolean            'view display area
Private m_boolViewControlBar As Boolean        'view control bar
Private m_boolViewStatusBar As Boolean         'view status bar
Private m_sngPreferredVideoWidth As Single   'suggested width in twips
Private m_sngPreferredVideoHeight As Single  'suggested width in twips

Private m_objMediaEvent As IMediaEvent        'Basic Event Object
Private m_objBasicAudio  As IBasicAudio         'Basic Audio Object
Private m_objBasicVideo As IBasicVideo          'Basic Video Object
Private m_objVideoWindow As IVideoWindow   'VideoWindow Object
Private m_objMediaControl As IMediaControl    'MediaControl Object
Private m_objMediaPosition As IMediaPosition 'MediaPosition Object

Private Const TIMER_GRANUlARITY As Long = 250                 '1/4 second timer interval, in milliseconds
Private Const MINIMUM_APP_WIDTH As Long = 4440             'minimum enforced application width, in twips
Private Const DEFAULT_VIDEO_WIDTH As Long = 4290          'default video window width, in twips
Private Const DEFAULT_VIDEO_HEIGHT As Long = 2640         'default video window height, in twips
Private Const DEFAULT_MENUBAR_HEIGHT As Long = 705    'static menubar / form resize breakpoint, in twips
Private Const ACTUAL_STATUSBAR_HEIGHT As Long = 1440  'fixed statusbar height, in twips

Private Const SWP_NOSIZE  As Long = &H1           'no size
Private Const SWP_NOMOVE  As Long = &H2         'no move
Private Const HWND_TOPMOST  As Long = -1        'topmost zorder
Private Const HWND_NOTOPMOST  As Long = -2   'notopmost zorder

Private Declare Function midiOutGetNumDevs Lib "winmm" () As Integer 'returns the number of midi out devices
Private Declare Function waveOutGetNumDevs Lib "winmm.dll" () As Long 'returns the number of wave out devices
Private Declare Function SetWindowPos Lib "user32" (ByVal hWnd As Long, ByVal hWndInsertAfter As Long, ByVal X As Long, ByVal Y As Long, ByVal cx As Long, ByVal cy As Long, ByVal wFlags As Long) As Long 'set the window position


            
' **************************************************************************************************************************************
' * PRIVATE INTERFACE- FORM EVENT HANDLERS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: Form_Initialize
            ' * procedure description:  Occurs when an application creates an instance of a Form, MDIForm, or class.
            ' *
            ' ******************************************************************************************************************************
            Private Sub Form_Initialize()
            On Local Error GoTo ErrLine
            
            'instantiate media control
            Set m_objMediaControl = New FilgraphManager
            
            'assign default value(S) to module-level data
            m_boolViewLinks = True
            m_boolViewDisplay = True
            m_boolViewControlBar = True
            m_boolViewStatusBar = True
            
            'assign default value(s) to module-level data
            m_sngPreferredVideoWidth = DEFAULT_VIDEO_WIDTH
            m_sngPreferredVideoHeight = DEFAULT_VIDEO_HEIGHT
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub


            ' ******************************************************************************************************************************
            ' * procedure name: Form_Terminate
            ' * procedure description:  Occurs when all references to an instance of a Form, MDIForm, or class are removed from memory.
            ' *
            ' ******************************************************************************************************************************
            Private Sub Form_Terminate()
            On Local Error GoTo ErrLine
            
            If ObjPtr(m_objMediaControl) > 0 Then
               Call m_objMediaControl.Stop
               If ObjPtr(m_objVideoWindow) > 0 Then
                  m_objVideoWindow.Left = Screen.Width * 8
                  m_objVideoWindow.Top = Screen.Height * 8
                  m_objVideoWindow.Visible = False
                  m_objVideoWindow.Owner = 0 'Assign the Owner to NULL
               End If
            End If
            
            'clean-up & dereference
            If ObjPtr(m_objBasicAudio) > 0 Then Set m_objBasicAudio = Nothing
            If ObjPtr(m_objBasicVideo) > 0 Then Set m_objBasicVideo = Nothing
            If ObjPtr(m_objMediaEvent) > 0 Then Set m_objMediaEvent = Nothing
            If ObjPtr(m_objMediaControl) > 0 Then Set m_objMediaControl = Nothing
            If ObjPtr(m_objVideoWindow) > 0 Then Set m_objVideoWindow = Nothing
            If ObjPtr(m_objMediaPosition) > 0 Then Set m_objMediaPosition = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub


            ' ******************************************************************************************************************************
            ' * procedure name: Form_Load
            ' * procedure description:  Occurs when a form is loaded.
            ' *
            ' ******************************************************************************************************************************
            Private Sub Form_Load()
            Dim nResultant As VbMsgBoxResult
            On Local Error GoTo ErrLine
            
            'verify that a sound card is installed on the local system
            If AppIsSoundCardInstalled = False Then
               nResultant = MsgBox("A sound card or comparable hardware was not detected." & vbCrLf & "Audio hardware is required for this sample to run, continue to load the sample?", vbExclamation + vbApplicationModal + vbYesNo)
               If nResultant = vbNo Then
                  Unload Me: Exit Sub
               End If
            End If
            
            'disable all the control buttons by default
            tbControlBar.Buttons("play").Enabled = False
            tbControlBar.Buttons("stop").Enabled = False
            tbControlBar.Buttons("pause").Enabled = False
                  
            'Set the granularity for the timer control
            'so that we can display the duration for
            'given video sequence.
            tmrTimer.Interval = TIMER_GRANUlARITY
            tmrTimer.Enabled = True 'Enable the timer..
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: Form_Unload
            ' * procedure description:  Occurs when a form is about to be removed from the screen.
            ' *
            ' ******************************************************************************************************************************
            Private Sub Form_Unload(Cancel As Integer)
            On Local Error GoTo ErrLine
            
            'disable the timer
            tmrTimer.Enabled = False
            
            'stop video playback
            m_boolLoaded = False
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: Form_Resize
            ' * procedure description:  Occurs when a form is first displayed or the size of an object changes.
            ' *                                       Resize the form & contingent controls based on individule control visibility.
            ' ******************************************************************************************************************************
            Private Sub Form_Resize()
            On Local Error GoTo ErrLine
            
            'enforce minimum width/height on the form
            If frmMain.Width < MINIMUM_APP_WIDTH Then
               If frmMain.Width <> MINIMUM_APP_WIDTH Then frmMain.Width = MINIMUM_APP_WIDTH
            End If
            If frmMain.Height < DEFAULT_MENUBAR_HEIGHT Then
               If frmMain.Height <> DEFAULT_MENUBAR_HEIGHT Then frmMain.Height = DEFAULT_MENUBAR_HEIGHT
            End If
            
            'additionally align the status area elements
            If fraInfo.Visible = True Then
               lnSeptum.X2 = lnSeptum.Parent.Width
                lnSeptum2.X2 = lnSeptum2.Parent.Width
               picPrettyWave.Left = (picPrettyWave.Parent.Width - picPrettyWave.Width * 2)
            End If
            
            'additionally reposition the active movie window if applicable
            If ObjPtr(m_objVideoWindow) > 0 Then
               m_objVideoWindow.Width = picMovieWindow.ScaleWidth / Screen.TwipsPerPixelX
               m_objVideoWindow.Height = picMovieWindow.ScaleHeight / Screen.TwipsPerPixelY
            End If
            
            If cbLinkListBar.Visible = True And picMovieWindow.Visible = True And cbControlBar.Visible = True And fraInfo.Visible = True Then
               'ALL CONTINGENT CONTROLS VISIBLE
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If frmMain.ScaleHeight - (cbLinkListBar.Height + cbControlBar.Height + fraInfo.Height) > 0 Then
                  If picMovieWindow.Height <> frmMain.ScaleHeight - (cbLinkListBar.Height + cbControlBar.Height + fraInfo.Height) Then _
                     picMovieWindow.Height = frmMain.ScaleHeight - (cbLinkListBar.Height + cbControlBar.Height + fraInfo.Height)
               Else: If picMovieWindow.Height <> 0 Then picMovieWindow.Height = 0
               End If
               
               'position the control toolbar
               If cbControlBar.Top <> (picMovieWindow.Top + picMovieWindow.Height) Then cbControlBar.Top = (picMovieWindow.Top + picMovieWindow.Height)
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
               
               'position the status window
               If fraInfo.Top <> cbControlBar.Top + cbControlBar.Height Then fraInfo.Top = cbControlBar.Top + cbControlBar.Height
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + cbControlBar.Height + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + cbControlBar.Height + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
               
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = False And cbControlBar.Visible = False And fraInfo.Visible = False Then
               'ALL CONTINGENT CONTROLS INVISIBLE
               
               'set form height to minimum menu bar height
               If frmMain.Height <> DEFAULT_MENUBAR_HEIGHT Then frmMain.Height = DEFAULT_MENUBAR_HEIGHT
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> DEFAULT_MENUBAR_HEIGHT Then _
                     frmMain.Height = DEFAULT_MENUBAR_HEIGHT
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
            
            
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = True And cbControlBar.Visible = False And fraInfo.Visible = False Then
              'ALL CONTINGENT CONTROLS INVISIBLE EXCEPT FOR ACTIVE MOVIE PANE
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> 0 Then fraChild.Top = 0
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If picMovieWindow.Parent.Height > 0 Then picMovieWindow.Height = picMovieWindow.Parent.Height
            
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
            
            ElseIf cbLinkListBar.Visible = True And picMovieWindow.Visible = False And cbControlBar.Visible = False And fraInfo.Visible = False Then
              'ALL CONTINGENT CONTROLS INVISIBLE EXCEPT FOR LINKLIST COOLBAR
              
              'set form height to the sum of the minimum menu bar height plus the minimum coolbar height
               If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height) Then frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height)
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = False And cbControlBar.Visible = True And fraInfo.Visible = False Then
              'ALL CONTINGENT CONTROLS INVISIBLE EXCEPT FOR CONTROL COOLBAR
               
               'set form height to the sum of the minimum menu bar height plus the minimum control height
               If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbControlBar.Height) Then frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbControlBar.Height)
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> 0 Then fraChild.Top = 0
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position the control toolbar
               If cbControlBar.Top <> 0 Then cbControlBar.Top = 0
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbControlBar.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbControlBar.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
            
            
            
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = False And cbControlBar.Visible = False And fraInfo.Visible = True Then
              'ALL CONTINGENT CONTROLS INVISIBLE EXCEPT FOR STATUS PANE
               
               'set form height to the sum of the minimum menu bar height plus the minimum control height
               If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height) Then frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height)
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> 0 Then fraChild.Top = 0
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position the status window
               If fraInfo.Top <> 0 Then fraInfo.Top = 0
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
            ElseIf cbLinkListBar.Visible = True And picMovieWindow.Visible = False And cbControlBar.Visible = True And fraInfo.Visible = True Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT FOR THE MOVIE WINDOW
               
               'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
               If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + cbControlBar.Height) Then frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + cbControlBar.Height)
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'position the control toolbar
               If cbControlBar.Top <> 0 Then cbControlBar.Top = 0
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
               
               'position the status window
               If fraInfo.Top <> cbControlBar.Top + cbControlBar.Height Then fraInfo.Top = cbControlBar.Top + cbControlBar.Height
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + cbControlBar.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + cbControlBar.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
            
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = True And cbControlBar.Visible = True And fraInfo.Visible = True Then
               'ALL CONTINGENT CONTROLS VISIBLE EXCEPT THE LINKLIST BAR
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> 0 Then fraChild.Top = 0
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If frmMain.ScaleHeight - (cbControlBar.Height + fraInfo.Height) > 0 Then
                  If picMovieWindow.Height <> frmMain.ScaleHeight - (cbControlBar.Height + fraInfo.Height) Then _
                     picMovieWindow.Height = frmMain.ScaleHeight - (cbControlBar.Height + fraInfo.Height)
               Else: If picMovieWindow.Height <> 0 Then picMovieWindow.Height = 0
               End If
               
               'position the control toolbar
               If cbControlBar.Top <> (picMovieWindow.Top + picMovieWindow.Height) Then cbControlBar.Top = (picMovieWindow.Top + picMovieWindow.Height)
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
               
               'position the status window
               If fraInfo.Top <> cbControlBar.Top + cbControlBar.Height Then fraInfo.Top = cbControlBar.Top + cbControlBar.Height
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbControlBar.Height + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbControlBar.Height + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
            
            
            ElseIf cbLinkListBar.Visible = True And picMovieWindow.Visible = True And cbControlBar.Visible = False And fraInfo.Visible = True Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT THE CONTROLBAR
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If frmMain.ScaleHeight - (cbLinkListBar.Height + fraInfo.Height) > 0 Then
                  If picMovieWindow.Height <> frmMain.ScaleHeight - (cbLinkListBar.Height + fraInfo.Height) Then _
                     picMovieWindow.Height = frmMain.ScaleHeight - (cbLinkListBar.Height + fraInfo.Height)
               Else: If picMovieWindow.Height <> 0 Then picMovieWindow.Height = 0
               End If
               
               'position the status window
               If fraInfo.Top <> picMovieWindow.Top + picMovieWindow.Height Then fraInfo.Top = picMovieWindow.Top + picMovieWindow.Height
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
            
            ElseIf cbLinkListBar.Visible = True And picMovieWindow.Visible = True And cbControlBar.Visible = True And fraInfo.Visible = False Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT THE STATUS PANE
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If frmMain.ScaleHeight - (cbLinkListBar.Height + cbControlBar.Height) > 0 Then
                  If picMovieWindow.Height <> frmMain.ScaleHeight - (cbLinkListBar.Height + cbControlBar.Height) Then _
                     picMovieWindow.Height = frmMain.ScaleHeight - (cbLinkListBar.Height + cbControlBar.Height)
               Else: If picMovieWindow.Height <> 0 Then picMovieWindow.Height = 0
               End If
               
               'position the control toolbar
               If cbControlBar.Top <> (picMovieWindow.Top + picMovieWindow.Height) Then cbControlBar.Top = (picMovieWindow.Top + picMovieWindow.Height)
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
            
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + cbControlBar.Height + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + cbControlBar.Height + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
            
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = False And cbControlBar.Visible = True And fraInfo.Visible = True Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT FOR THE MOVIE WINDOW AND THE LINKLIST BAR
               
               'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
               If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbControlBar.Height) Then frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbControlBar.Height)
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> 0 Then fraChild.Top = 0
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position the control toolbar
               If cbControlBar.Top <> 0 Then cbControlBar.Top = 0
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
               
               'position the status window
               If fraInfo.Top <> cbControlBar.Top + cbControlBar.Height Then fraInfo.Top = cbControlBar.Top + cbControlBar.Height
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
            
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbControlBar.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbControlBar.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
            
            
            
            ElseIf cbLinkListBar.Visible = True And picMovieWindow.Visible = True And cbControlBar.Visible = False And fraInfo.Visible = False Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT THE CONTROLBAR AND STATUS PANE
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If frmMain.ScaleHeight - (cbLinkListBar.Height) > 0 Then
                  If picMovieWindow.Height <> frmMain.ScaleHeight - (cbLinkListBar.Height) Then _
                     picMovieWindow.Height = frmMain.ScaleHeight - (cbLinkListBar.Height)
               Else: If picMovieWindow.Height <> 0 Then picMovieWindow.Height = 0
               End If
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
               
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = True And cbControlBar.Visible = True And fraInfo.Visible = False Then
               'ALL CONTINGENT CONTROLS VISIBLE EXCEPT THE LINKLIST BAR AND THE STATUS PANE
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> 0 Then fraChild.Top = 0
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If frmMain.ScaleHeight - (cbControlBar.Height) > 0 Then
                  If picMovieWindow.Height <> frmMain.ScaleHeight - (cbControlBar.Height) Then _
                     picMovieWindow.Height = frmMain.ScaleHeight - (cbControlBar.Height)
               Else: If picMovieWindow.Height <> 0 Then picMovieWindow.Height = 0
               End If
               
               'position the control toolbar
               If cbControlBar.Top <> (picMovieWindow.Top + picMovieWindow.Height) Then cbControlBar.Top = (picMovieWindow.Top + picMovieWindow.Height)
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
            
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbControlBar.Height + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbControlBar.Height + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
            
            ElseIf cbLinkListBar.Visible = True And picMovieWindow.Visible = False And cbControlBar.Visible = False And fraInfo.Visible = True Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT FOR THE MOVIE WINDOW AND CONTROLBAR
               
               'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
               If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height) Then frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height)
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'position the status window
               If fraInfo.Top <> 0 Then fraInfo.Top = 0
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
            
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + cbLinkListBar.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
               
               
            
            ElseIf cbLinkListBar.Visible = False And picMovieWindow.Visible = True And cbControlBar.Visible = False And fraInfo.Visible = True Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT THE LINKLIST BAR AND THE CONTROLBAR
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> 0 Then fraChild.Top = 0
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position the active movie window container
               If picMovieWindow.Top <> 0 Then picMovieWindow.Top = 0
               If picMovieWindow.Left <> 0 Then picMovieWindow.Left = 0
               If picMovieWindow.Width <> picMovieWindow.Parent.Width Then picMovieWindow.Width = picMovieWindow.Parent.Width
               If frmMain.ScaleHeight - (fraInfo.Height) > 0 Then
                  If picMovieWindow.Height <> frmMain.ScaleHeight - (fraInfo.Height) Then _
                     picMovieWindow.Height = frmMain.ScaleHeight - (fraInfo.Height)
               Else: If picMovieWindow.Height <> 0 Then picMovieWindow.Height = 0
               End If
               
               'position the status window
               If fraInfo.Top <> picMovieWindow.Top + picMovieWindow.Height Then fraInfo.Top = picMovieWindow.Top + picMovieWindow.Height
               If fraInfo.Left <> 0 Then fraInfo.Left = 0
               If fraInfo.Width <> fraInfo.Parent.Width Then fraInfo.Width = fraInfo.Parent.Width
               If fraInfo.Height <> ACTUAL_STATUSBAR_HEIGHT Then fraInfo.Height = ACTUAL_STATUSBAR_HEIGHT
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + picMovieWindow.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + fraInfo.Height + picMovieWindow.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
            
            
            ElseIf cbLinkListBar.Visible = True And picMovieWindow.Visible = False And cbControlBar.Visible = True And fraInfo.Visible = False Then
              'ALL CONTINGENT CONTROLS VISIBLE EXCEPT THE MOVIE WINDOW AND THE STATUS PANE
               
               'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
               If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + cbControlBar.Height) Then frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + cbControlBar.Height)
               
               'position main control container
               If fraMain.Top <> frmMain.ScaleTop Then fraMain.Top = frmMain.ScaleTop
               If fraMain.Left <> frmMain.ScaleLeft Then fraMain.Left = frmMain.ScaleLeft
               If fraMain.Width <> frmMain.ScaleWidth Then fraMain.Width = frmMain.ScaleWidth
               If fraMain.Height <> frmMain.ScaleHeight Then fraMain.Height = frmMain.ScaleHeight
               
               'position child control container
               If fraChild.Top <> cbLinkListBar.Height Then fraChild.Top = cbLinkListBar.Height
               If fraChild.Left <> 0 Then fraChild.Left = 0
               If fraChild.Width <> fraChild.Parent.Width Then fraChild.Width = fraChild.Parent.Width
               If fraChild.Height <> fraChild.Parent.Height Then fraChild.Height = fraChild.Parent.Height
               
               'position linklist coolbar
               If cbLinkListBar.Top <> 0 Then cbLinkListBar.Top = 0
               If cbLinkListBar.Left <> 0 Then cbLinkListBar.Left = 0
               If cbLinkListBar.Width <> cbLinkListBar.Container.Width Then cbLinkListBar.Width = cbLinkListBar.Container.Width
               If cbLinkListBar.Height <> 450 Then cbLinkListBar.Height = 450
               
               'position linklist frame
               If fraLinkList.Top <> 0 Then fraLinkList.Top = 0
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> fraLinkList.Parent.ScaleWidth Then fraLinkList.Width = fraLinkList.Parent.ScaleWidth
               If fraLinkList.Height <> fraLinkList.Parent.ScaleHeight Then fraLinkList.Height = fraLinkList.Parent.ScaleHeight
               
               'position radio linklist toolbar in the linklist frame
               If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
               If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
               If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
               If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
               
               'position music linklist toolbar in the linklist frame
               If tbMusicLinkList.Top = 0 Then tbMusicLinkList.Top = 0
               If tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
                  tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
               If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
               If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
               
               'position radio guide linklist toolbar in the linklist frame
               If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
               If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
                  tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
               If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
               If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
               
               'position the control toolbar
               If cbControlBar.Top <> 0 Then cbControlBar.Top = 0
               If cbControlBar.Left <> 0 Then cbControlBar.Left = 0
               If cbControlBar.Width <> cbControlBar.Parent.Width Then cbControlBar.Width = cbControlBar.Parent.Width
               If cbControlBar.Height <> 390 Then cbControlBar.Height = 390
               
               'reposition the form to take up the slack either way
               If picMovieWindow.Height <> m_sngPreferredVideoHeight Then
                  picMovieWindow.Height = m_sngPreferredVideoHeight
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Height <> (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + cbControlBar.Height) Then _
                     frmMain.Height = (DEFAULT_MENUBAR_HEIGHT + cbLinkListBar.Height + cbControlBar.Height)
                     Exit Sub
               End If
               If picMovieWindow.Width <> m_sngPreferredVideoWidth Then
                  picMovieWindow.Width = m_sngPreferredVideoWidth
                  'set form height to the sum of the minimum menu bar height plus the minimum controlbar height plus the minimum status pane height
                  If frmMain.Width <> picMovieWindow.Width Then _
                     frmMain.Width = picMovieWindow.Width
                     Exit Sub
               End If
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Resume Next
            Exit Sub
            End Sub
            
            
            
' **************************************************************************************************************************************
' * PRIVATE INTERFACE- OLE DRAGDROP EVENT HANDLERS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: Form_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub Form_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: Form_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub Form_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub



            ' ******************************************************************************************************************************
            ' * procedure name: cbControlBar_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub cbControlBar_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cbControlBar_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub cbControlBar_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cbLinkListBar_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub cbLinkListBar_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cbLinkListBar_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub cbLinkListBar_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            

            ' ******************************************************************************************************************************
            ' * procedure name: tbRadioLinkList_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbRadioLinkList_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbRadioLinkList_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbRadioLinkList_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picMovieWindow_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picMovieWindow_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picMovieWindow_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picMovieWindow_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbControlBar_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbControlBar_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbControlBar_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbControlBar_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbMediaGuideLinkList_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbMediaGuideLinkList_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbMediaGuideLinkList_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbMediaGuideLinkList_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbMusicLinkList_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbMusicLinkList_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbMusicLinkList_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbMusicLinkList_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: imgBalance_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub imgBalance_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: imgBalance_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub imgBalance_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: imgVolume_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub imgVolume_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: imgVolume_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub imgVolume_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblAuthor_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblAuthor_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblAuthor_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblAuthor_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblAuthorValue_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblAuthorValue_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblAuthorValue_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblAuthorValue_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblClip_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblClip_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblClip_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblClip_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblClipValue_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblClipValue_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblClipValue_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblClipValue_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblCopyright_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblCopyright_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblCopyright_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblCopyright_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblCopyrightValue_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblCopyrightValue_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblCopyrightValue_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblCopyrightValue_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblCurrentTime_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblCurrentTime_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblCurrentTime_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblCurrentTime_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblFileName_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblFileName_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblFileName_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblFileName_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblFileNameValue_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblFileNameValue_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblFileNameValue_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblFileNameValue_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picPrettyWave_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picPrettyWave_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picPrettyWave_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picPrettyWave_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slBalance_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub slBalance_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slBalance_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub slBalance_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slVolume_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub slVolume_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slVolume_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub slVolume_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            
' **************************************************************************************************************************************
' * PRIVATE INTERFACE- MENU EVENT HANDLERS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: mnuFile_Click
            ' * procedure description:  Occurs when the "File" option is elected from the applications main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuFile_Click()
            On Local Error GoTo ErrLine
            
            'set 'enabled' status
            If m_boolLoaded Then
               mnuFileClose.Enabled = True
            Else: mnuFileClose.Enabled = False
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuFileExit_Click
            ' * procedure description:  Occurs when the "Exit" option is invoked from the "File" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuFileExit_Click()
            Dim frm As Form
            On Local Error GoTo ErrLine
            
            'unload each loaded form
            For Each frm In Forms
                frm.Move Screen.Width * 8, Screen.Height * 8
                Unload frm
                Set frm = Nothing
            Next
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuFileOpen_Click
            ' * procedure description:  Occurs when the "Open" option is invoked from the "File" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuFileOpen_Click()
            Dim bstrFileName As String
            Dim nMinIdealWidth As Long
            Dim nMinIdealHeight As Long
            Dim nMaxIdealWidth As Long
            Dim nMaxIdealHeight As Long
            On Local Error GoTo ErrLine
            
            'query the user for a media file
            bstrFileName = _
            AppShowCommonDlgOpen(vbNullString, "AVI", "Media Files (*.au;*.avi;*.mov;*.mpg;*.mpeg;*.wav;*.aif;*.mid;*.mp2;*.mp3)|*.au;*.avi;*.mid;*.mov;*.mpg;*.mpeg;*.wav;*.aif;*.mp2;*.mp3|")
            If bstrFileName = vbNullString Then Exit Sub
            
            'attempt to play the file back
            Call AppPlayFile(bstrFileName)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuFileClose_Click
            ' * procedure description:  Occurs when the "Close" option is invoked from the "File" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuFileClose_Click()
            On Local Error GoTo ErrLine
            
            'stop video playback
            m_boolLoaded = False
            If ObjPtr(m_objMediaControl) > 0 Then
               Call m_objMediaControl.Stop
               If ObjPtr(m_objVideoWindow) > 0 Then
                  m_objVideoWindow.Left = Screen.Width * 8
                  m_objVideoWindow.Top = Screen.Height * 8
                  m_objVideoWindow.Visible = False
                  m_objVideoWindow.Owner = 0 'Assign the Owner to NULL
               End If
            End If
            
            'clean-up & dereference
            If ObjPtr(m_objBasicAudio) > 0 Then Set m_objBasicAudio = Nothing
            If ObjPtr(m_objBasicVideo) > 0 Then Set m_objBasicVideo = Nothing
            If ObjPtr(m_objMediaEvent) > 0 Then Set m_objMediaEvent = Nothing
            If ObjPtr(m_objMediaControl) > 0 Then Set m_objMediaControl = Nothing
            If ObjPtr(m_objVideoWindow) > 0 Then Set m_objVideoWindow = Nothing
            If ObjPtr(m_objMediaPosition) > 0 Then Set m_objMediaPosition = Nothing
            
            'assign default value(s) to module-level data
            m_sngPreferredVideoWidth = DEFAULT_VIDEO_WIDTH
            m_sngPreferredVideoHeight = DEFAULT_VIDEO_HEIGHT
            
            'resize the form
            Call Form_Resize
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuView_Click
            ' * procedure description:  Occurs when the "View" option is elected from the applications main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuView_Click()
            Static boolRunOnce As Boolean
            On Local Error GoTo ErrLine
            
            'default view on first click
            If boolRunOnce = False Then
               m_boolViewLinks = True
               m_boolViewDisplay = True
               m_boolViewControlBar = True
               m_boolViewStatusBar = True
               mnuViewLinks.Checked = True
               mnuViewDisplay.Checked = True
               mnuViewControlbar.Checked = True
               mnuViewStatusbar.Checked = True
               mnuViewAlwaysOnTop.Checked = False
               'reset static variable afterwards (runonce)
               boolRunOnce = True
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuViewAlwaysOnTop_Click
            ' * procedure description:  Occurs when the "Always On Top" option is invoked from the "View" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuViewAlwaysOnTop_Click()
            On Local Error GoTo ErrLine
            
            'assign menu 'checked' state
            If mnuViewAlwaysOnTop.Checked Then
               mnuViewAlwaysOnTop.Checked = False
               Call AppNotOnTop(frmMain.hWnd)
            Else
               mnuViewAlwaysOnTop.Checked = True
               Call AppStayOnTop(frmMain.hWnd)
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuViewControlbar_Click
            ' * procedure description:  Occurs when the "Control Bar" option is invoked from the "View" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuViewControlbar_Click()
            On Local Error GoTo ErrLine
            
            'assign menu 'checked' state
            If mnuViewControlbar.Checked Then
               mnuViewControlbar.Checked = False
            Else: mnuViewControlbar.Checked = True
            End If
            'assign the current view to module-level
            m_boolViewLinks = mnuViewLinks.Checked
            m_boolViewDisplay = mnuViewDisplay.Checked
            m_boolViewStatusBar = mnuViewStatusbar.Checked
            m_boolViewControlBar = mnuViewControlbar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuViewDisplay_Click
            ' * procedure description:  Occurs when the "Display" option is invoked from the "View" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuViewDisplay_Click()
            On Local Error GoTo ErrLine
            
            'assign menu 'checked' state
            If mnuViewDisplay.Checked Then
               mnuViewDisplay.Checked = False
            Else: mnuViewDisplay.Checked = True
            End If
            'assign the current view to module-level
            m_boolViewLinks = mnuViewLinks.Checked
            m_boolViewDisplay = mnuViewDisplay.Checked
            m_boolViewStatusBar = mnuViewStatusbar.Checked
            m_boolViewControlBar = mnuViewControlbar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuViewLinks_Click
            ' * procedure description:  Occurs when the "Links" option is invoked from the "View" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuViewLinks_Click()
            On Local Error GoTo ErrLine
            
            'assign menu 'checked' state
            If mnuViewLinks.Checked Then
               mnuViewLinks.Checked = False
            Else: mnuViewLinks.Checked = True
            End If
            'assign the current view to module-level
            m_boolViewLinks = mnuViewLinks.Checked
            m_boolViewDisplay = mnuViewDisplay.Checked
            m_boolViewStatusBar = mnuViewStatusbar.Checked
            m_boolViewControlBar = mnuViewControlbar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuViewStatusbar_Click
            ' * procedure description:  Occurs when the "Status Bar" option is invoked from the "View" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuViewStatusbar_Click()
            On Local Error GoTo ErrLine
            
            'assign menu 'checked' state
            If mnuViewStatusbar.Checked Then
               mnuViewStatusbar.Checked = False
            Else: mnuViewStatusbar.Checked = True
            End If
            'assign the current view to module-level
            m_boolViewLinks = mnuViewLinks.Checked
            m_boolViewDisplay = mnuViewDisplay.Checked
            m_boolViewStatusBar = mnuViewStatusbar.Checked
            m_boolViewControlBar = mnuViewControlbar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub



' **************************************************************************************************************************************
' * PRIVATE INTERFACE- CONTROL EVENT HANDLERS
' *
            ' ******************************************************************************************************************************
            ' * procedure name: picMovieWindow_Paint
            ' * procedure description:  Occurs when any part of a form or PictureBox control is moved, enlarged, or exposed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub picMovieWindow_Paint()
            Dim sngTop As Single
            Dim sngLeft As Single
            Dim sngWidth As Single
            Dim sngHeight As Single
            Dim objPicture As StdPicture
            On Local Error GoTo ErrLine
            
            If m_boolLoaded = True Then Exit Sub
            
            'load the poster frame from resource(s)
            Set objPicture = LoadResPicture(101, 0)
            
            If ObjPtr(objPicture) > 0 Then
               sngTop = ((picMovieWindow.ScaleHeight - objPicture.Height / 2)) / 2
               sngLeft = ((picMovieWindow.ScaleWidth - objPicture.Width / 2)) / 2.4
               sngWidth = objPicture.Width / 1.75
               sngHeight = objPicture.Height / 1.75
               Call picMovieWindow.PaintPicture(objPicture, sngLeft, sngTop, sngWidth, sngHeight)
            End If
            
            'clean-up & dereference
            If ObjPtr(objPicture) > 0 Then Set objPicture = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub

           
           
           ' ******************************************************************************************************************************
            ' * procedure name: tbControlBar_ButtonClick
            ' * procedure description:  Occurs when the user clicks on a Button object in a Toolbar control.
            ' *
            ' ******************************************************************************************************************************
            Private Sub tbControlBar_ButtonClick(ByVal Button As MSComctlLib.Button)
            Dim nState As Long
            On Local Error GoTo ErrLine
            
            'obtain the current playback state
            If ObjPtr(m_objMediaControl) > 0 Then
               Call m_objMediaControl.GetState(100, nState)
            End If
            
            Select Case Button.Key
                Case "play"
                         If ObjPtr(m_objMediaControl) > 0 Then
                            Select Case nState
                               Case 0 'State Stopped
                                    'run the file from the start position
                                    If ObjPtr(m_objMediaPosition) > 0 Then _
                                       m_objMediaPosition.CurrentPosition = 0
                                    Call m_objMediaControl.Run
                               
                               Case 1 'State Paused
                                    'run the file from the current position
                                    Call m_objMediaControl.Run
                               
                               Case 2 'State Playing
                                    'exit out of the procedure, this would not happen
                                    Exit Sub
                            End Select
                            'disable the play button
                            tbControlBar.Buttons("play").Enabled = False
                            tbControlBar.Buttons("stop").Enabled = True
                            tbControlBar.Buttons("pause").Enabled = True
                         End If
                         
                Case "pause"
                         If ObjPtr(m_objMediaControl) > 0 Then
                            Select Case nState
                               Case 0 'State Stopped
                                    'exit out of the procedure, this would not happen
                                    Exit Sub
                               
                               Case 1 'State Paused
                                    'exit out of the procedure, this would not happen
                                    Exit Sub
                               
                               Case 2 'State Playing
                                    Call m_objMediaControl.Pause
                            End Select
                            'disable the pause button
                            tbControlBar.Buttons("play").Enabled = True
                            tbControlBar.Buttons("stop").Enabled = True
                            tbControlBar.Buttons("pause").Enabled = False
                         End If
                         
                Case "stop"
                         If ObjPtr(m_objMediaControl) > 0 Then
                            Select Case nState
                               Case 0 'State Stopped
                                    'exit out of the procedure, this would not happen
                                    Exit Sub
                               
                               Case 1 'State Paused
                                    If ObjPtr(m_objMediaPosition) > 0 Then _
                                       m_objMediaPosition.CurrentPosition = 0
                                       Call m_objMediaControl.Stop
                               
                               Case 2 'State Playing
                                    If ObjPtr(m_objMediaPosition) > 0 Then _
                                       m_objMediaPosition.CurrentPosition = 0
                                       Call m_objMediaControl.Stop
                            End Select
                            'disable the stop button
                            tbControlBar.Buttons("play").Enabled = True
                            tbControlBar.Buttons("stop").Enabled = False
                            tbControlBar.Buttons("pause").Enabled = False
                         End If
                         
            End Select
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            

            ' ******************************************************************************************************************************
            ' * procedure name: tbMediaGuideLinkList_ButtonClick
            ' * procedure description:  Occurs when the user clicks on a Button object in a Toolbar control.
            ' *
            ' ******************************************************************************************************************************
            Private Sub tbMediaGuideLinkList_ButtonClick(ByVal Button As MSComctlLib.Button)
            Dim objExplorer As Object
            On Local Error GoTo ErrLine
            
            Set objExplorer = CreateObject("InternetExplorer.Application")
            If ObjPtr(objExplorer) > 0 Then
               objExplorer.Visible = True
               Call objExplorer.Navigate("http://windowsmedia.com/mediaguide/default.asp")
            End If
            
            'clean-up & dereference
            If ObjPtr(objExplorer) > 0 Then Set objExplorer = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbMusicLinkList_ButtonClick
            ' * procedure description:  Occurs when the user clicks on a Button object in a Toolbar control.
            ' *
            ' ******************************************************************************************************************************
            Private Sub tbMusicLinkList_ButtonClick(ByVal Button As MSComctlLib.Button)
            Dim objExplorer As Object
            On Local Error GoTo ErrLine
            
            Set objExplorer = CreateObject("InternetExplorer.Application")
            If ObjPtr(objExplorer) > 0 Then
               objExplorer.Visible = True
               Call objExplorer.Navigate("http://windowsmedia.com/mediaguide/default.asp?page=1")
            End If
            
            'clean-up & dereference
            If ObjPtr(objExplorer) > 0 Then Set objExplorer = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            

            ' ******************************************************************************************************************************
            ' * procedure name: tbRadioLinkList_ButtonClick
            ' * procedure description:  Occurs when the user clicks on a Button object in a Toolbar control.
            ' *
            ' ******************************************************************************************************************************
            Private Sub tbRadioLinkList_ButtonClick(ByVal Button As MSComctlLib.Button)
            Dim objExplorer As Object
            On Local Error GoTo ErrLine
            
            Set objExplorer = CreateObject("InternetExplorer.Application")
            If ObjPtr(objExplorer) > 0 Then
               objExplorer.Visible = True
               Call objExplorer.Navigate("http://windowsmedia.com/radiotuner/default.asp")
            End If
            
            'clean-up & dereference
            If ObjPtr(objExplorer) > 0 Then Set objExplorer = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slBalance_Change
            ' * procedure description:   Indicates that the contents of a control have changed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slBalance_Change()
            On Local Error GoTo ErrLine
            
            'Set the balance using the slider
            If ObjPtr(m_objMediaControl) > 0 Then _
               m_objBasicAudio.Balance = slBalance.Value
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slVolume_Change
            ' * procedure description:   Indicates that the contents of a control have changed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slVolume_Change()
            On Local Error GoTo ErrLine
            
            'Set the volume using the slider
            If ObjPtr(m_objMediaControl) > 0 Then _
               m_objBasicAudio.Volume = slVolume.Value
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slBalance_MouseMove
            ' * procedure description:    Occurs when the user moves the mouse.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slBalance_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            
            'Set the balance using the slider
            If ObjPtr(m_objMediaControl) > 0 Then _
               m_objBasicAudio.Balance = slBalance.Value
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slVolume_MouseMove
            ' * procedure description:    Occurs when the user moves the mouse.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slVolume_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            
            'Set the volume using the slider
            If ObjPtr(m_objMediaControl) > 0 Then _
               m_objBasicAudio.Volume = slVolume.Value
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cbControlBar_HeightChanged
            ' * procedure description:    Occurs when the Coolbar control's Height changes, if its Orientation is horizontal.
            ' *                                         Occurs when the Coolbar control's Width changes, if its Orientation is vertical.
            ' ******************************************************************************************************************************
            Private Sub cbControlBar_HeightChanged(ByVal NewHeight As Single)
            On Local Error GoTo ErrLine
            Call Form_Resize
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: cbLinkListBar_HeightChanged
            ' * procedure description:    Occurs when the Coolbar control's Height changes, if its Orientation is horizontal.
            ' *                                         Occurs when the Coolbar control's Width changes, if its Orientation is vertical.
            ' ******************************************************************************************************************************
            Private Sub cbLinkListBar_HeightChanged(ByVal NewHeight As Single)
            On Local Error GoTo ErrLine
            Call Form_Resize
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            

            ' ******************************************************************************************************************************
            ' * procedure name: tmrTimer_Timer
            ' * procedure description:    Occurs when a preset interval for a Timer control has elapsed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub tmrTimer_Timer()
            Dim nState As Long
            Dim nMediaPosition As Long
            Dim nMediaDuration As Long
            Dim bstrFormattedTimes As String
            On Local Error GoTo ErrLine
            
            If m_boolLoaded = True Then
            
               'obtain the current playback state
               If ObjPtr(m_objMediaControl) > 0 Then
                  Call m_objMediaControl.GetState(100, nState)
                  If ObjPtr(m_objMediaPosition) > 0 Then
                     If m_objMediaPosition.CurrentPosition = m_objMediaPosition.Duration Then
                        Call m_objMediaControl.Stop
                     End If
                  End If
               End If
   
               If nState = 0 Then 'stopped
                  If lblState.Caption <> "Stopped" Then lblState.Caption = "Stopped"
                  'disable the stop button
                  tbControlBar.Buttons("play").Enabled = True
                  tbControlBar.Buttons("stop").Enabled = False
                  tbControlBar.Buttons("pause").Enabled = False
                  
               ElseIf nState = 1 Then
                  If lblState.Caption <> "Paused" Then lblState.Caption = "Paused"
                  'disable the pause button
                  tbControlBar.Buttons("play").Enabled = True
                  tbControlBar.Buttons("stop").Enabled = True
                  tbControlBar.Buttons("pause").Enabled = False
               
               ElseIf nState = 2 Then
                  If lblState.Caption <> "Playing" Then lblState.Caption = "Playing"
                  'disable the play button
                  tbControlBar.Buttons("play").Enabled = False
                  tbControlBar.Buttons("stop").Enabled = True
                  tbControlBar.Buttons("pause").Enabled = True
                  
                  If ObjPtr(m_objMediaPosition) > 0 Then
                     nMediaDuration = CLng(m_objMediaPosition.Duration)
                     nMediaPosition = CLng(m_objMediaPosition.CurrentPosition)
                     bstrFormattedTimes = AppFormatSeconds(nMediaPosition) & "/" & AppFormatSeconds(nMediaDuration)
                     
                     'assign to ui
                     lblCurrentTime.Caption = bstrFormattedTimes
                  End If
               End If
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub




' **************************************************************************************************************************************
' * PRIVATE INTERFACE- APPLICATION PROCEDURES
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: AppChangeView
            ' * procedure description:  Changes the view of the application for the client.
            ' *
            ' ******************************************************************************************************************************
            Private Sub AppChangeView(Optional ShowMovieWindow As Boolean = True, Optional ShowLinkBar As Boolean = True, Optional ShowControlbar As Boolean = True, Optional ShowStatusBar As Boolean = True)
            On Local Error GoTo ErrLine
            'assign argument(s) over to the actual control(s) themselves
            cbLinkListBar.Visible = ShowLinkBar: picMovieWindow.Visible = ShowMovieWindow: cbControlBar.Visible = ShowControlbar: fraInfo.Visible = ShowStatusBar
            'resize the form and force a subsequent refresh of the window..
            Call Form_Resize
            'refresh the window, thus forcing a repaint
            'frmMain.Refresh
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppPlayFile
            ' * procedure description:  Renders the given file for the user within the context of the application.
            ' *
            ' ******************************************************************************************************************************
            Private Sub AppPlayFile(bstrFileName As String)
            On Local Error GoTo ErrLine
            
            tmrTimer.Enabled = False
            
            'stop video playback if applicable
            m_boolLoaded = False
            If ObjPtr(m_objMediaControl) > 0 Then
               Call m_objMediaControl.Stop
               If ObjPtr(m_objVideoWindow) > 0 Then _
                  m_objVideoWindow.Owner = 0 'Assign the Owner to NULL
            End If
            
            'clean-up & dereference
            If ObjPtr(m_objBasicAudio) > 0 Then Set m_objBasicAudio = Nothing
            If ObjPtr(m_objBasicVideo) > 0 Then Set m_objBasicVideo = Nothing
            If ObjPtr(m_objMediaEvent) > 0 Then Set m_objMediaEvent = Nothing
            If ObjPtr(m_objMediaControl) > 0 Then Set m_objMediaControl = Nothing
            If ObjPtr(m_objVideoWindow) > 0 Then Set m_objVideoWindow = Nothing
            If ObjPtr(m_objMediaPosition) > 0 Then Set m_objMediaPosition = Nothing
            
            'attempt to render the elected file
            Set m_objMediaControl = New FilgraphManager
            Call m_objMediaControl.RenderFile(bstrFileName)
            lblClipValue.Caption = bstrFileName
            If InStrRev(bstrFileName, "\") > 0 Then
               frmMain.Caption = "VB Player - " & Mid(bstrFileName, InStrRev(bstrFileName, "\") + 1, Len(bstrFileName))
            End If
            
            
            'determine if the content is audio or video only or both
            If HasAudio(m_objMediaControl) = True And HasVideo(m_objMediaControl) = True Then
            
               'Setup the IDE
               Call AppChangeView(True, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar)
               
               'Setup the Audio
               Set m_objBasicAudio = m_objMediaControl
               If ObjPtr(m_objBasicAudio) > 0 Then
                  m_objBasicAudio.Volume = slVolume.Value
                  m_objBasicAudio.Balance = slBalance.Value
               End If
               
               'Setup the Video
               Set m_objVideoWindow = m_objMediaControl
               If ObjPtr(m_objVideoWindow) > 0 Then
                  'set window style
                  m_objVideoWindow.WindowStyle = CLng(&H6000000)
                  'set window owner
                  m_objVideoWindow.Owner = picMovieWindow.hWnd
                  'set the video windows position
                  m_objVideoWindow.Top = CLng(picMovieWindow.Top)
                  m_objVideoWindow.Left = CLng(picMovieWindow.Left)
                  m_objVideoWindow.Width = CLng(picMovieWindow.Width / Screen.TwipsPerPixelX)
                  m_objVideoWindow.Height = CLng(picMovieWindow.Height / Screen.TwipsPerPixelY)
                  'set Preferred (default) video window size to module-level
                  m_sngPreferredVideoWidth = CLng(m_objVideoWindow.Width * Screen.TwipsPerPixelX)
                  m_sngPreferredVideoHeight = CLng(m_objVideoWindow.Height * Screen.TwipsPerPixelY)
                  'force a resize
                  Call Form_Resize
            End If
            
            
            
            ElseIf HasVideo(m_objMediaControl) = True Then
            
               'Setup the IDE
               Call AppChangeView(True, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar)
               
               'Setup the Video object.
               Set m_objVideoWindow = m_objMediaControl
               If ObjPtr(m_objVideoWindow) > 0 Then
                  'set window style
                  m_objVideoWindow.WindowStyle = CLng(&H6000000)
                  'set window owner
                  m_objVideoWindow.Owner = picMovieWindow.hWnd
                  'set the video windows position
                  m_objVideoWindow.Top = CLng(picMovieWindow.Top)
                  m_objVideoWindow.Left = CLng(picMovieWindow.Left)
                  m_objVideoWindow.Width = CLng(picMovieWindow.Width / Screen.TwipsPerPixelX)
                  m_objVideoWindow.Height = CLng(picMovieWindow.Height / Screen.TwipsPerPixelY)
                  'set Preferred (default) video window size to module-level
                  m_sngPreferredVideoWidth = CLng(m_objVideoWindow.Width * Screen.TwipsPerPixelX)
                  m_sngPreferredVideoHeight = CLng(m_objVideoWindow.Height * Screen.TwipsPerPixelY)
                  'force a resize
                  Call Form_Resize
            End If
            
            
            ElseIf HasAudio(m_objMediaControl) = True Then
               'Setup the IDE
               Call AppChangeView(False, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar)
               'Setup the Audio
               Set m_objBasicAudio = m_objMediaControl
               If ObjPtr(m_objBasicAudio) > 0 Then
                  m_objBasicAudio.Volume = slVolume.Value
                  m_objBasicAudio.Balance = slBalance.Value
               End If
            Else
               Exit Sub
            End If
            
            
            
            If ObjPtr(m_objMediaControl) > 0 Then
               'Setup the IMediaEvent object for the
               'sample toolbar (run, pause, play).
               Set m_objMediaEvent = m_objMediaControl
               'Setup the IMediaPosition object so that we
               'can display the duration of the selected
               'video as well as the elapsed time.
               Set m_objMediaPosition = m_objMediaControl
               'run the clip for the user
               Call m_objMediaControl.Run
            End If
            
            'set the module-level flag to indicate
            'that the file has been loaded sucessfully
            m_boolLoaded = True
            tmrTimer.Enabled = True
            Exit Sub
            
ErrLine:
            Err.Clear
            Resume Next
            Exit Sub
            End Sub
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppFormatSeconds
            ' * procedure description:  Formats numeric seconds into 00:00:00 clock format.
            ' *
            ' ******************************************************************************************************************************
            Private Function AppFormatSeconds(dblSeconds As Long) As String
            Dim nHours As Long
            Dim nMinutes As Long
            Dim nSeconds As Long
            Dim dtaTime As Date
            Dim bstrFormatReturn As String
            On Local Error GoTo ErrLine
            
            dtaTime = TimeSerial(0, 0, dblSeconds)
            bstrFormatReturn = Format(dtaTime, "HHMMSS")
            
            If Len(bstrFormatReturn) = 6 Then
               bstrFormatReturn = _
                     Mid(bstrFormatReturn, 1, 2) & ":" & Mid(bstrFormatReturn, 3, 2) & ":" & Mid(bstrFormatReturn, 5, 2)
            End If
            
            'return
            AppFormatSeconds = bstrFormatReturn
            Exit Function
            
ErrLine:
            Err.Clear
            Exit Function
            End Function
            
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppShowCommonDlgOpen
            ' * procedure description:
            ' *
            ' ******************************************************************************************************************************
            Public Function AppShowCommonDlgOpen(Optional bstrCurrentDirectory As String, Optional bstrDefaultExtension As String, Optional bstrFilter As String) As String
            Dim ctrl As Object
            On Local Error GoTo ErrLine
            
            'instantiate control
            If ObjPtr(CreateObject("MSComDlg.CommonDialog.1")) > 0 Then
               Set ctrl = CreateObject("MSComDlg.CommonDialog.1")
            ElseIf ObjPtr(CreateObject("MSComDlg.CommonDialog")) > 0 Then
               Set ctrl = CreateObject("MSComDlg.CommonDialog")
            End If
            
            If ObjPtr(ctrl) > 0 Then
               'set properties
               ctrl.Filter = bstrFilter
               ctrl.DefaultExt = bstrDefaultExtension
               ctrl.InitDir = bstrCurrentDirectory
               ctrl.ShowOpen
               'return to client
               AppShowCommonDlgOpen = ctrl.FileName
            End If
            
            'clean-up & dereference
            If ObjPtr(ctrl) > 0 Then Set ctrl = Nothing
            Exit Function
            
ErrLine:

            Err.Clear
            Exit Function
            End Function
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppShowCommonDlgSave
            ' * procedure description:
            ' *
            ' ******************************************************************************************************************************
            Public Function AppShowCommonDlgSave(Optional bstrCurrentDirectory As String, Optional bstrDefaultExtension As String, Optional bstrFilter As String) As String
            Dim ctrl As Object
            On Local Error GoTo ErrLine
            
            'instantiate control
            If ObjPtr(CreateObject("MSComDlg.CommonDialog.1")) > 0 Then
               Set ctrl = CreateObject("MSComDlg.CommonDialog.1")
            ElseIf ObjPtr(CreateObject("MSComDlg.CommonDialog")) > 0 Then
               Set ctrl = CreateObject("MSComDlg.CommonDialog")
            End If
            
            If ObjPtr(ctrl) > 0 Then
               'set properties
               ctrl.Filter = bstrFilter
               ctrl.DefaultExt = bstrDefaultExtension
               ctrl.InitDir = bstrCurrentDirectory
               ctrl.ShowSave
               'return to client
               AppShowCommonDlgSave = ctrl.FileName
            End If
            
            'clean-up & dereference
            If ObjPtr(ctrl) > 0 Then Set ctrl = Nothing
            Exit Function
            
ErrLine:

            Err.Clear
            Exit Function
            End Function
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppIsSoundCardInstalled
            ' * procedure description:  Returns true if the system has a soundcard installed that is "sufficient" to produce basic
            ' *                                       sound and midi functions.
            ' ******************************************************************************************************************************
            Private Function AppIsSoundCardInstalled() As Boolean
            On Local Error GoTo ErrLine
            
            'query available device output(s)
            If waveOutGetNumDevs > 0 And midiOutGetNumDevs > 0 Then AppIsSoundCardInstalled = True
            Exit Function
            
ErrLine:

            Err.Clear
            Exit Function
            End Function
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppStayOnTop
            ' * procedure description:  Moves a window to the topmost position in the Z plane.
            ' *
            ' ******************************************************************************************************************************
            Public Sub AppStayOnTop(hWnd As Long)
            On Local Error GoTo ErrLine
            SetWindowPos hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE Or SWP_NOSIZE
            Exit Sub
            
ErrLine:

            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppNotOnTop
            ' * procedure description:  Moves a window to the default position in the Z plane.
            ' *
            ' ******************************************************************************************************************************
            Public Sub AppNotOnTop(hWnd As Long)
            On Local Error GoTo ErrLine
            SetWindowPos hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE Or SWP_NOSIZE
            Exit Sub
            
ErrLine:

            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppOLEDragDrop
            ' * procedure description:  Moves a window to the default position in the Z plane.
            ' *
            ' ******************************************************************************************************************************
            Public Sub AppOLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppPlayFile(Data.Files(1))
            Exit Sub
            
ErrLine:

            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppOLEDragOver
            ' * procedure description:  Moves a window to the default position in the Z plane.
            ' *
            ' ******************************************************************************************************************************
            Public Sub AppOLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            Dim nCount As Long
            Dim nCount2 As Long
            Dim varSupport() As String
            Dim bstrSupport As String
            Dim bstrFileName As String
            On Local Error GoTo ErrLine
            
            'set default(s)
            Effect = vbDropEffectNone
            bstrSupport = ".au;.avi;.mov;.mpg;.mpeg;.wav;.aif;.mid;.mp2;.mp3"
            varSupport = Split(bstrSupport, ";") 'split the supported files into an array..
            
            For nCount = 1 To Data.Files.Count
                For nCount2 = LBound(varSupport) To UBound(varSupport)
                     If InStr(LCase(Data.Files(nCount)), LCase(varSupport(nCount2))) > 0 Then
                        'match located, supported media file dropped..
                        Effect = vbDropEffectCopy
                        bstrFileName = Data.Files(nCount)
                        'Data.Files.Clear: Data.Files.Add bstrFileName
                        Exit Sub
                     End If
                Next
            Next
            'reset effect
            Effect = vbDropEffectNone
            Exit Sub
            
ErrLine:

            Err.Clear
            Exit Sub
            End Sub
            
            
            
            
' **************************************************************************************************************************************
' * PRIVATE INTERFACE- QUARTZ WRAPPER PROCEDURES
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: HasAudio
            ' * procedure description:  Returns true if the specific instance of the control will support an audio stream.
            ' *
            ' ******************************************************************************************************************************
            Private Function HasAudio(objMediaControl As IMediaControl) As Boolean
            Dim nVolume As Long
            Dim objAudio As IBasicAudio
            On Local Error GoTo ErrLine
            
            'query audio interface
            If ObjPtr(objMediaControl) > 0 Then
              Set objAudio = objMediaControl
              nVolume = objAudio.Volume
              HasAudio = True
            End If
            Exit Function
            
ErrLine:

            Err.Clear
            Exit Function
            End Function
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: HasVideo
            ' * procedure description:  Returns true if the specific instance of the control will support a video stream.
            ' *
            ' ******************************************************************************************************************************
            Private Function HasVideo(objMediaControl As IMediaControl) As Boolean
            Dim boolVisible As Boolean
            Dim objVideo As IVideoWindow
            On Local Error GoTo ErrLine
            
            'query video window interface
            If ObjPtr(objMediaControl) > 0 Then
              Set objVideo = objMediaControl
              boolVisible = objVideo.Visible
              HasVideo = True
            End If
            Exit Function
            
ErrLine:

            Err.Clear
            Exit Function
            End Function
