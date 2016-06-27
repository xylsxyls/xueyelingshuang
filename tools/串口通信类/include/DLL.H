#ifndef __DLL_H__
#define __DLL_H__
#include "CommTbl.h"

enum HS_DEVICE_TYPE
{
	UNKNOW_DEVICE,
	PASSWORD_KEYBOARD,
	CASH_CODE,
	CARD_READER,
	VOUCHER_PRINTER,
	INVOICE_PRINTER
};

#define PORT_UNUSED			0
#define PORT_USED			1
#define PORT_KNOW			2
#define PORT_UNKNOW			3
struct PORT_DEVICE {
	CString strPort;
	HS_DEVICE_TYPE iType; 
	HANDLE hThread;// 线程句柄
	int iStatus; // 0 未占用 1 已占用 2 已识别 3 无法识别
	CString strInfo;// 存放识别信息
	PORT_DEVICE()
	{
		strPort = "";
		iType = UNKNOW_DEVICE;
		hThread = NULL;
		iStatus = PORT_UNUSED;
		strInfo = "";
	}
};


// =====================================================================
// 接口函数
// =====================================================================
// 加载硬件dll
bool LoadDlls();
// 释放硬件dll
bool FreeDlls();
// 返回 0 表示成功 非0表示失败 只支持串口设备检测
int CheckDevice(HS_DEVICE_TYPE iType, char* pPort);
// 自动检测串口硬件（目前只支持：密码键盘、纸币器、吞卡器、热敏打印机、发票打印机）
CCommTbl<PORT_DEVICE> AutoCheckPort();



// #####################################################################
// 密码键盘部分
typedef int (*p_InitKeyboard)(const char* szPort);
typedef int (*p_SetMasterKey)(short nKeyNo, const unsigned char* szMasterKey, bool bIsString);
typedef int (*p_SetWorkKey)(short nMasterKeyNo, short nWorkKeyNo, unsigned char* szWorkKey);
typedef int (*p_ActiveWorkKey)(short nMasterKeyNo, short nWorkKeyNo);
typedef int (*p_InputPasswordStart)(short nPasswordLen, 
									short nEncryptionMode, 
									short nTimeout, 
									short nPromptMode, 
									short nDispMode);
typedef int (*p_GetInputPassword)(unsigned char* szPassword, int nInLen, int& nOutLen);
typedef int (*p_ReadKeyValue)(unsigned char& chOut);
typedef int (*p_ClosePwdKeyboard)();
typedef int (*p_SetKeyboardType)(int nState);
typedef const char* (*p_GetKeyboardError)();
typedef int (*p_SetCardNo)(const unsigned char* szCardNo);
typedef int (*p_EncryptData)(const unsigned char* abIn, unsigned char* abOut);
typedef	int	(*p_SetEncryptParam)(short nMainParam , short nAuxiliary);
typedef	int	(*p_ComputeMac)(BYTE* abIn, int nInLen, BYTE* abOut);
typedef int (*p_DecryptData)(BYTE* abIn, BYTE* abOut);

extern p_InitKeyboard			_InitKeyboard;
extern p_SetMasterKey			_SetMasterKey;
extern p_SetWorkKey				_SetWorkKey;
extern p_ActiveWorkKey			_ActiveWorkKey;
extern p_InputPasswordStart		_InputPasswordStart;
extern p_GetInputPassword		_GetInputPassword;
extern p_ReadKeyValue			_ReadKeyValue;
extern p_ClosePwdKeyboard		_ClosePwdKeyboard;
extern p_SetKeyboardType		_SetKeyboardType;
extern p_GetKeyboardError		_GetKeyboardError;
extern p_SetCardNo				_SetCardNo;
extern p_EncryptData			_EncryptData;
extern	p_SetEncryptParam		_SetEncryptParam;
extern	p_ComputeMac			_ComputeMac;
extern p_DecryptData			_DecryptData;


// #####################################################################
// 纸币器部分
#define BILL_VALUE_1	1
#define BILL_VALUE_2	2
#define BILL_VALUE_5	4
#define BILL_VALUE_10	8
#define BILL_VALUE_20	16
#define BILL_VALUE_50	32
#define BILL_VALUE_100	64

typedef int (*p_OpenCashCode)(const char* szPort);
typedef int (*p_SetPollBillType)(int nType);
typedef int (*p_CloseCashCode)();
typedef int (*p_GetBillValue)(int& nValue);
typedef int (*p_GetCashCodeState)();
typedef int (*p_BackwardBill)();
typedef int (*p_SetPollBillEscrow)(int nType);
typedef int (*p_StackBills)();
typedef int (*p_ReturnBills)();
typedef int (*p_HoldBills)();
typedef int (*p_DisableBillTypes)();
typedef int (*p_SetWriteLogFlag)(BOOL bWrite);
typedef const char* (*p_GetCashCodeError)();

extern p_OpenCashCode	_OpenCashCode;
extern p_SetPollBillType _SetPollBillType;
extern p_CloseCashCode _CloseCashCode;
extern p_GetBillValue	_GetBillValue;
extern p_GetCashCodeState	_GetCashCodeState;
extern p_BackwardBill	_BackwardBill;
extern p_SetPollBillEscrow	_SetPollBillEscrow;
extern p_StackBills	_StackBills;
extern p_ReturnBills	_ReturnBills;
extern p_HoldBills		_HoldBills;
extern p_DisableBillTypes	_DisableBillTypes;
extern p_SetWriteLogFlag	_SetWriteLogFlag;
extern p_GetCashCodeError	_GetCashCodeError;

// #####################################################################
// 吞卡器部分
#define NO_CARD_DETECTED		1
#define CARD_AT_GATE			2
#define CARD_INSIDE_CARD_READER	3

typedef const char* (*p_GetCardReaderError)();
typedef int (*p_OpenCardReader)(const char* szPort);
typedef int (*p_EntryCard)();
typedef int (*p_EjectCard)(bool bToFront);
typedef int (*p_ReadMagneticCard)(char* szTrack1, int nLen1, char* szTrack2, int nLen2, char* szTrack3, int nLen3);
typedef int (*p_GetCardReaderStatus)();
typedef int (*p_CloseCardReader)();

extern p_GetCardReaderError _GetCardReaderError;
extern p_OpenCardReader _OpenCardReader;
extern p_EntryCard _EntryCard;
extern p_EjectCard _EjectCard;
extern p_ReadMagneticCard _ReadMagneticCard;
extern p_GetCardReaderStatus _GetCardReaderStatus;
extern p_CloseCardReader _CloseCardReader;
// #####################################################################


#endif