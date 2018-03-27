#pragma once
#include <windows.h>
#include <string>
#include "CSystemMacro.h"

class CSystemAPI CSystem{
public:
	//?ʵ��CPU��Ƶ
	static double GetCPUSpeedGHz();
	//?��ȡϵͳ���������δ�С
	static RECT GetTaskbarRect();
	//?��ȡ��Ļ�ֱ���
	static RECT GetWindowResolution();
	//?��ȡȥ����������������ʱ���������������Ļ�߶�
	static int GetVisibleHeight();
	//?��ƽ̨Sleep������
	static void Sleep(long long milliseconds);
	//?����UUID������1��ʾ��-����0��ʾ����
	static std::string uuid(int flag = 1);
	//?��ϵͳ�ļ���
	static void OpenFolder(const std::string& folder);
	//?��ϵͳ�ļ��в�ѡ���ļ�
	static void OpenFolderAndSelectFile(const std::string& file);
	//?�ⲿ���ļ�
	static void OpenFile(const std::string& file);
	//?�ⲿ����ҳ
	static void OpenWebPage(const std::string& webPage);
	//?�����ļ�
	static void CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over);
	//?��ȡ��ǰ����ϵͳλ��
	static int GetSystemBits();
	//?�����ض���
	static void ForbidRedir();
	//?�ָ��ض���
	static void RecoveryRedir();
	//?��ȡ��ǰϵͳ�û���������Administrator
	static std::string GetSysUserName();
	//?��ñ����д�����������
	static int GetCPUCount();
	//?���ַ���д�뵽������
	static void setClipboardData(HWND hWnd, const std::string& str);
	//?��ȡ����������
	static std::string GetClipboardData(HWND hWnd);
	//?��ȡ��������ֵ
	static std::string GetEnvironment(const char* name);
	//?����Ŀ¼
	static bool CreateDir(const std::string& dir);
	//?����Ŀ¼
	static bool DestroyDir(const std::string& dir);
	//?���Ŀ¼���ļ��Ƿ����
	static bool DirOrFileAccess(const std::string& dir);
	//?��ȡ����̨���
	static HWND GetConsoleHwnd();

public:
    static bool ifRedirFrobid;
    static PVOID oldValue;
};