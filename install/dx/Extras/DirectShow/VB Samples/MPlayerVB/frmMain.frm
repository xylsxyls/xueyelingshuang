VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmMain 
   Caption         =   "DirectX Media Player VB Sample"
   ClientHeight    =   5490
   ClientLeft      =   120
   ClientTop       =   645
   ClientWidth     =   4275
   Icon            =   "frmMain.frx":0000
   LinkTopic       =   "frmMain"
   LockControls    =   -1  'True
   OLEDropMode     =   1  'Manual
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5490
   ScaleWidth      =   4275
   Visible         =   0   'False
   Begin VB.PictureBox picAVAlignedLinkWindow 
      Align           =   1  'Align Top
      Appearance      =   0  'Flat
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   375
      Left            =   0
      Negotiate       =   -1  'True
      OLEDropMode     =   1  'Manual
      ScaleHeight     =   375
      ScaleWidth      =   4275
      TabIndex        =   23
      Tag             =   "0"
      Top             =   0
      Width           =   4275
      Begin VB.PictureBox fraLinkList 
         BorderStyle     =   0  'None
         Height          =   375
         Left            =   0
         ScaleHeight     =   375
         ScaleWidth      =   4275
         TabIndex        =   25
         Top             =   0
         Width           =   4275
         Begin MSComctlLib.Toolbar tbMusicLinkList 
            Height          =   360
            Left            =   1095
            TabIndex        =   26
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
            Left            =   2280
            TabIndex        =   27
            Top             =   0
            Width           =   1755
            _ExtentX        =   3096
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
         Begin MSComctlLib.Toolbar tbRadioLinkList 
            Height          =   360
            Left            =   0
            TabIndex        =   28
            Top             =   0
            Width           =   1155
            _ExtentX        =   2037
            _ExtentY        =   635
            ButtonWidth     =   1931
            ButtonHeight    =   635
            AllowCustomize  =   0   'False
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
      End
      Begin MSComctlLib.Toolbar tbNavigation 
         Height          =   330
         Left            =   0
         TabIndex        =   24
         Top             =   60
         Width           =   690
         _ExtentX        =   1217
         _ExtentY        =   582
         ButtonWidth     =   609
         ButtonHeight    =   582
         AllowCustomize  =   0   'False
         Wrappable       =   0   'False
         Style           =   1
         ImageList       =   "ctrlNavigationImageEnabled"
         DisabledImageList=   "ctrlNavigationImageListDisabled"
         HotImageList    =   "ctrlNavigationImageListHot"
         _Version        =   393216
         BeginProperty Buttons {66833FE8-8583-11D1-B16A-00C0F0283628} 
            NumButtons      =   2
            BeginProperty Button1 {66833FEA-8583-11D1-B16A-00C0F0283628} 
               Key             =   "gobackward"
               Object.ToolTipText     =   "Back"
               ImageIndex      =   1
            EndProperty
            BeginProperty Button2 {66833FEA-8583-11D1-B16A-00C0F0283628} 
               Key             =   "goforward"
               Object.ToolTipText     =   "Forward"
               ImageIndex      =   2
            EndProperty
         EndProperty
      End
      Begin VB.Line lnMenuDividerShadow 
         BorderColor     =   &H00808080&
         X1              =   0
         X2              =   4260
         Y1              =   0
         Y2              =   0
      End
      Begin VB.Line lnMenuDividerHighlight 
         BorderColor     =   &H00FFFFFF&
         X1              =   0
         X2              =   4260
         Y1              =   10
         Y2              =   10
      End
   End
   Begin VB.PictureBox picAVAlignedStatusWindow 
      Align           =   1  'Align Top
      Appearance      =   0  'Flat
      BackColor       =   &H00000000&
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   1520
      Left            =   0
      OLEDropMode     =   1  'Manual
      ScaleHeight     =   1515
      ScaleWidth      =   4275
      TabIndex        =   4
      Tag             =   "4"
      Top             =   3930
      Width           =   4275
      Begin VB.PictureBox picPrettyWave 
         Appearance      =   0  'Flat
         BackColor       =   &H00000000&
         BorderStyle     =   0  'None
         ForeColor       =   &H80000008&
         Height          =   200
         Left            =   3915
         OLEDropMode     =   1  'Manual
         Picture         =   "frmMain.frx":0442
         ScaleHeight     =   195
         ScaleWidth      =   195
         TabIndex        =   5
         Top             =   1245
         Width           =   200
      End
      Begin VB.Image imgFileFormatIcon 
         Height          =   240
         Left            =   120
         OLEDropMode     =   1  'Manual
         Stretch         =   -1  'True
         Top             =   660
         Width           =   240
      End
      Begin VB.Line lnSeptum3 
         BorderColor     =   &H8000000F&
         BorderWidth     =   5
         X1              =   0
         X2              =   4350
         Y1              =   0
         Y2              =   0
      End
      Begin VB.Line lnSeptum2 
         BorderColor     =   &H00808080&
         X1              =   0
         X2              =   4350
         Y1              =   1200
         Y2              =   1200
      End
      Begin VB.Label lblAuthor 
         Alignment       =   1  'Right Justify
         BackColor       =   &H00000000&
         Caption         =   "Author:"
         ForeColor       =   &H00FFFFFF&
         Height          =   255
         Left            =   -510
         OLEDropMode     =   1  'Manual
         TabIndex        =   15
         Top             =   675
         Width           =   1650
      End
      Begin VB.Label lblCopyright 
         Alignment       =   1  'Right Justify
         BackColor       =   &H00000000&
         Caption         =   "Copyright: "
         ForeColor       =   &H00FFFFFF&
         Height          =   255
         Left            =   -435
         OLEDropMode     =   1  'Manual
         TabIndex        =   14
         Top             =   930
         Width           =   1650
      End
      Begin VB.Label lblState 
         Alignment       =   2  'Center
         BackColor       =   &H00000000&
         ForeColor       =   &H00FFFFFF&
         Height          =   255
         Left            =   -360
         OLEDropMode     =   1  'Manual
         TabIndex        =   13
         Top             =   1245
         Width           =   1980
      End
      Begin VB.Label lblClip 
         Alignment       =   1  'Right Justify
         BackColor       =   &H00000000&
         Caption         =   "Clip:"
         ForeColor       =   &H00FFFFFF&
         Height          =   255
         Left            =   -510
         OLEDropMode     =   1  'Manual
         TabIndex        =   12
         Top             =   420
         Width           =   1650
      End
      Begin VB.Label lblClipValue 
         BackColor       =   &H00000000&
         ForeColor       =   &H00FFFFFF&
         Height          =   240
         Left            =   1290
         OLEDropMode     =   1  'Manual
         TabIndex        =   11
         Top             =   420
         Width           =   2850
      End
      Begin VB.Label lblAuthorValue 
         BackColor       =   &H00000000&
         ForeColor       =   &H00FFFFFF&
         Height          =   240
         Left            =   1290
         OLEDropMode     =   1  'Manual
         TabIndex        =   10
         Top             =   675
         Width           =   2850
      End
      Begin VB.Label lblCopyrightValue 
         BackColor       =   &H00000000&
         ForeColor       =   &H00FFFFFF&
         Height          =   240
         Left            =   1290
         OLEDropMode     =   1  'Manual
         TabIndex        =   9
         Top             =   930
         Width           =   2850
      End
      Begin VB.Label lblCurrentTime 
         Alignment       =   1  'Right Justify
         BackColor       =   &H00000000&
         ForeColor       =   &H00FFFFFF&
         Height          =   240
         Left            =   1665
         OLEDropMode     =   1  'Manual
         TabIndex        =   8
         Top             =   1245
         Width           =   2205
      End
      Begin VB.Line lnSeptum 
         BorderColor     =   &H00808080&
         X1              =   0
         X2              =   4350
         Y1              =   360
         Y2              =   360
      End
      Begin VB.Label lblFileName 
         Alignment       =   1  'Right Justify
         BackColor       =   &H00000000&
         Caption         =   "Show:"
         ForeColor       =   &H00FFFFFF&
         Height          =   255
         Left            =   -660
         OLEDropMode     =   1  'Manual
         TabIndex        =   7
         Top             =   120
         Width           =   1800
      End
      Begin VB.Label lblFileNameValue 
         BackColor       =   &H00000000&
         ForeColor       =   &H00FFFFFF&
         Height          =   240
         Left            =   1665
         OLEDropMode     =   1  'Manual
         TabIndex        =   6
         Top             =   120
         Width           =   2400
      End
   End
   Begin VB.PictureBox picAVAlignedControlWindow 
      Align           =   1  'Align Top
      Appearance      =   0  'Flat
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   375
      Left            =   0
      OLEDropMode     =   1  'Manual
      ScaleHeight     =   375
      ScaleWidth      =   4275
      TabIndex        =   2
      Tag             =   "3"
      Top             =   3555
      Width           =   4275
      Begin VB.Frame fraVolume 
         BorderStyle     =   0  'None
         Height          =   330
         Left            =   1400
         TabIndex        =   17
         Top             =   0
         Width           =   1240
         Begin VB.Frame fraVolumeSlider 
            BorderStyle     =   0  'None
            Caption         =   "Frame1"
            Height          =   200
            Left            =   280
            TabIndex        =   19
            Top             =   60
            Width           =   975
            Begin MSComctlLib.Slider slVolume 
               Height          =   270
               Left            =   0
               TabIndex        =   20
               TabStop         =   0   'False
               ToolTipText     =   "Volume Control"
               Top             =   0
               Width           =   1000
               _ExtentX        =   1773
               _ExtentY        =   476
               _Version        =   393216
               MousePointer    =   99
               MouseIcon       =   "frmMain.frx":051A
               OLEDropMode     =   1
               LargeChange     =   100
               SmallChange     =   50
               Min             =   -5000
               Max             =   0
               TickFrequency   =   500
            End
         End
         Begin VB.Image imgVolume 
            Height          =   240
            Left            =   0
            OLEDropMode     =   1  'Manual
            Picture         =   "frmMain.frx":067C
            Top             =   40
            Width           =   240
         End
      End
      Begin VB.Frame fraBalance 
         BorderStyle     =   0  'None
         Height          =   330
         Left            =   2980
         TabIndex        =   16
         Top             =   0
         Width           =   1240
         Begin VB.Frame fraBalanceSlider 
            BorderStyle     =   0  'None
            Caption         =   "Frame1"
            Height          =   200
            Left            =   280
            TabIndex        =   21
            Top             =   60
            Width           =   975
            Begin MSComctlLib.Slider slBalance 
               Height          =   270
               Left            =   0
               TabIndex        =   22
               TabStop         =   0   'False
               ToolTipText     =   "Balance Control"
               Top             =   0
               Width           =   1000
               _ExtentX        =   1773
               _ExtentY        =   476
               _Version        =   393216
               MousePointer    =   99
               MouseIcon       =   "frmMain.frx":0758
               OLEDropMode     =   1
               LargeChange     =   1000
               SmallChange     =   100
               Min             =   -10000
               Max             =   10000
               TickFrequency   =   2000
            End
         End
         Begin VB.Image imgBalance 
            Height          =   240
            Left            =   0
            OLEDropMode     =   1  'Manual
            Picture         =   "frmMain.frx":08BA
            Top             =   40
            Width           =   240
         End
      End
      Begin MSComctlLib.Toolbar tbControlBar 
         Height          =   330
         Left            =   0
         TabIndex        =   18
         Top             =   0
         Width           =   975
         _ExtentX        =   1720
         _ExtentY        =   582
         ButtonWidth     =   609
         ButtonHeight    =   582
         AllowCustomize  =   0   'False
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
      Begin VB.Line lnControlSeptumHighlight 
         BorderColor     =   &H00FFFFFF&
         Index           =   1
         X1              =   2800
         X2              =   2800
         Y1              =   0
         Y2              =   330
      End
      Begin VB.Line lnControlSeptumShadow 
         BorderColor     =   &H00808080&
         Index           =   1
         X1              =   2790
         X2              =   2790
         Y1              =   0
         Y2              =   330
      End
      Begin VB.Line lnControlSeptumHighlight 
         BorderColor     =   &H00FFFFFF&
         Index           =   0
         X1              =   1220
         X2              =   1220
         Y1              =   0
         Y2              =   330
      End
      Begin VB.Line lnControlSeptumShadow 
         BorderColor     =   &H00808080&
         Index           =   0
         X1              =   1200
         X2              =   1200
         Y1              =   0
         Y2              =   330
      End
   End
   Begin VB.PictureBox picAVAlignedPositionWindow 
      Align           =   1  'Align Top
      Appearance      =   0  'Flat
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   285
      Left            =   0
      OLEDropMode     =   1  'Manual
      ScaleHeight     =   285
      ScaleWidth      =   4275
      TabIndex        =   1
      Tag             =   "2"
      Top             =   3270
      Width           =   4275
      Begin MSComctlLib.Slider slCurrentPosition 
         Height          =   330
         Left            =   0
         TabIndex        =   3
         TabStop         =   0   'False
         ToolTipText     =   "Current Position"
         Top             =   0
         Width           =   4305
         _ExtentX        =   7594
         _ExtentY        =   582
         _Version        =   393216
         MousePointer    =   99
         Enabled         =   0   'False
         MouseIcon       =   "frmMain.frx":0D3E
         OLEDropMode     =   1
         Min             =   1
         Max             =   100
         SelectRange     =   -1  'True
         SelStart        =   1
         TickStyle       =   3
         Value           =   1
      End
   End
   Begin VB.PictureBox picAVAlignedMovieWindow 
      Align           =   1  'Align Top
      Appearance      =   0  'Flat
      BackColor       =   &H00000000&
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   2895
      Left            =   0
      MouseIcon       =   "frmMain.frx":0EA0
      MousePointer    =   99  'Custom
      OLEDropMode     =   1  'Manual
      ScaleHeight     =   2895
      ScaleWidth      =   4275
      TabIndex        =   0
      TabStop         =   0   'False
      Tag             =   "1"
      Top             =   375
      Width           =   4275
   End
   Begin MSComctlLib.ImageList ctrlNavigationImageListDisabled 
      Left            =   600
      Top             =   7860
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   16711935
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   2
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":0FF2
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":1544
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlControlList 
      Left            =   0
      Top             =   6045
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   16711935
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   5
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":1A96
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":1BA8
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":1CBA
            Key             =   ""
         EndProperty
         BeginProperty ListImage4 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":1DCC
            Key             =   ""
         EndProperty
         BeginProperty ListImage5 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":1EDE
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Timer tmrTimer 
      Interval        =   25
      Left            =   660
      Top             =   6120
   End
   Begin MSComctlLib.ImageList ctrlRadioLinkList 
      Left            =   0
      Top             =   6645
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
            Picture         =   "frmMain.frx":2430
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":271C
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMusicLinkList 
      Left            =   600
      Top             =   6645
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
            Picture         =   "frmMain.frx":2A30
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":2D18
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMediaGuideLinkList 
      Left            =   1200
      Top             =   6645
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
            Picture         =   "frmMain.frx":3004
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":34A0
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlRadioLinkListHot 
      Left            =   0
      Top             =   7245
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
            Picture         =   "frmMain.frx":3940
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMusicLinkListHot 
      Left            =   600
      Top             =   7245
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
            Picture         =   "frmMain.frx":3C54
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlMediaGuideLinkListHot 
      Left            =   1200
      Top             =   7245
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
            Picture         =   "frmMain.frx":3F40
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlNavigationImageListHot 
      Left            =   1200
      Top             =   7860
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   16711935
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   2
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":43E0
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":4932
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ctrlNavigationImageEnabled 
      Left            =   0
      Top             =   7860
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   16711935
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   2
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":4E84
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":53D6
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
      Begin VB.Menu mnuViewStatusbar 
         Caption         =   "&Status"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewDisplay 
         Caption         =   "&Display"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewPositionBar 
         Caption         =   "&Position"
      End
      Begin VB.Menu mnuViewControlbar 
         Caption         =   "&Controls"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewSeptum 
         Caption         =   "-"
      End
      Begin VB.Menu mnuViewAlwaysOnTop 
         Caption         =   "&Always On Top"
         Checked         =   -1  'True
         Shortcut        =   ^T
      End
   End
   Begin VB.Menu mnuPlay 
      Caption         =   "&Play"
      Begin VB.Menu mnuPlay_PlayPause 
         Caption         =   "Play/Pause"
      End
      Begin VB.Menu mnuPlay_Stop 
         Caption         =   "Stop"
      End
      Begin VB.Menu mnuPlay_Septum 
         Caption         =   "-"
      End
      Begin VB.Menu mnuPlay_Volume 
         Caption         =   "Volume"
         Begin VB.Menu mnuPlay_Volume_Up 
            Caption         =   "Up"
         End
         Begin VB.Menu mnuPlay_Volume_Down 
            Caption         =   "Down"
         End
         Begin VB.Menu mnuPlay_Volume_Mute 
            Caption         =   "Mute"
            Shortcut        =   ^M
         End
      End
   End
   Begin VB.Menu mnuGo 
      Caption         =   "&Go"
      Begin VB.Menu mnuGo_Back 
         Caption         =   "Back"
      End
      Begin VB.Menu mnuGo_Forward 
         Caption         =   "Forward"
      End
      Begin VB.Menu mnuGo_Septum 
         Caption         =   "-"
      End
      Begin VB.Menu mnuGo_MediaGuide 
         Caption         =   "Media Guide"
         Shortcut        =   ^H
      End
      Begin VB.Menu mnuGo_Music 
         Caption         =   "Music"
         Shortcut        =   ^U
      End
      Begin VB.Menu mnuGo_Radio 
         Caption         =   "Radio"
         Shortcut        =   ^F
      End
      Begin VB.Menu mnuGo_Home 
         Caption         =   "Windows Media Player Home Page"
      End
   End
   Begin VB.Menu mnuHelp 
      Caption         =   "&Help"
      Begin VB.Menu mnuHelp_About 
         Caption         =   "About DirectX Media Player VB Sample"
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

Private m_bstrHistory() As String                       'file history
Private m_HistoricalIndex As Long                    'user index
Private m_boolLoaded As Boolean                    'file loaded
Private m_boolViewLinks As Boolean                'view link bar
Private m_boolViewDisplay As Boolean            'view display area
Private m_boolViewControlBar As Boolean        'view control bar
Private m_boolViewStatusBar As Boolean         'view status bar
Private m_boolViewPositionBar As Boolean      'view position bar
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
Private Const ACTUAL_STATUSBAR_HEIGHT As Long = 1520  'fixed statusbar height, in twips
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
            
            'Good place to start -->
            'Instantiate media control (quartz)
            Set m_objMediaControl = _
                  New FilgraphManager
            
            'assign default value(s) to module-level data;
            'these are used to track visibility changes in the ui
            m_boolViewLinks = True
            m_boolViewDisplay = True
            m_boolViewControlBar = True
            m_boolViewStatusBar = True
            m_boolViewPositionBar = True
            
            'initialize the historical file listing
            ReDim Preserve m_bstrHistory(0)
            
            'assign default value(s) to module-level data
            'here we assign value(s) for the default video rect
            m_sngPreferredVideoWidth = DEFAULT_VIDEO_WIDTH
            m_sngPreferredVideoHeight = DEFAULT_VIDEO_HEIGHT
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
            
            'allocate and distribute resources
            imgVolume.Picture = ctrlControlList.ListImages(4).ExtractIcon
            imgBalance.Picture = ctrlControlList.ListImages(5).ExtractIcon
            
            'verify that a sound card is installed on the local system
            'there's no sense going forward without one in most cases..
            If AppIsSoundCardInstalled = False Then
               nResultant = MsgBox("A sound card or comparable hardware was not detected." & vbCrLf & "Audio hardware is required for this sample to run, continue to load the sample?", vbExclamation + vbApplicationModal + vbYesNo)
               If nResultant = vbNo Then
                  Unload Me: Exit Sub
               End If
            End If

            'default the checked menu items
            'more ui code- assigning checked/
            'unchecked values to menu items
            mnuViewLinks.Checked = True
            mnuViewDisplay.Checked = True
            mnuViewControlbar.Checked = True
            mnuViewStatusbar.Checked = True
            mnuViewPositionBar.Checked = True
            mnuViewAlwaysOnTop.Checked = False
               
            'disable playback control buttons by default
            'here we have ourselves a control bar and slider
            slCurrentPosition.Enabled = False
            tbControlBar.Buttons("play").Enabled = False
            tbControlBar.Buttons("stop").Enabled = False
            tbControlBar.Buttons("pause").Enabled = False
            
            'disable forward/backward navigation initally
            tbNavigation.Buttons("goforward").Enabled = False
            tbNavigation.Buttons("gobackward").Enabled = False
            
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
            Dim ctrl As Control
            On Local Error GoTo ErrLine
            
            'disable the timer before
            'unloading the form to make
            'things more stable overall..
            tmrTimer.Enabled = False
            
            'assign value to
            'stop video playback
            m_boolLoaded = False
            
             'move the form off the screen
            'to ensure a quick background repaint
            Me.Move Screen.Width * 8, Screen.Height * 8
            
            'always assign the owner of the video
            'window to null when your done playing
            'with quartz- otherwise you very well may
            'find yourself with a timer coming back and
            'looking for the window; thus throwing an assert
            If Not m_objMediaControl Is Nothing Then
               Call m_objMediaControl.Stop
               If Not m_objVideoWindow Is Nothing Then
                  m_objVideoWindow.Left = Screen.Width * 8
                  m_objVideoWindow.Top = Screen.Height * 8
                  m_objVideoWindow.Visible = False
                  m_objVideoWindow.Owner = 0 'Assign the Owner to NULL
               End If
            End If
            
            'there is a known issue when using multiple toolbar control(s)
            'on a single vb form which may cause a crash during unloading
            'in certain instances, so a quick fix is to hide all toolbar control(s)
            'before a vb form is unloaded.  A future fix & kb article will be included
            'in the next Microsoft Visual Studio SP5 or SP6 Service Pack release.
            For Each ctrl In Me.Controls
               If TypeName(ctrl) = "Toolbar" Then _
                  ctrl.Visible = False
            Next
            
            'clean-up & dereference all module-level data
            If Not m_objBasicAudio Is Nothing Then Set m_objBasicAudio = Nothing
            If Not m_objBasicVideo Is Nothing Then Set m_objBasicVideo = Nothing
            If Not m_objMediaEvent Is Nothing Then Set m_objMediaEvent = Nothing
            If Not m_objMediaControl Is Nothing Then Set m_objMediaControl = Nothing
            If Not m_objVideoWindow Is Nothing Then Set m_objVideoWindow = Nothing
            If Not m_objMediaPosition Is Nothing Then Set m_objMediaPosition = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: Form_Resize
            ' * procedure description:  Occurs when a form is first displayed or the size of an object changes.
            ' *                                       Resize the form & contingent controls based on individule control visibility.
            ' *                                       This is probably the most difficult part of the sample application; as there are
            ' *                                       a good many combinations of possibilities given to the user from the view menu.
            ' ******************************************************************************************************************************
            Private Sub Form_Resize()
            Dim ctrl As Control
            Dim sngDesiredHeight As Single
            Static nLastStyle As FormWindowStateConstants
            On Local Error GoTo ErrLine
            
            'repaint the movie window; due to the
            'auto alignment of the container it is necessary
            'to carry over paint events to the child controls
            'in some cases, as failure to do so could lead
            'to the specified window not getting all paint events
            If picAVAlignedMovieWindow.Visible Then _
               Call picAVAlignedMovieWindow_Paint
            
            If nLastStyle = vbNormal And frmMain.WindowState = vbMaximized Then
               'maximize in progress
               Call AppChangeView(True, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar, False)
            End If
            'remember last style
            nLastStyle = frmMain.WindowState
            
            'additionally align the status area elements
            If picAVAlignedStatusWindow.Visible = True Then
               If lnSeptum.X2 <> lnSeptum.Container.Width Then lnSeptum.X2 = lnSeptum.Container.Width
               If lnSeptum2.X2 <> lnSeptum2.Container.Width Then lnSeptum2.X2 = lnSeptum2.Container.Width
               If lnSeptum3.X2 <> lnSeptum3.Container.Width Then lnSeptum3.X2 = lnSeptum3.Container.Width
               If picPrettyWave.Left <> (picPrettyWave.Container.Width - picPrettyWave.Width * 2) Then _
                  picPrettyWave.Left = (picPrettyWave.Container.Width - picPrettyWave.Width * 2)
               If lblCurrentTime.Left <> (picPrettyWave.Left - lblCurrentTime.Width) - 200 Then _
                  lblCurrentTime.Left = (picPrettyWave.Left - lblCurrentTime.Width) - 200
            End If
            
            'align the linklist divider
            If fraLinkList.Visible Then
               If lnMenuDividerShadow.X1 <> lnMenuDividerShadow.Container.Left Then _
                  lnMenuDividerShadow.X1 = lnMenuDividerShadow.Container.Left
               If lnMenuDividerShadow.X2 <> lnMenuDividerShadow.Container.Width Then _
                  lnMenuDividerShadow.X2 = lnMenuDividerShadow.Container.Width
               If lnMenuDividerHighlight.X1 <> lnMenuDividerHighlight.Container.Left Then _
                  lnMenuDividerHighlight.X1 = lnMenuDividerHighlight.Container.Left
               If lnMenuDividerHighlight.X2 <> lnMenuDividerHighlight.Container.Width Then _
                  lnMenuDividerHighlight.X2 = lnMenuDividerHighlight.Container.Width
            End If
            
            'position linklist bar
            If picAVAlignedLinkWindow.Height <> 450 Then picAVAlignedLinkWindow.Height = 450
            
            'position linklist frame hyperlink frame
            If fraLinkList.Container.Width > (tbNavigation.Width + 4155) Then
               If fraLinkList.Top <> 30 Then fraLinkList.Top = 30
               If fraLinkList.Left <> fraLinkList.Container.Width - fraLinkList.Width Then fraLinkList.Left = fraLinkList.Container.Width - fraLinkList.Width
               If fraLinkList.Width <> 4155 Then fraLinkList.Width = 4155
               If fraLinkList.Height <> (fraLinkList.Container.Height - 30) Then fraLinkList.Height = (fraLinkList.Container.Height - 30)
            Else
               If fraLinkList.Top <> 30 Then fraLinkList.Top = 30
               If fraLinkList.Left <> 0 Then fraLinkList.Left = 0
               If fraLinkList.Width <> 4155 Then fraLinkList.Width = 4155
               If fraLinkList.Height <> (fraLinkList.Container.Height - 30) Then fraLinkList.Height = (fraLinkList.Container.Height - 30)
            End If
            
            'position radio linklist toolbar in the linklist frame
            If tbRadioLinkList.Top <> 0 Then tbRadioLinkList.Top = 0
            If tbRadioLinkList.Left <> 0 Then tbRadioLinkList.Left = 0
            If tbRadioLinkList.Width <> 1140 Then tbRadioLinkList.Width = 1140
            If tbRadioLinkList.Height <> 360 Then tbRadioLinkList.Height = 360
            
            'position music linklist toolbar in the linklist frame
            If tbMusicLinkList.Top <> 0 Then tbMusicLinkList.Top = 0
            If tbMusicLinkList.Left <> (tbRadioLinkList.Left + tbRadioLinkList.Width) Then _
               tbMusicLinkList.Left = (tbRadioLinkList.Left + tbRadioLinkList.Width)
            If tbMusicLinkList.Width <> 1140 Then tbMusicLinkList.Width = 1140
            If tbMusicLinkList.Height <> 360 Then tbMusicLinkList.Height = 360
            
            'position radio guide linklist toolbar in the linklist frame
            If tbMediaGuideLinkList.Top <> 0 Then tbMediaGuideLinkList.Top = 0
            If tbMediaGuideLinkList.Left <> (tbMusicLinkList.Left + tbMusicLinkList.Width) Then _
               tbMediaGuideLinkList.Left = (tbMusicLinkList.Left + tbMusicLinkList.Width)
            If tbMediaGuideLinkList.Width <> 1875 Then tbMediaGuideLinkList.Width = 1875
            If tbMediaGuideLinkList.Height <> 360 Then tbMediaGuideLinkList.Height = 360
            
            
            'position the slider in the position frame
            'allowing the focus rect to be hidden from the user
            If slCurrentPosition.Top <> -15 Then slCurrentPosition.Top = -15
            If slCurrentPosition.Left <> -15 Then slCurrentPosition.Left = -15
            If slCurrentPosition.Width <> slCurrentPosition.Container.Width + 30 Then slCurrentPosition.Width = slCurrentPosition.Container.Width + 30
            If slCurrentPosition.Height <> 330 Then slCurrentPosition.Height = 330

            'position the control toolbar
            If picAVAlignedControlWindow.Height <> 390 Then picAVAlignedControlWindow.Height = 390
            
            'position the slider in the volume frame
            'allowing the focus rect to be hidden from the user
            If slVolume.Top <> -15 Then slVolume.Top = -15
            If slVolume.Left <> -15 Then slVolume.Left = -15
            If slVolume.Width <> slVolume.Container.Width + 30 Then slVolume.Width = slVolume.Container.Width + 30
            If slVolume.Height <> 240 Then slVolume.Height = 240
            
            'position the slider in the balance frame
            'allowing the focus rect to be hidden from the user
            If slBalance.Top <> -15 Then slBalance.Top = -15
            If slBalance.Left <> -15 Then slBalance.Left = -15
            If slBalance.Width <> slBalance.Container.Width + 30 Then slBalance.Width = slBalance.Container.Width + 30
            If slBalance.Height <> 240 Then slBalance.Height = 240
            
            'position the status window
            If picAVAlignedStatusWindow.Height <> ACTUAL_STATUSBAR_HEIGHT Then picAVAlignedStatusWindow.Height = ACTUAL_STATUSBAR_HEIGHT
            
            
            'proceed to obtain the height of each picturebox
            'on the form that is a direct child of the given form.
            'the height will be used to recalculate the video window
            'and form height at the end of the procedure using recursion..
            For Each ctrl In Me.Controls
               If TypeName(ctrl) = "PictureBox" Then
                  If ctrl.Container.Name = frmMain.Name Then
                     If ctrl.Visible Then
                        'obtain height
                        sngDesiredHeight = sngDesiredHeight + ctrl.Height
                     End If
                  End If
               End If
            Next


                                  
            If picAVAlignedMovieWindow.Visible Then
                  If frmMain.ScaleHeight <> sngDesiredHeight Then
                     If picAVAlignedMovieWindow.Height + (frmMain.Height - ((frmMain.Height - frmMain.ScaleHeight) + sngDesiredHeight)) > 0 Then
                        picAVAlignedMovieWindow.Height = picAVAlignedMovieWindow.Height + (frmMain.Height - ((frmMain.Height - frmMain.ScaleHeight) + sngDesiredHeight))
                        If picAVAlignedMovieWindow.Height <> picAVAlignedMovieWindow.Height + (frmMain.Height - ((frmMain.Height - frmMain.ScaleHeight) + sngDesiredHeight)) Then
                           Call Form_Resize
                        End If
                     End If
                  End If
            Else
                  If picAVAlignedStatusWindow.Visible Then
                     frmMain.Height = (picAVAlignedStatusWindow.Top + picAVAlignedStatusWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
                  ElseIf picAVAlignedControlWindow.Visible Then
                     frmMain.Height = (picAVAlignedControlWindow.Top + picAVAlignedControlWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
                  ElseIf picAVAlignedPositionWindow.Visible Then
                     frmMain.Height = (picAVAlignedPositionWindow.Top + picAVAlignedPositionWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
                  ElseIf picAVAlignedMovieWindow.Visible Then
                     frmMain.Height = (picAVAlignedMovieWindow.Top + picAVAlignedMovieWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
                  ElseIf picAVAlignedLinkWindow.Visible Then
                     frmMain.Height = (picAVAlignedLinkWindow.Top + picAVAlignedLinkWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
                  Else: frmMain.Height = (frmMain.Height - frmMain.ScaleHeight)
                  End If
            End If
            
            'last, snap the video window and mantain the
            'proper aspect ratio for the given video window..
            If Not m_objVideoWindow Is Nothing Then
               Call AppSnapVideoWindow( _
                      picAVAlignedMovieWindow.Left, picAVAlignedMovieWindow.Top, picAVAlignedMovieWindow.Width, picAVAlignedMovieWindow.Height)
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
            ' * procedure name: fraBalance_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub fraBalance_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: fraBalance_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub fraBalance_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: fraLinkList_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub fraLinkList_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: fraLinkList_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub fraLinkList_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: fraVolume_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub fraVolume_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: fraVolume_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub fraVolume_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblState_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblState_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: lblState_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub lblState_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub


            ' ******************************************************************************************************************************
            ' * procedure name: tbNavigation_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbNavigation_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tbNavigation_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub tbNavigation_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub


            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedControlWindow_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedControlWindow_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedControlWindow_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedControlWindow_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedLinkWindow_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedLinkWindow_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedLinkWindow_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedLinkWindow_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
            On Local Error GoTo ErrLine
            Call AppOLEDragOver(Data, Effect, Button, Shift, X, Y, State)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedLinkWindow_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub slCurrentPosition_OLEDragDrop(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedLinkWindow_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub slCurrentPosition_OLEDragOver(Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
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
            ' * procedure name: picAVAlignedMovieWindow_OLEDragDrop
            ' * procedure description:  Occurs when data is dropped onto the control via an OLE drag/drop operation, and
            ' *                                       OLEDropMode is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedMovieWindow_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            Call AppOLEDragDrop(Data, Effect, Button, Shift, X, Y)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedMovieWindow_OLEDragOver
            ' * procedure description:  Occurs when the mouse is moved over the control during an OLE drag/drop operation,
            ' *                                       if its OLEDropMode property is set to manual.
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedMovieWindow_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
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
            
            'set 'enabled' status of the close
            'option when this menu is invoked;
            'dependent on the loaded status of the app.
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
            
            'move each form well off the edge of the screen;
            'thus allowing for a fast repaint; and unload each form
            For Each frm In Forms
                If frm.WindowState = vbNormal Then _
                   Call frm.Move(Screen.Width * 8, Screen.Height * 8)
                Unload frm: Set frm = Nothing
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
            On Local Error GoTo ErrLine
            
            'load the open dialog and
            'display it 'modal' to the app-
            'load the paths from the historical list
            frmMain.Enabled = False
            Load frmOpen: frmOpen.Show: frmOpen.LoadPaths
            
            'wait until the user has closed the dialog
            'here we are using a vbform much like a msgbox
            Do Until frmOpen.Visible = False: DoEvents
            Loop
            
            'hide the dialog (zero visibility) from the user
            'and enable the application window (non-modal)
            frmMain.Enabled = True
            frmOpen.Move 0, 0, Screen.Width * 8, Screen.Height * 8
            frmOpen.Visible = False: frmOpen.UpdatePaths
            
            'play back the elected file if specified..
            If frmOpen.CurrentPath <> vbNullString Then _
               Call AppPlayFile(frmOpen.CurrentPath)
            
            'clean-up & dereference
            Unload frmOpen: Set frmOpen = Nothing
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
            
            'clean-up the ui
            Set imgFileFormatIcon.Picture = _
                  LoadPicture(vbNullString)
            frmMain.Caption = App.Title
            lblState.Caption = "Closed"
            lblClipValue.Caption = vbNullString
            lblAuthorValue.Caption = vbNullString
            lblCurrentTime.Caption = vbNullString
            lblFileNameValue.Caption = vbNullString
            lblCopyrightValue.Caption = vbNullString
            Call slCurrentPosition.ClearSel
            slCurrentPosition.Value = 0
            slCurrentPosition.Enabled = False
            
            'stop video playback; don't forget to
            'assign the owner of the video window
            'to null here again; and clean-up resource(s)
            m_boolLoaded = False
            If Not m_objMediaControl Is Nothing Then
               Call m_objMediaControl.Stop
               If Not m_objVideoWindow Is Nothing Then
                  m_objVideoWindow.Left = Screen.Width * 8
                  m_objVideoWindow.Top = Screen.Height * 8
                  m_objVideoWindow.Visible = False
                  m_objVideoWindow.Owner = 0 'Assign the Owner to NULL
               End If
            End If
            
            'clean-up & dereference
            If Not m_objBasicAudio Is Nothing Then Set m_objBasicAudio = Nothing
            If Not m_objBasicVideo Is Nothing Then Set m_objBasicVideo = Nothing
            If Not m_objMediaEvent Is Nothing Then Set m_objMediaEvent = Nothing
            If Not m_objMediaControl Is Nothing Then Set m_objMediaControl = Nothing
            If Not m_objVideoWindow Is Nothing Then Set m_objVideoWindow = Nothing
            If Not m_objMediaPosition Is Nothing Then Set m_objMediaPosition = Nothing
            
            'assign default value(s) to module-level data
            'this will allow the application to reset itself like new
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
            ' * procedure name: mnuViewAlwaysOnTop_Click
            ' * procedure description:  Occurs when the "Always On Top" option is invoked from the "View" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuViewAlwaysOnTop_Click()
            On Local Error GoTo ErrLine
            
            'assign menu 'checked' state;
            'and proceed to set the window so that
            'it is always on top- no secret trick here..
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
            m_boolViewPositionBar = mnuViewPositionBar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar)
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
            
            'reset
            If mnuViewDisplay.Checked Then
               If frmMain.WindowState = vbMaximized Then _
                  frmMain.WindowState = vbNormal
            End If
            
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
            m_boolViewPositionBar = mnuViewPositionBar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar)
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
            m_boolViewPositionBar = mnuViewPositionBar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuViewPositionBar_Click
            ' * procedure description:  Occurs when the "Position" option is invoked from the "View" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuViewPositionBar_Click()
            On Local Error GoTo ErrLine
            
            'assign menu 'checked' state
            If mnuViewPositionBar.Checked Then
               mnuViewPositionBar.Checked = False
            Else: mnuViewPositionBar.Checked = True
            End If
            'assign the current view to module-level
            m_boolViewLinks = mnuViewLinks.Checked
            m_boolViewDisplay = mnuViewDisplay.Checked
            m_boolViewStatusBar = mnuViewStatusbar.Checked
            m_boolViewControlBar = mnuViewControlbar.Checked
            m_boolViewPositionBar = mnuViewPositionBar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar)
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
            m_boolViewPositionBar = mnuViewPositionBar.Checked
            'update the user interface with the elected view
            Call AppChangeView(m_boolViewDisplay, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuGo_Click
            ' * procedure description:  Occurs when the "Go" Menu is invoked from the main menu.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuGo_Click()
            On Local Error GoTo ErrLine
            
            'dynamically enable/disable
            'application menus based on state
            Select Case AppGetState
               Case AppStateClosed
                  mnuGo_Back.Enabled = False
                  mnuGo_Forward.Enabled = False
               Case AppStatePlaying
                  If tbNavigation.Buttons("goforward").Enabled Then
                     mnuGo_Forward.Enabled = True
                  Else: mnuGo_Forward.Enabled = False
                  End If
                  If tbNavigation.Buttons("gobackward").Enabled Then
                     mnuGo_Back.Enabled = True
                  Else: mnuGo_Back.Enabled = False
                  End If
               Case AppStatePaused
                  If tbNavigation.Buttons("goforward").Enabled Then
                     mnuGo_Forward.Enabled = True
                  Else: mnuGo_Forward.Enabled = False
                  End If
                  If tbNavigation.Buttons("gobackward").Enabled Then
                     mnuGo_Back.Enabled = True
                  Else: mnuGo_Back.Enabled = False
                  End If
               Case AppStateStopped
                  If tbNavigation.Buttons("goforward").Enabled Then
                     mnuGo_Forward.Enabled = True
                  Else: mnuGo_Forward.Enabled = False
                  End If
                  If tbNavigation.Buttons("gobackward").Enabled Then
                     mnuGo_Back.Enabled = True
                  Else: mnuGo_Back.Enabled = False
                  End If
            End Select
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuGo_Back_Click
            ' * procedure description:  Occurs when the "Back" option is invoked from the "Go" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuGo_Back_Click()
            On Local Error GoTo ErrLine
            Call tbNavigation_ButtonClick(tbNavigation.Buttons("gobackward"))
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuGo_Forward_Click
            ' * procedure description:  Occurs when the "Forward" option is invoked from the "Go" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuGo_Forward_Click()
            On Local Error GoTo ErrLine
            Call tbNavigation_ButtonClick(tbNavigation.Buttons("goforward"))
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuGo_Home_Click
            ' * procedure description:  Occurs when the "Home" option is invoked from the "Go" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuGo_Home_Click()
            Dim objExplorer As Object
            On Local Error GoTo ErrLine
            
            'create an instance of internet explorer, and navigate to the site
            Set objExplorer = CreateObject("InternetExplorer.Application")
            If Not objExplorer Is Nothing Then
               objExplorer.Visible = True
               Call objExplorer.Navigate("http://www.microsoft.com/windows/windowsmedia/en/download/default.asp")
            End If
            
            'clean-up & dereference
            If Not objExplorer Is Nothing Then Set objExplorer = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuGo_MediaGuide_Click
            ' * procedure description:  Occurs when the "Media Guide" option is invoked from the "Go" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuGo_MediaGuide_Click()
            On Local Error GoTo ErrLine
            Call tbMediaGuideLinkList_ButtonClick(tbMediaGuideLinkList.Buttons(1))
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuGo_Music_Click
            ' * procedure description:  Occurs when the "Music" option is invoked from the "Go" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuGo_Music_Click()
            On Local Error GoTo ErrLine
            Call tbMusicLinkList_ButtonClick(tbMusicLinkList.Buttons(1))
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuGo_Radio_Click
            ' * procedure description:  Occurs when the "Radio" option is invoked from the "Go" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuGo_Radio_Click()
            On Local Error GoTo ErrLine
            Call tbRadioLinkList_ButtonClick(tbRadioLinkList.Buttons(1))
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuHelp_About_Click
            ' * procedure description:  Occurs when the "About" option is invoked from the "Help" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuHelp_About_Click()
            On Local Error GoTo ErrLine
            Call frmDlg.Show(1, Me)
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuHelp_Topics_Click
            ' * procedure description:  Occurs when the "Topics" option is invoked from the "Help" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuHelp_Topics_Click()
            On Local Error GoTo ErrLine
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuPlay_Click
            ' * procedure description:  Occurs when the "Play" option is invoked from the "Play" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuPlay_Click()
            On Local Error GoTo ErrLine
            
            'dynamically enable/disable
            'application menus based on state
            Select Case AppGetState
               Case AppStateClosed
                  mnuPlay_Stop.Enabled = False
                  mnuPlay_PlayPause.Enabled = False
               Case AppStatePlaying
                  mnuPlay_Stop.Enabled = True
                  mnuPlay_PlayPause.Enabled = True
               Case AppStatePaused
                  mnuPlay_Stop.Enabled = True
                  mnuPlay_PlayPause.Enabled = True
               Case AppStateStopped
                  mnuPlay_Stop.Enabled = False
                  mnuPlay_PlayPause.Enabled = True
            End Select
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuPlay_PlayPause_Click
            ' * procedure description:  Occurs when the "Pause" option is invoked from the "Play" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuPlay_PlayPause_Click()
            On Local Error GoTo ErrLine
            
            'play/pause application
            Select Case AppGetState
               Case AppStatePlaying
                   Call tbControlBar_ButtonClick(tbControlBar.Buttons("pause"))
               Case AppStatePaused
                  Call tbControlBar_ButtonClick(tbControlBar.Buttons("play"))
               Case AppStateStopped
                  Call tbControlBar_ButtonClick(tbControlBar.Buttons("play"))
            End Select
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuPlay_Stop_Click
            ' * procedure description:  Occurs when the "Stop" option is invoked from the "Play" option on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuPlay_Stop_Click()
            On Local Error GoTo ErrLine
            
            'stop application
            If AppGetState = AppStatePlaying Then
               Call tbControlBar_ButtonClick(tbControlBar.Buttons("stop"))
            ElseIf AppGetState = AppStatePaused Then
               Call tbControlBar_ButtonClick(tbControlBar.Buttons("stop"))
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuPlay_Volume_Click
            ' * procedure description:  Occurs when the "Volume" submenu is invoked from the "Play" menu on the main menubar.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuPlay_Volume_Click()
            On Local Error GoTo ErrLine
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuPlay_Volume_Down_Click
            ' * procedure description:  Occurs when the "Down" option is invoked from the "Volume" submenu on the "Play" menu.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuPlay_Volume_Down_Click()
            On Local Error GoTo ErrLine
            
            'adjust volume down
            If slVolume.Value > slVolume.Min Then
               slVolume.Value = slVolume.Value - slVolume.LargeChange
            Else: slVolume.Value = slVolume.Min
            End If
            Call slVolume_Scroll
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuPlay_Volume_Mute_Click
            ' * procedure description:  Occurs when the "Mute" option is invoked from the "Volume" submenu on the "Play" menu.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuPlay_Volume_Mute_Click()
            Static boolMuted As Boolean
            On Local Error GoTo ErrLine
            
            If Not boolMuted Then
               If Not m_objBasicAudio Is Nothing Then
                  boolMuted = True: m_objBasicAudio.Volume = -5000
               End If
            Else
               If Not m_objBasicAudio Is Nothing Then
                  boolMuted = False: m_objBasicAudio.Volume = slVolume.Value
               End If
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: mnuPlay_Volume_Up_Click
            ' * procedure description:  Occurs when the "Up" option is invoked from the "Volume" submenu on the "Play" menu.
            ' *
            ' ******************************************************************************************************************************
            Private Sub mnuPlay_Volume_Up_Click()
            On Local Error GoTo ErrLine
            
            'adjust volume up
            If slVolume.Value < slVolume.Max Then
               slVolume.Value = slVolume.Value + slVolume.LargeChange
            Else: slVolume.Value = slVolume.Max
            End If
            Call slVolume_Scroll
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub




' **************************************************************************************************************************************
' * PRIVATE INTERFACE- CONTROL EVENT HANDLERS
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedMovieWindow_Click
            ' * procedure description:  Occurs when the movie window is clicked.
            ' *
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedMovieWindow_Click()
            On Local Error GoTo ErrLine
            
            'allow pause/run via click
            'note that the video window will
            'get the mouse message; this is
            'by design and unavoidable
            Select Case AppGetState
               Case AppStatePlaying
                  Call tbControlBar_ButtonClick(tbControlBar.Buttons("pause"))
               Case AppStatePaused
                  Call tbControlBar_ButtonClick(tbControlBar.Buttons("play"))
               Case AppStateStopped
                  Call tbControlBar_ButtonClick(tbControlBar.Buttons("play"))
            End Select
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub


            ' ******************************************************************************************************************************
            ' * procedure name: picAVAlignedMovieWindow_Paint
            ' * procedure description:  Occurs when any part of a form or PictureBox control is moved, enlarged, or exposed.
            ' *
            ' ******************************************************************************************************************************
            Private Sub picAVAlignedMovieWindow_Paint()
            Dim objPicture As Object
            Dim sngInternalTop As Single
            Dim sngInternalLeft As Single
            Dim sngInternalWidth As Single
            Dim sngInternalHeight As Single
            Dim sngNewVideoTop As Single
            Dim sngNewVideoLeft As Single
            Dim sngNewVideoWidth As Single
            Dim sngNewVideoHeight As Single
            Dim sngCurrentAspectRatio As Single
            On Local Error GoTo ErrLine
            
            
            'the paint event of the movie window picturebox
            'is the perfect place to take care of any poster frame(s)
            'that we may wish to paint (such as company logo's)
            'of course, only do the manual paint when the video is
            'not playing back or is otherwise in use..
            If m_objVideoWindow Is Nothing Then
            
               'obtain internal rect for display
               sngInternalTop = picAVAlignedMovieWindow.Top
               sngInternalLeft = picAVAlignedMovieWindow.Left
               sngInternalWidth = picAVAlignedMovieWindow.Width
               sngInternalHeight = picAVAlignedMovieWindow.Height
               
               'derive position for the poster frame
               sngNewVideoWidth = ((sngInternalHeight * m_sngPreferredVideoWidth) / m_sngPreferredVideoHeight)
               'validate dimentions of poster frame
               If sngInternalWidth < sngNewVideoWidth Then
                   'width is the limiting factor
                   sngNewVideoHeight = ((sngInternalWidth * m_sngPreferredVideoHeight) / m_sngPreferredVideoWidth)
                   sngNewVideoWidth = sngInternalWidth
                Else
                   'height is the limiting factor
                   sngNewVideoHeight = sngInternalHeight
                End If
                
               'assign position to the poster frame
               'these are arbitrary numbers, and pretty much
               'will vary dependent on the size and aspect ratio
               'of the bitmap you pulled out from your resource file..
               sngNewVideoWidth = sngNewVideoWidth / 2.2
               sngNewVideoHeight = sngNewVideoHeight / 1.8
               If sngNewVideoWidth > 2350 Then sngNewVideoWidth = 2350
               If sngNewVideoHeight > 1800 Then sngNewVideoHeight = 1800
               sngNewVideoTop = (sngInternalHeight - sngNewVideoHeight) / 2
               sngNewVideoLeft = (sngInternalWidth - sngNewVideoWidth) / 2
               
               'load the poster frame from resource(s)
               'included here we have the Microsoft wmp logo
               Set objPicture = LoadResPicture(101, 0)
               'assign new values to the poster frame
               If Not objPicture Is Nothing Then
                  Call picAVAlignedMovieWindow.Cls
                  Call picAVAlignedMovieWindow.PaintPicture(objPicture, sngNewVideoLeft, sngNewVideoTop, sngNewVideoWidth, sngNewVideoHeight)
               End If
               'clean-up & dereference
               If Not objPicture Is Nothing Then Set objPicture = Nothing
            End If
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
            
            'Exact playback control will vary across players,
            'but the behavior presented here is pretty standard.
            'Handle Play/Pause/Stop operations and map the
            'changes to the ui for the user.
            
            'Obtain the current playback state
            'using IMediaControl by calling GetState using
            'a 100 millisecond timeout.
            If Not m_objMediaControl Is Nothing Then
               Call m_objMediaControl.GetState(100, nState)
            End If
            
            Select Case Button.Key
                Case "play"
                         If Not m_objMediaControl Is Nothing Then
                            Select Case nState
                               Case 0 'State Stopped
                                    'run the file from the start position
                                    If Not m_objMediaPosition Is Nothing Then _
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
                         If Not m_objMediaControl Is Nothing Then
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
                         If Not m_objMediaControl Is Nothing Then
                            Select Case nState
                               Case 0 'State Stopped
                                    'exit out of the procedure, this would not happen
                                    Exit Sub
                               
                               Case 1 'State Paused
                                    If Not m_objMediaPosition Is Nothing Then _
                                       m_objMediaPosition.CurrentPosition = 0
                                       Call m_objMediaControl.Stop
                               
                               Case 2 'State Playing
                                    If Not m_objMediaPosition Is Nothing Then _
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
            ' * procedure name: tbNavigation_ButtonClick
            ' * procedure description:  Occurs when the user clicks on a Button object in a Toolbar control.
            ' *
            ' ******************************************************************************************************************************
            Private Sub tbNavigation_ButtonClick(ByVal Button As MSComctlLib.Button)
            On Local Error GoTo ErrLine
            
            'if the index has not yet been set,
            'then the current index is the currently
            'playing file that's loaded into the application
            If m_HistoricalIndex = 0 Then _
               m_HistoricalIndex = UBound(m_bstrHistory)
               
            Select Case Button.Key
               Case "goforward"
                  If m_HistoricalIndex <= UBound(m_bstrHistory) Then
                     m_HistoricalIndex = m_HistoricalIndex + 1
                     Call AppPlayFile(m_bstrHistory(m_HistoricalIndex), False)
                     If m_HistoricalIndex = UBound(m_bstrHistory) Then
                        Button.Enabled = False
                        tbNavigation.Buttons("gobackward").Enabled = True 'fix
                     Else
                        Button.Enabled = True
                        tbNavigation.Buttons("gobackward").Enabled = True
                     End If
                  End If
                  
               Case "gobackward"
                  If m_HistoricalIndex >= 2 Then
                     m_HistoricalIndex = m_HistoricalIndex - 1
                     Call AppPlayFile(m_bstrHistory(m_HistoricalIndex), False)
                  End If
                  If UBound(m_bstrHistory) >= 2 Then _
                     tbNavigation.Buttons("goforward").Enabled = True
                  If m_HistoricalIndex = 1 Then Button.Enabled = False
            End Select
            Exit Sub
            
ErrLine:
            Err.Clear
            Resume Next
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
            
            'create an instance of internet explorer, and navigate to the site
            Set objExplorer = CreateObject("InternetExplorer.Application")
            If Not objExplorer Is Nothing Then
               objExplorer.Visible = True
               Call objExplorer.Navigate("http://windowsmedia.com/mediaguide/default.asp")
            End If
            
            'clean-up & dereference
            If Not objExplorer Is Nothing Then Set objExplorer = Nothing
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
            
            'create an instance of internet explorer, and navigate to the site
            Set objExplorer = CreateObject("InternetExplorer.Application")
            If Not objExplorer Is Nothing Then
               objExplorer.Visible = True
               Call objExplorer.Navigate("http://windowsmedia.com/mediaguide/default.asp?page=1")
            End If
            
            'clean-up & dereference
            If Not objExplorer Is Nothing Then Set objExplorer = Nothing
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
            
            'create an instance of internet explorer, and navigate to the site
            Set objExplorer = CreateObject("InternetExplorer.Application")
            If Not objExplorer Is Nothing Then
               objExplorer.Visible = True
               Call objExplorer.Navigate("http://windowsmedia.com/radiotuner/default.asp")
            End If
            
            'clean-up & dereference
            If Not objExplorer Is Nothing Then Set objExplorer = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slBalance_Scroll
            ' * procedure description:   Indicates that the slider has been scrolled manually by the user.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slBalance_Scroll()
            On Local Error GoTo ErrLine
            
            'Set the balance using the slider;
            'Here we use IBasicAudio to assign
            'the balance for playback.
            If Not m_objMediaControl Is Nothing Then _
               m_objBasicAudio.Balance = slBalance.Value
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slVolume_Scroll
            ' * procedure description:   Indicates that the slider has been scrolled manually by the user.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slVolume_Scroll()
            On Local Error GoTo ErrLine
            
            'Set the volume using the slider;
            'Here we use IBasicAudio to assign
            'the volume for playback.
            If Not m_objMediaControl Is Nothing Then _
               m_objBasicAudio.Volume = slVolume.Value
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slCurrentPosition_Scroll
            ' * procedure description:   Indicates that the slider has been scrolled manually by the user.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slCurrentPosition_Scroll()
            Dim nVoume As Long
            On Local Error GoTo ErrLine
            
            'Set the volume using the slider;
            'Here we use IMediaControl to assign
            'the current position, in turn allowing for
            'basic 'scrubbing' though the video stream.
            If m_boolLoaded Then
               If Not m_objMediaControl Is Nothing Then
                  If Not m_objMediaPosition Is Nothing Then
                     'if the stream has audio, then mute the
                     'stream before attempting to change the
                     'current position
                     If Not m_objBasicAudio Is Nothing Then
                        nVoume = m_objBasicAudio.Volume
                        m_objBasicAudio.Volume = -5000
                     End If
                     'change the current position
                     m_objMediaPosition.CurrentPosition = slCurrentPosition.Value
                     'if the stream has audio, then mute the,
                     'then change the volume back to normal
                     If Not m_objBasicAudio Is Nothing Then
                        m_objBasicAudio.Volume = nVoume
                     End If
                  End If
               End If
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slCurrentPosition_MouseDown
            ' * procedure description:    Occurs when the user presses the mouse button while an object has the focus.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slCurrentPosition_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            
            'pause the application during scrubbing
            Call tbControlBar_ButtonClick(tbControlBar.Buttons("pause"))
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: slCurrentPosition_MouseUp
            ' * procedure description:   Occurs when the user releases the mouse button while an object has the focus.
            ' *
            ' ******************************************************************************************************************************
            Private Sub slCurrentPosition_MouseUp(Button As Integer, Shift As Integer, X As Single, Y As Single)
            Dim sngPercent As Single
            On Local Error GoTo ErrLine
            
            'position the slider manualy on 'clicks'
            If slCurrentPosition.Width > 0 Then
               sngPercent = X / slCurrentPosition.Width
               slCurrentPosition.Value = slCurrentPosition.Max * sngPercent
               Call slCurrentPosition_Scroll
            End If
            'run the application after scrubbing
            Call tbControlBar_ButtonClick(tbControlBar.Buttons("play"))
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: tmrTimer_Timer
            ' * procedure description:    Occurs when a preset interval for a Timer control has elapsed.
            ' *                                         The default presented in this module is 250 milliseconds or 1/4 second.
            ' *                                         Again, by default the timer is only enabled when the application is loaded.
            ' ******************************************************************************************************************************
            Private Sub tmrTimer_Timer()
            Dim nState As Long
            Dim nResultant As Long
            Dim nMediaPosition As Long
            Dim nMediaDuration As Long
            Dim bstrFormattedTimes As String
            On Local Error GoTo ErrLine
            
            'only when loaded
            If m_boolLoaded Then
            
               'obtain the current playback state
               If Not m_objMediaControl Is Nothing Then _
                  Call m_objMediaControl.GetState(100, nState)
                  
               'obtain & format the current position and duration
               nMediaDuration = CLng(m_objMediaPosition.Duration)
               nMediaPosition = CLng(m_objMediaPosition.CurrentPosition)
               
               'map the currrent position and duration to the ui's
               'various control(s) on the screen including the slider,
               'the status area, and the tooltip text of various control(s)
               If Not nMediaDuration > 0 Then
                  'if the position window is available, assign to it
                  If picAVAlignedPositionWindow.Visible Then
                     If slCurrentPosition.Min <> 0 Then slCurrentPosition.Min = 0
                     If slCurrentPosition.Max <> 0 Then _
                        slCurrentPosition.Max = 0
                     If slCurrentPosition.Value <> 0 Then _
                        slCurrentPosition.Value = 0
                     If slCurrentPosition.SelStart <> 0 Then slCurrentPosition.SelStart = 0
                     If slCurrentPosition.SelLength <> 0 Then _
                        slCurrentPosition.SelLength = 0
                     If slCurrentPosition.ToolTipText <> vbNullString Then _
                        slCurrentPosition.ToolTipText = vbNullString
                  End If
                  'reset time caption as well
                  If lblCurrentTime.Caption <> vbNullString Then _
                      lblCurrentTime.Caption = vbNullString
                  
               Else
                  'additionally assign playback time(s)
                  bstrFormattedTimes = AppFormatSeconds(nMediaPosition) _
                                                      & "/" & AppFormatSeconds(nMediaDuration)
                  If lblCurrentTime.Caption <> bstrFormattedTimes Then _
                     lblCurrentTime.Caption = bstrFormattedTimes
                  'if the position window is available, assign to it
                  If picAVAlignedPositionWindow.Visible Then
                     If slCurrentPosition.Min <> 0 Then slCurrentPosition.Min = 0
                     If slCurrentPosition.Max <> CLng(m_objMediaPosition.Duration) Then _
                        slCurrentPosition.Max = CLng(m_objMediaPosition.Duration)
                     If slCurrentPosition.Value <> CLng(m_objMediaPosition.CurrentPosition) Then _
                        slCurrentPosition.Value = CLng(m_objMediaPosition.CurrentPosition)
                     If slCurrentPosition.SelStart <> 0 Then slCurrentPosition.SelStart = 0
                     If slCurrentPosition.SelLength <> slCurrentPosition.Value Then _
                        slCurrentPosition.SelLength = slCurrentPosition.Value
                     If slCurrentPosition.ToolTipText <> bstrFormattedTimes Then _
                        slCurrentPosition.ToolTipText = bstrFormattedTimes
                  End If
               End If
                  
               'query for media completion state
               If Not m_objMediaEvent Is Nothing Then
                  Call m_objMediaEvent.WaitForCompletion(10, nResultant) 'throws error
                  If nResultant > 0 Then Call m_objMediaControl.Stop 'EC_COMPLETE
               End If
   
               If nState = 0 Then 'stopped
                  If nMediaDuration > 0 Then
                     If lblState.Caption <> "Stopped" Then _
                        lblState.Caption = "Stopped"
                     'disable the stop button
                     slCurrentPosition.Enabled = True
                     tbControlBar.Buttons("play").Enabled = True
                     tbControlBar.Buttons("stop").Enabled = False
                     tbControlBar.Buttons("pause").Enabled = False
                  Else
                     If lblState.Caption <> vbNullString Then _
                        lblState.Caption = vbNullString
                     slCurrentPosition.Enabled = False
                     tbControlBar.Buttons("play").Enabled = False
                     tbControlBar.Buttons("stop").Enabled = False
                     tbControlBar.Buttons("pause").Enabled = False
                  End If
                  
                  
               ElseIf nState = 1 Then
                  If nMediaDuration > 0 Then
                     If lblState.Caption <> "Paused" Then _
                        lblState.Caption = "Paused"
                     'disable the pause button
                     slCurrentPosition.Enabled = True
                     tbControlBar.Buttons("play").Enabled = True
                     tbControlBar.Buttons("stop").Enabled = True
                     tbControlBar.Buttons("pause").Enabled = False
                  Else
                     If lblState.Caption <> vbNullString Then _
                        lblState.Caption = vbNullString
                     slCurrentPosition.Enabled = False
                     tbControlBar.Buttons("play").Enabled = False
                     tbControlBar.Buttons("stop").Enabled = False
                     tbControlBar.Buttons("pause").Enabled = False
                  End If
                  
               ElseIf nState = 2 Then
                  If nMediaDuration > 0 Then
                     If lblState.Caption <> "Playing" Then _
                        lblState.Caption = "Playing"
                     'disable the play button
                     slCurrentPosition.Enabled = True
                     tbControlBar.Buttons("play").Enabled = False
                     tbControlBar.Buttons("stop").Enabled = True
                     tbControlBar.Buttons("pause").Enabled = True
                  Else
                     If lblState.Caption <> vbNullString Then _
                        lblState.Caption = vbNullString
                     slCurrentPosition.Enabled = False
                     tbControlBar.Buttons("play").Enabled = False
                     tbControlBar.Buttons("stop").Enabled = False
                     tbControlBar.Buttons("pause").Enabled = False
                  End If
               End If
            Else: tmrTimer.Enabled = False 'turn self off
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Resume Next
            Exit Sub
            End Sub




' **************************************************************************************************************************************
' * PRIVATE INTERFACE- APPLICATION PROCEDURES
' *
' *
            ' ******************************************************************************************************************************
            ' * procedure name: AppChangeView
            ' * procedure description:  Changes the view of the application for the client.
            ' *                                       Allows one to change the view of the application; and post an optional resize.
            ' *                                       Simply pass in the module level counter parts to preserve the previous settings
            ' *                                       For a given value (e.g. Call AppChangeView( m_boolViewDisplay, ...... etc....... )
            ' ******************************************************************************************************************************
            Private Sub AppChangeView(Optional ShowMovieWindow As Boolean = True, Optional ShowLinkBar As Boolean = True, Optional ShowControlbar As Boolean = True, Optional ShowStatusBar As Boolean = True, Optional ShowPositionBar As Boolean = True, Optional AutoResize As Boolean = True)
            Dim ctrl As Control
            On Local Error GoTo ErrLine
            
            'assign argument(s) over to the actual control(s) themselves
            picAVAlignedLinkWindow.Visible = ShowLinkBar: picAVAlignedMovieWindow.Visible = ShowMovieWindow: picAVAlignedControlWindow.Visible = ShowControlbar: picAVAlignedStatusWindow.Visible = ShowStatusBar: picAVAlignedPositionWindow.Visible = ShowPositionBar
            
            'reassign menu 'checked' state
            If ShowStatusBar Then
               mnuViewStatusbar.Checked = True
            Else: mnuViewStatusbar.Checked = False
            End If
            'reassign menu 'checked' state
            If ShowControlbar Then
               mnuViewControlbar.Checked = True
            Else: mnuViewControlbar.Checked = False
            End If
            'reassign menu 'checked' state
            If ShowLinkBar Then
               mnuViewLinks.Checked = True
            Else: mnuViewLinks.Checked = False
            End If
            'reassign menu 'checked' state
            If ShowMovieWindow Then
               mnuViewDisplay.Checked = True
            Else: mnuViewDisplay.Checked = False
            End If
            'reassign menu 'checked' state
            If ShowPositionBar Then
               mnuViewPositionBar.Checked = True
            Else: mnuViewPositionBar.Checked = False
            End If
            'reassign module-level variables to reflect update
            m_boolViewLinks = ShowLinkBar
            m_boolViewStatusBar = ShowStatusBar
            m_boolViewControlBar = ShowControlbar
            m_boolViewDisplay = ShowMovieWindow
            m_boolViewPositionBar = ShowPositionBar
            
            
            'enumerate each control on the form;
            'and locate the alignable picturebox where
            'the tag indicates that the desired position is first (from the top)
            For Each ctrl In Me.Controls
               If TypeName(ctrl) = "PictureBox" Then
                  If ctrl.Container.Name = frmMain.Name Then
                     If ctrl.Visible Then
                        'obtain tag
                        If ctrl.Tag = "4" Then
                           ctrl.Top = -10000: Exit For
                        End If
                     End If
                  End If
               End If
            Next
            
            
            'enumerate each control on the form;
            'and locate the alignable picturebox where
            'the tag indicates that the desired position is second (from the top)
            For Each ctrl In Me.Controls
               If TypeName(ctrl) = "PictureBox" Then
                  If ctrl.Container.Name = frmMain.Name Then
                     If ctrl.Visible Then
                        'obtain tag
                        If ctrl.Tag = "3" Then
                           ctrl.Top = -10000: Exit For
                        End If
                     End If
                  End If
               End If
            Next
            
            
            'enumerate each control on the form;
            'and locate the alignable picturebox where
            'the tag indicates that the desired position is third (from the top)
            For Each ctrl In Me.Controls
               If TypeName(ctrl) = "PictureBox" Then
                  If ctrl.Container.Name = frmMain.Name Then
                     If ctrl.Visible Then
                        'obtain tag
                        If ctrl.Tag = "2" Then
                           ctrl.Top = -10000: Exit For
                        End If
                     End If
                  End If
               End If
            Next
            
            
            'enumerate each control on the form;
            'and locate the alignable picturebox where
            'the tag indicates that the desired position is forth (from the top)
            For Each ctrl In Me.Controls
               If TypeName(ctrl) = "PictureBox" Then
                  If ctrl.Container.Name = frmMain.Name Then
                     If ctrl.Visible Then
                        'obtain tag
                        If ctrl.Tag = "1" Then
                           ctrl.Top = -10000: Exit For
                        End If
                     End If
                  End If
               End If
            Next
            
            
            'enumerate each control on the form;
            'and locate the alignable picturebox where
            'the tag indicates that the desired position is fifth (from the top)
            For Each ctrl In Me.Controls
               If TypeName(ctrl) = "PictureBox" Then
                  If ctrl.Container.Name = frmMain.Name Then
                     If ctrl.Visible Then
                        'obtain tag
                        If ctrl.Tag = "0" Then
                           ctrl.Top = -10000: Exit For
                        End If
                     End If
                  End If
               End If
            Next
            
            
            'finally, force a resize of the window
            'thus 'snapping' the bottom of the form
            'to the last visible alignable picturebox
            If frmMain.WindowState = vbNormal Then
               If picAVAlignedStatusWindow.Visible Then
                  frmMain.Height = (picAVAlignedStatusWindow.Top + picAVAlignedStatusWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
               ElseIf picAVAlignedControlWindow.Visible Then
                  frmMain.Height = (picAVAlignedControlWindow.Top + picAVAlignedControlWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
               ElseIf picAVAlignedPositionWindow.Visible Then
                  frmMain.Height = (picAVAlignedPositionWindow.Top + picAVAlignedPositionWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
               ElseIf picAVAlignedMovieWindow.Visible Then
                  frmMain.Height = (picAVAlignedMovieWindow.Top + picAVAlignedMovieWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
               ElseIf picAVAlignedLinkWindow.Visible Then
                  frmMain.Height = (picAVAlignedLinkWindow.Top + picAVAlignedLinkWindow.Height) + (frmMain.Height - frmMain.ScaleHeight)
               Else: frmMain.Height = (frmMain.Height - frmMain.ScaleHeight)
               End If

            End If
            
            'resize the form and force a subsequent refresh
            'of the window.. given the passed optional parameter
            If AutoResize Then Call Form_Resize
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppShowError
            ' * procedure description:  Displays an error similar to the media player error dialog.
            ' *                                       Pass in an error number, brief description, and advanced description.
            ' *                                       The error number evaluates out to the last DllError (not the vb automation errors)
            ' ******************************************************************************************************************************
            Private Sub AppShowError(Number As Long, bstrErrorDescription As String, Optional bstrAdvancedDescription As String)
            On Local Error GoTo ErrLine
            
            'load the error dialog;
            'and display it modal to the app
            Load frmHelp
            frmHelp.ShowError Number, bstrErrorDescription, bstrAdvancedDescription
            frmHelp.Show
            
            'disable application &
            'yield control to the dialog
            frmMain.Enabled = False
            Call AppStayOnTop(frmHelp.hWnd)
            
            'wait until the user has cleared the error;
            'basically were waiting on the form to become
            'invisible at this point so we can unload it.
            Do Until frmHelp.Visible = False: DoEvents
            Loop
            
            'restore application
            frmMain.Enabled = True
            Call AppNotOnTop(frmHelp.hWnd)
            
            'unload dialog, clean-up & dereference
            frmHelp.Move 0, 0, Screen.Width * 8, Screen.Height * 8
            frmHelp.Visible = False: Unload frmHelp: Set frmHelp = Nothing
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppAppendUserHistory
            ' * procedure description:  Append the currently opened file to the user's history list (used for go back/forward buttons)
            ' *
            ' ******************************************************************************************************************************
            Public Sub AppAppendUserHistory(bstrFilePath As String)
            On Local Error GoTo ErrLine
            
            'always reset the historical data
            'looking forward when a user appends
            'a new file to the list; and disable the button
            m_HistoricalIndex = 0
            tbNavigation.Buttons("goforward").Enabled = False
            
            'if the file has not yet been appended to the history list,
            'proceed to redimention the string array and append it now..
            If m_bstrHistory(UBound(m_bstrHistory)) <> LCase(Trim(bstrFilePath)) Then
               ReDim Preserve m_bstrHistory(UBound(m_bstrHistory) + 1)
               m_bstrHistory(UBound(m_bstrHistory)) = LCase(Trim(bstrFilePath))
               'update the ui as well
               If UBound(m_bstrHistory) >= 2 Then
                  tbNavigation.Buttons("goforward").Enabled = False
                  tbNavigation.Buttons("gobackward").Enabled = True
               End If
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Resume Next
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppPlayFile
            ' * procedure description:  Renders the given file for the user within the context of the application.
            ' *                                       This is the heart of the application's functionality.  This uses Quartz to
            ' *                                       link all the filters together that you need to render (playback) the audio/video stream.
            ' ******************************************************************************************************************************
            Friend Sub AppPlayFile(bstrFileName As String, Optional AppendHistory As Boolean = True)
            On Local Error GoTo ErrLine
            
            'verify file exists, this only applies to
            'local or remote network shared unc paths
            If Not InStr(1, bstrFileName, "://") > 0 Then
               If Not AppVerifyFileExists(bstrFileName) Then
                  Call AppShowError(80040216, "Cannot open.  Please verify that the path and filename are correct and try again.")
                  Exit Sub
               End If
            End If
            
            'disable timer, there is no
            'need to have it running at this point.
            tmrTimer.Enabled = False
            frmMain.lblState.Caption = "Opening.."
            
            'stop video playback if applicable
            'again, don't forget to assign the video
            'window's owner to null.
            m_boolLoaded = False
            If Not m_objMediaControl Is Nothing Then
               Call m_objMediaControl.Stop
               If Not m_objVideoWindow Is Nothing Then
                  m_objVideoWindow.Visible = False
                  m_objVideoWindow.Owner = 0 'Assign the Owner to NULL
               End If
            End If
            
            'clean-up & dereference module-level data
            If Not m_objBasicAudio Is Nothing Then Set m_objBasicAudio = Nothing
            If Not m_objBasicVideo Is Nothing Then Set m_objBasicVideo = Nothing
            If Not m_objMediaEvent Is Nothing Then Set m_objMediaEvent = Nothing
            If Not m_objMediaControl Is Nothing Then Set m_objMediaControl = Nothing
            If Not m_objVideoWindow Is Nothing Then Set m_objVideoWindow = Nothing
            If Not m_objMediaPosition Is Nothing Then Set m_objMediaPosition = Nothing
            
            'attempt to render the elected file
            'begin by reinitalizing the filter graph manager
            Set m_objMediaControl = New FilgraphManager
            
            'calling render file takes care of all the hard work
            'for you- allowing you to get down to the business
            'of simply playing the media file back without having
            'to do anything difficult with filters or Direct Media Objects.
            Call m_objMediaControl.RenderFile(bstrFileName)
            
            'if the optional was passed then additionally append this
            'file operation to the user's historical list to aid in navigation
            If AppendHistory Then Call AppAppendUserHistory(bstrFileName)
            
            'if you didn't get an error after the call to renderfile- then your almost in the clear.
            'Throw the filename on to the caption bar and get the associated file icon for the ui
            If InStrRev(bstrFileName, "\") > 0 Then
               lblClipValue.Caption = Mid(bstrFileName, InStrRev(bstrFileName, "\") + 1, Len(bstrFileName))
               frmMain.Caption = App.Title & " - " & Mid(bstrFileName, InStrRev(bstrFileName, "\") + 1, Len(bstrFileName))
            ElseIf InStrRev(bstrFileName, "/") > 0 Then
               lblClipValue.Caption = Mid(bstrFileName, InStrRev(bstrFileName, "/") + 1, Len(bstrFileName))
               frmMain.Caption = App.Title & " - " & Mid(bstrFileName, InStrRev(bstrFileName, "/") + 1, Len(bstrFileName))
            End If
            'just like media player, grab the icon for the associated file type using shell32 api's and map it to the ui
            If Not AppGetFileIcon(bstrFileName) Is Nothing Then Set imgFileFormatIcon = AppGetFileIcon(bstrFileName)
            
            'Now, we need to figure out of the video stream has audio, video, or both
            'so we know how to proceed.  In all cases were going to try to get pointers to the following
            'interfaces:  IMediaEvent, IMediaPosition, and IMediaControl.  If the file has any video streams,
            'were also going to need to get the IVideoWindow interface so we can see the video.  If the file
            'has any audio- IBasicAudio is another good one to grab for future use.
            
            'determine if the content is audio or video only or both..
            'see these two functions included with the sample to see how..
            If HasAudio(m_objMediaControl) = True And HasVideo(m_objMediaControl) = True Then
            
               'change the view of the form;
               'so we look cool like media player  =)
               Call AppChangeView( _
                      True, m_boolViewLinks, m_boolViewControlBar, _
                               m_boolViewStatusBar, m_boolViewPositionBar, True)
               
               'derive the audio interface
               Set m_objBasicAudio = m_objMediaControl
               If Not m_objBasicAudio Is Nothing Then
                  m_objBasicAudio.Volume = slVolume.Value
                  m_objBasicAudio.Balance = slBalance.Value
               End If
               
               'derive the video window interface
               Set m_objVideoWindow = m_objMediaControl
               
               If Not m_objVideoWindow Is Nothing Then
                  'set visibility to zero for a moment..
                  m_objVideoWindow.Visible = False
                  'set window style
                  m_objVideoWindow.WindowStyle = CLng(&H6000000)
                  'set window owner
                  m_objVideoWindow.Owner = picAVAlignedMovieWindow.hWnd
                  'set the video windows position
                  m_objVideoWindow.Top = CLng(picAVAlignedMovieWindow.Top)
                  m_objVideoWindow.Left = CLng(picAVAlignedMovieWindow.Left)
                  m_objVideoWindow.Width = CLng(picAVAlignedMovieWindow.Width / Screen.TwipsPerPixelX)
                  m_objVideoWindow.Height = CLng(picAVAlignedMovieWindow.Height / Screen.TwipsPerPixelY)
                  'set Preferred (default) video window size to module-level
                  m_sngPreferredVideoWidth = CLng(m_objVideoWindow.Width * Screen.TwipsPerPixelX)
                  m_sngPreferredVideoHeight = CLng(m_objVideoWindow.Height * Screen.TwipsPerPixelY)
                  'set visible
                  m_objVideoWindow.Visible = True
                  'force a resize
                  Call Form_Resize
            End If
            
            
            
            ElseIf HasVideo(m_objMediaControl) = True Then
            
               'change the view of the form;
               'so we look cool like media player  =)
               Call AppChangeView(True, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar, True)
               
               'derive the video window interface
               Set m_objVideoWindow = m_objMediaControl
               
               If Not m_objVideoWindow Is Nothing Then
                  'set visibility to zero for a moment..
                  m_objVideoWindow.Visible = False
                  'set window style
                  m_objVideoWindow.WindowStyle = CLng(&H6000000)
                  'set window owner
                  m_objVideoWindow.Owner = picAVAlignedMovieWindow.hWnd
                  'set the video windows position
                  m_objVideoWindow.Top = CLng(picAVAlignedMovieWindow.Top)
                  m_objVideoWindow.Left = CLng(picAVAlignedMovieWindow.Left)
                  m_objVideoWindow.Width = CLng(picAVAlignedMovieWindow.Width / Screen.TwipsPerPixelX)
                  m_objVideoWindow.Height = CLng(picAVAlignedMovieWindow.Height / Screen.TwipsPerPixelY)
                  'set Preferred (default) video window size to module-level
                  m_sngPreferredVideoWidth = CLng(m_objVideoWindow.Width * Screen.TwipsPerPixelX)
                  m_sngPreferredVideoHeight = CLng(m_objVideoWindow.Height * Screen.TwipsPerPixelY)
                  'set visible
                  m_objVideoWindow.Visible = True
                  'force a resize
                  Call Form_Resize
            End If
            
            
            ElseIf HasAudio(m_objMediaControl) = True Then
            
               'change the view of the form;
               'so we look cool like media player- note for
               'audio that you will probably want the view
               'to differ depending on the state of the form
               If Not Me.WindowState = vbMaximized Then
                  Call AppChangeView(False, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar, False)
               Else: Call AppChangeView(True, m_boolViewLinks, m_boolViewControlBar, m_boolViewStatusBar, m_boolViewPositionBar, True)
               End If
               
               'derive the audio interface
               Set m_objBasicAudio = m_objMediaControl
               
               If Not m_objBasicAudio Is Nothing Then
                  'assign the current slider value(s) over now
                  m_objBasicAudio.Volume = slVolume.Value
                  m_objBasicAudio.Balance = slBalance.Value
               End If
            Else
               Exit Sub
            End If
            
            
            
            If Not m_objMediaControl Is Nothing Then
               'Derive the IMediaEvent object for the
               'sample toolbar (run, pause, play).
               Set m_objMediaEvent = m_objMediaControl
               'Derive the IMediaPosition object so that we
               'can display the duration of the selected
               'video as well as the elapsed time.
               Set m_objMediaPosition = m_objMediaControl
               'run the clip for the user- this is what's we've been
               'waiting for- a simple call to Run and your off playing!!
               Call m_objMediaControl.Run
            End If
            
            'set the module-level flag to indicate
            'that the file has been loaded sucessfully
            'and be sure and enable the position bar and timer..
            m_boolLoaded = True
            tmrTimer.Enabled = True
            slCurrentPosition.Enabled = True
            Exit Sub
            
ErrLine:
            'wacky errors do occasionally occur, you can choose to display a basic error dialog- because
            'extended error information is not available using visual basic  (i.e. all thrown errors will be an automation error)
            Call AppShowError(Err.Number, "An unspecified or unknown error has occured.  ", Err.Description)
            Err.Clear
            Resume Next 'or you can try, try again..
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
            
            'here's a quick way to format the
            'the CurrentPosition into something
            'more useful- that can be shown to the user..
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
            'sample
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
            
            'query available device output(s) using winmm.dll
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
            ' * procedure name: AppSnapVideoWindow
            ' * procedure description:  Pass in the left, top, width, and height of the container- and the video window
            ' *                                       Will be automatically aligned to maintain the aspect ratio just like Media Player.
            ' ******************************************************************************************************************************
            Public Sub AppSnapVideoWindow(sngInternalLeft As Single, sngInternalTop As Single, sngInternalWidth As Single, sngInternalHeight As Single)
            Dim sngNewVideoTop As Single
            Dim sngNewVideoLeft As Single
            Dim sngNewVideoWidth As Single
            Dim sngNewVideoHeight As Single
            Dim sngCurrentAspectRatio As Single
            On Local Error GoTo ErrLine
            
            'only preform when video window is loaded
            If Not m_objVideoWindow Is Nothing Then
            
               'convert twips to pixels
               sngInternalLeft = sngInternalLeft / Screen.TwipsPerPixelX
               sngInternalTop = sngInternalTop / Screen.TwipsPerPixelY
               sngInternalWidth = sngInternalWidth / Screen.TwipsPerPixelX
               sngInternalHeight = sngInternalHeight / Screen.TwipsPerPixelY
               
               'derive position for the video window
               sngNewVideoWidth = (sngInternalHeight * m_sngPreferredVideoWidth) / m_sngPreferredVideoHeight
   
               'validate dimentions of video window
               If sngInternalWidth < sngNewVideoWidth Then
                   'width is the limiting factor
                   sngNewVideoHeight = (sngInternalWidth * m_sngPreferredVideoHeight) / m_sngPreferredVideoWidth
                   sngNewVideoWidth = sngInternalWidth
                Else
                   'height is the limiting factor
                   sngNewVideoHeight = sngInternalHeight
                End If
   
               'assign position to the video window
               sngNewVideoTop = ((sngInternalHeight - sngNewVideoHeight) / 2)
               sngNewVideoLeft = (sngInternalWidth - sngNewVideoWidth) / 2
               
               'assign new values to the video window
               If m_objVideoWindow.Left <> CLng(sngNewVideoLeft) Then _
                  m_objVideoWindow.Left = CLng(sngNewVideoLeft)
               If m_objVideoWindow.Top <> CLng(sngNewVideoTop) Then _
                  m_objVideoWindow.Top = CLng(sngNewVideoTop)
               If m_objVideoWindow.Width <> CLng(sngNewVideoWidth) Then _
                  m_objVideoWindow.Width = CLng(sngNewVideoWidth)
               If m_objVideoWindow.Height <> CLng(sngNewVideoHeight) Then _
                  m_objVideoWindow.Height = CLng(sngNewVideoHeight)
            End If
            Exit Sub
            
ErrLine:
            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppOLEDragDrop
            ' * procedure description:  Occurs when any file is dropped onto any control on the form that supports OLEDragDrop
            ' *
            ' ******************************************************************************************************************************
            Public Sub AppOLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
            On Local Error GoTo ErrLine
            
            'when a user drag's a file- don't be so quick
            'to assume it will show up in the Files collection.
            'Dropping a hyperlink from a webpage will show up
            'in vb6 as Format value 1..
            If Data.GetFormat(15) Then
               Call AppPlayFile(Data.Files(1)) 'play first unc file path
            ElseIf Data.GetFormat(1) Then
               Call AppPlayFile(CStr(Data.GetData(1))) 'play url from clipboard
            End If
            Exit Sub
            
ErrLine:

            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppOLEDragOver
            ' * procedure description:  Occurs when any file is dragged over and control on this form that supports OLEDragDrop
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
            bstrSupport = SUPPORTED_MEDIA
            varSupport = Split(bstrSupport, ";") 'split the supported files into an array..
            
            If Data.GetFormat(15) Then
               bstrFileName = CStr(Data.Files(1)) 'play first unc file path
            ElseIf Data.GetFormat(1) Then
               Call Data.Files.Add(CStr(Data.GetData(1))) 'play url from clipboard
            End If
            
            'enumerate the files in the collection
            'and compare there file extensions to
            'that of the supported file formats; thus
            'setting the drag cursor effect dynamically..
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
            
            'no go - reset effect
            Effect = vbDropEffectNone
            Exit Sub
            
ErrLine:

            Err.Clear
            Exit Sub
            End Sub
            
            
            ' ******************************************************************************************************************************
            ' * procedure name: AppGetState
            ' * procedure description:  Returns the current state of the application (e.g. playing, paused, stopped, etc..)
            ' *
            ' ******************************************************************************************************************************
            Friend Function AppGetState() As AppState
            Dim nState As Long
            On Local Error GoTo ErrLine
            
            If m_boolLoaded Then
               'obtain the current playback state
               If Not m_objMediaControl Is Nothing Then
                  Call m_objMediaControl.GetState(100, nState)
                  If nState = 0 Then 'stopped
                     AppGetState = AppStateStopped
                     Exit Function
                  ElseIf nState = 1 Then 'paused
                     AppGetState = AppStatePaused
                     Exit Function
                  ElseIf nState = 2 Then 'playing
                    AppGetState = AppStatePlaying
                    Exit Function
                  End If
               End If
            End If
            
            'otherwise return closed
            AppGetState = AppStateClosed
            Exit Function
            
ErrLine:

            Err.Clear
            Exit Function
            End Function
            
            
' **************************************************************************************************************************************
' * PRIVATE INTERFACE- HELPFUL QUARTZ WRAPPER PROCEDURES
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
            
            'query audio interface; attempt to
            'get the volume, if it's all good you
            'wont' throw and catch the error
            If Not objMediaControl Is Nothing Then
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
            
            'query video interface; attempt to
            'get the visibility, if it's all good you
            'wont' throw and catch the error
            If Not objMediaControl Is Nothing Then
              Set objVideo = objMediaControl
              boolVisible = objVideo.Visible
              HasVideo = True
            End If
            Exit Function
            
ErrLine:

            Err.Clear
            Exit Function
            End Function
