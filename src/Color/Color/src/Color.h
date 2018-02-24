#pragma once
#include "ColorMacro.h"
#include <string>

namespace xyls
{

class ColorAPI Color
{
public:
	/** 构造函数
	@param [in] r 颜色r值
	@param [in] g 颜色g值
	@param [in] b 颜色b值
	*/
	Color(unsigned char r, unsigned char g, unsigned char b);

	/** 构造函数
	@param [in] r 颜色r值
	@param [in] g 颜色g值
	@param [in] b 颜色b值
	@param [in] r 偏色r值
	@param [in] g 偏色g值
	@param [in] b 偏色b值
	*/
	Color(unsigned char r, unsigned char g, unsigned char b, unsigned char rc, unsigned char gc, unsigned char bc);

public:
	/** 输出颜色字符串，字母小写
	@return 返回颜色字符串，字母小写
	*/
	std::string toString() const;

	/** 输出反向颜色字符串，字母小写
	@return 返回反向颜色字符串，字母小写
	*/
	std::string toReserveString() const;

	/** 输出颜色字符串带偏色，字母小写
	@return 返回颜色字符串带偏色，字母小写
	*/
	std::string toCastString() const;

	/** 输出反向颜色字符串带偏色，字母小写
	@return 返回反向颜色字符串带偏色，字母小写
	*/
	std::string toReserveCastString() const;

private:
	int m_r;
	int m_g;
	int m_b;
	int m_rc;
	int m_gc;
	int m_bc;
};

}
