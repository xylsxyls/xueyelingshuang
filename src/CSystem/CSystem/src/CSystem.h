#pragma once
#include <windows.h>
#include <string>
#include <stdint.h>
#include <map>
#include <vector>
#include "CSystemMacro.h"

class CSystemAPI CSystem
{
public:
	//?ʵ��CPU��Ƶ
	static double GetCPUSpeedGHz();
	//?��ȡϵͳ���������δ�С
	static RECT GetTaskbarRect();
	//?��ȡ��Ļ�ֱ���
	static RECT GetWindowResolution();
	//?������Ļ���½����꣬������������
	static POINT taskbarRightBottomPoint();
	//?������Ļ�����
	static POINT screenCenterPoint();
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
	//?������̬�Ķ�ά���飬���ؿմ���ʧ�ܣ���Ϊ����ȫ���ɹ�����
	template <typename TypeClass>
	static TypeClass** CreateDyadicArray(int32_t row, int32_t column);
	//?���ٶ�̬�Ķ�λ���飬�ڲ����п�
	template <typename TypeClass>
	static void DestroyDyadicArray(TypeClass** classPtr, int32_t row);
    //?��ӡmap
	static void OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path = "");
    //?��ӡvector
	static void OutputVector(const std::vector<std::string>& stringVector, const std::string& path = "");
    //?���scanf������
    static void ClearScanf();
	//?��ȡִ�в����б�
	static std::vector<std::string> exeParam();
	//?��ȡ����
	static std::string PasswordScanf();
	//?��ȡ�߳�ID��
	static uint32_t SystemThreadId();
	//?��ȡCPU����
	static int32_t GetCPUCoreCount();
	//?����Ŀ¼�Ŀ����������Զ������ļ��У�ͬ������
	static bool ShellCopy(const char* from, const char* dest);
	//?��ȡ����ϵͳ�汾��
	static int32_t GetSystemVersionNum();
	//?��ȡ����PID
	static int32_t processPid(const std::string processName = "");
	//?��ȡ������
	static std::string processName(int32_t pid);
	//?��ȡ������
	static std::string getComputerName();
public:
    static bool ifRedirFrobid;
    static PVOID oldValue;
};

#include "CSystem.inl"