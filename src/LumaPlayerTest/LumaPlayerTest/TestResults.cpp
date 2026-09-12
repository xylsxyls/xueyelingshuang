#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "TestResults.h"
#include "LogManager/LogManagerAPI.h"
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <iostream>

TestResults::TestResults(const QString& directory, int32_t caseId) :
m_caseId(caseId),
m_directory(directory),
m_failures(0)
{
    QDir().mkpath(directory);
}

void TestResults::check(int32_t id, bool condition, const QString& detail)
{
    if (!condition)
    {
        ++m_failures;
        LOGERROR("Test assertion %d failed: %s", m_caseId > 0 ? m_caseId : id, detail.toUtf8().constData());
    }
    append(id, condition ? "PASS" : "FAIL", detail);
}

void TestResults::untested(int32_t id, const QString& detail)
{
    append(id, "UNTESTED", detail);
}

int32_t TestResults::failures() const
{
    return m_failures;
}

void TestResults::append(int32_t id, const QString& status, const QString& detail)
{
    QJsonObject item;
    item["case"] = m_caseId > 0 ? m_caseId : id;
    item["status"] = status;
    item["detail"] = detail;
    item["time"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_checks.append(item);
    QSaveFile file(m_directory + "/checks.json");
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(m_checks).toJson()) < 0 || !file.commit())
    {
        ++m_failures;
        std::cerr << "REPORT_WRITE_FAILED" << std::endl;
    }
    std::cout << (m_caseId > 0 ? m_caseId : id) << " " << status.toStdString() << " " << detail.toUtf8().constData() << std::endl;
}