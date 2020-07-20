#pragma once
#include <windows.h>
#include <string>
#include <stdint.h>
#include <map>
#include <vector>
#include "CSystemMacro.h"

#define SafeDelete(ptr) \
if (ptr != nullptr)\
{\
	delete ptr;\
	ptr = nullptr;\
}

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
	//��ȡ��һ��������ƥ��Ľ���pid����ձ�ʾ��ȡ������
	static int32_t processFirstPid(const std::string& processName = "");
	//?��ȡ����PID����ʱ10�������ң���ձ�ʾ��ȡ������
	static std::vector<int32_t> processPid(const std::string& processName = "");
	//?��ȡ����������ʱ10��������
	static std::string processName(int32_t pid);
	//?��ȡ����������ʱ10��������
	static std::wstring processNameW(int32_t pid);
	//?��ȡ������
	static std::string getComputerName();
	//���ر���������·������\����
	static std::string GetCurrentExePath();
	//���ؽ�������û�к�׺��
	static std::string GetCurrentExeName();
	//��ȡ�ļ������Ϣ��flag1��ʾ��ȡ�ļ�ȫ����flag2��ʾ��ȡ�ļ���׺����flag3��ʾ��ȡ�ļ�����flag4��ʾ��ȡǰ���·����\\����
	static std::string GetName(const std::string& path, int32_t flag);
	//ɾ���ļ�
	static bool deleteFile(const char* path);
	//����̨�������
	static std::string inputString(const std::string& tip);
	//�رս���
	static void killProcess(int32_t pid);
	//��ȡ���е������в�������һ��Ϊ�ļ�·��
	static std::vector<std::string> mainParam();
	//����������Ŀ¼��������Ŀ¼�����ļ��������ļ������ļ����Ŀ¼���Ѵ�������ɾ������Ŀ¼����������Ŀ¼����Ŀ¼
	static bool rename(const std::string& oldPath, const std::string& newPath);
	//�ļ��Ƿ����
	static bool fileExist(const std::string& filePath);
	//�жϵ�ǰ�������Ƿ���
	static bool isMouseLeftDown();
	//�жϵ�ǰ����Ҽ��Ƿ���
	static bool isMouseRightDown();
	//�жϵ�ǰ����м��Ƿ���
	static bool isMouseMidDown();
    //��QRect��4������С��0�ĵ㻻��0
	static RECT rectValid(const RECT& rect);
	//time_tת��Ϊ�ַ���ʱ�䣬������Ǳ���ʱ����Ǹ�������ʱ��
	static std::string timetToStr(time_t timet, bool isLocal = true);
	//��ȡcommon�е��ļ���·��
	static std::string commonFile(const std::string& name);
	//unicodeתansi
	static std::string UnicodeToAnsi(const std::wstring& wstrSrc);
	//ansiתunicode
	static std::wstring AnsiToUnicode(const std::string& strSrc);
	//��ȡ�ļ�
	std::string readFile(const std::string& path);

public:
    static bool ifRedirFrobid;
    static PVOID oldValue;
};

#include "CSystem.inl"