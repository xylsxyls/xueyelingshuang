#pragma once
#include "RapidJsonMacro.h"
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace rapidjson
{
	template <typename CharType> struct UTF8;
	class CrtAllocator;
	template <typename BaseAllocator> class MemoryPoolAllocator;
	template <typename Encoding, typename Allocator> class GenericValue;
	template <typename Encoding, typename Allocator, typename StackAllocator> class GenericDocument;
	typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator>> Value;
	typedef GenericDocument<UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator> Document;
}

/** JSON值封装，可表示对象、数组、字符串、整数和布尔值
*/
class RapidJsonAPI RapidJsonValue
{
public:
	/** 构造函数，默认创建空对象值
	*/
	RapidJsonValue();

	/** 拷贝构造函数，深拷贝JSON值内容
	@param [in] value 被拷贝的JSON值
	*/
	RapidJsonValue(const RapidJsonValue& value);

	/** 移动构造函数，接管JSON值内容，源对象会被重置为空对象
	@param [in,out] value 被移动的JSON值
	*/
	RapidJsonValue(RapidJsonValue&& value);

	/** 析构函数，释放内部rapidjson值对象
	*/
	~RapidJsonValue();

	/** 赋值函数，深拷贝JSON值内容
	@param [in] value 被拷贝的JSON值
	@return 返回当前JSON值引用
	*/
	RapidJsonValue& operator=(const RapidJsonValue& value);

	/** 移动赋值函数，接管JSON值内容，源对象会被重置为空对象
	@param [in,out] value 被移动的JSON值
	@return 返回当前JSON值引用
	*/
	RapidJsonValue& operator=(RapidJsonValue&& value);

	/** 将当前值重置为JSON对象
	*/
	void setObject();

	/** 将当前值重置为JSON数组
	*/
	void setArray();

	/** 将当前值设置为字符串
	@param [in] value 字符串内容
	*/
	void setString(const std::string& value);

	/** 将当前值设置为C字符串，空字符串指针会按空字符串写入
	@param [in] value 字符串内容
	*/
	void setString(const char* value);

	/** 将当前值设置为整数
	@param [in] value 整数内容
	*/
	void setInt(int32_t value);

	/** 将当前值设置为布尔值
	@param [in] value 布尔内容
	*/
	void setBool(bool value);

	/** 判断当前值是否是JSON对象
	@return 返回当前值是否为对象
	*/
	bool isObject() const;

	/** 判断当前值是否是JSON数组
	@return 返回当前值是否为数组
	*/
	bool isArray() const;

	/** 获取当前对象成员数或数组元素数
	@return 对象返回成员数，数组返回元素数，其他类型返回0
	*/
	size_t size() const;

	/** 为数组预留容量，当前值不是数组时不生效
	@param [in] capacity 预留元素数量
	*/
	void reserve(size_t capacity);

	/** 向对象添加字符串字段，当前值不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addString(const char* key, const std::string& value);

	/** 向对象添加C字符串字段，当前值不是对象时会自动转为对象，空字符串指针会按空字符串写入
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addString(const char* key, const char* value);

	/** 向对象添加整数字段，当前值不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addInt(const char* key, int32_t value);

	/** 向对象添加布尔字段，当前值不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addBool(const char* key, bool value);

	/** 向对象添加JSON值字段，当前值不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 已构造的JSON值
	*/
	void addValue(const char* key, const RapidJsonValue& value);

	/** 向对象添加字符串数组字段，当前值不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] values 字符串数组
	*/
	void addStringArray(const char* key, const std::vector<std::string>& values);

	/** 向数组追加字符串元素，当前值不是数组时会自动转为数组
	@param [in] value 字符串元素
	*/
	void pushString(const std::string& value);

	/** 向数组追加C字符串元素，当前值不是数组时会自动转为数组，空字符串指针会按空字符串写入
	@param [in] value 字符串元素
	*/
	void pushString(const char* value);

	/** 向数组追加整数元素，当前值不是数组时会自动转为数组
	@param [in] value 整数元素
	*/
	void pushInt(int32_t value);

	/** 向数组追加布尔元素，当前值不是数组时会自动转为数组
	@param [in] value 布尔元素
	*/
	void pushBool(bool value);

	/** 向数组追加JSON值元素，当前值不是数组时会自动转为数组
	@param [in] value 已构造的JSON值
	*/
	void pushValue(const RapidJsonValue& value);

	/** 将当前JSON值序列化为字符串
	@return 返回JSON文本
	*/
	std::string toString() const;

	/** 将当前值拷贝到底层rapidjson值中，供封装类之间传递底层值使用
	@param [out] rapidJsonValue rapidjson::Value指针
	@param [in] rapidJsonAllocator rapidjson分配器指针
	@return 无
	*/
	void copyToRapidJsonValue(rapidjson::Value* rapidJsonValue, rapidjson::Document* rapidJsonDocument) const;

private:
	// 为当前JSON值提供内存分配器的文档对象，由RapidJsonValue负责释放
	rapidjson::Document* m_document;
	// 当前JSON值，由RapidJsonValue负责释放
	rapidjson::Value* m_value;
};