#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerTest.h"
#include "TestProcessHelper.h"
#include "TestCommandTask.h"
#include "TestMediaHelper.h"
#include <stdexcept>
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QtWidgets>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDir>
#include <map>
#include <set>
#include <algorithm>

LumaPlayerTest::LumaPlayerTest(const QString& root, const QString& media, const QString& fixture, bool selfCheck) :
m_media(media),
m_commandThread(0),
m_commandPending(false),
m_fixture(fixture),
m_videoPath(nullptr),
m_videoBrowse(nullptr),
m_debugCheck(nullptr),
m_debugPath(nullptr),
m_releasePath(nullptr),
m_debugBrowse(nullptr),
m_releaseBrowse(nullptr),
m_stop(nullptr),
m_active(false),
m_cancel(false),
m_autoExit(QApplication::arguments().contains("--auto-exit")),
m_childPid(0),
m_root(root),
m_index(0),
m_done(false),
m_terminal(true),
m_closeSent(false),
m_timedOut(false),
m_selfCheck(selfCheck),
m_cancelAt(-1),
m_status(nullptr),
m_progress(nullptr),
m_output(nullptr)
{
    QObject::connect(this, &LumaPlayerTest::commandReady, this, &LumaPlayerTest::executeCommand, Qt::QueuedConnection);
    m_ui.setupUi(this);
    setWindowTitle(m_config.m_title);
    resize(900, 580);
    QWidget* panel = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(panel);
    QGridLayout* paths = new QGridLayout();
    m_debugCheck = new QCheckBox(m_config.m_debugLabel, panel);
    m_debugPath = new QLineEdit(panel);
    m_releasePath = new QLineEdit(panel);
    m_debugBrowse = new QPushButton(m_config.m_choosePath, panel);
    m_releaseBrowse = new QPushButton(m_config.m_choosePath, panel);
    m_debugPath->setReadOnly(true);
    m_releasePath->setReadOnly(true);
    m_debugCheck->setObjectName("debugCheck");
    m_debugPath->setObjectName("debugPath");
    m_releasePath->setObjectName("releasePath");
    m_debugBrowse->setObjectName("debugBrowse");
    m_releaseBrowse->setObjectName("releaseBrowse");
    paths->addWidget(m_debugCheck, 1, 0);
    paths->addWidget(m_debugPath, 1, 1);
    paths->addWidget(m_debugBrowse, 1, 2);
    paths->addWidget(new QLabel(m_config.m_releaseLabel, panel), 2, 0);
    paths->addWidget(m_releasePath, 2, 1);
    paths->addWidget(m_releaseBrowse, 2, 2);
    m_videoPath = new QLineEdit(panel);
    m_videoPath->setObjectName("videoPath");
    m_videoPath->setReadOnly(true);
    m_videoBrowse = new QPushButton(m_config.m_choosePath, panel);
    m_videoBrowse->setObjectName("videoBrowse");
    paths->addWidget(new QLabel(m_config.m_videoLabel, panel), 0, 0);
    paths->addWidget(m_videoPath, 0, 1);
    paths->addWidget(m_videoBrowse, 0, 2);
    const QStringList mediaOptions = QApplication::arguments();
    const int32_t mediaIndex = mediaOptions.indexOf("--video-dir");
    m_videoPath->setText(mediaIndex >= 0 && mediaIndex + 1 < mediaOptions.size() ?
        QFileInfo(mediaOptions[mediaIndex + 1]).absoluteFilePath() : m_config.m_videoDirectory);
    QObject::connect(m_videoBrowse, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getExistingDirectory(this, m_config.m_choosePath, m_videoPath->text());
        if (!path.isEmpty())
        {
            m_videoPath->setText(QDir::cleanPath(path));
        }
    });
    paths->setColumnStretch(1, 1);
    layout->addLayout(paths);
    const QStringList options = QApplication::arguments();
    const int32_t cfg = options.indexOf("--configuration");
    m_debugCheck->setChecked(cfg >= 0 && cfg + 1 < options.size() &&
        (options[cfg + 1] == "both" || options[cfg + 1] == "debug"));
    for (int32_t i = 0; i < 2; ++i)
    {
        const int32_t index = options.indexOf(i == 0 ? "--release-dir" : "--debug-dir");
        const QString directory = QDir::cleanPath(index >= 0 && index + 1 < options.size() ?
            QFileInfo(options[index + 1]).absoluteFilePath() :
            QApplication::applicationDirPath() + (i == 0 ? "/../LumaPlayer" : "/../LumaPlayer/debug"));
        if (QFileInfo(directory + (i == 0 ? "/LumaPlayer1.0.exe" : "/LumaPlayerd.exe")).isFile())
        {
            (i == 0 ? m_releasePath : m_debugPath)->setText(directory);
        }
    }
    QObject::connect(m_debugCheck, &QCheckBox::toggled, this, [this](bool) { updatePathControls(); });
    QObject::connect(m_debugBrowse, &QPushButton::clicked, this, [this]() { choosePath(true); });
    QObject::connect(m_releaseBrowse, &QPushButton::clicked, this, [this]() { choosePath(false); });
    updatePathControls();
    QHBoxLayout* controls = new QHBoxLayout();
    const QStringList modes = QStringList() << "specified" << "instant" << "pressure" << "all";
    for (int32_t index = 0; index < 4; ++index)
    {
        QPushButton* button = new QPushButton(m_config.m_buttons[index], panel);
        m_buttons.push_back(button);
        controls->addWidget(button);
        const QString mode = modes[index];
        QObject::connect(button, &QPushButton::clicked, this, [this, mode]() {
            if (!validatePaths())
            {
                return;
            }
            QString ids;
            if (mode == "specified")
            {
                bool accepted = false;
                ids = QInputDialog::getText(this, m_config.m_buttons[0], m_config.m_idPrompt, QLineEdit::Normal, QString(), &accepted);
                if (!accepted)
                {
                    return;
                }
            }
            beginRun(mode, ids);
        });
    }
    m_stop = new QPushButton(m_config.m_stopText, panel);
    m_stop->setEnabled(false);
    controls->addWidget(m_stop);
    QObject::connect(m_stop, &QPushButton::clicked, this, [this]() {
        beginRun("stop", QString());
    });
    m_status = new QLabel(m_config.m_title, panel);
    m_status->setWordWrap(true);
    m_progress = new QProgressBar(panel);
    m_output = new QPlainTextEdit(panel);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(600);
    layout->addLayout(controls);
    layout->addWidget(m_status);
    layout->addWidget(m_progress);
    layout->addWidget(m_output, 1);
    setCentralWidget(panel);
    for (std::map<TestCaseId, TestCaseMetadata>::const_iterator it = kTestCases.begin(); it != kTestCases.end(); ++it)
    {
        m_output->appendPlainText(QString::number(it->first) + "  " + m_config.m_buttons[it->second.m_type == TestInstant ? 1 : 2] + "  " + it->second.m_description);
    }
    QObject::connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
    QObject::connect(&m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
    QObject::connect(&m_process, &QProcess::started, this, [this]() { m_childPid = m_process.processId(); saveReport(); });
    QObject::connect(&m_timer, &QTimer::timeout, this, &LumaPlayerTest::tick);
    QObject::connect(&m_process, &QProcess::readyReadStandardOutput, this, &LumaPlayerTest::drainOutput);
    QObject::connect(&m_process, &QProcess::readyReadStandardError, this, &LumaPlayerTest::drainOutput);
}

void LumaPlayerTest::start()
{
    m_commandThread = CTaskThreadManager::Instance().Init();
    if (m_commandThread == 0)
    {
        m_status->setText(m_config.m_commandFailed);
        return;
    }
    const QStringList args = QApplication::arguments();
    const int32_t run = args.indexOf("--run");
    const int32_t ids = args.indexOf("--ids");
    if (run >= 0 && run + 1 < args.size())
    {
        beginRun(args[run + 1], ids >= 0 && ids + 1 < args.size() ? args[ids + 1] : QString());
    }
}

LumaPlayerTest::~LumaPlayerTest()
{
    // 主事件循环已经结束，窗口基类尚未析构，任务不会使用悬空this。
    if (m_commandThread != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_commandThread);
    }
}

void LumaPlayerTest::beginRun(const QString& mode, const QString& ids)
{
    if (m_commandPending && mode == "stop")
    {
        if (m_prepareTask != nullptr)
        {
            m_prepareTask->StopTask();
        }
        return;
    }
    if (m_commandPending || (m_active && mode != "stop"))
    {
        return;
    }
    if (mode != "stop" && !validatePaths())
    {
        return;
    }
    try
    {
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_commandThread);
        if (thread == nullptr)
        {
            throw std::runtime_error("test command thread unavailable");
        }
        const QString directory = m_videoPath->text();
        const QString output = m_root + "/temp/fixtures";
        const QString executable = QApplication::applicationFilePath();
        std::shared_ptr<TestCommandTask> task(new TestCommandTask([this, mode, ids, directory, output, executable](const std::atomic<bool>& exit) {
            std::vector<int32_t> selected;
            std::vector<int32_t> skipped;
            QString error;
            QString media;
            QString fixture;
            try
            {
                if (mode != "stop")
                {
                    error = TestCaseRegistry::select(mode, ids, &selected, &skipped);
                    if (error.isEmpty())
                    {
                        error = TestMediaHelper::prepare(directory, output, executable, exit, &media, &fixture);
                    }
                }
            }
            catch (...)
            {
                error = m_config.m_commandFailed;
            }
            QStringList selectedText;
            QStringList skippedText;
            for (size_t i = 0; i < selected.size(); ++i)
            {
                selectedText << QString::number(selected[i]);
            }
            for (size_t i = 0; i < skipped.size(); ++i)
            {
                skippedText << QString::number(skipped[i]);
            }
            emit commandReady(mode, selectedText, skippedText, error, media, fixture);
        }));
        m_prepareTask = task;
        m_commandPending = true;
        m_stop->setEnabled(true);
        if (mode != "stop")
        {
            m_status->setText(m_config.m_mediaPreparing);
        }
        updatePathControls();
        thread->PostTask(task, 1);
    }
    catch (...)
    {
        m_commandPending = false;
        updatePathControls();
        m_status->setText(m_config.m_commandFailed);
        if (m_autoExit)
        {
            QApplication::exit(2);
        }
    }
}

void LumaPlayerTest::executeCommand(const QString& mode, const QStringList& selected,
    const QStringList& skipped, const QString& error, const QString& media, const QString& fixture)
{
    m_commandPending = false;
    m_prepareTask.reset();
    m_stop->setEnabled(m_active);
    updatePathControls();
    if (mode == "stop")
    {
        m_cancel = true;
        return;
    }
    if (m_active)
    {
        return;
    }
    m_selected.clear();
    m_skipped.clear();
    for (int32_t i = 0; i < selected.size(); ++i)
    {
        m_selected.push_back(selected[i].toInt());
    }
    for (int32_t i = 0; i < skipped.size(); ++i)
    {
        m_skipped.push_back(skipped[i].toInt());
    }
    if (!error.isEmpty())
    {
        m_status->setText(error);
        if (m_autoExit)
        {
            QApplication::exit(2);
        }
        return;
    }
    m_media = media;
    m_fixture = fixture;
    m_output->appendPlainText(m_config.m_videoLabel + ": " + m_media);
    const QStringList options = QApplication::arguments();
    const int32_t configurationOption = options.indexOf("--configuration");
    if (configurationOption >= 0 && (configurationOption + 1 >= options.size() ||
        (options[configurationOption + 1] != "release" && options[configurationOption + 1] != "debug" && options[configurationOption + 1] != "both")))
    {
        m_status->setText(m_config.m_invalidConfiguration);
        if (m_autoExit)
        {
            QApplication::exit(2);
        }
        return;
    }
    m_jobs.clear();
    m_records = QJsonArray();
    m_checks = QJsonArray();
    m_index = 0;
    m_done = false;
    m_cancel = false;
    m_active = true;
    m_mode = mode;
    m_batch = m_root + "/reports/" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz");
    QDir().mkpath(m_batch);
    QDir().mkpath(m_root + "/temp");
    const QString base = QApplication::applicationDirPath();
    const QStringList args = QApplication::arguments();
    const int32_t first = 0;
    const int32_t last = m_debugCheck->isChecked() ? 1 : 0;
    updatePathControls();
    for (size_t selectedIndex = 0; selectedIndex < m_selected.size(); ++selectedIndex)
    {
        const int32_t id = m_selected[selectedIndex];
        const TestCaseMetadata& metadata = kTestCases.at(static_cast<TestCaseId>(id));
        for (int32_t configuration = first; configuration <= last; ++configuration)
        {
            const QString config = configuration == 0 ? "Release" : "Debug";
            const int32_t repeats = metadata.m_engine == "player" ? (configuration == 0 ? m_config.m_releaseStarts : m_config.m_debugStarts) : 1;
            for (int32_t iteration = 0; iteration < repeats; ++iteration)
            {
                TestJob job;
                job.m_caseId = id;
                job.m_name = QString::number(id) + " " + metadata.m_description + " / " + config + " / " + QString::number(iteration + 1);
                job.m_player = metadata.m_engine == "player";
                job.m_directory = m_batch + "/" + QString::number(id) + "-" + config + "-" + QString::number(iteration + 1);
                job.m_timeoutMs = metadata.m_timeoutMs;
                const int32_t timeoutOption = args.indexOf("--job-timeout-ms");
                if (timeoutOption >= 0 && timeoutOption + 1 < args.size())
                {
                    bool valid = false;
                    const int32_t timeout = args[timeoutOption + 1].toInt(&valid);
                    if (valid && timeout >= 100 && timeout <= 600000)
                    {
                        job.m_timeoutMs = timeout;
                    }
                }
                if (job.m_player)
                {
                    job.m_program = configuration == 0 ? m_releasePath->text() + "/LumaPlayer1.0.exe" : m_debugPath->text() + "/LumaPlayerd.exe";
                    if (iteration % 4 != 0)
                    {
                        job.m_arguments << (iteration % 4 == 1 ? m_media : (iteration % 4 == 2 ? m_fixture : QFileInfo(m_fixture).absolutePath() + QStringLiteral("/中文 空格.avi")));
                    }
                }
                else
                {
                    job.m_program = configuration == 0 ? base + "/LumaPlayerTest1.0.exe" : m_debugPath->text() + "/LumaPlayerTestd.exe";
                    job.m_arguments << "--mode" << "case" << "--case" << QString::number(id) << "--media" << m_media << "--fixture" << m_fixture << "--output" << job.m_directory;
                }
                m_jobs.push_back(job);
            }
        }
    }
    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        m_buttons[i]->setEnabled(false);
    }
    m_stop->setEnabled(true);
    m_progress->setRange(0, static_cast<int32_t>(m_jobs.size()));
    m_progress->setValue(0);
    m_output->appendPlainText(m_config.m_selectionText + selected.join(","));
    if (!skipped.isEmpty())
    {
        m_output->appendPlainText(m_config.m_skippedText + skipped.join(","));
    }
    m_total.restart();
    m_timer.start(100);
    saveReport();
    QTimer::singleShot(0, this, SLOT(next()));
}

void LumaPlayerTest::next()
{
    if (m_index >= m_jobs.size() || m_cancel)
    {
        m_done = true;
        m_active = false;
        updatePathControls();
        m_timer.stop();
        saveReport();
        for (size_t i = 0; i < m_buttons.size(); ++i)
        {
            m_buttons[i]->setEnabled(true);
        }
        m_stop->setEnabled(false);
        if (m_autoExit)
        {
            bool success = !m_cancel;
            for (int32_t i = 0; i < m_records.size(); ++i)
            {
                success = success && m_records[i].toObject()["passed"].toBool();
            }
            QApplication::exit(success ? 0 : 1);
        }
        else
        {
            QMessageBox::information(this, m_config.m_title, m_status->text() + "\n" + m_root);
        }
        return;
    }
    const TestJob& job = m_jobs[m_index];
    QDir().mkpath(job.m_directory);
    m_terminal = false;
    m_closeSent = false;
    m_timedOut = false;
    m_cancelAt = -1;
    m_childPid = 0;
    m_elapsed.restart();
    m_status->setText(job.m_name + "   " + QString::number(m_index + 1) + "/" + QString::number(m_jobs.size()));
    m_output->appendPlainText(m_config.m_statusNames[4] + " " + job.m_name);
    m_process.setWorkingDirectory(m_root + "/temp");
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    LOGINFO("Test start id=%d program=%s", job.m_caseId, job.m_program.toUtf8().constData());
    m_process.start(job.m_program, job.m_arguments);
    saveReport();
}

void LumaPlayerTest::tick()
{
    if (!m_active || m_terminal || m_index >= m_jobs.size())
    {
        return;
    }
    const TestJob& job = m_jobs[m_index];
    if (job.m_player && !m_closeSent && m_elapsed.elapsed() >= 2500 && TestProcessHelper::hasWindow(m_childPid))
    {
        bool ready = job.m_arguments.isEmpty();
        const QDir directory(QFileInfo(job.m_program).absolutePath());
        const QStringList logs = directory.entryList(QStringList() << ("*_" + QString::number(m_childPid) + "_*.log"), QDir::Files);
        for (int32_t i = 0; i < logs.size() && !ready; ++i)
        {
            QFile file(directory.filePath(logs[i]));
            if (file.open(QIODevice::ReadOnly))
            {
                ready = file.readAll().contains("Window fitted to media");
            }
        }
        if (ready)
        {
            m_closeSent = TestProcessHelper::closeWindow(m_childPid);
        }
    }
    if (m_cancel || m_elapsed.elapsed() > job.m_timeoutMs)
    {
        m_timedOut = !m_cancel;
        if (m_cancelAt < 0)
        {
            m_cancelAt = m_elapsed.elapsed();
            TestProcessHelper::closeWindow(m_childPid);
        }
        if (m_elapsed.elapsed() - m_cancelAt > 2000)
        {
            // 只终止自己创建的测试子进程，取消和超时均不记通过
            m_process.kill();
        }
    }
}

void LumaPlayerTest::drainOutput()
{
    if (m_index >= m_jobs.size())
    {
        return;
    }
    const QByteArray output = m_process.readAllStandardOutput() + m_process.readAllStandardError();
    QFile file(m_jobs[m_index].m_directory + "/process-output.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        file.write(output);
    }
    if (!output.isEmpty())
    {
        m_output->appendPlainText(QString::fromUtf8(output).right(1200));
    }
}

void LumaPlayerTest::finished(int code, QProcess::ExitStatus status)
{
    if (m_terminal || m_index >= m_jobs.size())
    {
        return;
    }
    drainOutput();
    m_terminal = true;
    const TestJob& job = m_jobs[m_index];
    bool passed = code == 0 && status == QProcess::NormalExit && !m_timedOut && !m_cancel;
    if (job.m_player)
    {
        passed = passed && m_closeSent;
        const QDir directory(QFileInfo(job.m_program).absolutePath());
        const QStringList logs = directory.entryList(QStringList() << ("*_" + QString::number(m_childPid) + "_*.log"), QDir::Files);
        for (int32_t i = 0; i < logs.size(); ++i)
        {
            if (!QFile::copy(directory.filePath(logs[i]), job.m_directory + "/" + logs[i]))
            {
                passed = false;
            }
        }
    }
    else
    {
        QFile checks(job.m_directory + "/checks.json");
        bool hasChecks = false;
        if (checks.open(QIODevice::ReadOnly))
        {
            const QJsonArray values = QJsonDocument::fromJson(checks.readAll()).array();
            for (int32_t i = 0; i < values.size(); ++i)
            {
                QJsonObject value = values[i].toObject();
                hasChecks = true;
                passed = passed && value["case"].toInt() == job.m_caseId && value["status"].toString() == "PASS";
                value["job"] = job.m_name;
                m_checks.append(value);
            }
        }
        passed = passed && hasChecks;
    }
    QJsonObject record;
    record["id"] = job.m_caseId;
    record["name"] = job.m_name;
    record["program"] = job.m_program;
    record["pid"] = static_cast<double>(m_childPid);
    record["exitCode"] = static_cast<double>(static_cast<uint32_t>(code));
    record["crashed"] = status == QProcess::CrashExit;
    record["timeout"] = m_timedOut;
    record["canceled"] = m_cancel;
    record["passed"] = passed;
    record["timeoutBudgetMs"] = job.m_timeoutMs;
    record["arguments"] = job.m_arguments.join(" ");
    record["elapsedMs"] = static_cast<double>(m_elapsed.elapsed());
    record["directory"] = job.m_directory;
    m_records.append(record);
    m_output->appendPlainText((m_cancel ? m_config.m_statusNames[3] : m_config.m_statusNames[passed ? 1 : 2]) + " " + job.m_name);
    ++m_index;
    m_progress->setValue(static_cast<int32_t>(m_index));
    saveReport();
    QTimer::singleShot(0, this, SLOT(next()));
}

void LumaPlayerTest::processError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        finished(-1, QProcess::CrashExit);
    }
}

void LumaPlayerTest::saveReport()
{
    std::map<int32_t, int32_t> states;
    std::map<int32_t, int32_t> planned;
    std::map<int32_t, int32_t> completed;
    for (size_t i = 0; i < m_jobs.size(); ++i)
    {
        ++planned[m_jobs[i].m_caseId];
    }
    for (int32_t i = 0; i < m_records.size(); ++i)
    {
        const QJsonObject record = m_records[i].toObject();
        const int32_t id = record["id"].toInt();
        const int32_t state = record["canceled"].toBool() ? 3 : (record["passed"].toBool() ? 1 : 2);
        ++completed[id];
        if (states.count(id) == 0 || state == 2 || (states[id] != 2 && state == 3))
        {
            states[id] = state;
        }
    }
    uint32_t instant = 0, pressure = 0, pass = 0, fail = 0, cancel = 0;
    QString details;
    QJsonArray caseResults;
    for (size_t i = 0; i < m_selected.size(); ++i)
    {
        const int32_t id = m_selected[i];
        const TestCaseMetadata& item = kTestCases.at(static_cast<TestCaseId>(id));
        int32_t state = states.count(id) ? states.at(id) : 0;
        const bool running = m_active && !m_terminal && m_index < m_jobs.size() && m_jobs[m_index].m_caseId == id && m_childPid > 0;
        if ((state != 0 || running) && completed[id] < planned[id] && state != 2)
        {
            state = m_done ? 3 : 4;
        }
        if (state != 0)
        {
            instant += item.m_type == TestInstant;
            pressure += item.m_type == TestPressure;
            pass += state == 1;
            fail += state == 2;
            cancel += state == 3;
        }
        QJsonObject result;
        result["id"] = id;
        result["description"] = item.m_description;
        result["type"] = item.m_type == TestInstant ? "instant" : "pressure";
        result["status"] = m_config.m_statusNames[state];
        caseResults.append(result);
        details += QString::number(id) + " " + m_config.m_statusNames[state] + " " + item.m_description + "\r\n";
    }
    const uint32_t total = instant + pressure;
    const uint32_t notRun = static_cast<uint32_t>(m_selected.size()) - total;
    const QString summary = QString::fromStdWString(CStringManager::Format(m_config.m_summaryFormat.c_str(),
        total, instant, pressure, pass, fail, cancel, notRun, static_cast<uint32_t>(m_skipped.size()), m_total.elapsed() / 1000.0));
    if (m_done)
    {
        m_status->setText(summary);
    }
    QJsonObject report;
    report["state"] = m_done ? "FINISHED" : "RUNNING";
    report["pid"] = static_cast<double>(QApplication::applicationPid());
    report["batch"] = m_batch;
    report["mode"] = m_mode;
    report["realMedia"] = m_media;
    report["generatedFixture"] = m_fixture;
    report["videoDirectory"] = m_videoPath->text();
    report["releaseDirectory"] = m_releasePath->text();
    report["debugDirectory"] = m_debugPath->text();
    report["includeDebug"] = m_debugCheck->isChecked();
    report["summary"] = summary;
    report["executedCases"] = static_cast<int32_t>(total);
    report["instantCases"] = static_cast<int32_t>(instant);
    report["pressureCases"] = static_cast<int32_t>(pressure);
    report["failedCases"] = static_cast<int32_t>(fail);
    report["passedCases"] = static_cast<int32_t>(pass);
    report["canceledCases"] = static_cast<int32_t>(cancel);
    report["notExecutedCases"] = static_cast<int32_t>(notRun);
    report["completedJobs"] = static_cast<int32_t>(m_index);
    report["totalJobs"] = static_cast<int32_t>(m_jobs.size());
    report["jobs"] = m_records;
    report["cases"] = caseResults;
    report["checks"] = m_checks;
    QJsonArray unselected;
    for (std::map<TestCaseId, TestCaseMetadata>::const_iterator it = kTestCases.begin(); it != kTestCases.end(); ++it)
    {
        if (std::find(m_selected.begin(), m_selected.end(), it->first) == m_selected.end())
        {
            unselected.append(static_cast<int32_t>(it->first));
        }
    }
    report["unselectedCases"] = unselected;
    report["limitations"] = m_config.m_limitations;
    QJsonArray skipped;
    for (size_t i = 0; i < m_skipped.size(); ++i)
    {
        skipped.append(m_skipped[i]);
    }
    report["deletedSkipped"] = skipped;
    if (m_index < m_jobs.size())
    {
        report["currentJob"] = m_jobs[m_index].m_name;
        report["currentPid"] = static_cast<double>(m_childPid);
    }
    for (int32_t copy = 0; copy < 2; ++copy)
    {
        const QString directory = copy == 0 ? m_batch : m_root;
        QSaveFile json(directory + "/report.json");
        QSaveFile readable(directory + "/测试报告.txt");
        bool ok = json.open(QIODevice::WriteOnly) && readable.open(QIODevice::WriteOnly);
        if (ok)
        {
            ok = json.write(QJsonDocument(report).toJson()) > 0 &&
                readable.write(QByteArray::fromHex("efbbbf") + (summary + "\r\n" + m_batch + "\r\n" + m_config.m_limitations + "\r\n" + details).toUtf8()) > 0;
            ok = json.commit() && readable.commit() && ok;
        }
        if (!ok)
        {
            LOGERROR("Cannot save test report directory=%s", directory.toUtf8().constData());
            m_cancel = true;
            m_status->setText(directory + " report write failed");
        }
    }
}

void LumaPlayerTest::closeEvent(QCloseEvent* event)
{
    if (m_active || m_commandPending)
    {
        beginRun("stop", QString());
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
    QApplication::quit();
}

void LumaPlayerTest::updatePathControls()
{
    const bool enabled = !m_active && !m_commandPending;
    m_videoPath->setEnabled(enabled);
    m_videoBrowse->setEnabled(enabled);
    m_debugCheck->setEnabled(enabled);
    m_debugPath->setEnabled(enabled && m_debugCheck->isChecked());
    m_debugBrowse->setEnabled(enabled && m_debugCheck->isChecked());
    m_releasePath->setEnabled(enabled);
    m_releaseBrowse->setEnabled(enabled);
}

void LumaPlayerTest::choosePath(bool debug)
{
    QLineEdit* edit = debug ? m_debugPath : m_releasePath;
    const QString path = QFileDialog::getExistingDirectory(this, m_config.m_choosePath, edit->text());
    if (path.isEmpty())
    {
        return;
    }
    const QString exe = debug ? "/LumaPlayerd.exe" : "/LumaPlayer1.0.exe";
    if (!QFileInfo(path + exe).isFile())
    {
        QMessageBox::information(this, m_config.m_title, m_config.m_pathRequired + exe.mid(1));
        return;
    }
    edit->setText(QDir::cleanPath(path));
}

bool LumaPlayerTest::validatePaths()
{
    if (m_videoPath->text().isEmpty() || !QDir(m_videoPath->text()).exists())
    {
        QMessageBox::information(this, m_config.m_title, m_config.m_videoLabel + ": " + m_config.m_choosePath);
        return false;
    }
    QStringList missing;
    if (m_releasePath->text().isEmpty() || !QFileInfo(m_releasePath->text() + "/LumaPlayer1.0.exe").isFile())
    {
        missing << "LumaPlayer1.0.exe";
    }
    if (m_debugCheck->isChecked() && (m_debugPath->text().isEmpty() ||
        !QFileInfo(m_debugPath->text() + "/LumaPlayerd.exe").isFile()))
    {
        missing << "LumaPlayerd.exe";
    }
    QString error;
    if (!missing.isEmpty())
    {
        error = m_config.m_pathRequired + missing.join(", ");
    }
    else if (m_debugCheck->isChecked() && !QFileInfo(m_debugPath->text() + "/LumaPlayerTestd.exe").isFile())
    {
        error = m_config.m_debugRunnerRequired;
    }
    if (!error.isEmpty())
    {
        m_status->setText(error);
        m_output->appendPlainText(error);
        QMessageBox::information(this, m_config.m_title, error);
        return false;
    }
    return true;
}