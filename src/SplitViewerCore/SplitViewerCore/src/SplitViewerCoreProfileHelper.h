#pragma once
#include "SplitViewerCoreDocument.h"
#include <stdint.h>
#include "SplitViewerCoreProfile.h"
#include <set>

/** Transactional UTF-16 profile codec. */
class SplitViewerCoreProfileHelper
{
public:
    /** 使用固定区域设置格式化整数。
    @param [in] value 输入值
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static std::wstring formatInt(int value);
    /** 使用固定区域设置格式化浮点数。
    @param [in] value 输入值
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static std::wstring formatDouble(double value);
    /** 解析整数；失败返回默认值。
    @param [in] value 输入值
    @param [in] defaultValue 解析失败或字段缺失时的默认值
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static int parseInt(const std::wstring& value, int defaultValue);
    /** 解析有限浮点数；失败返回默认值。
    @param [in] value 输入值
    @param [in] defaultValue 解析失败或字段缺失时的默认值
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static double parseDouble(const std::wstring& value, double defaultValue);
    /** 写入配置节中的键值。
    @param [in,out] profile 配置键值集合
    @param [in] section 配置节名
    @param [in] key 字段名称
    @param [in] value 输入值
    */
    static void setValue(SplitViewerCoreProfile& profile,
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& value);
    /** 读取配置节中的键值，缺失时使用默认值。
    @param [in] profile 配置键值集合
    @param [in] section 配置节名
    @param [in] key 字段名称
    @param [in] defaultValue 解析失败或字段缺失时的默认值
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static std::wstring getValue(const SplitViewerCoreProfile& profile,
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& defaultValue);
    /** 按先序编号保存节点及其子树。
    @param [in,out] profile 配置键值集合
    @param [in] prefix 节点节名前缀
    @param [in] node 文档持有的节点
    @param [in,out] nextId 下一个节点编号
    */
    static void saveNode(SplitViewerCoreProfile& profile,
    const std::wstring& prefix,
    const SplitViewerCoreNode* node,
    int& nextId);
    /** 验证深度、节点编号及循环引用后加载子树；失败返回空。
    @param [in] profile 配置键值集合
    @param [in] prefix 节点节名前缀
    @param [in] id 窗口句柄或节点编号
    @param [in] depth 当前递归深度
    @param [in,out] visited 已访问的节点编号集合
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static SplitViewerCoreNode* loadNode(const SplitViewerCoreProfile& profile,
    const std::wstring& prefix,
    int id,
    int depth,
    std::set<int>& visited);
    /** 将节和键值写为旧版兼容配置文本。
    @param [in] profile 配置键值集合
    @param [in,out] text 配置文本
    */
    static void writeProfileText(const SplitViewerCoreProfile& profile, std::wstring& text);
    /** 解析有长度限制的UTF-16LE配置字节。
    @param [in] bytes 字节缓冲区
    @param [in,out] profile 配置键值集合
    @return 是否满足条件或操作成功。
    */
    static bool parseProfileText(const std::vector<uint8_t>& bytes,
    SplitViewerCoreProfile& profile);
    /** 将文档编码为旧版兼容的UTF-16LE配置。
    @param [in] document 文档模型
    @param [in,out] bytes 字节缓冲区
    @return 是否满足条件或操作成功。
    */
    static bool serializeProfile(const SplitViewerCoreDocument& document, std::vector<uint8_t>& bytes);
    /** 完整验证配置后替换文档；失败保留原文档。
    @param [in] bytes 字节缓冲区
    @param [in,out] document 文档模型
    @return 是否满足条件或操作成功。
    */
    static bool deserializeProfile(const std::vector<uint8_t>& bytes, SplitViewerCoreDocument& document);
};