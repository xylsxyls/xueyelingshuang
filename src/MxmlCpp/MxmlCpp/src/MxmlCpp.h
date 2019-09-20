#pragma once
#include "MxmlCppMacro.h"
#include <string>

struct _mxml_node_s;
typedef struct _mxml_node_s mxml_node_t;

/** xml解析类
*/
class MxmlCpp
{
public:
	/** 构造函数
	*/
	MxmlCpp();

	/** 析构函数
	*/
	~MxmlCpp();

public:
	/** 加载xml字符串
	@param [in] xml xml字符串
	@return 返回是否加载成功，加载成功不一定能读取每个字段的值，不标准的xml无法读取内部的值
	*/
	bool load(const std::string& xml);

	/** 进入子节点
	@param [in] element 子节点名称，子节点名称如果为空则进入第一个子节点
	@param [in] attr 属性名称，有则进入到含有该属性的第一个节点
	@param [in] value 属性值，有则进入到匹配属性值的第一个节点，该项要求attr必须有值
	@return 返回是否进入成功
	*/
	bool firstChild(const std::string& element = "", const std::string& attr = "", const std::string& value = "");

	/** 进入到上一个节点
	@return 返回是否进入成功
	*/
	bool pre();

	/** 进入到下一个节点
	@return 返回是否进入成功
	*/
	bool next();

	/** 进入到父节点
	@return 返回是否进入成功
	*/
	bool parent();

	/** 获取节点名称
	@return 返回节点名称
	*/
	std::string element();

	/** 获取属性值
	@param [in] name 属性名
	@return 返回属性值
	*/
	std::string attr(const std::string& name);

	/** 获取节点内容
	@return 返回节点内容
	*/
	std::string text();

	/** 生成xml字符串
	@return 返回xml字符串
	*/
	std::string toString();

private:
	mxml_node_t* m_xml;
	mxml_node_t* m_xmlRead;
};