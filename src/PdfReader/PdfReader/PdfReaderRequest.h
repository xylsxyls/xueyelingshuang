#pragma once
#include "PdfReaderCoreBridge.h"
#include <stdint.h>

/** 请求类型；副作用有序执行，预览可按代次取消
*/
enum PdfReaderOperation
{
    PdfReaderOpen,
    PdfReaderInsert,
    PdfReaderMove,
    PdfReaderSave,
    PdfReaderSaveMain,
    PdfReaderValidateRange,
    PdfReaderSaveRange,
    PdfReaderSaveEach,
    PdfReaderRender,
    PdfReaderClose
};

/** 不可变请求参数快照，不借用GUI内存
*/
struct PdfReaderRequest
{
public:
    // 会话内递增请求ID和预览代次
    uint64_t m_id;
    uint64_t m_epoch;
    // 操作及UTF-8转换前的路径、密码、范围、前缀
    PdfReaderOperation m_operation;
    QString m_path;
    QString m_password;
    QString m_text;
    // 页码/插入点与移动目标
    int32_t m_index;
    int32_t m_target;
    // 渲染目标像素大小
    QSize m_size;
    // 显式允许分页覆盖
    bool m_overwrite;
public:
    /** 初始化无副作用的请求参数
    */
    PdfReaderRequest();
};