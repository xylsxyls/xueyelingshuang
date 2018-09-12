#pragma once
#include "CGetPathMacro.h"
#include <vector>
#include <stdint.h>
#include <Windows.h>

class CGetPathAPI CGetPath
{
public:
	//����ocx����·������\����
    static std::string GetRegOcxPath(const std::string& classid);
	//���ر���������·������\����
    static std::string GetCurrentExePath();
	//���ؽ�������û�к�׺��
	static std::string GetCurrentExeName();
	//ͨ�����ڻ�ȡ�ļ���·�������ܻ�ȡ�ļ�·������\���ţ�����MFC���
    static std::string GetFolderFromWindow(HWND hWnd);
	//�������ѡȡ�ļ���ȡ�ļ�·�����ɹ����ؾ���·����ʧ�ܷ���CString�ַ���-1
	static std::string GetFileFromWindow(HWND hwnd);
#ifdef WTL
    static std::std::string GetFileFromWindow(HWND hwnd);
#endif
	//FileStr���ļ������׺�������㣬�������ļ��У��õ��ľ���·���ŵ�vector���������ǵݹ麯��
	//strPath��������봫�ļ��У�Ѱ���ļ������£����������Ѱ�ҵ�ǰ·�������µ������ļ��У�������\\������
	//flag = 1��ʾ�����ļ���2��ʾ�����ļ���׺����3��ʾ���������ļ���FileStr��������
    //pUnVisitPath�ѵ�ǰ�ļ����²��ɷ��ʵ��ļ����г��������մ����洢�������ļ���·����\����
	static std::vector<std::string> FindFilePath(const std::string& FileStr, const std::string& strPath = "", int32_t flag = 1, std::vector<std::string> *pUnVisitPath = NULL);
	//��ȡ����pid�����������
	static std::vector<int32_t> GetProcessID(const std::string& strProcessName);
	//����pid��ȡ���ھ��
    static HWND GetHwndByProcessId(DWORD dwProcessId);
    //��ȡ�ļ������Ϣ��flag1��ʾ��ȡ�ļ�ȫ����flag2��ʾ��ȡ�ļ���׺����flag3��ʾ��ȡ�ļ�����flag4��ʾ��ȡǰ���·����\\����
    static std::string GetName(const std::string& path, int32_t flag);
};