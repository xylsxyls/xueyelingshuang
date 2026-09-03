#pragma once

#include "PdfEngineMacro.h"
#include "PdfDocument.h"
#include "PdfEnginePageRef.h"

#include <stdint.h>
#include <string>
#include <vector>

/** PDFium生命周期管理和PDF页面保存服务
*/
class PdfEngineAPI PdfEngine
{
public:
    /** 构造未初始化的PDF引擎对象
    */
    PdfEngine();

    /** 析构函数，作为兜底释放当前对象持有的PDFium初始化引用
    */
    ~PdfEngine();

private:
    /** 禁止拷贝构造，避免多个PDF引擎对象复制同一份初始化状态
    @param [in] other 另一个PDF引擎对象
    */
    PdfEngine(const PdfEngine& other);

    /** 禁止赋值，避免PDFium生命周期引用计数被错误复制
    @param [in] other 另一个PDF引擎对象
    @return 返回当前对象引用
    */
    PdfEngine& operator=(const PdfEngine& other);

public:
    /** 初始化PDFium库，重复调用会直接成功
    @param [out] errorText 初始化失败原因，可为空
    @return 初始化成功返回true，否则返回false
    */
    bool init(std::string* errorText = nullptr);

    /** 释放当前对象持有的PDFium初始化引用
    */
    void uninit();

    /** 判断当前对象是否已经完成init
    @return 已初始化返回true，否则返回false
    */
    bool isInit() const;

    /** 将指定页面序列保存成一个新的PDF文件
    @param [in] outputFilePath 输出PDF文件路径
    @param [in] pages 要保存的页面引用列表，顺序即输出顺序
    @param [out] errorText 保存失败原因，可为空
    @return 保存成功返回true，否则返回false
    */
    bool savePages(const std::wstring& outputFilePath,
                   const std::vector<PdfEnginePageRef>& pages,
                   std::string* errorText = nullptr);

    /** 解析用户输入的页码范围文本
    @param [in] rangeText 页码范围文本，例如3-5,7-8
    @param [in] pageCount 当前PDF总页数
    @param [out] pageIndexes 解析后的0基页面索引列表
    @param [out] errorText 解析失败原因，可为空
    @return 解析成功返回true，否则返回false
    */
    static bool parsePageRanges(const std::string& rangeText,
                                int32_t pageCount,
                                std::vector<int32_t>* pageIndexes,
                                std::string* errorText = nullptr);

private:
    // 当前对象是否已经完成init
    bool m_isInit;
};