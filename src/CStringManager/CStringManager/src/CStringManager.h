#pragma once
#include <vector>
#include <string>
#include <stdint.h>
#include "CStringManagerMacro.h"

/** 字符串工具集合，提供切分、查找、替换、格式化、大小写、编码转换和URL编解码等通用能力
*/
class CStringManagerAPI CStringManager
{
public:
	/** 查找左右成对符号中与指定位置匹配的另一侧位置，从0开始计数
	@param [in] str 要查找的字符串
	@param [in] cLeft 左侧符号
	@param [in] cRight 右侧符号
	@param [in] nSelect 当前选中的左侧或右侧符号位置
	@return 返回匹配符号的位置，找不到时返回std::string::npos
	*/
	static size_t FindOther(const std::string& str, char cLeft, char cRight, size_t nSelect);

	/** 按字符串分隔符切分字符串，保留空字段
	@param [in] splitString 要切分的字符串
	@param [in] separate_character 分隔符字符串
	@return 返回切分后的字符串列表
	*/
	static std::vector<std::string> split(const std::string& splitString, const std::string& separate_character);

	/** 按字符分隔符切分字符串，保留空字段
	@param [in] result 输出切分后的字符串列表
	@param [in] splitString 要切分的字符串
	@param [in] separate_character 分隔符字符
	*/
	static void split(std::vector<std::string>& result, const std::string& splitString, char separate_character);

	/** 按宽字符串分隔符切分宽字符串，保留空字段
	@param [in] splitString 要切分的宽字符串
	@param [in] separate_character 分隔符宽字符串
	@return 返回切分后的宽字符串列表
	*/
	static std::vector<std::wstring> split(const std::wstring& splitString, const std::wstring& separate_character);

	/** 循环替换字符串，直到结果中不再存在旧字符串；如果新字符串包含旧字符串，则只做单轮替换避免无限增长
	@param [in,out] str 被修改的字符串
	@param [in] old_value 要替换的旧字符串
	@param [in] new_value 替换后的新字符串
	*/
	static void ReplaceAll(std::string& str, const std::string& old_value, const std::string& new_value);

	/** 截取字符串中间一段
	@param [in] str 原字符串
	@param [in] offset 起始下标
	@param [in] count 截取长度
	@return 返回截取结果，起始下标越界时返回空字符串
	*/
	static std::string Mid(const std::string& str, size_t offset, size_t count);

	/** 截取字符串左侧指定长度
	@param [in] str 原字符串
	@param [in] count 截取长度
	@return 返回左侧字符串，长度超过原字符串时返回整个字符串
	*/
	static std::string Left(const std::string& str, size_t count);

	/** 截取字符串右侧指定长度
	@param [in] str 原字符串
	@param [in] count 截取长度
	@return 返回右侧字符串，长度超过原字符串时返回整个字符串
	*/
	static std::string Right(const std::string& str, size_t count);

	/** 从指定位置开始查找字符串
	@param [in] str 原字符串
	@param [in] right 要查找的字符串
	@param [in] offset 起始下标
	@return 返回匹配位置，找不到时返回std::string::npos
	*/
	static size_t Find(const std::string& str, const std::string& right, size_t offset = 0);

	/** 从指定位置开始查找字符
	@param [in] str 原字符串
	@param [in] ch 要查找的字符
	@param [in] offset 起始下标
	@return 返回匹配位置，找不到时返回std::string::npos
	*/
	static size_t Find(const std::string& str, char ch, size_t offset = 0);

	/** 从右向左查找字符串
	@param [in] str 原字符串
	@param [in] right 要查找的字符串
	@return 返回匹配位置，找不到时返回std::string::npos
	*/
	static size_t ReserveFind(const std::string& str, const std::string& right);

	/** 从右向左查找字符
	@param [in] str 原字符串
	@param [in] ch 要查找的字符
	@return 返回匹配位置，找不到时返回std::string::npos
	*/
	static size_t ReserveFind(const std::string& str, char ch);

	/** 获取字符串长度
	@param [in] str 原字符串
	@return 返回字符串长度
	*/
	static size_t GetLength(const std::string& str);

	/** 删除字符串中的一段内容
	@param [in,out] str 被修改的字符串
	@param [in] offset 起始下标
	@param [in] count 删除长度
	@return 返回删除后的字符串长度
	*/
	static size_t Delete(std::string& str, size_t offset, size_t count);

	/** 在指定位置插入字符串
	@param [in,out] str 被修改的字符串
	@param [in] offset 插入位置，超过长度时追加到末尾
	@param [in] right 要插入的字符串
	@return 返回插入后的字符串长度
	*/
	static size_t Insert(std::string& str, size_t offset, const std::string& right);

	/** 在指定位置插入字符
	@param [in,out] str 被修改的字符串
	@param [in] offset 插入位置，超过长度时追加到末尾
	@param [in] ch 要插入的字符
	@return 返回插入后的字符串长度
	*/
	static size_t Insert(std::string& str, size_t offset, char ch);

	/** 替换字符串中的全部旧字符串
	@param [in,out] str 被修改的字符串
	@param [in] oldstr 要替换的旧字符串
	@param [in] newstr 替换后的新字符串
	@return 返回替换次数
	*/
	static size_t Replace(std::string& str, const std::string& oldstr, const std::string& newstr);

	/** 替换C字符串中的全部旧字符串
	@param [in] str 原C字符串，允许为空指针
	@param [in] oldstr 要替换的旧字符串
	@param [in] newstr 替换后的新字符串
	@return 返回替换后的新字符串
	*/
	static std::string Replace(const char* str, const std::string& oldstr, const std::string& newstr);

	/** 替换宽字符串中的全部旧字符串
	@param [in,out] str 被修改的宽字符串
	@param [in] oldstr 要替换的旧宽字符串
	@param [in] newstr 替换后的新宽字符串
	@return 返回替换次数
	*/
	static size_t Replace(std::wstring& str, const std::wstring& oldstr, const std::wstring& newstr);

	/** 替换字符串中的全部指定字符
	@param [in,out] str 被修改的字符串
	@param [in] ch1 要替换的旧字符
	@param [in] ch2 替换后的新字符
	@return 返回替换次数
	*/
	static size_t Replace(std::string& str, char ch1, char ch2);

	/** 格式化字符串并写入输出参数
	@param [in,out] str 输出字符串
	@param [in] fmt printf风格格式字符串
	*/
	static void Format(std::string& str, const char* fmt, ...);

	/** 格式化字符串
	@param [in] fmt printf风格格式字符串
	@return 返回格式化结果
	*/
	static std::string Format(const char* fmt, ...);

	/** 格式化宽字符串并写入输出参数，Linux下会先按Ansi格式化再转宽字符串
	@param [in,out] str 输出宽字符串
	@param [in] fmt printf风格宽格式字符串
	*/
	static void Format(std::wstring& str, const wchar_t* fmt, ...);

	/** 格式化宽字符串，Linux下会先按Ansi格式化再转宽字符串
	@param [in] fmt printf风格宽格式字符串
	@return 返回格式化结果
	*/
	static std::wstring Format(const wchar_t* fmt, ...);

	/** 原地反转字符串
	@param [in,out] str 被修改的字符串
	*/
	static void MakeReverse(std::string& str);

	/** 转换为大写字符串
	@param [in] src 原字符串
	@return 返回转换后的字符串
	*/
	static std::string MakeUpper(const std::string& src);

	/** 转换为小写字符串
	@param [in] src 原字符串
	@return 返回转换后的字符串
	*/
	static std::string MakeLower(const std::string& src);

	/** 转换为大写宽字符串
	@param [in] src 原宽字符串
	@return 返回转换后的宽字符串
	*/
	static std::wstring MakeUpper(const std::wstring& src);

	/** 转换为小写宽字符串
	@param [in] src 原宽字符串
	@return 返回转换后的宽字符串
	*/
	static std::wstring MakeLower(const std::wstring& src);

	/** 取出两个字符串中间的内容，不包括头尾标记
	@param [in] src 原字符串
	@param [in] leftString 左侧标记字符串
	@param [in] rightString 右侧标记字符串
	@return 返回中间内容，标记不存在或顺序非法时返回空字符串
	*/
	static std::string GetMidString(const std::string& src, const std::string& leftString, const std::string& rightString);

	/** C字符串转64位无符号整数
	@param [in] str 数字字符串，空指针返回0
	@return 返回转换后的整数，失败返回0
	*/
	static uint64_t atoui64(const char* str);

	/** C字符串转64位有符号整数
	@param [in] str 数字字符串，空指针返回0
	@return 返回转换后的整数，失败返回0
	*/
	static int64_t atoi64(const char* str);

	/** 32位有符号整数转字符串
	@param [in] value 要转换的整数
	@return 返回十进制字符串
	*/
	static std::string toStringInt32(int32_t value);

	/** 64位有符号整数转字符串
	@param [in] value 要转换的整数
	@return 返回十进制字符串
	*/
	static std::string toStringInt64(int64_t value);

	/** Ansi字符串转Unicode宽字符串
	@param [in] strSrc Ansi字符串
	@return 返回Unicode宽字符串，转换失败返回空字符串
	*/
	static std::wstring AnsiToUnicode(const std::string& strSrc);

	/** Unicode宽字符串转Ansi字符串
	@param [in] wstrSrc Unicode宽字符串
	@return 返回Ansi字符串，转换失败返回空字符串
	*/
	static std::string UnicodeToAnsi(const std::wstring& wstrSrc);

	/** Ansi字符串转UTF-8字符串
	@param [in] strSrc Ansi字符串
	@return 返回UTF-8字符串，转换失败返回空字符串
	*/
	static std::string AnsiToUtf8(const std::string& strSrc);

	/** UTF-8字符串转Ansi字符串
	@param [in] strSrc UTF-8字符串
	@return 返回Ansi字符串，转换失败返回空字符串
	*/
	static std::string Utf8ToAnsi(const std::string& strSrc);

	/** Unicode宽字符串转UTF-8字符串
	@param [in] wstrSrc Unicode宽字符串
	@return 返回UTF-8字符串，转换失败返回空字符串
	*/
	static std::string UnicodeToUtf8(const std::wstring& wstrSrc);

	/** UTF-8字符串转Unicode宽字符串
	@param [in] strSrc UTF-8字符串
	@return 返回Unicode宽字符串，转换失败返回空字符串
	*/
	static std::wstring Utf8ToUnicode(const std::string& strSrc);

	/** URL编码字符串
	@param [in] sIn 原字符串
	@return 返回URL编码后的字符串
	*/
	static std::string UrlEncode(const std::string& sIn);

	/** URL解码字符串
	@param [in] sIn URL编码字符串
	@return 返回URL解码后的字符串，残缺的百分号编码会按原字符保留
	*/
	static std::string UrlDecode(const std::string& sIn);
};