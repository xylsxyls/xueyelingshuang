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

	/** 创建一个xml字符串，会销毁读取的字符串或上次创建的字符串
	@return 返回是否创建成功
	*/
	bool create();

	/** 添加子节点
	@param [in] name 子节点名
	@return 返回是否创建成功
	*/
	bool addChild(const std::string& name);

	/** 添加有文字的节点
	@param [in] name 子节点名
	@param [in] text 子节点内容
	@return 返回是否创建成功
	*/
	bool addChild(const std::string& name, const std::string& text);

	/** 添加节点属性
	@param [in] name 节点属性名
	@param [in] attr 属性内容
	@return 返回是否添加成功
	*/
	bool addAttr(const std::string& name, const std::string& attr);

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
	mxml_node_t* m_xmlVisit;
};