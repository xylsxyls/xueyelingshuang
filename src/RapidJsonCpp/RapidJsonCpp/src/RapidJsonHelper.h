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

/** RapidJSON封装内部工具类，提供多个封装类共用的容量和序列化操作
*/
class RapidJsonHelper
{
public:
    /** 解析完整 UTF-8 文本，失败时释放原有资源并重置为空对象
    @param [out] document 接收结果的文档
    @param [in] json 要解析的完整字节序列
    @return 成功返回 true，语法或编码错误返回 false
    */
    static bool parse(rapidjson::Document& document, const std::string& json);

	/** 将标准库size_t转换为RapidJSON数组容量类型
	@param [in] value 标准库容量或长度，超出目标类型范围时抛出 std::length_error
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

	/** 将RapidJSON值序列化为JSON文本
	@param [in] value 待序列化的RapidJSON值
	@return 返回JSON文本；含无效 UTF-8 或非有限数时返回空字符串
	*/
	static std::string valueToString(const rapidjson::Value* value);

private:
    /** 检查 Unicode 转义中是否出现没有高位配对的低位代理码元
    @param [in] json 待解析的 JSON 原始文本
    @return 发现孤立低位码元或截断的转义时返回 true
    */
    static bool hasUnpairedSurrogate(const std::string& json);
};