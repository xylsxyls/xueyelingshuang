#pragma once

enum ReplaceTypeEnum
{
	//替换单个文件，一个参数，文件路径
	REPLACE_FILE,
	//替换目录下所有文件，一个参数，目录
	REPLACE_DIR,
	//替换目录下匹配部分文件名的所有文件，3个参数，目录，匹配的文件名，是否区分大小写，1表示区分，0表示不区分
	REPLACE_FILES,
};

enum ReplaceCommandEnum
{
	//匹配替换，两个参数，匹配字符串和替换字符串，区分大小写
	REPLACE,
	//行部分匹配整行替换，两个参数，行部分匹配字符串，整行字符串，区分大小写
	LINE,
	//文件重命名，3个参数，匹配的部分文件名，替换部分文件名的字符串，匹配的部分文件名是否区分大小写，1表示区分，0表示不区分
	NAME,
	//行部分匹配，行内匹配替换，3个参数，部分匹配的内容，行内匹配的字符串，替换的字符串
	PART
};

struct ReplaceParam
{
	//替换类型
	ReplaceTypeEnum m_replaceType;
	//替换方式
	ReplaceCommandEnum m_replaceCommand;
	//单个文件路径
	std::string m_filePath;
	//目录
	std::string m_folder;
	//部分匹配的文件名
	std::string m_fileName;
	//目录下需要修改的匹配文件名是否区分大小写，true表示区分
	bool m_isMatchCase;
	//匹配的字符串
	std::string m_oldStr;
	//替换的字符串
	std::string m_newStr;
	//替换的整行字符串
	std::string m_newLine;
	//匹配的文件名
	std::string m_oldName;
	//替换文件名匹配部分的字符串
	std::string m_newName;
	//替换时匹配的文件名是否区分大小写，true表示区分
	bool m_isNameMatchCase;
	//行内部分匹配内容
	std::string m_oldPart;
	//开始替换的行序号，从1开始
	int32_t m_beginLine;
	//结束替换的行序号，从1开始
	int32_t m_endLine;

	/** 构造函数
	*/
	ReplaceParam()
	{
		m_replaceType = REPLACE_FILE;
		m_replaceCommand = REPLACE;
		m_isMatchCase = true;
		m_isNameMatchCase = true;
		m_beginLine = 0;
		m_endLine = 0;
	}
};

struct ReplaceNameParam
{
	//单个文件路径
	std::string m_filePath;
	//匹配的文件名
	std::string m_oldName;
	//替换文件名匹配部分的字符串
	std::string m_newName;
	//替换时匹配的文件名是否区分大小写，true表示区分
	bool m_isNameMatchCase;

	/** 构造函数
	*/
	ReplaceNameParam()
	{
		m_isNameMatchCase = true;
	}
};

struct ReplaceContentParam
{
	//替换方式
	ReplaceCommandEnum m_replaceCommand;
	//单个文件路径
	std::string m_filePath;
	//匹配的字符串
	std::string m_oldStr;
	//替换的字符串
	std::string m_newStr;
	//替换的整行字符串
	std::string m_newLine;
	//行内部分匹配内容
	std::string m_oldPart;
	//开始替换的行序号，从1开始
	int32_t m_beginLine;
	//结束替换的行序号，从1开始
	int32_t m_endLine;
};