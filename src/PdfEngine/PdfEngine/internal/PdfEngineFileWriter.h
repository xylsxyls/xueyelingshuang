#pragma once

#include <stdio.h>

struct FPDF_FILEWRITE_;
typedef struct FPDF_FILEWRITE_ FPDF_FILEWRITE;

/** PDFium保存文件时使用的内部写文件回调对象
*/
class PdfEngineFileWriter
{
public:
    /** 构造绑定到指定FILE对象的写文件回调
    @param [in] file 已打开的二进制输出文件，不负责关闭
    */
    explicit PdfEngineFileWriter(FILE* file);

    /** 析构函数，释放内部PDFium写文件结构
    */
    ~PdfEngineFileWriter();

private:
    /** 禁止拷贝构造，避免多个回调对象释放同一个内部写文件结构
    @param [in] other 另一个写文件回调对象
    */
    PdfEngineFileWriter(const PdfEngineFileWriter& other);

    /** 禁止赋值，避免内部写文件结构生命周期被错误复制
    @param [in] other 另一个写文件回调对象
    @return 返回当前对象引用
    */
    PdfEngineFileWriter& operator=(const PdfEngineFileWriter& other);

public:
    /** 获取PDFium写文件结构指针
    @return 返回PDFium写文件结构指针，创建失败时返回nullptr
    */
    FPDF_FILEWRITE* nativeWriter() const;

    /** 判断写文件过程中是否发生失败
    @return 写入失败返回true，否则返回false
    */
    bool failed() const;

    /** 仿函数形式执行一次数据块写入
    @param [in] data 待写入数据块
    @param [in] size 数据块字节数
    @return 写入成功返回1，否则返回0
    */
    int operator()(const void* data, unsigned long size);

private:
    // 已打开的二进制输出文件，不负责关闭
    FILE* m_file;
    // 写文件过程中是否发生失败
    bool m_failed;
    // 内部PDFium写文件结构指针
    FPDF_FILEWRITE* m_nativeWriter;
};