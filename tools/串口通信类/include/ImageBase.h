// ImageBase.h: interface for the CImageBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEBASE_H__E83FF323_C934_4A57_8951_7A7C57B15AF0__INCLUDED_)
#define AFX_IMAGEBASE_H__E83FF323_C934_4A57_8951_7A7C57B15AF0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>

class CImageBase  
{
public:
	CImageBase();
	virtual ~CImageBase();

public:
	struct TFrame    // structure that keeps a single frame info
{
	IPicture *m_pPicture;  // pointer to the interface used for drawing
	SIZE     m_frameSize;
	SIZE     m_frameOffset;
	UINT     m_nDelay;     // delay (in 1/100s of a second)
	UINT     m_nDisposal;  // disposal method
};

#pragma pack(1)   // turn byte alignment on

enum GIFBlockTypes
{
	BLOCK_UNKNOWN,
	BLOCK_APPEXT,
	BLOCK_COMMEXT,
	BLOCK_CONTROLEXT,
	BLOCK_PLAINTEXT,
	BLOCK_IMAGE,
	BLOCK_TRAILER
};

enum ControlExtValues // graphic control extension packed field values
{
	GCX_PACKED_DISPOSAL,  // disposal method
	GCX_PACKED_USERINPUT,
	GCX_PACKED_TRANSPCOLOR
};

enum LSDPackedValues  // logical screen descriptor packed field values
{
	LSD_PACKED_GLOBALCT,
	LSD_PACKED_CRESOLUTION,
	LSD_PACKED_SORT,
	LSD_PACKED_GLOBALCTSIZE
};

enum IDPackedValues   // image descriptor packed field values
{
	ID_PACKED_LOCALCT,
	ID_PACKED_INTERLACE,
	ID_PACKED_SORT,
	ID_PACKED_LOCALCTSIZE
};

struct TGIFHeader       // GIF header  
{
	char m_cSignature[3]; // Signature - Identifies the GIF Data Stream
						  // This field contains the fixed value 'GIF'
	char m_cVersion[3];	// Version number. May be one of the following:
						// "87a" or "89a"
};

struct TGIFLSDescriptor // Logical Screen Descriptor
{
	WORD m_wWidth;	// 2 bytes. Logical screen width
	WORD m_wHeight; // 2 bytes. Logical screen height

	unsigned char m_cPacked;      // packed field	

	unsigned char m_cBkIndex;     // 1 byte. Background color index
	unsigned char m_cPixelAspect; // 1 byte. Pixel aspect ratio
	inline int GetPackedValue(enum LSDPackedValues Value);
};

struct TGIFAppExtension // application extension block
{
	unsigned char m_cExtIntroducer; // extension introducer (0x21)
	unsigned char m_cExtLabel; // app. extension label (0xFF)
	unsigned char m_cBlockSize; // fixed value of 11
	char m_cAppIdentifier[8];   // application identifier
	char m_cAppAuth[3];  // application authentication code
};

struct TGIFControlExt // graphic control extension block
{
	unsigned char m_cExtIntroducer; // extension introducer (0x21)
	unsigned char m_cControlLabel;  // control extension label (0xF9)
	unsigned char m_cBlockSize; // fixed value of 4
	unsigned char m_cPacked;    // packed field
	WORD m_wDelayTime;	// delay time
	unsigned char m_cTColorIndex; // transparent color index
	unsigned char m_cBlockTerm;   // block terminator (0x00)
public:
	inline int GetPackedValue(enum ControlExtValues Value);
};

struct TGIFCommentExt  // comment extension block
{
	unsigned char m_cExtIntroducer; // extension introducer (0x21)
	unsigned char m_cCommentLabel;  // comment extension label (0xFE)
};

struct TGIFPlainTextExt // plain text extension block
{
	unsigned char m_cExtIntroducer;  // extension introducer (0x21)
	unsigned char m_cPlainTextLabel; // text extension label (0x01)
	unsigned char m_cBlockSize; // fixed value of 12
	WORD m_wLeftPos;    // text grid left position
	WORD m_wTopPos;     // text grid top position
	WORD m_wGridWidth;  // text grid width
	WORD m_wGridHeight; // text grid height
	unsigned char m_cCellWidth;  // character cell width
	unsigned char m_cCellHeight; // character cell height
	unsigned char m_cFgColor; // text foreground color index
	unsigned char m_cBkColor; // text background color index
};

struct TGIFImageDescriptor // image descriptor block
{
	unsigned char m_cImageSeparator; // image separator byte (0x2C)
	WORD m_wLeftPos; // image left position
	WORD m_wTopPos;  // image top position
	WORD m_wWidth;   // image width
	WORD m_wHeight;  // image height
	unsigned char m_cPacked; // packed field
	inline int GetPackedValue(enum IDPackedValues Value);
};

#pragma pack() // turn byte alignment off

public:
	
public:
	BOOL	Load(LPCTSTR lpszFileName, BOOL bUseMemDC = FALSE);
	BOOL	Load(HGLOBAL hGlobal, DWORD dwSize, BOOL bUseMemDC = FALSE);
	BOOL	Load(UINT uID, BOOL bUseMemDC = FALSE);
	BOOL	Draw(HDC hdc, int nPosX, int nPosY);
//	BOOL	DrawMemDC(HDC hdc, int nPosX, int nPosY);	//test
	CString	GetLastError(){return m_strLastError;}
	CSize	GetPictureSize(){return m_PictureSize;}
	BOOL	IsBKColor(CPoint ptPos);
//	BOOL	ToRefresh(){return m_uLastFrame != m_uCurrFrame;}
//	void	ResetRefresh(){m_uLastFrame = m_uCurrFrame;}
//	void	SetParent(void* pParent){m_pParent = pParent;}
	void	SetPicturePos(int nPosX, int nPosY);
	void	SetParentWnd(HWND hWnd){m_hParentWnd = hWnd;}
	void	ShowPicture(BOOL bShow);
	HWND	GetParentWnd(){return m_hParentWnd;}
	
protected:
	CString		m_strLastError;
	IPicture*	m_pPicture;
	CSize		m_PictureSize;
	HDC			m_hDispDC;
	HDC			m_hBakDC;
	HDC			m_hMemDC;
	COLORREF	m_clrBG1;
	COLORREF	m_clrBG2;
	HBITMAP		m_hBitmap1;//mem dc
	HBITMAP		m_hBitmap2;//disp dc
	HBITMAP		m_hBakBitmap;
	HBITMAP		m_hOldMemBitmap;
	HBITMAP		m_hOldDispBitmap;
	HBITMAP		m_hOldBakBitmap;

	LPBYTE		m_pRawData;
	UINT		m_uDataSize;
	UINT		m_uGlobalCTSize;
	UINT		m_uCurrFrame;
	UINT		m_uCurrOffset;
	TGIFHeader*	m_pGIFHeader;
	TGIFLSDescriptor*	m_pGIFLSDescriptor;
	std::vector<TFrame> m_arrFrames;
	COLORREF	m_clrBackground;
	HANDLE		m_hFrameThread;
	BOOL		m_bExitThread;
	HANDLE		m_hExitEvent;
	HANDLE		m_hShowEvent;
	UINT		m_uLastFrame;
	void*		m_pParent;
	HWND		m_hParentWnd;
	CPoint		m_ptPicturePos;
	CRITICAL_SECTION m_DrawSection;

protected:
	void	UnLoad();
	void	PrepareDC();
	BOOL	LoadPictureInfo(HGLOBAL hGlobal, DWORD dwSize, BOOL bUseMemDC);
	
	void	ResetDataPointer();
	int		GetNextBlockLen() const;
	BOOL	SkipNextBlock();
	BOOL	SkipNextGraphicBlock();
	enum GIFBlockTypes	GetNextBlock() const;
	UINT	GetSubBlocksLen(UINT nStartingOffset) const;
	HGLOBAL	GetNextGraphicBlock(UINT *pBlockLen, UINT *pDelay, 
								SIZE *pBlockSize, SIZE *pBlockOffset, UINT *pDisposal);
	static DWORD WINAPI	_FrameThreadProc(LPVOID pParam);
	DWORD	FrameThreadProc();

public:
	
};

#endif // !defined(AFX_IMAGEBASE_H__E83FF323_C934_4A57_8951_7A7C57B15AF0__INCLUDED_)
