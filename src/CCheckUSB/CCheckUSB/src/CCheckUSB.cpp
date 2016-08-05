#include <SDKDDKVer.h>
#include "CCheckUSB.h"
#include <dbt.h>
#include <afxmt.h>

//多线程
//需定义DWORD dwThreadId = NULL;函数实现为DWORD WINAPI add1_fun(LPVOID lpParam)
//DWORD WINAPI add1_fun(LPVOID lpParam){},DWORD WINAPI不可变
//变量只能定义一个，如果有多个需要打包
#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID)

CCheckUSB *Son = NULL;
//用于限制只有一个CCheckUSB
BOOL ifInit = 0;
//1表示正在循环，0表示已经退出循环
BOOL stateIn = 1;
BOOL stateOut = 1;

CCheckUSB::CCheckUSB(){
	if(ifInit == 0){
		Son = this;
		ifInit = 1;
	}
	else AfxMessageBox("只允许存在一个CCheckUSB");

	h_CheckIn = 0;
	h_CheckOut = 0;
	ifCheckIn = 0;
	ifCheckOut = 0;
}

CCheckUSB::~CCheckUSB(){
	if(ifInit == 1){
		if(ifCheckIn == 1){
			//变为不是1之后就开始退出消息检测循环
			CMutex mutex;
			mutex.Lock();
			if(stateIn == 1) stateIn = 3;
			mutex.Unlock();
			//阻塞直到退出循环之后才可以执行释放动作
			while(stateIn != 0);
			//此时关闭之后句柄就已经无效了不需要再销毁
			CloseWindow(Son->h_CheckIn);
			if(IsWindow(Son->h_CheckIn) != 0) AfxMessageBox("未成功关闭");
			//成功关闭窗口之后就可以反注册类了
			BOOL nSucceed = UnregisterClass("USBIn",AfxGetInstanceHandle());
			if(nSucceed == 0) AfxMessageBox("反注册类失败");
		}
		
		if(ifCheckOut == 1){
			//变为不是1之后就开始退出消息检测循环
			CMutex mutex;
			mutex.Lock();
			//如果在循环内则让他跳出循环
			if(stateOut == 1) stateOut = 3;
			mutex.Unlock();
			//阻塞直到退出循环之后才可以执行释放动作
			while(stateOut != 0);
			//此时关闭之后句柄就已经无效了不需要再销毁
			CloseWindow(Son->h_CheckOut);
			if(IsWindow(Son->h_CheckOut) != 0) AfxMessageBox("未成功关闭");
			//成功关闭窗口之后就可以反注册类了
			BOOL nSucceed = UnregisterClass("USBOut",AfxGetInstanceHandle());
			if(nSucceed == 0) AfxMessageBox("反注册类失败");
		}
	}
	//在销毁之后就可以继续再产生CCheckUSB了，所以这里需要重置为0
	ifInit = 0;
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

	WNDCLASS wndclsIn = {0};
	wndclsIn.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclsIn.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
	wndclsIn.hIcon = (HICON)LoadIcon(NULL, IDI_APPLICATION);
	wndclsIn.lpfnWndProc = WndProcIn;
	wndclsIn.lpszClassName = szClassName;
	
	if(RegisterClass(&wndclsIn) == 0){
		AfxMessageBox("RegisterClass Failed!\r\n");
		return 0;
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
	while(stateIn == 1){
		GetMessage(&msg, NULL, NULL, NULL);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	CMutex mutex;
	mutex.Lock();
	stateIn = 0;
	mutex.Unlock();
	return 1;
}

void CCheckUSB::CheckUSBIn(){
	ifCheckIn = 1;
	//默认在循环中
	stateIn = 1;
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

	WNDCLASS wndclsOut = {0};
	wndclsOut.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclsOut.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
	wndclsOut.hIcon = (HICON)LoadIcon(NULL, IDI_APPLICATION);
	wndclsOut.lpfnWndProc = WndProcIn;
	wndclsOut.lpszClassName = szClassName;

	if(RegisterClass(&wndclsOut) == 0){
		AfxMessageBox("RegisterClass Failed!\r\n");
		return 0;
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
	while(stateOut == 1){
		GetMessage(&msg, NULL, NULL, NULL);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	CMutex mutex;
	mutex.Lock();
	stateOut = 0;
	mutex.Unlock();
	return 1;
}

void CCheckUSB::CheckUSBOut(){
	ifCheckOut = 1;
	//默认是在循环中
	stateOut = 1;
	DWORD ThreadID = 0;
	Create_Thread(CheckOut,0,ThreadID);
	return;
}