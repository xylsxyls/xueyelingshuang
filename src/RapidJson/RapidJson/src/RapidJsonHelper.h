#pragma once
#include <stddef.h>
#include <string>

namespace rapidjson
{
	template <typename CharType> struct UTF8;
	class CrtAllocator;
	template <typename BaseAllocator> class MemoryPoolAllocator;
	template <typename Encoding, typename Allocator> class GenericValue;
	template <typename Encoding, typename Allocator, typename StackAllocator> class GenericDocument;
	typedef unsigned SizeType;
	typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator>> Value;
	typedef GenericDocument<UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator> Document;
}

/** RapidJSON封装内部工具类，集中放置多个类会复用但不适合放入CSystem的辅助逻辑
*/
class RapidJsonHelper
{
public:
	/** 将标准库size_t转换为RapidJSON数组容量类型
	@param [in] value 标准库容量或长度
	@return 返回RapidJSON接口需要的SizeType
	*/
	static rapidjson::SizeType jsonSize(size_t value);

	/** 获取对象首次写入字段时的默认预留成员数量，用于降低连续添加小对象字段时的扩容次数
	@return 返回默认预留成员数量
	*/
	static rapidjson::SizeType defaultObjectMemberCapacity();

	/** 确保RapidJSON值是数组类型，不是数组时会重置为数组
	@param [in,out] value 需要检查和转换的RapidJSON值
	@return 无
	*/
	static void ensureArray(rapidjson::Value* value);

	/** 设置RapidJSON字符串值，保证字符串内容拷贝到当前文档分配器
	@param [out] jsonValue 需要写入的底层JSON值
	@param [in] value 需要写入JSON的字符串
	@param [in] document 提供分配器的RapidJSON文档
	@return 无
	*/
	static void setString(rapidjson::Value* jsonValue, const std::string& value, rapidjson::Document* document);

	/** 向对象中加入成员，目标不是对象时会先转换成对象
	@param [in,out] object 要写入成员的RapidJSON值
	@param [in] key 成员名称
	@param [in] value 成员值
	@param [in] document 提供分配器的RapidJSON文档
	@return 无
	*/
	static void addMember(rapidjson::Value* object,
						  const char* key,
						  rapidjson::Value* value,
						  rapidjson::Document* document);

	/** 将RapidJSON值序列化为JSON文本
	@param [in] value 待序列化的RapidJSON值
	@return 返回JSON文本
	*/
	static std::string valueToString(const rapidjson::Value* value);
};