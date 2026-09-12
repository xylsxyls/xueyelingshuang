#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerTest.h"
#include "CoreTestTask.h"
#include "GuiTestRunner.h"
#include "TestResults.h"
#include "TestCaseRegistry.h"
#include "LumaPlayer.h"
#include "Config.h"
#include "LumaPlayerDialogSession.h"
#include "CDump/CDumpAPI.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QApplication>
#include <QDir>
#include <QTimer>
#include <memory>
#include <exception>

/** 从命令行读取完整参数值
@param [in] arguments 已按操作系统规则解析的参数
@param [in] name 选项名
@param [in] fallback 缺失默认值
@return 选项后的值或默认值
*/
static QString ReadOption(const QStringList& arguments, const QString& name, const QString& fallback)
{
    const int32_t index = arguments.indexOf(name);
    return index >= 0 && index + 1 < arguments.size() ? arguments[index + 1] : fallback;
}

int main(int argc, char* argv[])
{
    const bool dump = CDump::declareDumpFile();
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    const QStringList args = app.arguments();
    const QString root = ReadOption(args, "--output", QApplication::applicationDirPath());
    const QString media = ReadOption(args, "--media", QString());
    const QString fixture = ReadOption(args, "--fixture", QString());
    const QString mode = ReadOption(args, "--mode", "runner");
    const int32_t caseId = ReadOption(args, "--case", "0").toInt();
    if (mode == "case" && kTestCases.count(static_cast<TestCaseId>(caseId)) == 0)
    {
        return 2;
    }
    QDir().mkpath(root);
    LogManagerConfig log;
    log.m_path = root.toUtf8().constData();
    log.m_outputConsole = false;
    log.m_archiveOldLog = true;
    LogManager::instance().init(log);
    LOGINFO("LumaPlayerTest1.0 startup mode=%s dump=%d", mode.toUtf8().constData(), dump ? 1 : 0);
    int result = 1;
    uint32_t threadId = 0;
    try
    {
        if (mode == "runner")
        {
            LumaPlayerTest window(root, media, fixture, args.contains("--self-check"));
            window.show();
            QTimer::singleShot(0, &window, [&window]() { window.start(); });
            result = app.exec();
        }
        else if (mode == "media-probe")
        {
            FFmpegCpp video;
            result = video.open(media.toUtf8().constData()) && video.hasVideo() &&
                video.durationMilliseconds() > 0 ? 0 : 1;
            video.close();
        }
        else if (mode.startsWith("self-"))
        {
            TestResults report(root);
            if (mode == "self-timeout")
            {
                QTimer timer;
                timer.start(1000);
                result = app.exec();
            }
            else
            {
                report.check(21, mode == "self-pass", "test runner intentional self-check");
                result = mode == "self-pass" ? 0 : 7;
            }
        }
        else if (mode == "case" && kTestCases.at(static_cast<TestCaseId>(caseId)).m_engine == "gui")
        {
            g_config.init(QStringList());
            if (caseId == CaseFractionalKeys)
            {
                g_config.m_keyboardSeekSeconds = 0.5;
            }
            g_config.m_logInitialized.store(true);
            LumaPlayerDialogSession dialogs;
            LumaPlayer player;
            GuiTestRunner runner(&player, fixture, root, QString::number(caseId));
            player.show();
            runner.start();
            result = app.exec();
            g_config.m_logInitialized.store(false);
        }
        else
        {
            std::shared_ptr<TestRunState> state(new TestRunState());
            std::shared_ptr<CoreTestTask> task(new CoreTestTask(QString::number(caseId), media, fixture, root, state));
            threadId = CTaskThreadManager::Instance().Init();
            std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
            if (thread == nullptr)
            {
                throw std::runtime_error("test worker creation failed");
            }
            thread->PostTask(task, 1);
            thread.reset();
            QTimer timer;
            QObject::connect(&timer, &QTimer::timeout, &app, [state]() {
                if (state->m_done.load())
                {
                    QApplication::exit(state->m_result.load());
                }
            });
            timer.start(20);
            result = app.exec();
            task->StopTask();
            // 事件循环已经结束，工作任务已发布实际完成；最终按管理器ID回收，不UninitAll
            CTaskThreadManager::Instance().Uninit(threadId);
            threadId = 0;
        }
    }
    catch (const std::exception& error)
    {
        LOGERROR("Test main exception: %s", error.what());
    }
    catch (...)
    {
        LOGERROR("Test main unknown exception");
    }
    if (threadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(threadId);
    }
    LOGINFO("LumaPlayerTest finish result=%d", result);
    LogManager::instance().uninit(0);
    return result;
}