#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include "CiniMacro.h"

/** ini操作类
*/
class CiniAPI Cini
{
public:
	/** 构造函数
	@param [in] iniPath 文件路径，windows下使用ansi或utf-8，linux下只能使用utf-8，都是无BOM
	@param [in] useSimpleIni 是否使用SimpleIni开源库，linux下一定使用开源库
	*/
	Cini(const std::string& iniPath, bool useSimpleIni = false);

	/** 析构函数
	*/
	~Cini();

public:
	/** 设置Windows系统API读取时缓冲区最大程度，默认是1024
	@param [in] length 最大长度
	*/
	void setMaxLength(int32_t length);

	/** 设置Windows系统API读取时节点最多个数，默认是1024
	@param [in] sectionNum 最大节点个数
	*/
	void setMaxSection(int32_t sectionNum);

	/** 读取数据
	@param [in] key 键值
	@param [in] section 节点值
	*/
	std::string readIni(const std::string& key, const std::string& section = "Section") const;

	/** 写入数据
	@param [in] key 键值，不存在则创建，存在则修改
	@param [in] value 数据
	@param [in] section 节点值
	*/
	void writeIni(const std::string& key, const std::string& value, const std::string& section = "Section") const;

	/** 获取所有节点（该接口只在SimpleIni下有效）
	@return 返回所有节点
	*/
	std::vector<std::string> getAllSection() const;

	/** 获取所有节点下的键值名（该接口只在SimpleIni下有效）
	@return 返回所有节点下的键值名
	*/
	std::vector<std::string> getAllKey(const std::string& section = "Section") const;

	/** 获取该节点下第一个键值名
	@param [in] section 节点值
	@return 返回该节点下第一个键值名
	*/
	std::string getFirstKey(const std::string& section = "Section") const;

	/** 删除数据
	@param [in] key 键值
	@param [in] section 节点值
	*/
	void deleteKey(const std::string& key, const std::string& section = "Section") const;

	/** 删除节点
	@param [in] section 节点值
	*/
	void deleteSection(const std::string& section = "Section") const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_iniPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_length;
	int32_t m_sectionNum;
	bool m_useSimpleIni;
	void* m_simpleIni;
	bool m_loadSuccess;
};