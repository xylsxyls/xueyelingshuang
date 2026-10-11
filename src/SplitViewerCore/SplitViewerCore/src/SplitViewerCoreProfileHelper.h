#pragma once
#include "SplitViewerCoreDocument.h"
#include "SplitViewerCoreProfile.h"
#include <stdint.h>
#include <set>

/** 旧版兼容UTF-16LE配置编解码，完整校验后才提交文档，不持有文件或窗口资源
*/
class SplitViewerCoreProfileHelper
{
public:
    /** 使用固定区域设置格式化浮点数
    @param [in] value 要以经典区域设置及10位小数写入配置的浮点数
    @return 使用经典区域设置和固定精度生成的配置文本
    */
    static std::wstring formatDouble(double value);

    /** 写入配置节中的键值
    @param [in,out] profile 配置键值集合
    @param [in] section 配置节名
    @param [in] key 字段名称
    @param [in] value 本次写入的字段文本，覆盖相同节和键的旧值
    */
    static void setValue(SplitViewerCoreProfile& profile, const std::wstring& section, const std::wstring& key,
        const std::wstring& value);

    /** 读取配置节中的键值，缺失时使用默认值
    @param [in] profile 配置键值集合
    @param [in] section 配置节名
    @param [in] key 字段名称
    @param [in] defaultValue 解析失败或字段缺失时的默认值
    @return 找到的字段文本，节或字段不存在时返回defaultValue
    */
    static std::wstring getValue(const SplitViewerCoreProfile& profile, const std::wstring& section,
        const std::wstring& key, const std::wstring& defaultValue);

    /** 按先序编号保存节点及其子树；限制深度与数量，避免生成无法读回的配置
    @param [in,out] profile 配置键值集合
    @param [in] prefix 节点节名前缀
    @param [in] node 文档持有的节点
    @param [in,out] nextId 下一个节点编号
    @param [in] depth 当前深度，根为0
    @param [in] config 本文档独立配置快照，限制递归、数量及字节预算
    @return 节点有效且未超过读入限制
    */
    static bool saveNode(SplitViewerCoreProfile& profile, const std::wstring& prefix, const SplitViewerCoreNode* node,
        int32_t& nextId, const SplitViewerCoreConfig& config, int32_t depth = 0);

    /** 验证深度、节点编号及循环引用后加载子树；失败返回空
    @param [in] profile 配置键值集合
    @param [in] prefix 节点节名前缀
    @param [in] id 节点配置节编号，须在本实例允许范围内且不重复引用
    @param [in] depth 当前递归深度
    @param [in,out] visited 已访问的节点编号集合
    @param [in] config 本文档独立配置快照，限制递归、数量及字节预算
    @return 成功时将新子树所有权交给调用方；任一校验失败返回nullptr
    */
    static SplitViewerCoreNode* loadNode(const SplitViewerCoreProfile& profile, const std::wstring& prefix, int32_t id,
        int32_t depth, std::set<int32_t>& visited, const SplitViewerCoreConfig& config);

    /** 将节和键值写为旧版兼容配置文本
    @param [in] profile 配置键值集合
    @param [in,out] text 配置文本
    */
    static void writeProfileText(const SplitViewerCoreProfile& profile, std::wstring& text);

    /** 解析有长度限制的UTF-16LE配置字节
    @param [in] bytes 字节缓冲区
    @param [in,out] profile 配置键值集合
    @param [in] config 本文档独立配置快照，限制递归、数量及字节预算
    @return 解析有长度限制的UTF-16LE配置字节是否成功
    */
    static bool parseProfileText(
        const std::vector<uint8_t>& bytes, SplitViewerCoreProfile& profile, const SplitViewerCoreConfig& config);

    /** 将文档编码为旧版兼容的UTF-16LE配置
    @param [in] document 文档模型
    @param [in,out] bytes 字节缓冲区
    @return 将文档编码为旧版兼容的UTF-16LE配置是否成功
    */
    static bool serializeProfile(const SplitViewerCoreDocument& document, std::vector<uint8_t>& bytes);

    /** 完整验证配置后替换文档；失败保留原文档
    @param [in] bytes 字节缓冲区
    @param [in,out] document 文档模型
    @return 完整验证配置后替换文档；失败保留原文档是否成功
    */
    static bool deserializeProfile(const std::vector<uint8_t>& bytes, SplitViewerCoreDocument& document);
};