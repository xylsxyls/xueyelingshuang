#pragma once
#include "PdfReaderRequest.h"

/** 单次请求终态及一致的文档快照；不把提交成功当作执行成功
*/
struct PdfReaderResult
{
public:
    // 原始请求身份和参数
    PdfReaderRequest m_request;
    // 操作是否实际成功、预览是否被取消、C API错误码和详情
    bool m_success;
    bool m_cancelled;
    int32_t m_code;
    QString m_error;
    // 仅修改文档的操作携带元数据；失败打开保留上一份GUI快照
    bool m_hasSnapshot;
    QVector<PdfReaderCoreCPageInfo> m_pages;
    // 执行线程产生独占图像；GUI收到后才转换QPixmap
    QImage m_image;
public:
    /** 建立尚未成功的结果
    */
    PdfReaderResult();
};