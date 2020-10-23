#pragma once
#ifdef _WIN32
#include <windows.h>
#endif
#include <string>
#include <stdint.h>
#include <map>
#include <vector>
#include "CSystemMacro.h"

class CSystemAPI CSystem
{
public:
#ifdef _WIN32
	//��ȡϵͳ���������δ�С
	static RECT GetTaskbarRect();
	//��ȡ��Ļ�ֱ���
	static RECT GetWindowResolution();
	//������Ļ���½����꣬������������
	static POINT taskbarRightBottomPoint();
	//������Ļ�����
	static POINT screenCenterPoint();
	//���ַ���д�뵽������
	static void setClipboardData(HWND hWnd, const std::string& str);
	//��ȡ����������
	static std::string GetClipboardData(HWND hWnd);
	//��ȡ����̨���
	static HWND GetConsoleHwnd();
	//����pid��ȡ���ھ��
	static HWND GetHwndByProcessId(uint32_t dwProcessId);
	//����ocx����·������\����
	static std::string GetRegOcxPath(const std::string& classid);
	//�����ض���64λϵͳ����System32ʱ����뵽SysWOW64��
	static void* ForbidRedir();
	//�ָ��ض���
	static void RecoveryRedir(void* oldValue);
	//��ȡȥ����������������ʱ���������������Ļ�߶�
	static int GetVisibleHeight();
	//��ϵͳ�ļ���
	static void OpenFolder(const std::string& folder);
	//��ϵͳ�ļ��в�ѡ���ļ�
	static void OpenFolderAndSelectFile(const std::string& file);
	//�ⲿ���ļ�
	static void OpenFile(const std::string& file);
	//�ⲿ����ҳ
	static void OpenWebPage(const std::string& webPage);
	//�жϵ�ǰ�������Ƿ���
	static bool isMouseLeftDown();
	//�жϵ�ǰ����Ҽ��Ƿ���
	static bool isMouseRightDown();
	//�жϵ�ǰ����м��Ƿ���
	static bool isMouseMidDown();
	//����Ŀ¼�Ŀ����������Զ������ļ��У�ͬ������
	static bool ShellCopy(const char* from, const char* dest);
	//��ȡ��һ��������ƥ��Ľ���pid
	static uint32_t processFirstPid(const std::wstring& processNameW);
	//��ȡ����PID����ʱ10��������
	static std::vector<uint32_t> processPid(const std::wstring& processNameW);
	//��ȡ����������ʱ10��������
	static std::wstring processNameW(uint32_t pid);
#endif
	//ʵ��CPU��Ƶ��GHzλ��λ
	static double GetCPUSpeedGHz();
	//��ƽ̨Sleep������
	static void Sleep(long long milliseconds);
	//����UUID������1��ʾ��-����0��ʾ������Сд
	static std::string uuid(int flag = 1);
	//�����ļ�
	static void CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over);
	//��ȡ��ǰ����ϵͳλ��
	static int GetSystemBits();
	//��ȡ��ǰϵͳ�û���������Administrator
	static std::string GetSysUserName();
	//��ȡ��������ֵ
	static std::string GetEnvironment(const char* name);
	//����Ŀ¼��Ŀ¼���ڻ��ߴ����༶�����ڵ�Ŀ¼�᷵��false��������\������
	static bool CreateDir(const std::string& dir);
	//����Ŀ¼��ֻ��ɾ�����ļ��У�������\������
	static bool DestroyDir(const std::string& dir);
	//���Ŀ¼���ļ��Ƿ���ڣ����Ŀ¼������\�����ԣ�ͬһĿ¼ԭ���Ͳ����ܳ����ļ��к��ļ����������
	static bool DirOrFileExist(const std::string& dir);
	//������̬�Ķ�ά���飬���ؿմ���ʧ�ܣ���Ϊ����ȫ���ɹ�����
	template <typename TypeClass>
	static TypeClass** CreateDyadicArray(int32_t row, int32_t column);
	//���ٶ�̬�Ķ�λ���飬�ڲ����п�
	template <typename TypeClass>
	static void DestroyDyadicArray(TypeClass** classPtr, int32_t row);
	//��RECT��4������С��0�ĵ㻻��0
	template <typename TRECT>
	static TRECT rectValid(const TRECT& rect);
	//��QRect��4������С��0�ĵ㻻��0
	template <typename TQRect>
	static TQRect qrectValid(const TQRect& rect);
    //��ӡmap
	static void OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path = "");
    //��ӡvector
	static void OutputVector(const std::vector<std::string>& stringVector, const std::string& path = "");
    //���scanf������
    static void ClearScanf();
	//��ȡִ�в����б�windows�²����ȡ������linux�»��ȡ����һ���ǿ�ִ���ļ��ľ���·��
	static std::vector<std::string> exeParam(int argc = 0, char** argv = nullptr);
	//��ȡ����
	static std::string PasswordScanf();
	//��ȡϵͳ����ִ�н���������false��ֻ��ִ��һ�䣬�����true����|findstr
	static int32_t SystemCommand(const std::string& command, std::string& result, bool isShowCmd = false);
	//��ȡ�߳�ID��
	static uint32_t SystemThreadId();
	//��ȡCPU�߳�����˫�����̼߳���ȡ��4���ĺ����̻߳�ȡ��4�����豸��������CPU����һ��
	//�̳߳���������Ǵ�������ֱ��ʹ���߳���������������ݿ����ݽ������ļ��ϴ����ء��������ݴ�����ЩIO���������һ��ϵ����һ����2
	static int32_t GetCPUCoreCount();
	//��ȡ����ϵͳ�汾�ţ�linux��Ϊ1604��1804...
	static int32_t GetSystemVersionNum();
	//��ȡ������pid
	static uint32_t currentProcessPid();
	//��ȡ��һ��������ƥ��Ľ���pid��linux��Ϊ���һ���򿪵ĸ����ֵĽ���
	static uint32_t processFirstPid(const std::string& processName);
	//��ȡ����PID����ʱ10�������ң�linux�º�򿪵Ľ��̺���ǰ��
	static std::vector<uint32_t> processPid(const std::string& processName);
	//��ȡ����������ʱ10��������
	static std::string processName(uint32_t pid);
	//��ȡ������
	static std::string getComputerName();
	//���ر���������·������\����
	static std::string GetCurrentExePath();
	//���ؽ�������û�к�׺��
	static std::string GetCurrentExeName();
	//��ȡ��ʱĿ¼����\����
	static std::string GetSystemTempPath();
	//��ȡ�ļ������Ϣ��flag1��ʾ��ȡ�ļ�ȫ����flag2��ʾ��ȡ�ļ���׺����flag3��ʾ��ȡ�ļ�����flag4��ʾ��ȡǰ���·����\\����
	static std::string GetName(const std::string& path, int32_t flag);
	//ɾ���ļ���windows��ֻ��ɾ���ļ�����ɾ���ļ��У�linux�¿���ɾ���ļ�����ļ��У�������\������
	static bool deleteFile(const char* path);
	//����̨����ʾ����
	static std::string inputString(const std::string& tip);
	//�رս���
	static void killProcess(int32_t pid);
	//����������Ŀ¼��������Ŀ¼�����ļ��������ļ������ļ����Ŀ¼���Ѵ�������ɾ������Ŀ¼����������Ŀ¼����Ŀ¼����Ŀ¼�����ǿ�Ŀ¼���������ɹ�
	//��Ŀ¼�����ǲ����ڵ�Ŀ¼���������Զ༶�����ڣ���Ŀ¼�ڲ��������ļ����ļ��У����ļ����Բ����ڣ���Ŀ¼�����ļ����������ɹ���ᱻɾ��
	static bool rename(const std::string& oldPath, const std::string& newPath);
	//time_tת��Ϊ�ַ���ʱ�䣬������Ǳ���ʱ����Ǹ�������ʱ��
	static std::string timetToStr(time_t timet, bool isLocal = true);
	//��ȡcommon�е��ļ���·��
	static std::string commonFile(const std::string& name);
	//��ȡ�ļ�
	static std::string readFile(const std::string& path);
	//�����ļ�
	static void saveFile(const std::string& content, const std::string& path);
	/** �����ļ����ڲ�û�н�ֹ·���ض���
	@param [in] strPath ���봫�ļ���·����Ѱ���ļ������£���������ַ��������exe����·����������\������
	@param [in] flag 1��ʾ�����ļ���2��ʾ�����ļ���׺����3��ʾ���������ļ���fileStr��������
	@param [in] fileStr ���ļ�������׺�����׺�������㣬�������ļ���
	@param [in] EveryFilePath ��ÿ���һ���ļ�·��֮ǰ���ô˺���
	@param [in] unVisitPath �ѵ�ǰ�ļ����²��ɷ��ʵ��ļ����г��������մ����洢�������ļ���·����\����
	@return �������в��ҵ��ļ��ľ���·��
	*/
	static std::vector<std::string> findFilePath(const std::string& strPath,
		int32_t flag = 3,
		const std::string& fileStr = "",
		void (*EveryFilePath)(const std::string&) = nullptr,
		std::vector<std::string>* unVisitPath = nullptr);
};

#include "CSystem.inl"