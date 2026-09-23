#pragma once
#include <QString>

/** 配置、桌面资源与 Shell 适配的独立回归；不依赖产品生成预期数据 */
class PdfReaderConfigurationTests
{
public:
    /** 执行稳定 ID 15..16，失败抛出异常供统一报告收集 */
    static void run(int id, const QString& input, const QString& directory);
};

/** 独立编译单元验证默认 C++ API 配置与初始化协议 */
bool PdfReaderCppConfigSmoke();
