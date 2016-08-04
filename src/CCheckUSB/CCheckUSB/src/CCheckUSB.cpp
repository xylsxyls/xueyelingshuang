#include <SDKDDKVer.h>
#include "CCheckUSB.h"
#include <dbt.h>

//多线程
//需定义DWORD dwThreadId = NULL;函数实现为DWORD WINAPI add1_fun(LPVOID lpParam)
//DWORD WINAPI add1_fun(LPVOID lpParam){},DWORD WINAPI不可变
//变量只能定义一个，如果有多个需要打包
#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID)

CCheckUSB *Son = NULL;
BOOL ifInit = 0;

void CCheckUSB::init(CCheckUSB *pSon){
	if(ifInit == 0){
		Son = pSon;
		ifInit = 1;
	}
	else AfxMessageBox("只允许存在一个CCheckUSB");
}

CCheckUSB::CCheckUSB(){
	h_CheckIn = 0;
	h_CheckOut = 0;
}

CCheckUSB::~CCheckUSB(){
	if(ifInit == 1){
		while(Son->h_CheckIn == 0);
		DestroyWindow(Son->h_CheckIn);
		while(Son->h_CheckOut == 0);
		DestroyWindow(Son->h_CheckOut);
	}
	ifInit = 0;
	Son = NULL;
}

char FirstDriveFromMask(ULONG unitmask){
	char i;
	for(i = 0;i < 26;++i){
		if(unitmask & 0x1) break;
		unitmask = unitmask >> 1;
	}
	return(i + 'A');
}

LRESULT CALLBACK WndProcIn(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam){
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
	PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
	char driveName;
	CString diskpath = "";
	if(uMsg == WM_DEVICECHANGE){
		if(wParam == DBT_DEVICEARRIVAL){
			driveName = FirstDriveFromMask(lpdbv->dbcv_unitmask);
			diskpath.Format("%c:\\",driveName);
			Son->USBIn(diskpath);
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI CheckIn(LPVOID lpParam){
	TCHAR szClassName[] = "USBIn";
	WNDCLASS wndcls = {0};
	wndcls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndcls.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
	wndcls.hIcon = (HICON)LoadIcon(NULL, IDI_APPLICATION);
	wndcls.lpfnWndProc = WndProcIn;
	wndcls.lpszClassName = szClassName;

	if(RegisterClass(&wndcls) == 0){
		//AfxMessageBox("RegisterClass Failed!\r\n");
		//return 0;
	}

	if(Son->h_CheckIn != 0) AfxMessageBox("已有窗口");
	Son->h_CheckIn = CreateWindow(szClassName,szClassName,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,NULL,NULL);
	if(NULL == Son->h_CheckIn){
		AfxMessageBox("CreateWindow Failed!\r\n");
		return 0;
	}
	ShowWindow(Son->h_CheckIn, SW_HIDE);
	UpdateWindow(Son->h_CheckIn);

	MSG msg;
	while(GetMessage(&msg, NULL, NULL, NULL)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 1;
}

void CCheckUSB::CheckUSBIn(){
	DWORD ThreadID = 0;
	Create_Thread(CheckIn,0,ThreadID);
	return;
}

LRESULT CALLBACK WndProcOut(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam){
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
	PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
	char driveName;
	CString diskpath = "";

	if(uMsg == WM_DEVICECHANGE){
		if(wParam == DBT_DEVICEREMOVECOMPLETE){
			driveName = FirstDriveFromMask(lpdbv->dbcv_unitmask);
			diskpath.Format("%c:\\",driveName);
			Son->USBOut(diskpath);
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI CheckOut(LPVOID lpParam){
	TCHAR szClassName[] = "USBOut";
	WNDCLASS wndcls = {0};
	wndcls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndcls.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
	wndcls.hIcon = (HICON)LoadIcon(NULL, IDI_APPLICATION);
	wndcls.lpfnWndProc = WndProcOut;
	wndcls.lpszClassName = szClassName;

	if(RegisterClass(&wndcls) == 0){
		//AfxMessageBox("RegisterClass Failed!\r\n");
		//return 0;
	}

	if(Son->h_CheckOut != 0) AfxMessageBox("已有窗口");
	Son->h_CheckOut = CreateWindow(szClassName,szClassName,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,NULL,NULL);
	if(NULL == Son->h_CheckOut){
		AfxMessageBox("CreateWindow Failed!\r\n");
		return 0;
	}
	ShowWindow(Son->h_CheckOut, SW_HIDE);
	UpdateWindow(Son->h_CheckOut);

	MSG msg;
	while(GetMessage(&msg, NULL, NULL, NULL)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 1;
}

void CCheckUSB::CheckUSBOut(){
	DWORD ThreadID = 0;
	Create_Thread(CheckOut,0,ThreadID);
	return;
}