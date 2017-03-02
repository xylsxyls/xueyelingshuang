#include "CLog.h"
#include <shlobj.h>
#include "IntDateTime/IntDateTimeAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <thread>
#include <sys/stat.h>
#include <fstream>

CLog::CLog(int loglevel, const string& moduleName, int sizekb){
	this->loglevel = loglevel;
	this->sizekb = sizekb;
#ifdef _UNICODE
	CreateDirectory(L"log", NULL);
#else
	CreateDirectory("log", NULL);
#endif
	fileName = moduleName;
	if (sizekb == -1) fileNumber = -1;
	else fileNumber = 1;
	string strPath = ChooseLogFile();
	log = new ofstream(strPath, ios::app);
	*((ofstream*)log) << "[" + IntDateTime().timeToString() + "] begin" << endl;
	delete (ofstream*)log;
}

CLog::CLog(const string& logPath, int loglevel){
	log = new ofstream(logPath, ios::app);
	this->loglevel = loglevel;
	this->sizekb = -1;
	this->filePath = logPath;
	fileNumber = -1;
	log = new ofstream(filePath, ios::app);
	*((ofstream*)log) << "[" + IntDateTime().timeToString() + "] begin" << endl;
	delete (ofstream*)log;
}

string CLog::ChooseLogFile(){
	if (filePath != "") return filePath;
	//?说明不是自定义文件
	else{
		//?说明不限制大小
		if (sizekb == -1){
			string strPath;
			IntDateTime currentTime;
			string date;
			CStringManager::Format(date, "%d", currentTime.getDate());
			CStringManager::Format(strPath, "%s.log", ("log/" + fileName + date).c_str());
			return strPath;
		}
		else{
			string strPath;
			mutex.lock();
			int fileNumberTemp = fileNumber;
			mutex.unlock();
			while (true){
				IntDateTime currentTime;
				string date;
				CStringManager::Format(date, "%d", currentTime.getDate());
				CStringManager::Format(strPath, "%s%d.log", ("log/" + fileName + date + "_").c_str(), fileNumberTemp);
				long fileSize;
				struct stat stat_buf;
				if (stat(strPath.c_str(), &stat_buf) < 0) fileSize = -1;
				else fileSize = stat_buf.st_size;
				if (fileSize > sizekb * 1024) fileNumberTemp++;
				else break;
			}
			mutex.lock();
			fileNumber = fileNumberTemp;
			mutex.unlock();
			return strPath;
		}
	}
}

void CLog::Print(logLevel logFlag, const string& fileMacro, int lineMacro, const char* format, ...){
	string strPath = ChooseLogFile();
	if ((loglevel & logFlag) == 0) return;
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
	mutex.lock();
	log = new ofstream(strPath, ios::app);
	int nRight = fileMacro.find_last_of("/\\");
	string fileMacroTemp;
	fileMacroTemp = CStringManager::Mid(fileMacro, nRight + 1, fileMacro.length() - nRight - 1);
	//?这里str就是要打印的日志
	*((ofstream*)log) << "[" + IntDateTime().timeToString() + "][" + strFlag + "][ThreadID:" << std::this_thread::get_id() << "][" << fileMacroTemp << "][" << lineMacro << "] : " << str << endl;
	delete (ofstream*)log;
	mutex.unlock();
}