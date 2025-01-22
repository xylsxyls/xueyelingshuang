﻿#include "CCharset.h"
#include <windows.h>

#define  STR_BUFFER_SIZE	512
#define  STR_LENGTH_SIZE	20
#define	 INT_FORMAT_D		10
#define  INT_FORMAT_X		16
#define  STR_FORMAT_D		"%d"
#define  STR_FORMAT_X		"%x"
#define  STR_FORMAT_2OX		"%02X"


std::string CCharset::StrFormat (  const char* szFormat, ... )
{
	va_list va;
	va_start ( va, szFormat );
	char szTemp [STR_BUFFER_SIZE] = { 0 };
	vsprintf_s ( szTemp, szFormat, va );
	va_end ( va );
	std::string strRet;
	strRet.assign ( szTemp );

	return strRet;
}

std::string CCharset::IntToStr ( int nValue, bool bHex /*= false*/  )
{
	char szTmp[STR_LENGTH_SIZE];
	if ( bHex )
	{
		_itoa_s( nValue, szTmp, INT_FORMAT_X );
	}
	else
	{
		_itoa_s( nValue, szTmp, INT_FORMAT_D );
	}	

	return std::string( szTmp );
}

int CCharset::StrToInt ( const std::string& strSrc, bool bHex )
{
	int nRet = 0;	
	if ( bHex )
	{
		sscanf_s ( strSrc.c_str(), STR_FORMAT_X, &nRet );
	}
	else
	{
		sscanf_s ( strSrc.c_str(), STR_FORMAT_D, &nRet );
	}	

	return nRet;
}

int CCharset::CharToHex( const char szChar )
{
	if ( '0' <= szChar && '9' >= szChar )
	{
		return szChar - '0';
	}
	if ( 'a' <= szChar && 'f' >= szChar )
	{
		return szChar - 'a' + 10;
	}
	else if ( 'A' <= szChar && 'F' >= szChar )
	{
		return szChar - 'A' + 10;
	}

	return -1;
}

std::string CCharset::StrToHex(const std::string &strData)
{
	std::string strRet;
	strRet.reserve(strData.size() * 2);

	for ( size_t i = 0; i < strData.size(); i++ )
	{
		char szBuffer[10];
		sprintf_s(szBuffer, "%02x", (unsigned char)strData[i]);
		strRet.append(szBuffer, 2);
	}

	return strRet;
}

std::string CCharset::HexToStr(const std::string &strData)
{
	std::string strRet;
	strRet.assign(( strData.size() + 1 ) / 2, '\0');

	for ( size_t i = 0; i < strData.size(); i++ )
	{
		int m = CharToHex( strData[i] );
		if ( m >= 0 )
		{
			if ( i%2 == 0 )
			{
				strRet[i/2] |= (m & 0xf) * 16;
			}
			else
			{
				strRet[i/2] |= (m & 0xf);
			}
		}         		        
	}      

	return strRet;
}

std::wstring CCharset::Utf8ToUnicode ( const std::string& strSrc )
{
	// 分配目标空间 
	int iAllocSize = static_cast<int>( strSrc.size() + 10 );
	WCHAR* pwszBuffer = new WCHAR[ iAllocSize ];
	if ( NULL == pwszBuffer )
	{
		return L"";
	}
	int iCharsRet = MultiByteToWideChar( CP_UTF8, 0, strSrc.c_str(), 
		static_cast<int>( strSrc.size() ),
		pwszBuffer, iAllocSize );
	// 成功
	std::wstring wstrRet;
	if ( 0 < iCharsRet  )
	{
		wstrRet.assign ( pwszBuffer, static_cast<size_t>( iCharsRet ) );
	}

	// 释放内存
	delete[] pwszBuffer;

	return wstrRet;
}

std::string CCharset::UnicodeToUtf8 ( const std::wstring& wstrSrc )
{	
	// 分配目标空间, 一个16位Unicode字符最多可以转为4个字节
	int iAllocSize = static_cast<int>( wstrSrc.size() * 4 + 10 );
	char* pszBuffer = new char[ iAllocSize ];
	if ( NULL == pszBuffer )
	{
		return "";
	}
	int iCharsRet = WideCharToMultiByte( CP_UTF8, 0, wstrSrc.c_str(), 
		static_cast<int>( wstrSrc.size() ),
		pszBuffer, iAllocSize, NULL, NULL );
	// 成功
	std::string strRet;
	if ( 0 < iCharsRet )
	{
		strRet.assign ( pszBuffer, static_cast<size_t>( iCharsRet ) );
	}

	// 释放内存
	delete[] pszBuffer;

	return strRet;
}

std::string CCharset::Utf8ToAnsi( const std::string& strSrc )
{
	std::wstring wstrTemp = Utf8ToUnicode ( strSrc );

	// 分配目标空间, 长度为 Ansi 编码的两倍
	int iAllocSize = static_cast<int>( strSrc.size() * 2 + 10 );
	char* pszBuffer = new char[ iAllocSize ];
	if ( NULL == pszBuffer )
	{
		return "";
	}
	int iCharsRet = WideCharToMultiByte( CP_ACP, 0, wstrTemp.c_str(), 
		static_cast<int>( wstrTemp.size() ),
		pszBuffer, iAllocSize, NULL, NULL );
	// 成功
	std::string strRet;
	if ( 0 < iCharsRet )
	{
		strRet.assign ( pszBuffer, static_cast<size_t>( iCharsRet ) );
	}

	// 释放内存
	delete[] pszBuffer;

	return strRet;
}

std::string CCharset::AnsiToUtf8 ( const std::string& strSrc )
{
	// 分配目标空间, 长度为 Ansi 编码的两倍
	int iAllocSize = static_cast<int>( strSrc.size() * 2 + 10 );
	WCHAR* pwszBuffer = new WCHAR[ iAllocSize ];
	if ( NULL == pwszBuffer )
	{
		return "";
	}
	int iCharsRet = MultiByteToWideChar( CP_ACP, 0, strSrc.c_str(),
		static_cast<int>( strSrc.size() ),
		pwszBuffer, iAllocSize );
	//成功
	std::wstring wstrTemp;
	if ( 0 < iCharsRet )
	{
		wstrTemp.assign ( pwszBuffer, static_cast<size_t>( iCharsRet ) );
	}

	// 释放内存
	delete[] pwszBuffer;

	return UnicodeToUtf8( wstrTemp );
}

std::wstring CCharset::AnsiToUnicode (const std::string& strSrc )
{
	// 分配目标空间
	int iAllocSize = static_cast<int>( strSrc.size() + 10 );
	WCHAR* pwszBuffer = new WCHAR[ iAllocSize ];
	if ( NULL == pwszBuffer )
	{
		return L"";
	}	
	int iCharsRet = MultiByteToWideChar( CP_ACP, 0, strSrc.c_str(), 
		static_cast<int>( strSrc.size() ),
		pwszBuffer, iAllocSize );
	std::wstring wstrRet;
	if ( 0 < iCharsRet )
	{
		wstrRet.assign ( pwszBuffer, static_cast<size_t>( iCharsRet ) );
	}

	delete[] pwszBuffer;

	return wstrRet;
}

std::string CCharset::UnicodeToAnsi ( const std::wstring& wstrSrc )
{
	// 分配目标空间, 一个16位Unicode字符最多可以转为4个字节
	int iAllocSize = static_cast<int>( wstrSrc.size() * 4 + 10 );
	char* pwszBuffer = new char[ iAllocSize ];
	if ( NULL == pwszBuffer )
	{
		return "";
	}	
	int iCharsRet = WideCharToMultiByte( CP_ACP, 0, wstrSrc.c_str(), 
		static_cast<int>( wstrSrc.size() ),
		pwszBuffer, iAllocSize, NULL, NULL );
	std::string strRet;
	if ( 0 < iCharsRet )
	{
		strRet.assign ( pwszBuffer, static_cast<size_t>( iCharsRet ) );
	}

	delete[] pwszBuffer;

	return strRet;
}

//BSTR CCharset::Utf8ToBSTR( const std::string& strSrc )
//{
//	BSTR strRet = NULL;
//
//	//获得目标空间的大小
//	int iSize = static_cast<int>( strSrc.size() );
//	int iAllocSize = MultiByteToWideChar ( CP_UTF8, 0, strSrc.c_str(), iSize, NULL, NULL );
//	if ( 0 < iAllocSize )
//	{
//		strRet = ::SysAllocStringByteLen ( NULL, iAllocSize );
//		if ( NULL != strRet )
//		{
//			int iResult = MultiByteToWideChar( CP_UTF8, 0, strSrc.c_str(), iSize, strRet, iAllocSize );
//			if ( iResult != iAllocSize )
//			{
//				SysFreeString ( strRet );
//				strRet = NULL;
//			}
//		} 
//
//	}            
//	
//	if ( NULL == strRet )
//	{
//		strRet = ::SysAllocString ( L"" );
//	}
//
//	return strRet;
//}

//std::string CCharset::BSTRToUtf8 ( const BSTR& bstrSrc )
//{
//	// 获得目标空间的大小
//	int iAllocSize = WideCharToMultiByte ( CP_UTF8, 0, bstrSrc, -1, NULL, NULL, NULL, FALSE ) - 1;
//	if ( 0 >= iAllocSize )
//	{
//		return "";
//	}
//
//	std::string strRet( iAllocSize, '\0' );
//
//	// 进行转换
//	int nResult = WideCharToMultiByte ( CP_UTF8, 0, bstrSrc, -1, const_cast<char*>( strRet.c_str() ),
//										iAllocSize, NULL, FALSE );
//
//	return strRet;
//}


void CCharset::EncodeUnicodeUrl(std::wstring& unicodeUrl)
{
    std::string utf8Url = UnicodeToUtf8(unicodeUrl);
    std::string encodeUrl = UrlEncode(utf8Url);
    unicodeUrl = Utf8ToUnicode(encodeUrl);
}

std::string CCharset::UrlEncode(const std::string& c)
{
    std::string escaped;
    uint32_t max = (uint32_t)c.length();
    for (uint32_t i = 0; i < max; i++)
    {
        if ((48 <= c[i] && c[i] <= 57) ||//0-9
            (65 <= c[i] && c[i] <= 90) ||//ABC...XYZ
            (97 <= c[i] && c[i] <= 122) || //abc...xyz
            (strchr("~-_./:()'", c[i])))
        {
            escaped.append(&c[i], 1);
        }
        else if (c[i] == ' ')
        {
            escaped.append("%20");
        }
        else
        {
            escaped.append("%");
            escaped.append(Char2Hex(c[i]));//converts char 255 to string "FF"
        }
    }
    return escaped;
}

std::string CCharset::Char2Hex(unsigned char dec)
{
    char dig1 = (dec & 0xF0) >> 4;
    char dig2 = (dec & 0x0F);
    if (0 <= dig1 && dig1 <= 9) dig1 += 48;    //0,48 in ascii
    if (10 <= dig1 && dig1 <= 15) dig1 += 65 - 10; //A,65 in ascii
    if (0 <= dig2 && dig2 <= 9) dig2 += 48;
    if (10 <= dig2 && dig2 <= 15) dig2 += 65 - 10;

    std::string r;
    r.append(&dig1, 1);
    r.append(&dig2, 1);
    return r;
}