#pragma once
#include "CDumpMacro.h"
#include <string>

/** 会同时输出dump文件和崩溃前堆栈信息记录文件
*/
class CDumpAPI CDump
{
public:
	/** 程序崩溃生成dump文件
	@param [in] dumpFileDir 生成dump的目录，带\符号，传空则在exe同级目录下创建
	@return 返回是否执行成功，只有第一次执行会成功
	*/
	static bool declareDumpFile(const std::string& dumpFileDir = "");
};