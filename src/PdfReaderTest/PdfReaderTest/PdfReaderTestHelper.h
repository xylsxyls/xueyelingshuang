#pragma once
#define PDFREADERCORE_USE_C_API
#include "PdfReaderCore/PdfReaderCoreAPI.h"
#include <QString>
#include <QByteArray>
#include <QList>
#include <memory>

/** 测试断言、独立 PDF 素材及 Core 结果核验，无共享测试状态
*/
class PdfReaderTestHelper
{
public:
    /** 断言必须满足的独立预期；失败由当前用例捕获并记录
    @param [in] condition 必须成立的独立预期
    @param [in] message 失败时写入异常的原因
    */
    static void require(bool condition, const char* message);

    /** 生成固定红/绿/蓝页，页面尺寸区分顺序；不使用产品保存算法制作预期
    @param [in] directory 目标用例目录
    @param [in] name 样本文件名
    @param [in] count 页数，必须大于零
    @return 已写入的样本路径
    */
    static QString fixture(const QString& directory, const QString& name, int count = 3);

    /** 读取文件字节用于覆盖前后不变量与报告哈希
    @param [in] path 读取路径
    @return 完整文件字节，失败抛出测试异常
    */
    static QByteArray bytes(const QString& path);

    /** 持有测试自己的 C 句柄，通过 finally 风格 deleter 确保失败也释放
    @param [in] path 要打开的样本路径
    @return 拥有独立 Core 句柄的智能指针，离开作用域自动销毁
    */
    static std::shared_ptr<PdfReaderCoreCContext> core(const QString& path);

    /** 通过独立页面尺寸序列核对保存后重开结果
    @param [in] core 已打开的 Core 句柄，同步借用
    @param [in] expected 各页期望宽度，用于核对页序
    */
    static void widths(PdfReaderCoreHandle core, const QList<int>& expected);
};