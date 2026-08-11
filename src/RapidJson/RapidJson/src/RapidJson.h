#pragma once
#include "RapidJsonMacro.h"
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

class RapidJsonAPI RapidJson
{
public:
	/** 构造函数，默认创建空对象文档
	*/
	RapidJson();

	/** 拷贝构造函数，深拷贝文档和值
	@param [in] rapidJson 被拷贝的JSON对象
	*/
	RapidJson(const RapidJson& rapidJson);

	/** 从底层rapidjson值构造对象，传入空指针时创建JSON null
	@param [in] value 底层rapidjson::Value指针，必须在本次调用期间有效，不允许传入悬空指针
	*/
	explicit RapidJson(const rapidjson::Value* value);

	/** 析构函数，释放内部rapidjson对象
	*/
	~RapidJson();

	/** 解析json
	@param [in] json json字符串
	@return 返回是否解析成功
	*/
	bool parse(const std::string& json);

	/** 是否有该成员
	@param [in] key 关键字
	@return 返回是否有关键字成员
	*/
	bool hasMember(const std::string& key);

	/** 转化为字符串
	@return 返回转化的字符串
	*/
	std::string toString();

	/** 判断当前选中值是否为空值
	@return 返回当前选中值是否为空值
	*/
	bool isNull() const;

	/** 判断当前选中值是否为整数
	@return 返回当前选中值是否为整数
	*/
	bool isInt() const;

	/** 读取当前选中整数值
	@param [in] defaultValue 当前选中值不是整数时返回的默认值
	@return 返回当前整数值或默认值
	*/
	int32_t toIntOrDefault(int32_t defaultValue) const;

	/** 读取当前选中字符串值
	@param [in] defaultValue 当前选中值不是字符串时返回的默认值
	@return 返回当前字符串值或默认值
	*/
	std::string toStringOrDefault(const std::string& defaultValue) const;

	/** 获取底层rapidjson::Value指针，供封装内部桥接使用，调用方不能在RapidJson对象析构后继续保存或使用该指针
	@return 返回底层rapidjson::Value指针
	*/
	const rapidjson::Value* toRapidJsonValue() const;

	/** 转化为数组
	@return 返回转化的数组
	*/
	std::vector<RapidJson> toArray();

	/** 将当前值或当前下标字段赋值为整数
	@param [in] value 整数值
	@return 返回赋值后的JSON对象
	*/
	RapidJson& operator=(int32_t value);

	/** 将当前值或当前下标字段赋值为字符串
	@param [in] value 字符串值
	@return 返回赋值后的JSON对象
	*/
	RapidJson& operator=(const std::string& value);

	/** 将当前值或当前下标字段赋值为C字符串，空字符串指针会按空字符串写入
	@param [in] value 字符串值
	@return 返回赋值后的JSON对象
	*/
	RapidJson& operator=(const char* value);

	/** 将当前值或当前下标字段赋值为底层rapidjson值，传入空指针时写入JSON null
	@param [in] value 底层rapidjson::Value指针，必须在本次调用期间有效，不允许传入悬空指针
	@return 返回赋值后的JSON对象
	*/
	RapidJson& operator=(const rapidjson::Value* value);

	/** 将当前下标字段赋值为另一个JSON对象
	@param [in] value JSON对象
	@return 返回赋值后的JSON对象
	*/
	RapidJson& operator=(const RapidJson& value);

	/** 将当前下标字段赋值为JSON对象数组
	@param [in] value JSON对象数组
	@return 返回赋值后的JSON对象
	*/
	RapidJson& operator=(const std::vector<RapidJson>& value);

	/** 选择或创建对象成员，后续赋值会写入该成员，不要在同一表达式中对同一个RapidJson对象多次调用该函数
	@param [in] key 成员键名
	@return 返回当前对象引用
	*/
	RapidJson& operator[](const std::string& key);

private:
	// 底层JSON文档对象，由RapidJson负责释放
	rapidjson::Document* m_document;
	// 当前选中的JSON值，由RapidJson负责释放
	rapidjson::Value* m_value;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// operator[]选中的待写入成员键名
	std::string m_key;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};