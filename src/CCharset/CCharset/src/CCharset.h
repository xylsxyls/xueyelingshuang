#pragma once
#include <string>
#include <stdint.h>
#include "CCharsetMacro.h"

class CCharsetAPI CCharset{
public:
	/***********************************************************************************
	* name       : StrFormat
	* description: 根据传入的格式进行字符串的格式化;该接口只能格式化后长度为512的字符串
	* input      : szFormat:字符串的格式; ...:可变参数;
	* output     : NA
	* return     : 转化后的字符串
	* remark     : NA
	***********************************************************************************/
    static std::string StrFormat (  const char* szFormat, ... );

	/***********************************************************************************
	* name       : IntToStr
	* description: 将整数转化为字符串
	* input      : nValue:要转化的整型数值; bHex:是否是十六进制整型数
	* output     : NA
	* return     : 转化后的字符串
	* remark     : NA
	***********************************************************************************/
    static std::string IntToStr ( int nValue, bool bHex = false );

	/***********************************************************************************
	* name       : StrToInt
	* description: 将整数转化为字符串
	* input      : strSrc:要转化的字符串; bHex:是否是十六进制整型数
	* output     : NA
	* return     : 转化后的整型数
	* remark     : NA
	***********************************************************************************/
	static int StrToInt ( const std::string& strSrc, bool bHex = false );

	/***********************************************************************************
	* name       : CharToHex
	* description: 将字符转化为16进制编码
	* input      : szChar:要转化的字符;
	* output     : NA
	* return     : 返回的十六进制编码
	* remark     : NA
	***********************************************************************************/
	static int CharToHex( const char szChar );

	/***********************************************************************************
	* name       : StrToHex
	* description: 将一般字符串转换为十六进制字符串编码
	* input      : strSrc:要转化的字符串;
	* output     : NA
	* return     : 返回的十六进制编码的字符串
	* remark     : NA
	***********************************************************************************/
	static std::string StrToHex ( const std::string& strSrc );

	/***********************************************************************************
	* name       : HexToStr
	* description: 将十六进制的字符串转化为一般字符串
	* input      : strSrc:要转化的字符串;
	* output     : NA
	* return     : 转化后的字符串
	* remark     : NA
	***********************************************************************************/
	static std::string HexToStr ( const std::string& strSrc );

	/***********************************************************************************
	* name       : Utf8ToUnicode
	* description: 字符集转换: UTF8 - Unicode	
	* input      : strSrc:要转化的Utf8字符集编码的字符串;
	* output     : NA
	* return     : 转化后的Unicode字符集编码的字符串
	* remark     : NA
	***********************************************************************************/
	static std::wstring Utf8ToUnicode ( const std::string& strSrc );

	/***********************************************************************************
	* name       : UnicodeToUtf8
	* description: 字符集转换: Unicode - UTF8	
	* input      : wstrSrc:要转化的Unicode字符集编码的字符串;
	* output     : NA
	* return     : 转化后的UTF8字符集编码的字符串
	* remark     : NA
	***********************************************************************************/
	static std::string UnicodeToUtf8 ( const std::wstring& wstrSrc );

	/***********************************************************************************
	* name       : Utf8ToAnsi
	* description: 字符集转换: UTF8 - Ansi
	* input      : strSrc:要转化的UTF8字符集编码的字符串;
	* output     : NA
	* return     : 转化后的Ansi字符集编码的字符串
	* remark     : NA
	***********************************************************************************/
	static std::string Utf8ToAnsi( const std::string& strSrc );

	/***********************************************************************************
	* name       : AnsiToUtf8
	* description: 字符集转换: Ansi - UTF8
	* input      : strSrc:要转化的Ansi字符集编码的字符串;
	* output     : NA
	* return     : 转化后的UTF8字符集编码的字符串
	* remark     : NA
	***********************************************************************************/
	static std::string AnsiToUtf8 ( const std::string& strSrc );

	/***********************************************************************************
	* name       : AnsiToUnicode
	* description: 字符集转换: Ansi - Unicode
	* input      : strSrc:要转化的Ansi字符集编码的字符串;
	* output     : NA
	* return     : 转化后的Unicode字符集编码的字符串
	* remark     : NA
	***********************************************************************************/
	static std::wstring AnsiToUnicode (const std::string& strSrc );

	/***********************************************************************************
	* name       : UnicodeToAnsi
	* description: 字符集转换: Unicode - Ansi
	* input      : strSrc:要转化的Unicode字符集编码的字符串;
	* output     : NA
	* return     : 转化后的Ansi字符集编码的字符串
	* remark     : NA
	***********************************************************************************/
	static std::string UnicodeToAnsi ( const std::wstring& wstrSrc );

	///***********************************************************************************
	//* name       : Utf8ToBSTR
	//* description: 字符集转换: UTF8 - BSTR
	//* input      : strSrc:要转化的UTF8字符集编码的字符串;
	//* output     : NA
	//* return     : 转化后的的BSTR字符串
	//* remark     : NA
	//***********************************************************************************/
	//static BSTR Utf8ToBSTR( const std::string& strSrc );

	///***********************************************************************************
	//* name       : BSTRToUtf8
	//* description: 字符集转换: BSTR - UTF8
	//* input      : bstrSrc:要转化的BSTR字符串;
	//* output     : NA
	//* return     : 转化后的UTF8字符集编码的字符串
	//* remark     : NA
	//***********************************************************************************/
	//static std::string BSTRToUtf8 ( const BSTR& bstrSrc );

    /** URL路径转码，只能转码搜索关键词部分，比如中文，该转码会替换问号之类的符号，不能传一整串URL
    @param [in, out] unicodeUrl 需要进行转码的部分url
    */
    static void EncodeUnicodeUrl(std::wstring& unicodeUrl);

    /** URLEncode编码
    */
    static std::string UrlEncode(const std::string& str);

    /** 字符转换为16进制
    */
    static std::string Char2Hex(unsigned char c);
};