#pragma once

#include "PdfEngineMacro.h"
#include "PdfEngineBitmap.h"
#include "PdfEnginePageSize.h"

#include <stdint.h>
#include <string>

/** 一个已经打开或待打开的PDF文档
*/
class PdfEngineAPI PdfDocument
{
public:
    /** 构造未打开PDF文件的文档对象
    */
    PdfDocument();

    /** 析构函数，作为兜底关闭当前持有的PDF文档句柄
    */
    ~PdfDocument();

private:
    /** 禁止拷贝构造，避免多个文档对象关闭同一个PDFium文档句柄
    @param [in] other 另一个PDF文档对象
    */
    PdfDocument(const PdfDocument& other);

    /** 禁止赋值，避免PDFium文档句柄生命周期被错误复制
    @param [in] other 另一个PDF文档对象
    @return 返回当前对象引用
    */
    PdfDocument& operator=(const PdfDocument& other);

public:
    /** 打开PDF文件
    @param [in] filePath PDF文件路径
    @param [in] password PDF打开密码，无密码时传空字符串
    @param [out] errorText 打开失败原因，可为空
    @return 打开成功返回true，否则返回false
    */
    bool open(const std::wstring& filePath,
              const std::string& password = std::string(),
              std::string* errorText = nullptr);

    /** 打开无密码PDF文件并返回失败原因
    @param [in] filePath PDF文件路径
    @param [out] errorText 打开失败原因，可为空
    @return 打开成功返回true，否则返回false
    */
    bool open(const std::wstring& filePath, std::string* errorText);

    /** 关闭当前PDF文档
    */
    void close();

    /** 判断当前PDF文档是否已经打开
    @return 已打开返回true，否则返回false
    */
    bool isOpen() const;

    /** 获取当前PDF文件路径
    @return 返回PDF文件路径，未打开文件时返回空字符串
    */
    std::wstring filePath() const;

    /** 获取当前PDF文档总页数
    @return 返回PDF总页数，未打开文件时返回0
    */
    int32_t pageCount() const;

    /** 获取指定页面尺寸
    @param [in] pageIndex 0基页面索引
    @param [out] pageSize 页面尺寸输出对象
    @param [out] errorText 获取失败原因，可为空
    @return 获取成功返回true，否则返回false
    */
    bool getPageSize(int32_t pageIndex,
                     PdfEnginePageSize* pageSize,
                     std::string* errorText = nullptr) const;

    /** 将指定页面渲染为BGRA位图
    @param [in] pageIndex 0基页面索引
    @param [in] pixelWidth 输出位图宽度，单位为像素
    @param [in] pixelHeight 输出位图高度，单位为像素
    @param [out] bitmap 渲染后的位图输出对象
    @param [out] errorText 渲染失败原因，可为空
    @return 渲染成功返回true，否则返回false
    */
    bool renderPageToBgra(int32_t pageIndex,
                          int32_t pixelWidth,
                          int32_t pixelHeight,
                          PdfEngineBitmap* bitmap,
                          std::string* errorText = nullptr) const;

    /** 获取供PdfEngine内部保存页面使用的底层文档指针
    @return 返回底层文档指针，未打开文件时返回nullptr
    */
    void* nativeDocumentForPdfEngine() const;

private:
    /** 获取底层PDFium文档指针，仅供PdfEngine保存页面时使用
    @return 返回底层文档指针，未打开文件时返回nullptr
    */
    void* nativeDocument() const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 当前PDF文件路径
    std::wstring m_filePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 底层PDFium文档句柄
    void* m_document;
    // 当前PDF文档总页数
    int32_t m_pageCount;
};