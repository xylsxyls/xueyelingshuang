#pragma once

#include "PdfReaderCoreMacro.h"
#include "PdfReaderCoreConfig.h"
#include "PdfReaderCoreModels.h"
#include "PdfEngine/PdfEngineAPI.h"

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

/** 页面信息。尺寸单位为PDF point，通常可直接作为100%显示尺寸。 */
struct PdfReaderCoreAPI PdfReaderCorePageInfo
{
    double width;
    double height;

    PdfReaderCorePageInfo();
};

/** PdfReader的跨平台业务核心。此类不依赖Qt或Windows。 */
class PdfReaderCoreAPI PdfReaderCore
{
public:
    PdfReaderCore();
    ~PdfReaderCore();

private:
    PdfReaderCore(const PdfReaderCore& other);
    PdfReaderCore& operator=(const PdfReaderCore& other);

public:
    bool init(std::string* errorText = nullptr);
    /** 使用调用方配置初始化；运行中拒绝重新配置，先 uninit 再调用 */
    bool init(const PdfReaderCoreConfig& config, std::string* errorText = nullptr);
    void uninit();
    bool isInit() const;

    bool open(const std::wstring& filePath,
              const std::string& password = std::string(),
              std::string* errorText = nullptr);
    void close();
    bool isOpen() const;
    std::wstring filePath() const;
    std::string lastError() const;

    int32_t pageCount() const;
    bool pageInfo(int32_t pageIndex,
                  PdfReaderCorePageInfo* info,
                  std::string* errorText = nullptr);
    bool renderPage(int32_t pageIndex,
                    int32_t pixelWidth,
                    int32_t pixelHeight,
                    PdfEngineBitmap* bitmap,
                    std::string* errorText = nullptr);

    bool insertDocument(const std::wstring& filePath,
                        const std::string& password,
                        int32_t insertIndex,
                        std::string* errorText = nullptr);
    bool movePage(int32_t fromIndex, int32_t toIndex,
                  std::string* errorText = nullptr);

    bool saveTo(const std::wstring& outputFilePath,
                std::string* errorText = nullptr);
    bool saveToMain(std::string* errorText = nullptr);
    /** 验证页码选集，界面先报错再选择输出路径；解析规则与保存共用 */
    bool validatePageRange(const std::string& rangeText, std::string* errorText = nullptr);
    bool savePageRange(const std::string& rangeText,
                       const std::wstring& outputFilePath,
                       std::string* errorText = nullptr);
    bool saveEachPage(const std::wstring& outputDirectory,
                      const std::string& namePrefix = std::string(),
                      std::string* errorText = nullptr, bool overwrite = false);

private:
    PdfDocument* openDocument(const std::wstring& filePath,
                              const std::string& password,
                              std::string* errorText);
    bool validatePageIndex(int32_t pageIndex, std::string* errorText);
    std::vector<PdfEnginePageRef> pageRefs() const;
    void setError(const std::string& errorText);
    /** 将所选页写到同目录临时文件再替换；失败保留原目标和恢复文件 */
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