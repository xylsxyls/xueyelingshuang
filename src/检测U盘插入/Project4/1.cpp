#include <tchar.h>  
#include <string>  
#include <iostream>  
#include <Windows.h>  
  
#include <dbt.h>  
  
/*------------------------------------------------------------------ 
FirstDriveFromMask( unitmask ) 
 
Description 
Finds the first valid drive letter from a mask of drive letters. 
The mask must be in the format bit 0 = A, bit 1 = B, bit 2 = C,  
and so on. A valid drive letter is defined when the  
corresponding bit is set to 1. 
 
Returns the first drive letter that was found. 
--------------------------------------------------------------------*/  
char FirstDriveFromMask(ULONG unitmask)  
{  
    char i;  
      
    for (i = 0; i < 26; ++i)  
    {  
        if (unitmask & 0x1)  
            break;  
        unitmask = unitmask >> 1;  
    }  
      
    return(i + 'A');  
}  
  
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)  
{  
    PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;  
    PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;  
    TCHAR szMsg[80];  
    char driveName;  
      
    switch(uMsg)  
    {  
    case WM_DEVICECHANGE:  
        switch(wParam )  
        {  
        case DBT_DEVICEARRIVAL:  
                driveName = FirstDriveFromMask(lpdbv->dbcv_unitmask);  
                sprintf(szMsg, "USB Drive %c: has inserted.\n", driveName);  
                printf("%s\r\n", szMsg);  
                MessageBox(hWnd, szMsg, TEXT("WM_DEVICECHANGE"), MB_OK);  
            break;  
        case DBT_DEVICEREMOVECOMPLETE:  
                driveName = FirstDriveFromMask(lpdbv->dbcv_unitmask);  
                sprintf(szMsg, "USB Drive %c: has removed.\n", driveName);  
                printf("%s\r\n", szMsg);  
                MessageBox(hWnd, szMsg, TEXT("WM_DEVICECHANGE"), MB_OK );  
            break;  
        default:  
            ;  
        }  
        break;  
        default:  
            ;  
    }  
      
    return DefWindowProc(hWnd, uMsg, wParam, lParam);  
}  
  
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])  
{  
    TCHAR szClassName[] = _T("MyApp");  
    WNDCLASS wndcls = {0};  
    wndcls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  
    wndcls.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);  
    wndcls.hIcon = (HICON)LoadIcon(NULL, IDI_APPLICATION);  
    wndcls.lpfnWndProc = WndProc;  
    wndcls.lpszClassName = szClassName;  
    if(!RegisterClass(&wndcls))  
    {  
        printf("RegisterClass Failed!\r\n");  
        return 0;  
    }  
      
    HWND hWnd = CreateWindow(szClassName, szClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,  
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);  
    if(NULL == hWnd)  
    {  
        printf("CreateWindow Failed!\r\n");  
        return 0;  
    }  
    ShowWindow(hWnd, SW_HIDE);  
    UpdateWindow(hWnd);  
      
    MSG msg;  
    while(GetMessage(&msg, NULL, NULL, NULL))  
    {  
        TranslateMessage(&msg);  
        DispatchMessage(&msg);  
    }  
    return 0;  
}