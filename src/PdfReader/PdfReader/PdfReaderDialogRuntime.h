#pragma once

/** 应用级弹窗注册作用域，必须晚于 QApplication 创建且早于 QApplication 销毁。 */
class PdfReaderDialogRuntime
{
public:
    /** 注册唯一业务工厂，失败时抛出异常。 */
    PdfReaderDialogRuntime();
    /** 关闭托管弹窗并释放管理器和工厂。 */
    ~PdfReaderDialogRuntime();
private:
    PdfReaderDialogRuntime(const PdfReaderDialogRuntime&) = delete;
    PdfReaderDialogRuntime& operator=(const PdfReaderDialogRuntime&) = delete;
};
