#include "CLog.h"
#include <shlobj.h>
#include "IntDateTime/IntDateTimeAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <thread>
#include <sys/stat.h>
#include <fstream>
#include <tchar.h>

CLog* logThis = NULL;

int CLog::PrivateCreateLogApi(int loglevel, const string& moduleName, int sizekb){
	if (logThis == NULL) logThis = new CLog(loglevel, moduleName, sizekb);
	return (int)logThis;
}

string wstring2string(const wstring& wstr){
	string result;
	//?获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	//?宽字节编码转换成多字节编码
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	//?删除缓冲区并返回值
	result.append(buffer);
	delete[] buffer;
	return result;
}

CLog::CLog(int loglevel, const string& moduleName, int sizekb){
	SaveLogLevel(loglevel);
	SaveModuleName(moduleName);
	SaveSizekb(sizekb);
	string strPath = ChooseLogFile();
	log = new ofstream(strPath, ios::app);
	*((ofstream*)log) << "[" + IntDateTime().timeToString() + "] begin" << endl;
	strPathPre = strPath;
	//delete (ofstream*)log;
}

CLog::CLog(const string& logPath, int loglevel){
	log = new ofstream(logPath, ios::app);
	this->loglevel = loglevel;
	this->sizekb = -1;
	this->filePath = logPath;
	fileNumber = -1;
	log = new ofstream(filePath, ios::app);
	*((ofstream*)log) << "[" + IntDateTime().timeToString() + "] begin" << endl;
	//delete (ofstream*)log;
}

void CLog::SaveLogLevel(int loglevel){
	this->loglevel = loglevel;
}

void CLog::SaveModuleName(const string& moduleName){
#ifdef _UNICODE
	CreateDirectory(L"log", NULL);
#else
	CreateDirectory("log", NULL);
#endif
	if (moduleName == ""){
		string temp = "";
		TCHAR szFilePath[MAX_PATH + 1] = {};
		GetModuleFileName(NULL, szFilePath, MAX_PATH);
#ifdef _UNICODE
		temp = wstring2string(wstring(szFilePath));
#else
		temp = szFilePath;
#endif
		int nTemp = temp.find_last_of("/\\");
		fileName = temp.substr(nTemp + 1, temp.length() - nTemp - 1);
	}
	else fileName = moduleName;
}

void CLog::SaveSizekb(int sizekb){
	this->sizekb = sizekb;
	if (sizekb == -1) fileNumber = -1;
	else fileNumber = 1;
}

string CLog::ChooseLogFile(){
	if (filePath != "") return filePath;
	//?说明不是自定义文件
	else{
		//?说明不限制大小
		if (sizekb == -1){
			string strPath;
			string date;
			CStringManager::Format(date, "%d", IntDateTime().getDate());
			CStringManager::Format(strPath, "%s.log", ("log/" + fileName + date).c_str());
			return strPath;
		}
		else{
			string strPath;
			int fileNumberTemp = fileNumber;
			while (true){
				string date;
				CStringManager::Format(date, "%d", IntDateTime().getDate());
				CStringManager::Format(strPath, "%s%d.log", ("log/" + fileName + date + "_").c_str(), fileNumberTemp);
				long fileSize;
				struct stat stat_buf;
				if (stat(strPath.c_str(), &stat_buf) < 0) fileSize = -1;
				else fileSize = stat_buf.st_size;
				if (fileSize > sizekb * 1024) fileNumberTemp++;
				else break;
			}
			fileNumber = fileNumberTemp;
			return strPath;
		}
	}
}

void CLog::CheckAndChangeLogFile(const string& strPath){
	if (strPath == strPathPre) return;
	else{
		delete (ofstream*)log;
		log = new ofstream(strPath, ios::app);
		strPathPre = strPath;
	}
}

void CLog::Print(logLevel logFlag, const string& fileMacro, int lineMacro, const char* format, ...){
	string strPath = ChooseLogFile();
	mutex.lock();
	CheckAndChangeLogFile(strPath);
	mutex.unlock();
	if ((this->loglevel & logFlag) == 0) return;
	string strFlag;
	if (logFlag == INFO) strFlag = " INFO";
	if (logFlag == DBG) strFlag = "DEBUG";
	if (logFlag == WARNING) strFlag = "WARNING";
	if (logFlag == ERR) strFlag = "ERROR";
	if (logFlag == FATAL) strFlag = "FATAL";
	string str;
	va_list args = NULL;
	va_start(args, format);
	int size = _vscprintf(format, args);
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	str.resize(size);
	//?即便分配了足够内存，长度必须加1，否则会崩溃
	vsprintf_s(&str[0], size + 1, format, args);
	va_end(args);
	string fileMacroTemp;
	int nRight = fileMacro.find_last_of("/\\");
	fileMacroTemp = CStringManager::Mid(fileMacro, nRight + 1, fileMacro.length() - nRight - 1);
	mutex.lock();
	//?这里str就是要打印的日志
	*((ofstream*)log) << "[" + IntDateTime().timeToString() + "][" + strFlag + "][ThreadID:" << std::this_thread::get_id() << "][" << fileMacroTemp << "][" << lineMacro << "] : " << str << endl;
	mutex.unlock();
	this;
}