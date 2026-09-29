#pragma once

#include "PdfReaderCoreMacro.h"
#include "PdfReaderCoreConfig.h"
#include "PdfReaderCoreModels.h"
#include "PdfEngine/PdfEngineAPI.h"

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

/** 页面信息。尺寸单位为PDF point，通常可直接作为100%显示尺寸
*/
struct PdfReaderCoreAPI PdfReaderCorePageInfo
{
    double width;
    double height;

    /** 初始化为空页面尺寸
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    PdfReaderCorePageInfo();
};

/** PdfReader的跨平台业务核心。此类不依赖Qt或Windows
*/
class PdfReaderCoreAPI PdfReaderCore
{
public:
    /** 建立无引擎资源的Core实例
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    PdfReaderCore();

    /** 销毁当前会话及引擎引用
    */
    ~PdfReaderCore();

private:
    /** 建立无引擎资源的Core实例
    @param [in] other 禁止复制的源实例
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    PdfReaderCore(const PdfReaderCore& other);
    PdfReaderCore& operator=(const PdfReaderCore& other);

public:
    /** 使用调用方配置初始化Core，配置复制到会话
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool init(std::string* errorText = nullptr);

    /** 使用调用方配置初始化；运行中拒绝重新配置，先 uninit 再调用
    */
    bool init(const PdfReaderCoreConfig& config, std::string* errorText = nullptr);

    /** 关闭文档后释放引擎引用
    */
    void uninit();

    /** 查询当前实例引擎是否初始化
    @return 操作成功返回true；失败返回false
    */
    bool isInit() const;

    /** 打开PDF，失败保留当前文档
    @param [in] filePath 源PDF路径
    @param [in] password 密码；空字符串表示未提供
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool open(const std::wstring& filePath,
        const std::string& password = std::string(),
        std::string* errorText = nullptr);

    /** 关闭当前文档并释放页面资源
    */
    void close();

    /** 查询Core或GUI快照是否存在文档
    @return 操作成功返回true；失败返回false
    */
    bool isOpen() const;

    /** 返回当前主PDF路径
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    std::wstring filePath() const;

    /** 取得最近实际Core操作错误
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    std::string lastError() const;

    /** 返回当前文档页数
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    int32_t pageCount() const;

    /** 读取指定页尺寸
    @param [in] pageIndex 从0开始的页码
    @param [out] info 页面信息输出，不可空
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool pageInfo(int32_t pageIndex,
        PdfReaderCorePageInfo* info,
        std::string* errorText = nullptr);

    /** 在像素预算内渲染页图像
    @param [in] pageIndex 从0开始的页码
    @param [in] pixelWidth 目标像素宽度
    @param [in] pixelHeight 目标像素高度
    @param [out] bitmap 渲染位图输出，不可空
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool renderPage(int32_t pageIndex,
        int32_t pixelWidth,
        int32_t pixelHeight,
        PdfEngineBitmap* bitmap,
        std::string* errorText = nullptr);

    /** 将源PDF页面插入指定位置
    @param [in] filePath 源PDF路径
    @param [in] password 密码；空字符串表示未提供
    @param [in] insertIndex 插入点，允许等于当前页数
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool insertDocument(const std::wstring& filePath,
        const std::string& password,
        int32_t insertIndex,
        std::string* errorText = nullptr);

    /** 将当前页移到指定目标
    @param [in] fromIndex 源页码
    @param [in] toIndex 目标页码
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool movePage(int32_t fromIndex, int32_t toIndex,
        std::string* errorText = nullptr);

    /** 按当前工作页序另存为PDF
    @param [in] outputFilePath 输出PDF路径
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool saveTo(const std::wstring& outputFilePath,
        std::string* errorText = nullptr);

    /** 覆盖原PDF，失败保留恢复副本和当前工作页序
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool saveToMain(std::string* errorText = nullptr);

    /** 验证页码选集，界面先报错再选择输出路径；解析规则与保存共用
    */
    bool validatePageRange(const std::string& rangeText, std::string* errorText = nullptr);

    /** 按有效范围输出PDF
    @param [in] rangeText 用户输入的页码范围
    @param [in] outputFilePath 输出PDF路径
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool savePageRange(const std::string& rangeText,
        const std::wstring& outputFilePath,
        std::string* errorText = nullptr);

    /** 逐页输出，覆盖须显式允许
    @param [in] outputDirectory 输出目录
    @param [in] namePrefix 分页输出文件名前缀，不能包含路径分隔符、冒号或NUL；空值使用page
    @param [out] errorText 可空的本次错误详情输出
    @param [in] overwrite 是否已明确允许覆盖
    @return 操作成功返回true；失败返回false
    */
    bool saveEachPage(const std::wstring& outputDirectory,
        const std::string& namePrefix = std::string(),
        std::string* errorText = nullptr, bool overwrite = false);

private:
    /** 检查页码是否属于当前工作文档
    @param [in] pageIndex 从0开始的页码
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool validatePageIndex(int32_t pageIndex, std::string* errorText);

    /** 建立当前工作页序的引擎引用列表
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    std::vector<PdfEnginePageRef> pageRefs() const;

    /** 替换当前错误详情
    @param [out] errorText 可空的本次错误详情输出
    */
    void setError(const std::string& errorText);

    /** 将所选页写到同目录临时文件再替换；失败保留原目标和恢复文件
    */
    bool savePages(const std::wstring& outputPath,
        const std::vector<PdfEnginePageRef>& refs, std::string* errorText);

private:
    PdfReaderCoreConfig m_config;
    PdfEngine m_engine;
    bool m_isInit;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::wstring m_mainPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_mainPassword;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::vector<std::unique_ptr<PdfDocument>> m_documents;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::vector<PdfReaderCorePageEntry> m_pages;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_lastError;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};