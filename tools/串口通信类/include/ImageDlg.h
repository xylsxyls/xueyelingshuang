#if !defined(AFX_IMAGEDLG_H__5A60840C_15F9_4D34_948C_11394F79A50F__INCLUDED_)
#define AFX_IMAGEDLG_H__5A60840C_15F9_4D34_948C_11394F79A50F__INCLUDED_

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the IMAGEDLG_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// IMAGEDLG_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#include "ImageBase.h"
#include <AFXTEMPL.H>
#include "CommTbl.h"

#define ID_BUTTON_BASE 1000
#define ID_EVENT_TIMEOUT 8888
#define ID_EVENT_TIME_SCROLL 8889


enum TEXT_ALIGN{ALIGN_LEFT=0, ALIGN_RIGHT=1, ALIGN_LEFT_SCROOL=2, ALIGN_TOP_SCROOL=3} ;

typedef struct IMAGE_INFO
{
	CImageBase*	pImageNormal;
	CImageBase*	pImgMouseDown;
	CRect	rc;
	UINT	uID;
	int		nStatus;
	BOOL	bIsShow;
	BOOL	bIgnoreBKColor;
	BOOL	bChangeImg;
	BOOL	bDelNormalImg;
	BOOL	bDelMouseDownImg;
	BOOL	bEnable;
	HWND	hParentNormal;
	HWND	hParentMouseDown;
	int		nProperty;
	enum{STATUS_NORMAL, STATUS_MOUSEON, STATUS_MOUSEDOWN, STATUS_MOUSEUP};
	enum{PROPERTY_IMAGE, PROPERTY_BUTTON, PROPERTY_CHECKBOX};
	IMAGE_INFO()
	{
		rc = CRect(0, 0, 0, 0);
		bIsShow = TRUE;
		bChangeImg = FALSE;
		bIgnoreBKColor = TRUE;
		uID = 0;
		nStatus = STATUS_NORMAL;
		pImageNormal = NULL;
		pImgMouseDown = NULL;
		bDelMouseDownImg = FALSE;
		bDelNormalImg = FALSE;
		bEnable = TRUE;
		hParentNormal = NULL;
		hParentMouseDown = NULL;
		nProperty = PROPERTY_IMAGE;
	}
	virtual ~IMAGE_INFO()
	{
		if (pImageNormal && bDelNormalImg)
		{
			delete pImageNormal;
		}
		else if (hParentNormal && pImageNormal)
		{
			pImageNormal->SetParentWnd(hParentNormal);
		}
		pImageNormal = NULL;
		if (pImgMouseDown && bDelMouseDownImg)
		{
			delete pImgMouseDown;
		}
		else if (hParentMouseDown && pImgMouseDown)
		{
			pImgMouseDown->SetParentWnd(hParentMouseDown);
		}
		pImgMouseDown = NULL;
	}
}*LPIMAGE_INFO;

typedef struct GLOBAL_IMAGE
{
	CString		strImageName;
	UINT		uImageID;
	CImageBase*	pImage;
	GLOBAL_IMAGE()
	{
		pImage = NULL;
		uImageID = -1;
		strImageName = "";
	}
	GLOBAL_IMAGE(const GLOBAL_IMAGE& GlobalImage)
	{
		pImage = GlobalImage.pImage;
		uImageID = GlobalImage.uImageID;
		strImageName = GlobalImage.strImageName;
	}
	void operator = (const GLOBAL_IMAGE& GlobalImage)
	{
		pImage = GlobalImage.pImage;
		uImageID = GlobalImage.uImageID;
		strImageName = GlobalImage.strImageName;
	}
	//virtual ~GLOBAL_IMAGE()
	//{
	//	if (pImage)
	//	{
	//		delete pImage;
	//		pImage = NULL;
	//	}
	//}
}*LPGLOBAL_IMAGE;

typedef struct CTL_ID_COLOR {
	CString		strText;
	int			iFont;
	CString		strFontName;
	int			iId;
    COLORREF	color;
    CRect		rc;
	BOOL		bShow;
	TEXT_ALIGN	iAlign;//对齐方式 0 左对齐 1右对齐 2左滚动
	UINT		nSpeed;// 滚动速度 0为静止 (像素/100毫秒)
	int		iScollPos;//当前滚动偏移量
	CTL_ID_COLOR()
	{
		strText="";
		iFont=0;
		iId=0;
		color=RGB(0,0,0);
		bShow=TRUE;
		iAlign=ALIGN_LEFT;
		nSpeed=0;
		iScollPos = 0;
	}
	static bool CompareID(CTL_ID_COLOR* pMask, CTL_ID_COLOR* pData)
	{
		if(pMask->iId == pData->iId) return true;
		return false;
	}
}*LPCTL_ID_COLOR;

typedef struct TIME_OUT_SET {
	UINT		uTimeOut;//超时时间 单位秒
	int			uCurTimeOut;//当前剩余超时时间 单位秒
	int			uStatus; // 状态 1运行 0停止 2暂停
	int			iX;
	int			iY;
	UINT		uFont;
	COLORREF    color;
	TIME_OUT_SET()
	{
		uTimeOut = 0;
		uCurTimeOut = 0;
		iX = -1;
		iY = -1;
		uFont = 32;
		color = RGB(0,0,0);
	}
}*LPTIME_OUT_SET;


BOOL LoadGlobalImage(LPCTSTR lpszImageName);//路径必须是全局路径
BOOL LoadGlobalImage(UINT uID);
void FreeGlobalImage();

// This class is exported from the ImageDlg.dll
class CImageDlg : public CDialog
{
public:
	CImageDlg(UINT uID, CWnd* pParent = NULL);   // standard constructor
    ~CImageDlg();
public:
	
public:
	//添加需要显示的图片
	BOOL	AddImage(int nPosX, int nPosY, LPCTSTR lpszImageName, UINT uID = 0, LPCTSTR lpszImgMouseDown = "");
	BOOL	InsertImage(int nIndex, int nPosX, int nPosY, LPCTSTR lpszImageName, UINT uID = 0, LPCTSTR lpszImgMouseDown = "");
	BOOL    AddImage(int nPosX, int nPosY, UINT uImageName, UINT uID=0, UINT uMouseDownImage=0);
	BOOL	InsertImage(int nIndex, int nPosX, int nPosY, UINT uImageName, UINT uID = 0, UINT uMouseDownImage = 0);
	BOOL	DrawImages(HDC hDC);
	BOOL	DrawTexts(HDC hDC);	
	BOOL	SetButtonStatus(UINT uID, int nStatus = IMAGE_INFO::STATUS_MOUSEDOWN);
	BOOL	SetImageProperty(UINT uID, int nType = IMAGE_INFO::PROPERTY_CHECKBOX);

	//此函数仅针对设置属性为PROPERTY_CHECKBOX的图片类型。
	BOOL	SetCheckBoxStatus(UINT uID, BOOL bChecked);
	BOOL	GetCheckBoxStatus(UINT uID, BOOL& bChecked);

	void    DeleteImage(UINT uID);
	void	RedrawImage(CImageBase* pImage);
	CString	GetLastError();
	void	ShowImage(UINT uID, BOOL bIsShow);
	void	SetShowHide(UINT uID, BOOL bIsShow);
	void	ShowText(UINT uID, BOOL bIsShow=TRUE);
	void    ChangeText(UINT uID, CString strText);
	void    UpdateStatic(UINT uID=0);
	void	OutSystemTime();
	BOOL	EnableButton(int nID, BOOL bEnable = TRUE);
	BOOL	FindButton(int nID);
    // 添加文本显示
    BOOL    AddText(int nPosX, int nPosY, LPCTSTR lpszText, int iFont, COLORREF color=RGB(0,0,0), UINT uID=0, TEXT_ALIGN iAlign=ALIGN_LEFT, BOOL bRepaint=TRUE);
	BOOL    AddText(int nPosX, int nPosY, LPCTSTR lpszText, int iFont, LPCTSTR lpFontName=_T("宋体"), COLORREF color=RGB(0,0,0), UINT uID=0, TEXT_ALIGN iAlign=ALIGN_LEFT, BOOL bRepaint=TRUE);
	BOOL	AddScrollText(CRect rect, LPCTSTR lpszText, int iFont, LPCTSTR lpFontName=_T("宋体"), COLORREF color=RGB(0,0,0), UINT uID=0, UINT uSpeed=10, TEXT_ALIGN iAlign=ALIGN_LEFT_SCROOL, BOOL bRepaint=TRUE);
	// 启动超时定时器 uTimeOut=-1 使用默认时间（或之前设置的时间）
	BOOL    StartTimeOut(UINT uTimeOut=60, BOOL bShow=FALSE, int iX=-1, int iY=-1, UINT uFont=32, COLORREF color=RGB(255,0,0));
	// 关闭超时定时器
	BOOL    StopTimeOut();
	// 暂停
	void    PauseTimeOut(BOOL bPause);
	// 刷新滚动文字每100ms 内部调用
	void RefreashText();
	
public:
	TIME_OUT_SET m_stTimeOut;

protected:
	CArray<LPIMAGE_INFO, LPIMAGE_INFO>	m_apImages;
	CString		m_strLastError;
	HDC			m_hMemDC;
	HDC			m_hMemDC2;//复制 m_hMemDC 并增加文字
	HDC			m_hMemDC3;//复制 m_hMemDC2 并增加滚动文字
	HBITMAP		m_hMemBitmap;
	HBITMAP		m_hMemBitmap2;
	HBITMAP		m_hMemBitmap3;
	HBITMAP		m_hOldMemBitmap;
	HBITMAP		m_hOldMemBitmap2;
	HBITMAP		m_hOldMemBitmap3;
	BOOL		m_bMemDrawn;
	CRITICAL_SECTION	m_ImagesSection;
	CString		m_strExePath;
	HWND		m_hOldImageWnd;
	CSize		m_cz;


	struct ST_FONT
	{
		UINT iFont;
		CString strFontName;
		CFont* pFont;
		static bool Compare(ST_FONT* pMask, ST_FONT* pData)
		{
			if(pMask->iFont==pData->iFont && pMask->strFontName==pData->strFontName) return true;
			return false;
		}
	};
	CCommTbl<ST_FONT> m_vctFont;
	CCommTbl<CTL_ID_COLOR> m_vctStatic;

protected:
	void	OnButtonDown(LPIMAGE_INFO pImageInfo);
	void	OnButtonUp(LPIMAGE_INFO pImageInfo);
	BOOL	DrawImages();
	void	RedrawImages(CRect rc);
	void	ResetAllBtnStatus();
	void	PauseAnimations(BOOL bPause);
	void	ResetBtnStatus(UINT uID);
	void	SetButtonStatus(LPIMAGE_INFO pImageInfo, int nStatus);
	CImageBase*	GetGlobalImage(LPCTSTR lpszImageName);
	CImageBase*	GetGlobalImage(UINT uImageName);
    
    CFont* GetFontPoint(int iFont, LPCTSTR lpFontName=_T("宋体"));

private:
	void	GetCurExePath();

public:
	
	// Dialog Data
	//{{AFX_DATA(CImageDlg)
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageDlg)
	public:
    virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL
	
	// Implementation
protected:
	
	// Generated message map functions
	//{{AFX_MSG(CImageDlg)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	BOOL	OnLButtonDown2(UINT nFlags, CPoint point);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CArray<GLOBAL_IMAGE, GLOBAL_IMAGE>	g_aGlobalImages;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMAGEDLG_H__5A60840C_15F9_4D34_948C_11394F79A50F__INCLUDED_)

