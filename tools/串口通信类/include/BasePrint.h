#if !defined(AFX_BASEPRINT_H__BFA1546C_90BC_445C_BF86_F1D45F804F6A__INCLUDED_)
#define AFX_BASEPRINT_H__BFA1546C_90BC_445C_BF86_F1D45F804F6A__INCLUDED_

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BASEPRINT_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.



typedef enum{CUT_PAPER = 1, BACKWARD_PAPER, FORWARD_PAPER, LINE_SPACE, RESET_PRINTER, LEFT_MARGIN, 
			RIGHT_MARGIN, BLACK_MARK_DETECTION, CODE_HEIGH=15}PRINT_COMMAND; 

#if !defined(__VOUCHER_PRINTER_INCLUDED__)
#define __VOUCHER_PRINTER_INCLUDED__

typedef int (*p_InitVoucherPrinter)(const char* szPort);
typedef int (*p_PrintVoucherData)(const char* szData, int nLen);
typedef const char* (*p_GetVoucherPrinterError)();
typedef int (*p_EndVoucherPrinter)();
typedef int (*p_HaveVoucherPaper)(const char* szPort);
typedef int (*p_PrintVoucherCommand)(int nCmdType, int nParam);
typedef int (*p_PrintBitmap)(const char* szFileName);
typedef int	(*p_SetVoucherBaudRate)(int nBaudRate);
/*
打印条码
参数说明：pData-需要打印的条码数据
iType-条码类型(1-EAN13编码 2-CODE128A 3-CODE128B 4-CODE128C )
iBytePos-条码值打印位置(0-不打印 1-打印上方 2-打印下方 3-打印上下方 CODE128无法打印上方)
*/
typedef	int (*p_PrinterCode)(char* pData, UINT iType, UINT iBytePos);


extern p_InitVoucherPrinter _InitVoucherPrinter;
extern p_PrintVoucherData _PrintVoucherData;
extern p_GetVoucherPrinterError _GetVoucherPrinterError;
extern p_EndVoucherPrinter _EndVoucherPrinter;
extern p_HaveVoucherPaper _HaveVoucherPaper;
extern p_PrintVoucherCommand _PrintVoucherCommand;
extern p_PrintBitmap _PrintBitmap;
extern p_SetVoucherBaudRate _SetVoucherBaudRate;
extern p_PrinterCode _PrinterCode;

#endif

#if !defined(__INVOICE_PRINTER_INCLUDED__)
#define __INVOICE_PRINTER_INCLUDED__

typedef int (*p_InitInvoicePrinter)(const char* szPort);
typedef int (*p_PrintInvoiceData)(const char* szData, int nLen);
typedef int (*p_HaveInvoicePaper)(const char* szPort);
typedef int (*p_EndInvoicePrinter)();
typedef int (*p_PrintInvoiceCommand)(int nCmdType, int nParam);
typedef const char* (*p_GetInvoicePrinterError)();

extern p_InitInvoicePrinter _InitInvoicePrinter;
extern p_PrintInvoiceData _PrintInvoiceData;
extern p_HaveInvoicePaper _HaveInvoicePaper;
extern p_EndInvoicePrinter _EndInvoicePrinter;
extern p_PrintInvoiceCommand _PrintInvoiceCommand;
extern p_GetInvoicePrinterError _GetInvoicePrinterError;

#endif

#if !defined(__PRINTER_PARALLEL_STATUS__)
#define __PRINTER_PARALLEL_STATUS__
typedef short  (_stdcall*p_Inp32)(short PortAddress);
typedef void (_stdcall*p_Out32)(short PortAddress, short data);
extern p_Inp32 _Inp32;
extern p_Out32 _Out32;
#endif



// This class is exported from the BasePrint.dll
class CBasePrint
{
public:
	CBasePrint();
	virtual ~CBasePrint();
	
public:
	
public:
	BOOL	InitInvoicePrint(LPCTSTR lpszPort);
	BOOL	PrintInvoice(LPCTSTR lpszData, int nLen = -1, BOOL bNewLine = TRUE);
	void	EndInvoicePrint();
	void	AddInvoicePrintParam(LPCTSTR lpszData);
	void	AddInvoicePrintParam(int nData);
	void	AddInvoicePrintParam(float fData);
	void	InsertInvoicePrintParam(int nIndex, LPCTSTR lpszData);
	void	InsertInvoicePrintParam(int nIndex, int nData);
	void	InsertInvoicePrintParam(int nIndex, float fData);
	void	AddInvoicePrintContent(LPCTSTR lpszData);
	BOOL	PrintInvoiceFromFile(LPCTSTR lpszFileName);
	BOOL	HaveInvoicePaper(LPCTSTR lpszPort);
	
	BOOL	InitVoucherPrint(LPCTSTR lpszPort);
	BOOL	PrintVoucher(LPCTSTR lpszData, int nLen = -1, BOOL bNewLine = TRUE);
	void	EndVoucherPrint();
	void	AddVoucherPrintParam(LPCTSTR lpszData, BOOL bToFromat = FALSE);
	void	AddVoucherPrintParam(int nData);
	void	AddVoucherPrintParam(float fData);
	void	InsertVoucherPrintParam(int nIndex, LPCTSTR lpszData);
	void	InsertVoucherPrintParam(int nIndex, int nData);
	void	InsertVoucherPrintParam(int nIndex, float fData);
	void	AddVoucherPrintContent(LPCTSTR lpszData);
	BOOL	PrintVoucherFromFile(LPCTSTR lpszFileName);
	void	ResetAllParam();
	BOOL	HaveVoucherPaper(LPCTSTR lpszPort);

	void	SetDataFilePath(LPCTSTR lpszPath);
	BOOL	LoadInvoiceLib(LPCTSTR lpszPath);
	BOOL	LoadVoucherLib(LPCTSTR lpszPath);
	const char*	GetLastError();
	
	BOOL	Inp32(short PortAddress, short& data);
	BOOL    Out32(short PortAddress, short data);
	
	
protected:
	//	HANDLE	m_hInvoicePrint;
	CStringArray	m_astrInvoiceParam;
	CStringArray	m_astrInvoiceContent;
	int				m_nAllLineCount;
	int				m_nAlreadyPrintCount;
	//	HANDLE	m_hVoucherPrint;
	CStringArray	m_astrVoucherParam;
	CStringArray	m_astrVoucherContent;
	int		m_nInvoiceAlignType;
	int		m_nVoucherAlignType;
	enum{ALIGN_LEFT = 1, ALIGN_CENTER, ALIGN_RIGHT};
	BOOL	m_bInvoiceAutoNewLine;
	BOOL	m_bVoucherAutoNewLine;
	int		m_nVoucherContentColumns;
	int		m_nVoucherCurIndex;
	int		m_nContentStartPos;
	int		m_nContentEndPos;
	CString	m_strDataFilePath;
	CString m_strLastError;
	
	HMODULE		m_hInvoiceLibHandle;
	HMODULE		m_hVoucherLibHandle;
	
protected:
	BOOL	PrintLoadedInvoiceString(LPCTSTR lpszData, BOOL bNewLine, BOOL bContent, int nContentIndex = -1);
	BOOL	PrintLoadedVoucherString(LPCTSTR lpszData, BOOL bNewLine, BOOL bContent, int nContentIndex = -1);
	CString	ClearInvalidString(LPCTSTR lpszData);
	BOOL	PrintInvoiceCommand(LPCTSTR lpszData);
	BOOL	PrintInvoiceContent(LPCTSTR lpszData);
	BOOL	InvoiceBackwardPaper(int nLine);
	BOOL	InvoiceForwardPaper(int nLine);
	BOOL	InvoiceCutPaper();
	BOOL	SetInvoiceLineSpace(int nSpace);//nSpace/180英寸,nSpace<360
	CString GetInvoiceContentData(int nLine, int nRow);
	
	BOOL	PrintVoucherCommand(LPCTSTR lpszData);
	BOOL	PrintVoucherContent(LPCTSTR lpszData);
	CString	GetVoucherContentData(int nLine, int nRow);
	CString	GetExecuteContent(LPCTSTR lpszData, CString& strRemain);
	CString	ExecutePrintData(LPCTSTR lpszPrintData, LPCTSTR lpszExecuteData);
	
	BOOL	WriteLogFile(LPCTSTR lpszFileName, LPCTSTR lpszData, BOOL bNewLine = TRUE);
	BOOL	ExecuteSentence(CString& strData, BOOL bInvoice);
	void	InitDllParam();

    int     PrintVoucherData(const char* szData, int nLen);
    int     GetDateLineCnt(const char* szData);

public:
    int     GetLineCnt();

private:
    UINT    m_iLineCnt;
};

#endif