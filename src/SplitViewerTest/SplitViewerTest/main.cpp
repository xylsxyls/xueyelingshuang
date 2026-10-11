#include "SplitViewerTest.h"
#include "SplitViewerTestRuntime.h"
#include "SplitViewerCoreTests.h"
#include "SplitViewerUiTests.h"
#include "SplitViewerTestCaseHelper.h"
#include "SplitViewerTestBatchHelper.h"
#include "SplitViewerShadowTests.h"
#include "SplitViewerMoveUiTests.h"
#include "../../SplitViewer/SplitViewer/Config.h"
#ifdef Q_OS_WIN
#include "CDump/CDumpAPI.h"
#endif
#include <QtWidgets/QApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <stdint.h>

/** 初始化测试宿主并分派GUI、编号批次和受控子进程入口，退出时由Runtime统一收尾
@param [in] argc 操作系统传入的命令行参数数量，交由QApplication处理
@param [in] argv 参数数组；编号选择、报告路径和播放器路径由对应入口校验
@return 普通测试0成功、1失败、2非法输入、3协作取消；GUI及专用驱动返回各自实际结果
*/
int main(int argc, char* argv[])
{
#ifdef _WIN32
    CDump::declareDumpFile();
#endif
    QApplication app(argc, argv);
    Config::instance();
    SplitViewerTestConfig::instance();
    SplitViewerTestRuntime runtime;
    if (!runtime.init())
    {
        return 1;
    }
    if (app.arguments().contains(QStringLiteral("--shadow-drag-driver")))
    {
        return SplitViewerShadowTests::runDragDriver(app.arguments());
    }
    if (app.arguments().contains(QStringLiteral("--ui-audit")))
    {
        const QStringList args = app.arguments();
        const int index = args.indexOf(QStringLiteral("--ui-audit"));
        if (index + 1 >= args.size())
        {
            return 2;
        }
        int selected = 0;
        bool hasSelector = false;
        QString selection;
        foreach (const QString& arg, args)
        {
            if (arg.startsWith(QStringLiteral("--case=")))
            {
                std::vector<int32_t> ids;
                std::wstring error;
                if (hasSelector || !SplitViewerTestCaseHelper::parse(arg.mid(7).toStdWString(), ids, error) ||
                    ids.size() != 1)
                {
                    return 2;
                }
                selected = ids[0];
                hasSelector = true;
            }
            else if (arg.startsWith(QStringLiteral("--cases=")))
            {
                selection = arg.mid(8);
                if (hasSelector || selection.isEmpty())
                {
                    return 2;
                }
                hasSelector = true;
            }
        }
        if (!selection.isEmpty())
        {
            return SplitViewerTestBatchHelper::run(selection, args.at(index + 1));
        }
        return SplitViewerRunUiTests(args.at(index + 1), selected);
    }
    if (app.arguments().size() == 2 &&
        QFileInfo(app.arguments().at(1)).fileName() == g_testConfig.m_lifetimeProbeFileName)
    {
        return SplitViewerMoveUiTests::runLifetimeProbe();
    }
    if (app.arguments().size() == 2 &&
        QFileInfo(app.arguments().at(1)).fileName() == QStringLiteral("startup-probe.avi"))
    {
        return SplitViewerMoveUiTests::runStartupProbe(app.arguments().at(1));
    }
    if (app.arguments().contains(QStringLiteral("--autotest")))
    {
        for (int32_t id = 1; id <= 7; ++id)
        {
            if (!SplitViewerCoreTests::run(id))
            {
                return 1;
            }
        }
        return 0;
    }
    SplitViewerTest window;
    window.show();
    return app.exec();
}