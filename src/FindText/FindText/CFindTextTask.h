#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>

class CFindTextDlg;

class CFindTextTask : public CTask
{
public:
	CFindTextTask(const std::string& path, const std::string& key, CFindTextDlg* pFindTextDlg);
public:
	virtual void DoTask();

	void PostToClient();

	//?单个路径一次查找
	void FindFromPath(const std::string& path, const std::string& key);

	//? 查找文件名
	//? path [in] 打开文件的路径
	//? key [in] 搜索的关键词
	//? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
	void FindFromFileName(const std::string& path, const std::string& key, bool& bAddFileName);

	//? 打开文件查找
	//? path [in] 打开文件的路径
	//? key [in] 搜索的关键词
	//? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
	void OpenFileFind(const std::string& path, const std::string& key, bool& bAddFileName);

	//?搜索结束
	//? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
	void FindEnd(bool& bAddFileName);

	//?获取utf-8的每行偏移位置
	//? line [in] 行号，从0开始
	//? oneLine [in] 每行内容
	int GetUtf8Offset(int line, const std::string& oneLine);
	//?获取unicode的每行偏移位置
	//? line [in] 行号，从0开始
	//? oneLine [in][out] 每行内容，会在string末尾添加一个\0
	int GetUnicodeOffset(int line, const std::string& oneLine);

	//?按照不同编码格式查找一行并添加
	//? oneLine [in] 搜索的这一行，gbk格式
	//? path [in] 打开文件的路径
	//? key [in] 搜索的关键词
	//? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
	//? line [in] 行号
	//? addstring [out] 要添加的字符串
	//? format [in] 标明此字符串是什么格式的
	//? 返回找到的位置，0开始
	int FindAdd(const std::string& oneLine, const std::string& path, const std::string& key, bool& bAddFileName, int line, std::string& addstring, const std::string& format);

	//?对比并添加字符串
	//? addOffset [in] 需要添加的字符串中关键词偏移量
	//? addstring [in] 需要添加的字符串
	//? showOffset [in] 需要展示的字符集对应所找到关键词的偏移量
	//? path [in] 打开文件的路径
	//? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
	//? ifCurAdd [in][out] 当前是否已经添加
	void CompareShowAdd(int addOffset, const std::string& addstring, int showOffset, const std::string& path, bool& bAddFileName, bool& ifCurAdd);

private:
	std::string m_path;
	std::string m_key;
	std::string m_strFind;
	CFindTextDlg* m_pFindTextDlg = nullptr;

};