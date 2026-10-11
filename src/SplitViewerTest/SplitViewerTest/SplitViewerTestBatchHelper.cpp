#include "SplitViewerTestBatchHelper.h"
#include "SplitViewerTestCaseHelper.h"
#include "SplitViewerUiTests.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

int SplitViewerTestBatchHelper::run(const QString& selection, const QString& directory)
{
    std::vector<int32_t> ids;
    std::wstring error;
    if (!SplitViewerTestCaseHelper::parse(selection.toStdWString(), ids, error) ||
        !SplitViewerTestCaseHelper::validate())
    {
        return 2;
    }
    if (!QDir().mkpath(directory))
    {
        return 1;
    }
    QFile report(QDir(directory).filePath(QStringLiteral("ui-results.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return 1;
    }
    QTextStream output(&report);
    output.setCodec("UTF-8");
    int32_t total = 0;
    int32_t failures = 0;
    for (int32_t id : ids)
    {
        // 每个场景完成自己的原生窗口和系统设置恢复后才响应批次取消。
        if (QFile::exists(QDir(directory).filePath(QStringLiteral("cancel.request"))))
        {
            output << "total=" << total << " failures=" << failures << " canceled=1\n";
            output.flush();
            return 3;
        }
        const QString current = QDir(directory).filePath(QString::number(id));
        const int32_t result = SplitViewerRunUiTests(current, id);
        QFile detail(QDir(current).filePath(QStringLiteral("ui-results.txt")));
        const bool readable = detail.open(QIODevice::ReadOnly | QIODevice::Text);
        const QByteArray bytes = readable ? detail.readAll() : QByteArray();
        const bool passed = result == 0 && SplitViewerTestCaseHelper::reportSucceeded(bytes.toStdString(), 1);
        output << id << " "
               << QString::fromStdWString(
                      SplitViewerTestCaseHelper::kCases.at(static_cast<SplitViewerTestCaseId>(id)).m_name)
               << (passed ? " PASS\n" : " FAIL\n");
        output.flush();
        ++total;
        failures += passed ? 0 : 1;
    }
    output << "total=" << total << " failures=" << failures << "\n";
    output.flush();
    return failures == 0 ? 0 : 1;
}