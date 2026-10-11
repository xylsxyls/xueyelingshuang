#pragma once
#include <string>
#include <time.h>
#include <stdint.h>
#include "IntDateTimeMacro.h"

/** 整数编码的本地日期时间值，保存YYYYMMDD日期和HHMMSSmmm时间，不持有外部资源
文本解析只拆分并移除分隔符，不校验日期范围；秒运算忽略毫秒并受本地时区及夏令时影响
同一对象的并发读写由调用方同步，内部转换不使用localtime共享静态缓冲区
*/
class IntDateTimeAPI IntDateTime
{
public:
	// 日期整数，格式YYYYMMDD，零与零时间一起表示空值
	int32_t m_date;
	// 时间整数，格式HHMMSSmmm，末三位为毫秒，零也可表示午夜
	int32_t m_time;

	/** 使用系统当前本地时间构造，时间转换失败时保持为空
	*/
	IntDateTime();

	/** 复制日期和时间整数，不校验或转换时区
	@param [in] intDateTime 要复制的日期时间值
	*/
	IntDateTime(const IntDateTime& intDateTime);

	/** 直接保存整数日期和时间，不校验字段范围
	@param [in] date 日期整数，格式YYYYMMDD
	@param [in] time 时间整数，格式HHMMSSmmm，末三位为毫秒
	*/
	IntDateTime(int32_t date, int32_t time);

	/** 解析日期时间文本，空指针或分段不合要求时保持为空
	@param [in] time 零结尾文本，格式YYYY-MM-DD或YYYY-MM-DD HH:MM:SS；只移除分隔符，不校验日期范围
	*/
	IntDateTime(const char* time);

	/** 解析日期时间文本，分段不合要求时保持为空
	@param [in] time 文本格式YYYY-MM-DD或YYYY-MM-DD HH:MM:SS；省略时间时设为零，不校验日期范围
	*/
	IntDateTime(const std::string& time);

	/** 将时间戳转换为本地日期时间，失败时保持为空
	@param [in] time 系统time_t时间戳，单位秒，结果毫秒为零
	*/
	IntDateTime(time_t time);

	/** 复制日期和时间整数
	@param [in] intDateTime 要赋给当前对象的日期时间值
	@return 赋值后的当前对象副本，不是引用
	*/
	IntDateTime operator=(const IntDateTime& intDateTime);

	/** 从文本赋值，分段不合要求时保留原值
	@param [in] intDateTime 文本格式YYYY-MM-DD或YYYY-MM-DD HH:MM:SS；省略时间时设为零，不校验日期范围
	@return 当前对象副本，不是引用
	*/
	IntDateTime operator=(const std::string& intDateTime);

	/** 将时间戳赋为本地日期时间，转换失败时清空
	@param [in] time 系统time_t时间戳，单位秒，结果毫秒为零
	@return 当前对象副本，不是引用
	*/
	IntDateTime operator=(const time_t& time);

    /** 在本地时间基准上增加秒数，忽略原毫秒
    @param [in] intDateTime 原日期时间，调用方保证日期有效
    @param [in] seconds 增加的秒数，可为负数
    @return 新日期时间，毫秒为零；本地时间转换失败时为空
    */
    friend IntDateTime operator+(const IntDateTime& intDateTime, int32_t seconds)
	{
        struct tm time = {};
        time.tm_year = intDateTime.getYear() - 1900;
        time.tm_mon = intDateTime.getMonth() - 1;
        time.tm_mday = intDateTime.getDay();
        time.tm_hour = intDateTime.getHour();
        time.tm_min = intDateTime.getMin();
        time.tm_sec = intDateTime.getSeconds();
		time.tm_isdst = -1;
        time_t timeint64 = mktime(&time);
        IntDateTime result;
        result = timeint64 + seconds;
        return result;
    }

    /** 在本地时间基准上增加秒数，支持秒数在左侧的写法
    @param [in] seconds 增加的秒数，可为负数
    @param [in] intDateTime 原日期时间，调用方保证日期有效
    @return 新日期时间，毫秒为零；本地时间转换失败时为空
    */
    friend IntDateTime operator+(int32_t seconds, const IntDateTime& intDateTime)
	{
		struct tm time = {};
		time.tm_year = intDateTime.getYear() - 1900;
		time.tm_mon = intDateTime.getMonth() - 1;
		time.tm_mday = intDateTime.getDay();
		time.tm_hour = intDateTime.getHour();
		time.tm_min = intDateTime.getMin();
		time.tm_sec = intDateTime.getSeconds();
		time.tm_isdst = -1;
        time_t timeint64 = mktime(&time);
        IntDateTime result;
		result = timeint64 + seconds;
        return result;
    }

    /** 在本地时间基准上减去秒数，忽略原毫秒
    @param [in] intDateTime 原日期时间，调用方保证日期有效
    @param [in] seconds 减去的秒数，可为负数
    @return 新日期时间，毫秒为零；本地时间转换失败时为空
    */
    friend IntDateTime operator-(const IntDateTime& intDateTime, int32_t seconds)
	{
		struct tm time = {};
		time.tm_year = intDateTime.getYear() - 1900;
		time.tm_mon = intDateTime.getMonth() - 1;
		time.tm_mday = intDateTime.getDay();
		time.tm_hour = intDateTime.getHour();
		time.tm_min = intDateTime.getMin();
		time.tm_sec = intDateTime.getSeconds();
		time.tm_isdst = -1;
        time_t timeint64 = mktime(&time);
        IntDateTime result;
		result = timeint64 - seconds;
        return result;
    }

    /** 计算两个本地日期时间的秒差，忽略毫秒并受夏令时影响
    @param [in] intDateTime1 被减日期时间，调用方保证日期有效
    @param [in] intDateTime2 减去的日期时间，调用方保证日期有效
    @return 前者减后者的秒数；强转为int32_t，调用方保证结果不超范围
    */
    friend int32_t operator-(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		struct tm time = {};
		time.tm_year = intDateTime1.getYear() - 1900;
		time.tm_mon = intDateTime1.getMonth() - 1;
		time.tm_mday = intDateTime1.getDay();
		time.tm_hour = intDateTime1.getHour();
		time.tm_min = intDateTime1.getMin();
		time.tm_sec = intDateTime1.getSeconds();
		time.tm_isdst = -1;
        time_t timeint64 = mktime(&time);

        struct tm timeSub = {};
		timeSub.tm_year = intDateTime2.getYear() - 1900;
		timeSub.tm_mon = intDateTime2.getMonth() - 1;
		timeSub.tm_mday = intDateTime2.getDay();
		timeSub.tm_hour = intDateTime2.getHour();
		timeSub.tm_min = intDateTime2.getMin();
		timeSub.tm_sec = intDateTime2.getSeconds();
		timeSub.tm_isdst = -1;
        time_t timeint64Sub = mktime(&timeSub);

		return (int32_t)difftime(timeint64, timeint64Sub);
    }

    /** 按日期整数、时间整数依次比较是否严格小于
    @param [in] intDateTime1 左侧日期时间
    @param [in] intDateTime2 右侧日期时间
    @return 左侧小于右侧时返回true，不转换时区
    */
    friend bool operator< (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return true;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return false;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return true;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return false;
			}
			else
			{
				return false;
			}
        }
    }

    /** 按日期整数、时间整数依次比较是否严格大于
    @param [in] intDateTime1 左侧日期时间
    @param [in] intDateTime2 右侧日期时间
    @return 左侧大于右侧时返回true，不转换时区
    */
    friend bool operator> (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return false;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return true;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return false;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return true;
			}
			else
			{
				return false;
			}
        }
    }

    /** 按日期整数、时间整数依次比较是否小于或等于
    @param [in] intDateTime1 左侧日期时间
    @param [in] intDateTime2 右侧日期时间
    @return 左侧小于或等于右侧时返回true
    */
    friend bool operator<=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return true;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return false;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return true;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return false;
			}
			else
			{
				return true;
			}
        }
    }

    /** 按日期整数、时间整数依次比较是否大于或等于
    @param [in] intDateTime1 左侧日期时间
    @param [in] intDateTime2 右侧日期时间
    @return 左侧大于或等于右侧时返回true
    */
    friend bool operator>=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return false;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return true;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return false;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return true;
			}
			else
			{
				return true;
			}
        }
    }

    /** 比较日期和时间整数是否全部相等
    @param [in] intDateTime1 左侧日期时间
    @param [in] intDateTime2 右侧日期时间
    @return 日期和时间均相等时返回true，包含毫秒
    */
    friend bool operator==(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date == intDateTime2.m_date && intDateTime1.m_time == intDateTime2.m_time)
		{
			return true;
		}
		else
		{
			return false;
		}
    }

    /** 比较日期和时间整数是否存在不同
    @param [in] intDateTime1 左侧日期时间
    @param [in] intDateTime2 右侧日期时间
    @return 日期或时间不同则返回true，包含毫秒
    */
    friend bool operator!=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date == intDateTime2.m_date && intDateTime1.m_time == intDateTime2.m_time)
		{
			return false;
		}
		else
		{
			return true;
		}
    }

    /** 与整数零比较以判断是否为空，非零整数始终不匹配
    @param [in] intDateTime 待判断的日期时间
    @param [in] num 比较整数，仅支持零的空值语义
    @return num为零且日期和时间均为零时返回true
    */
    friend bool operator==(const IntDateTime& intDateTime, int32_t num)
	{
		if (num != 0)
		{
			return false;
		}
		if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
    }

    /** 与整数零比较以判断是否为空，支持整数在左侧的写法
    @param [in] num 比较整数，仅支持零的空值语义
    @param [in] intDateTime 待判断的日期时间
    @return num为零且日期和时间均为零时返回true
    */
    friend bool operator==(int32_t num, const IntDateTime& intDateTime)
	{
		if (num != 0)
		{
			return false;
		}
		if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
    }

	/** 判断日期和时间是否同时为零
	@return 两个整数均为零时返回true，不用于判断日期是否合法
	*/
	bool empty() const;

	/** 将日期和时间整数同时设为零
	*/
	void clear();

	/** 直接设置整数日期和时间，不校验字段范围
	@param [in] date 日期整数，格式YYYYMMDD
	@param [in] time 时间整数，格式HHMMSSmmm，末三位为毫秒
	*/
	void setTime(int32_t date, int32_t time);

	/** 解析文本设置日期时间，分段不合要求时保留原值
	@param [in] time 文本格式YYYY-MM-DD或YYYY-MM-DD HH:MM:SS；省略时间时设为零，不校验日期范围
	*/
	void setTime(const std::string& time);

	/** 将时间戳设置为本地日期时间，转换失败时清空
	@param [in] time 系统time_t时间戳，单位秒，结果毫秒为零
	*/
	void setTime(time_t time);

	/** 获取日期整数
	@return YYYYMMDD编码的原值，空值为零
	*/
	int32_t getDate() const;

	/** 获取时间整数
	@return HHMMSSmmm编码的原值，末三位为毫秒，零可表示午夜或空值
	*/
	int32_t getTime() const;

	/** 从日期整数拆出年份
	@return 日期除以10000所得年份，不校验范围
	*/
	int32_t getYear() const;

	/** 从日期整数拆出月份
	@return 日期中月份字段；有效日期为1至12，空值为零
	*/
	int32_t getMonth() const;

	/** 从日期整数拆出日
	@return 日期末两位；有效日期为1至31，空值为零
	*/
	int32_t getDay() const;

	/** 从时间整数拆出小时
	@return 时间除以10000000所得小时；有效时间为0至23
	*/
	int32_t getHour() const;

	/** 从时间整数拆出分钟
	@return 时间中分钟字段；有效时间为0至59
	*/
	int32_t getMin() const;

	/** 从时间整数拆出秒
	@return 时间中秒字段，不包含毫秒；有效时间为0至59
	*/
	int32_t getSeconds() const;

	/** 格式化完整日期时间，不包含毫秒
	@return 年-月-日 时:分:秒文本；年份按整数输出，其余字段至少两位
	*/
	std::string toString() const;

	/** 格式化带横线的日期
	@return 年-月-日文本；年份按整数输出，月和日至少两位
	*/
	std::string dateToString() const;

	/** 格式化不带分隔符的日期
	@return 年月日文本；年份按整数输出，月和日至少两位，不含时间
	*/
	std::string dateNumToString() const;

	/** 格式化带冒号的时间，不包含毫秒
	@return 时:分:秒文本，各字段至少两位
	*/
	std::string timeToString() const;

private:

	/** 使用可重入系统接口转换本地时间，避免共享静态缓冲区
	@param [in] time 系统time_t时间戳，单位秒
	@param [out] timeInfo 转换结果，仅成功时可使用字段
	@return Windows或Unix/Apple转换成功时返回true；不支持的平台或转换失败返回false
	*/
	static bool localTime(time_t time, struct tm& timeInfo);
};