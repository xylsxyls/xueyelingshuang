#pragma once
#include <QIcon>

/** Qt 图标创建辅助；只在 QApplication 创建之后调用，不持有全局图像 */
class PdfReaderIconHelper
{
public:
    /** 返回与 SplitViewer 相同风格的蓝色圆形问号 */
    static QIcon aboutIcon();
    /** 返回资源内应用图标，EXE 的 Win32 资源使用同源 ICO */
    static QIcon applicationIcon();
};