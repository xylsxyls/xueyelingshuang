#pragma once
#include "RapidJsonCppMacro.h"
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

/** 拥有根文档与当前选中值，下标选择根对象成员，赋值后可继续读取该成员
*/
class RapidJsonCppAPI RapidJsonCpp
{
public:
	/** 构造函数，默认创建空对象文档
	*/
	RapidJsonCpp();

	/** 拷贝构造函数，深拷贝文档和值
	@param [in] rapidJson 被拷贝的JSON对象
	*/
	RapidJsonCpp(const RapidJsonCpp& rapidJson);

	/** 从底层rapidjson值构造对象，传入空指针时创建JSON null
	@param [in] value 底层rapidjson::Value指针，必须在本次调用期间有效，不允许传入悬空指针
	*/
	explicit RapidJsonCpp(const rapidjson::Value* value);

	/** 析构函数，释放内部rapidjson对象
	*/
	~RapidJsonCpp();

	/** 解析json
	@param [in] json 完整 UTF-8 JSON，可含转义的 NUL
	@return 返回是否解析成功
	*/
	bool parse(const std::string& json);

	/** 是否有该成员
	@param [in] key 关键字
	@return 返回是否有关键字成员
	*/
	bool hasMember(const std::string& key);

	/** 将整个根文档序列化为紧凑 JSON
	@return 返回 JSON 文本，含非法 UTF-8 或非有限数时返回空字符串
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

	/** 借用当前选中值，后续选择或写入会更新它，节点内部指针和迭代器也会失效
	@return 返回随所有者析构而失效的只读指针
	*/
	const rapidjson::Value* toRapidJsonValue() const;

	/** 转化为数组
	@return 返回转化的数组
	*/
	std::vector<RapidJsonCpp> toArray();

	/** 将当前值或当前下标字段赋值为整数
	@param [in] value 整数值
	@return 返回赋值后的JSON对象
	*/
	RapidJsonCpp& operator=(int32_t value);

	/** 将当前值或当前下标字段赋值为字符串
	@param [in] value 字符串值
	@return 返回赋值后的JSON对象
	*/
	RapidJsonCpp& operator=(const std::string& value);

	/** 将当前值或当前下标字段赋值为C字符串，空字符串指针会按空字符串写入
	@param [in] value 字符串值
	@return 返回赋值后的JSON对象
	*/
	RapidJsonCpp& operator=(const char* value);

	/** 将当前值或当前下标字段赋值为底层rapidjson值，传入空指针时写入JSON null
	@param [in] value 底层rapidjson::Value指针，必须在本次调用期间有效，不允许传入悬空指针
	@return 返回赋值后的JSON对象
	*/
	RapidJsonCpp& operator=(const rapidjson::Value* value);

	/** 将当前下标字段赋值为另一个JSON对象
	@param [in] value JSON对象
	@return 返回赋值后的JSON对象
	*/
	RapidJsonCpp& operator=(const RapidJsonCpp& value);

	/** 将当前下标字段赋值为JSON对象数组
	@param [in] value JSON对象数组
	@return 返回赋值后的JSON对象
	*/
	RapidJsonCpp& operator=(const std::vector<RapidJsonCpp>& value);

	/** 选择或创建对象成员，后续赋值会写入该成员，不要在同一表达式中对同一个RapidJsonCpp对象多次调用该函数
	@param [in] key 成员键名
	@return 返回当前对象引用
	*/
	RapidJsonCpp& operator[](const std::string& key);

private:
    /** 重置独立分配的选中值快照，已借出的 Value 指针继续指向当前选择
    @param [in] value 来自快照以外的节点，空指针表示选择空值
    @return 无
    */
    void setSelected(const rapidjson::Value* value);

    /** 写入已拥有的临时节点，同时更新当前选中值
    @param [in,out] value 使用本文档分配器创建的节点，写入后可被移动
    @return 当前封装对象
    */
    RapidJsonCpp& assignValue(rapidjson::Value& value);

private:
	// 底层JSON文档对象，由RapidJsonCpp负责释放
	rapidjson::Document* m_document;
	// 当前选中值及其独立内存池，切换选择时释放上一个快照
	rapidjson::Document* m_value;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// operator[]选中的待写入成员键名
	std::string m_key;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 是否已经通过operator[]选择了成员，允许空字符串作为合法JSON成员名
	bool m_hasKey;
};