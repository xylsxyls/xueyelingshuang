#pragma once

#include "Config.h"
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QImage>

#define PDFREADERCORE_USE_C_API
#include "PdfReaderCore/PdfReaderCoreAPI.h"

class PdfReaderCoreBridge
{
public:
    /** 复制工作线程所需错误文案，不建立PDF句柄
    */
    explicit PdfReaderCoreBridge();

    /** 在唯一执行者释放Core句柄
    */
    ~PdfReaderCoreBridge();

    /** 使用调用方配置初始化Core，配置复制到会话
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool init(QString* errorText = nullptr);

    /** 初始化时复制 Core 参数；之后修改调用方配置不影响现有实例
    */
    bool init(const PdfReaderCoreCConfig& config, QString* errorText = nullptr);

    /** 释放Core内部PDF资源，可重复调用
    */
    void shutdown();

    /** 查询Core或GUI快照是否存在文档
    @return 操作成功返回true；失败返回false
    */
    bool isOpen() const;

    /** 返回当前文档页数
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    int pageCount() const;

    /** 取得最近实际Core操作错误
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    QString lastError() const;

    /** 最后一次实际 C API 调用的结果码
    @return PdfReaderCoreCResult枚举值
    */
    int32_t lastResult() const;

    /** 打开PDF，失败保留当前文档
    @param [in] path PDF文件路径
    @param [in] password 密码；空字符串表示未提供
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool open(const QString& path, const QString& password, QString* errorText = nullptr);

    /** 关闭当前文档并释放页面资源
    */
    void close();

    /** 读取指定页尺寸
    @param [in] index 从0开始的页码或插入点
    @param [out] info 页面信息输出，不可空
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool pageInfo(int index, PdfReaderCoreCPageInfo* info, QString* errorText = nullptr);

    /** 在像素预算内渲染页图像
    @param [in] index 从0开始的页码或插入点
    @param [in] width 目标像素宽度
    @param [in] height 目标像素高度
    @param [out] errorText 可空的本次错误详情输出
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    QImage renderPage(int index, int width, int height, QString* errorText = nullptr);

    /** 将源PDF页面插入指定位置
    @param [in] path PDF文件路径
    @param [in] password 密码；空字符串表示未提供
    @param [in] index 从0开始的页码或插入点
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool insertDocument(const QString& path, const QString& password, int index, QString* errorText = nullptr);

    /** 将当前页移到指定目标
    @param [in] from 源页码
    @param [in] to 目标页码
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool movePage(int from, int to, QString* errorText = nullptr);

    /** 按当前工作页序另存为PDF
    @param [in] path PDF文件路径
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool saveTo(const QString& path, QString* errorText = nullptr);

    /** 覆盖原PDF，失败保留恢复副本和当前工作页序
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool saveToMain(QString* errorText = nullptr);

    /** 验证范围，错误状态属于本次调用
    @param [in] range 用户输入的页码范围
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool validatePageRange(const QString& range, QString* errorText = nullptr);

    /** 按有效范围输出PDF
    @param [in] range 用户输入的页码范围
    @param [in] path PDF文件路径
    @param [out] errorText 可空的本次错误详情输出
    @return 操作成功返回true；失败返回false
    */
    bool savePageRange(const QString& range, const QString& path, QString* errorText = nullptr);

    /** 逐页输出，覆盖须显式允许
    @param [in] directory 输出目录
    @param [in] prefix 分页输出文件名前缀
    @param [out] errorText 可空的本次错误详情输出
    @param [in] overwrite 是否已明确允许覆盖
    @return 操作成功返回true；失败返回false
    */
    bool saveEachPage(const QString& directory, const QString& prefix, QString* errorText = nullptr, bool overwrite = false);

private:
    /** 复制Core错误文本，不借用内部存储
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    QString takeError() const;

private:
    // 构造时复制错误文案，工作线程不读取应用配置
    const QString m_createError;
    const QString m_internalError;
    const QString m_pageError;
    const QString m_renderError;
    int32_t m_lastResult;
    uint64_t m_maxRenderPixels;
    PdfReaderCoreHandle m_handle;
};