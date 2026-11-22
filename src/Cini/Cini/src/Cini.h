#pragma once
#include "CiniMacro.h"
#include <stdint.h>
#include <string>
#include <vector>

/** 同步访问 INI 配置。SimpleIni 实例独占解析状态，调用方负责同一实例及文件的并发协调
*/
class CiniAPI Cini
{
public:
    /** 选择访问方式并在 SimpleIni 模式下读取初始快照
    @param [in] iniPath 文件路径副本；Windows 使用当前窄字符代码页，其他平台使用 UTF-8，空路径在 SimpleIni 模式用于内存操作；含零字节的路径使实例不可用
    @param [in] useSimpleIni Windows 为 true 时使用 SimpleIni，false 使用系统 Profile；其余平台始终使用 SimpleIni
    */
    Cini(const std::string& iniPath, bool useSimpleIni = false);

    /** 释放解析状态，析构不写文件
    */
    ~Cini();

    /** 替换 SimpleIni 内存配置，空文本清空配置，不触发写盘
    @param [in] data 完整 UTF-8 INI 字节，可带 BOM，不含内嵌零字符
    @return 解析成功返回 true；Profile 模式、不可用实例或载入失败返回 false，失败的解析不能用于后续写回
    */
    bool loadData(const std::string& data);

    /** 设置 Profile 查询容量，非正值停用读取及枚举
    @param [in] length 缓冲区上限，单位字节，包含终止零，默认 1024
    */
    void setMaxLength(int32_t length);

    /** 设置 Profile 分区枚举上限，非正值停止枚举
    @param [in] sectionNum 最大分区数；与查询容量的乘积为枚举字节预算，默认 1024 个
    */
    void setMaxSection(int32_t sectionNum);

    /** 查找指定分区中的键
    @param [in] key 键名
    @param [in] section 分区名，默认 Section
    @return 值的独立副本；缺键、空值或不可读均为空，Profile 可按容量截断
    */
    std::string readIni(const std::string& key, const std::string& section = "Section") const;

    /** 新增或覆盖一个键，有路径时尝试同步保存；非法字段不改变配置
    @param [in] key 非空单行键名，不含等号或零字节，不以注释或节标记起始；名称首尾不能含空格或制表符
    @param [in] value 单行文本，可为空，不能含零字节
    @param [in] section 单行分区名，默认 Section；不含右方括号或零字节，首尾不能含空格或制表符
    */
    void writeIni(const std::string& key, const std::string& value, const std::string& section = "Section") const;

    /** 枚举分区
    @return 分区名副本；Profile 仅返回容量内的完整名称
    */
    std::vector<std::string> getAllSection() const;

    /** 枚举 SimpleIni 分区中的键
    @param [in] section 分区名，默认 Section
    @return 键名副本，Profile 模式、分区不存在或无键时返回空集合
    */
    std::vector<std::string> getAllKey(const std::string& section = "Section") const;

    /** 获取后端枚举顺序中的第一个键
    @param [in] section 分区名，默认 Section
    @return 键名副本，不存在时为空；Profile 可按容量截断
    */
    std::string getFirstKey(const std::string& section = "Section") const;

    /** 删除一个键，有路径时尝试同步保存
    @param [in] key 键名
    @param [in] section 分区名，默认 Section
    */
    void deleteKey(const std::string& key, const std::string& section = "Section") const;

    /** 删除整个分区，包括空分区，有路径时尝试同步保存
    @param [in] section 分区名，默认 Section
    */
    void deleteSection(const std::string& section = "Section") const;

private:
    /** 禁止复制解析状态
    @param [in] other 另一实例
    */
    Cini(const Cini& other) = delete;

    /** 禁止共享解析状态
    @param [in] other 另一实例
    @return 当前实例引用
    */
    Cini& operator=(const Cini& other) = delete;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 构造时复制的读写路径，空路径在 SimpleIni 模式只操作内存
    std::string m_iniPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // Profile 单次查询的字节上限
    int32_t m_length;
    // Profile 枚举的分区数上限
    int32_t m_sectionNum;
    // 独占的 CSimpleIniA 解析器，Profile 模式或路径无效时为空
    void* m_simpleIni;
    // SimpleIni 已载入数据或可从空配置开始，读取失败后禁止隐式覆盖文件
    bool m_ready;
};