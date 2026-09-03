#pragma once

#include <mutex>

/** PDFium全局初始化状态对象
*/
class PdfEngineInstance
{
public:
    /** 获取PDF引擎全局状态对象
    @return 返回PDF引擎全局状态对象引用
    */
    static PdfEngineInstance& instance();

    /** 增加PDFium初始化引用，必要时初始化PDFium库
    */
    void addInitRef();

    /** 释放一次PDFium初始化引用，最后一个引用释放时销毁PDFium库
    */
    void releaseInitRef();

    /** 判断PDFium库是否已经初始化
    @return 已初始化返回true，否则返回false
    */
    bool isPdfiumInitialized() const;

private:
    /** 构造未初始化的PDFium全局状态对象
    */
    PdfEngineInstance();

private:
    // PDFium初始化状态互斥锁
    mutable std::mutex m_mutex;
    // PDFium库是否已经初始化
    bool m_isPdfiumInitialized;
    // PDFium初始化引用计数
    long m_initRefCount;
};