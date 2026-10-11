#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerTest.h"
#include "CoreTestTask.h"
#include "GuiTestRunner.h"
#include "TestResults.h"
#include "TestCaseRegistry.h"
#include "TestMediaHelper.h"
#include "LumaPlayer.h"
#include "LumaPlayerHelper.h"
#include "Config.h"
#include "LumaPlayerDialogSession.h"
#include "CDump/CDumpAPI.h"
#include "DialogManager/DialogManagerAPI.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QSaveFile>
#include <QLibrary>
#include <memory>
#include <exception>

/** 将DialogManager日志回调交给LogManager
@param [in] level DialogManager日志级别
@param [in] message 借用的诊断文本，空值使用入口约定的默认文本
*/
static void ForwardDialogLog(DialogLogLevel level, const char* message)
{
    if (message == nullptr)
    {
        return;
    }
    switch (level)
    {
    case DIALOG_LOG_ERROR:
    {
        LOGERROR("%s", message);
        break;
    }
    case DIALOG_LOG_WARNING:
    {
        LOGWARNING("%s", message);
        break;
    }
    default:
    {
        LOGINFO("%s", message);
        break;
    }
    }
}

/** 读取命令行选项的后继值，缺失时使用默认值
@param [in] arguments 命令行参数列表
@param [in] name 要查找的参数名称
@param [in] fallback 未提供参数时的默认值
@return 选项值或调用方给定的默认值
*/
static QString ReadOption(const QStringList& arguments, const QString& name, const QString& fallback)
{
    const int32_t index = arguments.indexOf(name);
    return index >= 0 && index + 1 < arguments.size() ? arguments[index + 1] : fallback;
}

int main(int argc, char* argv[])
{
    const uint32_t karaokeRuntimeError = LumaPlayerHelper::prepareKaraokeRuntime();
    // Optional regression fixture: a native file-dialog extension can load its private old CRT.
    const QByteArray shellRuntimePath = qgetenv("LUMAPLAYER_TEST_SHELL_RUNTIME");
    QLibrary shellRuntime(QString::fromUtf8(shellRuntimePath));
    if (!shellRuntimePath.isEmpty() && !shellRuntime.load())
    {
        return 3;
    }
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
    DialogManager::setLogCallback(ForwardDialogLog);
    LOGINFO("LumaPlayerTest1.0 startup mode=%s dump=%d", mode.toUtf8().constData(), dump ? 1 : 0);
    LOGINFO("Karaoke system CRT bootstrap Win32=%u shellFixture=%s", karaokeRuntimeError, shellRuntimePath.constData());
    int result = 1;
    uint32_t threadId = 0;
    try
    {
        // Expected failures leave this block and still run the shared shutdown below.
        do
        {
            if (mode == "runner")
            {
                LumaPlayerDialogSession dialogs;
                LumaPlayerTest window(root, media, fixture, args.contains("--self-check"));
                window.show();
                QTimer::singleShot(0, &window, [&window]()
                    {
                        window.start();
                    });
                result = app.exec();
            }
            else if (mode == "media-probe")
            {
                FFmpegCpp video;
                result = video.open(media.toUtf8().constData()) && video.hasVideo() &&
                        video.durationMilliseconds() > 0
                    ? 0
                    : 1;
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
                QString guiFixture = fixture;
                if (guiFixture.isEmpty())
                {
                    const std::atomic<bool> canceled(false);
                    const QByteArray data = TestMediaHelper::generate(canceled);
                    guiFixture = root + "/fixture.avi";
                    QSaveFile file(guiFixture);
                    if (data.isEmpty() || !file.open(QIODevice::WriteOnly) ||
                        file.write(data) != data.size() || !file.commit())
                    {
                        LOGERROR("Cannot generate GUI fixture");
                        break;
                    }
                }
                g_config.init(QStringList());
                if (caseId == CaseKaraokeDeviceMonitor && args.contains("--microphone-buffer-ms"))
                {
                    bool valid = false;
                    const int32_t bufferMs = ReadOption(args, "--microphone-buffer-ms", "").toInt(&valid);
                    if (!valid || bufferMs < 10 || bufferMs > 200)
                    {
                        LOGERROR("Microphone buffer comparison range is 10..200 ms");
                        break;
                    }
                    g_config.m_microphoneDeviceBufferMs = bufferMs;
                }
                if (caseId == CaseFractionalKeys)
                {
                    g_config.m_keyboardSeekSeconds = 0.5;
                }
                g_config.m_logInitialized.store(true);
                LumaPlayerDialogSession dialogs;
                LumaPlayer player;
                GuiTestRunner runner(&player, guiFixture, root, QString::number(caseId));
                player.show();
                runner.start();
                result = app.exec();
                if (!runner.completed())
                {
                    result = 1;
                }
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
                    LOGERROR("Test worker creation failed");
                    break;
                }
                thread->PostTask(task, 1);
                thread.reset();
                QTimer timer;
                QObject::connect(&timer, &QTimer::timeout, &app, [state]()
                    {
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
        } while (false);
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
    DialogManager::setLogCallback(nullptr);
    LogManager::instance().uninit(0);
    return result;
}