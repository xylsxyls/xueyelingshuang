#pragma once
#include <vector>
#include <string>
#include <stdint.h>
#include <cstdarg>
#include "CStringManagerMacro.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

// ANSI依赖Windows ACP或桌面GBK区域设置，Android、iOS和Wasm不公开这组接口
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && (!defined(__APPLE__) || !TARGET_OS_IPHONE)
#define CSTRING_MANAGER_HAS_ANSI 1
#else
#define CSTRING_MANAGER_HAS_ANSI 0
#endif

#if CSTRING_MANAGER_HAS_ANSI && !defined(_WIN32)
#include <locale>
#endif

/** 字符串工具集合，提供切分、查找、替换、格式化、大小写、编码转换和URL编解码等通用能力
Android、iOS和Wasm使用UTF8接口，ANSI转换仅在CSTRING_MANAGER_HAS_ANSI为1时公开
*/
class CStringManagerAPI CStringManager
{
public:
    /** 按容量复制字符串并补NUL，支持先查询完整缓冲区大小
    @param [in] value 输入字节串，按字节截断，不保证截断后UTF-8字符完整
    @param [out] buffer 输出缓冲区，可空；允许与value重叠，调用方须保证目标范围可写
    @param [in] capacity 缓冲区容量，0时不写入；非0时最多写capacity-1字节并补NUL
    @return 完整字节数加1（包含NUL），与实际截断长度无关
    @note 缓冲区可与源字符串现有的可写字符区域重叠，调用方须保证 capacity 字节可写。
    */
    static size_t CopyToBuffer(const std::string& value, char* buffer, size_t capacity);

	/** 查找左右成对符号中与指定位置匹配的另一侧位置，从0开始计数
	@param [in] str 要查找的字符串
	@param [in] cLeft 左侧符号
	@param [in] cRight 右侧符号
	@param [in] nSelect 当前选中的左侧或右侧符号位置；左右符号相同时按出现顺序两两配对，不解释转义字符
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

	/** 替换所有匹配，增长替换仅扫描原输入一遍
    长度相同或缩短时重新检查跨边界匹配。增长时跳过已插入的内容，确保调用终止。
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

    /** 格式化窄字符串；格式和参数允许借用原输出，完成后才替换
    @param [in,out] str 输出；空格式指针、格式化或分配失败清空
    @param [in] fmt 有效可信的printf格式，参数类型/地址必须与格式匹配
    */
    static void Format(std::string& str, const char* fmt, ...);

    /** 格式化窄字符串
    @param [in] fmt 有效可信的printf格式；空指针返回空
    @return 完整结果或空；返回值对象本身分配失败仍可能抛异常，边界需捕获
    */
    static std::string Format(const char* fmt, ...);

    /** 按窄字符 printf 格式生成以首个 NUL 结尾的文本，按实际输出长度分配
    @param [in] fmt 可信的格式字符串，可变实参须与格式匹配
    @return 首个 NUL 前的文本，空格式指针或格式化失败时返回空串
    */
    static std::string StrFormat(const char* fmt, ...);

    /** 格式化宽字符串，不经ANSI转换；格式和参数允许借用原输出
    @param [in,out] str 输出；空格式指针、格式化或分配失败清空
    @param [in] fmt 有效可信的宽printf格式；跨平台宽字符串用%ls，宽字符用%lc
    */
    static void Format(std::wstring& str, const wchar_t* fmt, ...);

    /** 格式化宽字符串，不依赖zh_CN.GBK等额外locale安装
    @param [in] fmt 有效可信的宽printf格式；窄字符转换仍遵循系统当前locale
    @return 完整结果或空；返回值对象本身分配失败仍可能抛异常，边界需捕获
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
	@param [in] str 十进制C字符串，可有前导空白、符号和尾随字符
	@return 空指针或无数字返回0；按strtoull处理符号，超出范围返回UINT64_MAX
	*/
	static uint64_t atoui64(const char* str);

	/** C字符串转64位有符号整数
	@param [in] str 十进制C字符串，可有前导空白、符号和尾随字符
	@return 空指针或无数字返回0；溢出按strtoll饱和至INT64_MIN或INT64_MAX
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

    /** 将整数写为十进制或小写十六进制 ASCII 文本
    @param [in] value 待转换的整数
    @param [in] hex 为 true 时按 unsigned int 位模式输出十六进制，无 0x 前缀
    @return 不带分组符或前导零的整数文本
    */
    static std::string IntToStr(int value, bool hex = false);

    /** 解析整数前缀，跳过开头的空白，允许正负号，在首个非数字字节处停止
    @param [in] text 待解析的文本，内嵌 NUL 结束读取
    @param [in] hex 为 true 时使用十六进制，允许 0x 前缀及 unsigned int 范围的绝对值
    @return 十进制有符号值或十六进制对应的 int 位模式，无数字或越界时返回 0
    */
    static int StrToInt(const std::string& text, bool hex = false);

    /** 读取一位 ASCII 十六进制数字
    @param [in] digit 待解析的字符，允许大小写
    @return 数值 0 至 15，其他字符返回 -1
    */
    static int CharToHex(char digit);

    /** 将一个字节写为两位大写十六进制
    @param [in] byte 待编码的字节
    @return 两个 ASCII 字符，必要时前补零
    */
    static std::string Char2Hex(unsigned char byte);

    /** 将二进制数据写为连续的小写十六进制文本
    @param [in] bytes 任意字节串，允许内嵌 NUL
    @return 每个输入字节对应两个字符，空输入返回空串
    */
    static std::string StrToHex(const std::string& bytes);

    /** 按位置将十六进制文本还原为字节
    @param [in] digits 每两个字符对应一个字节，接受大小写
    @return 非法字符占据的半字节取零，末尾的单个字符放在最后一字节的高四位
    */
    static std::string HexToStr(const std::string& digits);

#if CSTRING_MANAGER_HAS_ANSI
    /** Ansi字符串转Unicode宽字符串
    @param [in] strSrc Ansi字符串；Windows使用系统ACP，POSIX使用zh_CN.GBK
    @return 返回Unicode宽字符串，空输入或转换失败返回空字符串；保留内嵌零字符
    @note 结果字符串仍需分配，极低内存时可能抛出分配异常；调用边界必须捕获，失败不泄漏缓冲
    */
    static std::wstring AnsiToUnicode(const std::string& strSrc);

    /** Unicode宽字符串转Ansi字符串
    @param [in] wstrSrc Unicode宽字符串
    @return 返回Ansi字符串，空输入或转换失败返回空字符串；保留内嵌零字符
    @note 结果字符串仍需分配，极低内存时可能抛出分配异常；调用边界必须捕获，失败不泄漏缓冲
    */
    static std::string UnicodeToAnsi(const std::wstring& wstrSrc);

    /** Ansi字符串转UTF-8字符串
    @param [in] strSrc Ansi字符串；Windows使用系统ACP，POSIX使用zh_CN.GBK
    @return 返回UTF-8字符串，空输入或转换失败返回空字符串；保留内嵌零字符
    @note 结果字符串仍需分配，极低内存时可能抛出分配异常；调用边界必须捕获，失败不泄漏缓冲
    */
    static std::string AnsiToUtf8(const std::string& strSrc);

    /** UTF-8字符串转Ansi字符串
    @param [in] strSrc UTF-8字符串
    @return 返回Ansi字符串，空输入或转换失败返回空字符串；保留内嵌零字符
    @note 结果字符串仍需分配，极低内存时可能抛出分配异常；调用边界必须捕获，失败不泄漏缓冲
    */
    static std::string Utf8ToAnsi(const std::string& strSrc);
#endif

    /** Unicode宽字符串转UTF-8字符串
    @param [in] wstrSrc Unicode宽字符串
    @return 返回UTF-8字符串，空输入或转换失败返回空字符串；保留内嵌零字符
    @note 结果字符串仍需分配，极低内存时可能抛出分配异常；调用边界必须捕获，失败不泄漏缓冲
    */
    static std::string UnicodeToUtf8(const std::wstring& wstrSrc);

    /** UTF-8字符串转Unicode宽字符串
    @param [in] strSrc UTF-8字符串
    @return 返回Unicode宽字符串，空输入或转换失败返回空字符串；保留内嵌零字符
    @note 结果字符串仍需分配，极低内存时可能抛出分配异常；调用边界必须捕获，失败不泄漏缓冲
    */
    static std::wstring Utf8ToUnicode(const std::string& strSrc);

    /** 严格解析十进制32位整数，允许首尾空白，不接受尾随垃圾或溢出
    @param [in] text 待解析宽字符串
    @param [in] defaultValue 解析失败返回的值
    @return 完整解析的整数或defaultValue
    */
    static int32_t parseInt32(const std::wstring& text, int32_t defaultValue);

    /** 按经典区域设置严格解析有限浮点数
    @param [in] text 使用小数点的宽字符串，允许首尾空白
    @param [in] defaultValue 非法、溢出或非有限输入的返回值
    @return 完整解析的有限值或defaultValue
    */
    static double parseFiniteDouble(const std::wstring& text, double defaultValue);

    /** 使用经典区域设置输出固定小数位的有限数值，供持久化格式使用
    @param [in] value 有限数值
    @param [in] precision 小数位数，范围0至17
    @return 固定小数点的宽字符串，非法参数返回空串
    */
    static std::wstring formatFixedDouble(double value, int32_t precision);

    /** 严格解码UTF-16LE，兼容16位及32位wchar_t，不依赖系统代码页
    @param [in] bytes 输入字节，允许LE BOM；拒绝BE BOM、奇数长度及孤立代理项
    @param [out] text 成功时替换输出，失败保留原值
    @return 编码有效且转换完成返回true；内存分配异常由调用方处理
    */
    static bool utf16LeToWide(const std::vector<uint8_t>& bytes, std::wstring& text);

    /** 严格编码为UTF-16LE，兼容16位及32位wchar_t
    @param [in] text 输入Unicode宽字符串，拒绝孤立代理项及越界码点
    @param [out] bytes 成功时替换输出，失败保留原值
    @param [in] addBom 是否在开头写入LE BOM
    @return 编码有效且转换完成返回true；内存分配异常由调用方处理
    */
    static bool wideToUtf16Le(const std::wstring& text, std::vector<uint8_t>& bytes, bool addBom = true);

	/** URL编码字符串
	@param [in] sIn 原字符串
	@return ASCII字母和数字原样保留，其余每个字节转为大写百分号编码
	*/
	static std::string UrlEncode(const std::string& sIn);

    /** 按路径规则对字节串做大写百分号编码
    @param [in] path 待编码的字节串，文本通常使用 UTF-8
    @return 保留 ASCII 字母、数字、~-_./:()' 和内嵌 NUL，其余字节编码为 %HH
    */
    static std::string EncodeUrlPath(const std::string& path);

    /** 先将宽字符路径转为 UTF-8，再按 EncodeUrlPath 规则转义
    @param [in,out] path 输入宽字符路径，返回时替换为编码结果
    */
    static void EncodeUnicodeUrl(std::wstring& path);

	/** URL解码字符串
	@param [in] sIn URL编码字符串
	@return 返回URL解码后的字符串，残缺的百分号编码会按原字符保留
	*/
	static std::string UrlDecode(const std::string& sIn);

    /** 使用已有可变参数表格式化窄字符串，供日志等封装转发参数
    @param [in,out] output 成功替换内容，允许格式及字符串实参借用原内容；失败清空
    @param [in] format 目标CRT支持的可信printf格式，实参类型须匹配；为空时清空output
    @param [in] arguments 已初始化且未消费的参数表，本函数不消费原表；调用方仍负责va_end
    遵循目标CRT的格式与区域设置规则，不修改全局locale；分配异常在函数内捕获
    格式非法或实参类型不匹配属于调用错误，不在可恢复失败保证内
    */
    static void formatV(std::string& output, const char* format, std::va_list arguments);

    /** 使用已有可变参数表格式化宽字符串，不经系统ANSI转换
    @param [in,out] output 成功替换内容，允许格式及字符串实参借用原内容；失败清空
    @param [in] format 目标CRT支持的可信宽printf格式，跨平台宽文本使用%ls；为空时清空output
    @param [in] arguments 已初始化且未消费的参数表，本函数不消费原表；调用方仍负责va_end
    非ASCII转换须由宿主提供相应LC_CTYPE；不修改全局locale，分配异常在函数内捕获
    字符与格式遵循目标CRT，跨端使用%ls传入宽文本；非法格式或类型不匹配属于调用错误
    */
    static void formatV(std::wstring& output, const wchar_t* format, std::va_list arguments);

private:
    /** 将窄格式结果写入独立临时存储，由公开入口处理异常和别名
    @param [out] output 不与格式或实参重叠的临时结果
    @param [in] format 目标CRT支持的可信格式，允许空指针
    @param [in] arguments 借用的参数表，各次CRT调用均复制并配对va_end
    */
    static void formatNarrowArguments(std::string& output, const char* format, std::va_list arguments);

    /** 将宽格式结果写入独立临时存储，按需扩容并保留原参数表
    @param [out] output 不与格式或实参重叠的临时结果
    @param [in] format 目标CRT支持的可信宽格式，允许空指针
    @param [in] arguments 借用的参数表，各次CRT调用均复制并配对va_end
    */
    static void formatWideArguments(std::wstring& output, const wchar_t* format, std::va_list arguments);

#ifdef _WIN32
    /** 查询Windows转换长度并将宽字符串写入独立字节串
    @param [in] source 借用至返回的宽字符串，允许内嵌NUL
    @param [in] codePage CP_UTF8或CP_ACP，沿用系统flags=0的替代字符规则
    @return 完整转换结果，空输入、长度非法、系统或分配失败返回空串
    */
    static std::string convertWindowsWide(const std::wstring& source, uint32_t codePage);

    /** 查询Windows转换长度并将字节串写入独立宽字符串
    @param [in] source 借用至返回的字节串，允许内嵌NUL
    @param [in] codePage CP_UTF8或CP_ACP，沿用系统flags=0的替代字符规则
    @return 完整转换结果，空输入、长度非法、系统或分配失败返回空串
    */
    static std::wstring convertWindowsBytes(const std::string& source, uint32_t codePage);

#endif

#if CSTRING_MANAGER_HAS_ANSI && !defined(_WIN32)
    /** 为POSIX的ANSI转换创建GBK区域设置，由locale管理转换facet的生命周期
    @return 拥有zh_CN.GBK转换facet的locale；环境不支持时抛出，由公开转换函数捕获
    */
    static std::locale createAnsiLocale();
#endif
};