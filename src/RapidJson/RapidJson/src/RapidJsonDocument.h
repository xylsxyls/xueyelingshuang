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
	template <typename Encoding, typename Allocator, typename StackAllocator> class GenericDocument;
	typedef GenericDocument<UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator> Document;
}

class RapidJsonValue;

/** JSON文档封装，表示一个完整JSON根节点并提供常用字段读写接口
*/
class RapidJsonAPI RapidJsonDocument
{
public:
	/** 构造函数，默认创建空对象文档
	*/
	RapidJsonDocument();

	/** 拷贝构造函数，深拷贝JSON文档内容
	@param [in] document 被拷贝的JSON文档
	*/
	RapidJsonDocument(const RapidJsonDocument& document);

	/** 移动构造函数，接管JSON文档内容，源对象会被重置为空对象
	@param [in,out] document 被移动的JSON文档
	*/
	RapidJsonDocument(RapidJsonDocument&& document);

	/** 析构函数，释放内部rapidjson文档对象
	*/
	~RapidJsonDocument();

	/** 赋值函数，深拷贝JSON文档内容
	@param [in] document 被拷贝的JSON文档
	@return 返回当前文档对象引用
	*/
	RapidJsonDocument& operator=(const RapidJsonDocument& document);

	/** 移动赋值函数，接管JSON文档内容，源对象会被重置为空对象
	@param [in,out] document 被移动的JSON文档
	@return 返回当前文档对象引用
	*/
	RapidJsonDocument& operator=(RapidJsonDocument&& document);

	/** 解析JSON字符串
	@param [in] json JSON文本
	@return 返回是否解析成功
	*/
	bool parse(const std::string& json);

	/** 将根节点重置为JSON对象
	*/
	void setObject();

	/** 将根节点重置为JSON数组
	*/
	void setArray();

	/** 判断根节点是否是JSON对象
	@return 返回根节点是否为对象
	*/
	bool isObject() const;

	/** 判断根节点是否是JSON数组
	@return 返回根节点是否为数组
	*/
	bool isArray() const;

	/** 向根对象添加字符串字段，根节点不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addString(const char* key, const std::string& value);

	/** 向根对象添加C字符串字段，根节点不是对象时会自动转为对象，空字符串指针会按空字符串写入
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addString(const char* key, const char* value);

	/** 向根对象添加整数字段，根节点不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addInt(const char* key, int32_t value);

	/** 向根对象添加布尔字段，根节点不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	void addBool(const char* key, bool value);

	/** 向根对象添加JSON值字段，根节点不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] value 已构造的JSON值
	*/
	void addValue(const char* key, const RapidJsonValue& value);

	/** 向根对象添加字符串数组字段，根节点不是对象时会自动转为对象
	@param [in] key 字段名
	@param [in] values 字符串数组
	*/
	void addStringArray(const char* key, const std::vector<std::string>& values);

	/** 读取字符串字段
	@param [in] key 字段名
	@param [in] defaultValue 字段不存在或类型不匹配时返回的默认值
	@return 返回字段字符串值或默认值
	*/
	std::string getStringOrDefault(const char* key, const std::string& defaultValue) const;

	/** 读取整数字段
	@param [in] key 字段名
	@param [in] defaultValue 字段不存在或类型不匹配时返回的默认值
	@return 返回字段整数值或默认值
	*/
	int32_t getIntOrDefault(const char* key, int32_t defaultValue) const;

	/** 读取布尔字段
	@param [in] key 字段名
	@param [in] defaultValue 字段不存在或类型不匹配时返回的默认值
	@return 返回字段布尔值或默认值
	*/
	bool getBoolOrDefault(const char* key, bool defaultValue) const;

	/** 读取对象或数组字段
	@param [in] key 字段名
	@return 字段存在时返回字段值拷贝，否则返回空对象值
	*/
	RapidJsonValue getValue(const char* key) const;

	/** 读取对象数组字段
	@param [in] key 字段名
	@return 返回数组中的对象或值拷贝，字段不存在或类型不匹配时返回空数组
	*/
	std::vector<RapidJsonValue> getArrayValueOrEmpty(const char* key) const;

	/** 读取字符串数组字段
	@param [in] key 字段名
	@return 返回字符串数组，字段不存在或类型不匹配时返回空数组
	*/
	std::vector<std::string> getStringArrayOrEmpty(const char* key) const;

	/** 将整个JSON文档序列化为字符串
	@return 返回JSON文本
	*/
	std::string toString() const;

private:
	// 底层JSON文档对象，由RapidJsonDocument负责释放
	rapidjson::Document* m_document;
};