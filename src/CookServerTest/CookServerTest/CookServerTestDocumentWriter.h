#pragma once
#include <string>

/** CookServer文档生成器，负责把当前功能说明和部署说明写到exe同级目录
*/
class CookServerTestDocumentWriter
{
public:
	/** 构造函数
	*/
	CookServerTestDocumentWriter();

	/** 析构函数
	*/
	~CookServerTestDocumentWriter();

	/** 写入CookServer功能说明和部署文档
	@return 返回是否全部写入成功
	*/
	bool writeDocuments() const;

private:
	/** 写入单个文本文件
	@param [in] path 文件完整路径
	@param [in] text 文件内容
	@return 返回是否写入成功
	*/
	bool writeTextFile(const std::string& path, const std::string& text) const;

	/** 生成CookServer功能说明文档内容
	@return 返回文档文本
	*/
	std::string buildFeatureDocument() const;

	/** 生成CookServer部署文档内容
	@return 返回文档文本
	*/
	std::string buildDeployDocument() const;

};