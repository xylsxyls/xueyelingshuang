#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "GuiTestRunner.h"
#include "TestCaseRegistry.h"
#include "LumaPlayerAudioRender.h"
#include "LumaPlayerVideoRender.h"
#include "LumaPlayerCoreBridge.h"
#include "LumaPlayerLogicController.h"
#include "LumaPlayerKaraokeProgressView.h"
#include "Config.h"
#include "LogManager/LogManagerAPI.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include "QtControls/DialogBase.h"
#include "QtControls/Widget.h"
#include "QtControls/Menu.h"
#include <QtWidgets>
#include <QElapsedTimer>
// 仅测试观察器读取现有成员，不修改其状态，不影响产品编译与布局
#define private public
#include "LumaPlayer.h"
#undef private
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <algorithm>
#include <thread>
#include <chrono>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

bool GuiTestRunner::clickDesktopPoint(const QPoint& point)
{
    QCursor::setPos(point);
#ifdef Q_OS_WIN
    const HWND foreground = GetForegroundWindow();
    const HWND hit = WindowFromPoint(POINT{point.x(), point.y()});
    DWORD foregroundProcess = 0, hitProcess = 0;
    GetWindowThreadProcessId(foreground, &foregroundProcess);
    GetWindowThreadProcessId(hit, &hitProcess);
    QWidget* target = QApplication::widgetAt(point);
    LOGINFO("Native click x=%d y=%d target=%s name=%s foregroundPid=%u hitPid=%u leftDown=%d",
        point.x(), point.y(), target ? target->metaObject()->className() : "none",
        target ? target->objectName().toUtf8().constData() : "none",
        foregroundProcess, hitProcess, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0);
    INPUT input[2] = {};
    input[0].type = input[1].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    return SendInput(2, input, sizeof(INPUT)) == 2;
#else
    QPointer<QWidget> target = QApplication::widgetAt(point);
    if (!target)
    {
        return false;
    }
    const QPoint local = target->mapFromGlobal(point);
    QMouseEvent press(QEvent::MouseButtonPress, local, point, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, point, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    if (target)
    {
        QApplication::sendEvent(target, &release);
    }
    return true;
#endif
}

bool GuiTestRunner::acceptDesktopSavePath(const QString& path)
{
#ifdef Q_OS_WIN
    std::vector<INPUT> input;
    /** 将一个系统按键追加到待提交输入序列，不立即发送
    @param [in] value Windows虚拟键码
    @param [in] flags Windows键盘输入标记
    */
    const auto key = [&input](WORD value, DWORD flags)
    {
        INPUT event = {};
        event.type = INPUT_KEYBOARD;
        event.ki.wVk = value;
        event.ki.dwFlags = flags;
        input.push_back(event);
    };
    key(VK_MENU, 0);
    key('N', 0);
    key('N', KEYEVENTF_KEYUP);
    key(VK_MENU, KEYEVENTF_KEYUP);
    key(VK_CONTROL, 0);
    key('A', 0);
    key('A', KEYEVENTF_KEYUP);
    key(VK_CONTROL, KEYEVENTF_KEYUP);
    for (int i = 0; i < path.size(); ++i)
    {
        INPUT event = {};
        event.type = INPUT_KEYBOARD;
        event.ki.wScan = path[i].unicode();
        event.ki.dwFlags = KEYEVENTF_UNICODE;
        input.push_back(event);
        event.ki.dwFlags |= KEYEVENTF_KEYUP;
        input.push_back(event);
    }
    key(VK_RETURN, 0);
    key(VK_RETURN, KEYEVENTF_KEYUP);
    return SendInput(static_cast<UINT>(input.size()), input.data(), sizeof(INPUT)) == input.size();
#else
    Q_UNUSED(path);
    return false;
#endif
}

GuiTestRunner::GuiTestRunner(LumaPlayer* player, const QString& media, const QString& directory, const QString& mode) :
QObject(nullptr),
m_player(player),
m_results(directory, mode.toInt()),
m_directory(directory),
m_index(0),
m_started(false),
m_aborting(false),
m_position(0),
m_dragMatches(0),
m_unexpectedPause(false),
m_monitorFrames(false),
m_observedFrame(0),
m_lastFrameMs(0),
m_maxFrameGapMs(0),
m_originalHelpWidth(g_config.m_helpWidth)
{
    QObject::connect(&m_timer, &QTimer::timeout, this, &GuiTestRunner::tick);
    const int32_t id = mode.toInt();
    if (id == CaseEmptyUi)
    {
        add(4, "empty window top reveal and bottom absent", [this]()
            {
                move(QPoint(100, 5));
            },
            [this]()
            {
                return m_player->m_topVisibleHeight > 0 && m_player->m_bottomVisibleHeight == 0;
            });
        add(4, "central plus has no tooltip", [this]()
            {
                move(m_player->plusButtonRect().center());
            },
            [this]()
            {
                return m_elapsed.elapsed() > 200 && !QToolTip::isVisible();
            });
        add(14, "pin empty window keeps bottom hidden", [this]()
            {
                move(QPoint(100, 5));
            },
            [this]()
            {
                return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight;
            });
        add(14, "pin via real button", [this]()
            {
                click(m_player->pinButtonRect().center());
            },
            [this]()
            {
                return m_player->m_pinned && m_player->m_bottomVisibleHeight == 0;
            });
    }
    else
    {
        add(0, "prepare top overlay", [this]()
            {
                move(QPoint(100, 5));
            },
            [this]()
            {
                return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight;
            });
        add(0, "prepare fixed controls", [this]()
            {
                click(m_player->pinButtonRect().center());
            },
            [this]()
            {
                return m_player->m_pinned;
            });
        add(3, "load fixture", [this, media]()
            {
                m_player->loadMedia(media);
            },
            [this]()
            {
                return m_player->m_hasMedia && !m_player->m_pendingMediaLoad &&
                    m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying && !m_player->m_cachedFrame.isNull();
            },
            15000);
        if (id != CaseProgressClick && id != CaseAbRateLatency && id != CaseRateSeekLoopRace &&
            id != CaseInitialMediaFit && id != CaseHelpUi)
        {
            add(0, "prepare pause", [this]()
                {
                    if (m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying)
                    {
                        m_player->m_core.pauseAsync();
                    }
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused && m_player->m_bottomVisibleHeight == m_player->bottomOverlayHeight();
                });
        }
        if (id == CaseAbMenu || id == CasePausedMenu || id == CaseFrameKey || id == CaseResetUi)
        {
            add(0, "prepare A", [this]()
                {
                    menuAt(10000000, 0);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopA && !m_player->m_hasDragPosition;
                });
            add(0, "prepare B", [this]()
                {
                    menuAt(20000000, 1);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopB && !m_player->m_hasDragPosition;
                });
        }
        switch (id)
        {
        case CaseKaraokeSmallPopup:
        {
            add(63, "small player opens complete DialogManager tool window", [this]()
                {
                    m_player->resize(m_player->minimumSize());
                    click(m_player->microphoneButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_microphonePopupVisible &&
                        m_player->m_microphoneView && m_player->m_microphoneView->isVisible() &&
                        !m_player->rect().contains(m_player->microphonePopupRect()) &&
                        m_player->m_microphoneView->size() == QSize(360, 430);
                });
            add(63, "all controls visible without scrolling and inside desktop", [this]()
                {
                    m_player->m_microphoneView->window()->grab().save(m_directory + "/small-popup.png");
                },
                [this]()
                {
                    return m_player->microphonePopupRect().contains(m_player->microphoneMuteButtonRect()) &&
                        QApplication::desktop()->availableGeometry(m_player).contains(m_player->microphonePanelGeometry()) &&
                        m_player->m_microphoneView->findChildren<QScrollBar*>().isEmpty();
                });
            add(63, "small popup right edge matches volume popup and stays inside player right", []()
                {

                },
                [this]()
                {
                    return m_player->microphonePopupRect().right() == m_player->volumePopupRect().right() &&
                        m_player->microphonePopupRect().right() < m_player->width() &&
                        m_player->m_microphoneView->window()->geometry() == m_player->microphonePanelGeometry();
                });
            if (!LumaPlayerMicrophoneCapture::availableDeviceNames().isEmpty())
            {
                add(63, "native device dropdown click remains usable outside video bounds", [this]()
                    {
                        selectMicrophoneDevice(0, 0);
                    },
                    [this]()
                    {
                        return m_position == 1 && m_player->currentMicrophone().m_device == 0 &&
                            m_player->currentMicrophone().m_muted && m_player->m_microphoneView &&
                            m_player->m_microphonePopupVisible;
                    },
                    3000);
            }
            add(63, "selected mic keyboard adds 10 displayed points (100 actual gain points)", [this]()
                {
                    move(m_player->microphoneGainTrackRect().center());
                    key(Qt::Key_Up);
                    LOGINFO("Mic gain check gain=%d step=%d volume=%d popup=%d pointer=%d modal=%d", m_player->currentMicrophone().m_gain,
                        g_config.m_microphoneGainStep, m_player->m_volumePercent, m_player->m_microphonePopupVisible ? 1 : 0,
                        m_player->microphonePopupRect().contains(m_player->mapFromGlobal(QCursor::pos())) ? 1 : 0,
                        m_player->modalBlocksInput() ? 1 : 0);
                },
                [this]()
                {
                    return m_player->currentMicrophone().m_gain == 110 &&
                        m_player->currentMicrophone().m_task.m_gainPercent.load() == 1100 && m_player->m_volumePercent == 100;
                });
            add(63, "add microphone two initially muted and independently at 100 percent", [this]()
                {
                    click(m_player->microphoneAddRect().center());
                },
                [this]()
                {
                    return m_player->m_microphones.size() == 2 && m_player->m_selectedMicrophone == 1 &&
                        m_player->currentMicrophone().m_muted && m_player->currentMicrophone().m_gain == 100;
                });
            add(63, "gain target follows highlighted mic", [this]()
                {
                    move(m_player->microphoneGainTrackRect().center());
                    key(Qt::Key_Down);
                    const QRect slot = m_player->microphoneSlotRect(0);
                    click(QPoint(slot.center().x(), slot.top() + 8));
                },
                [this]()
                {
                    return m_player->m_selectedMicrophone == 0 && m_player->currentMicrophone().m_gain == 110 &&
                        m_player->m_microphones[1]->m_gain == 90;
                });
            add(63, "add mic three with equally divided device columns", [this]()
                {
                    click(m_player->microphoneAddRect().center());
                },
                [this]()
                {
                    return m_player->m_microphones.size() == 3 && m_player->m_selectedMicrophone == 2 &&
                        std::abs(m_player->microphoneSlotRect(0).width() - m_player->microphoneSlotRect(2).width()) <= 1;
                });
            if (LumaPlayerMicrophoneCapture::availableDeviceNames().size() > 1)
            {
                add(63, "one click on unselected mic device name opens dropdown and switches device", [this]()
                    {
                        selectMicrophoneDevice(0, 1);
                    },
                    [this]()
                    {
                        return m_position == 1 && m_player->m_selectedMicrophone == 0 &&
                            m_player->currentMicrophone().m_device == 1 && m_player->currentMicrophone().m_muted;
                    },
                    3000);
                add(63, "select another lane header without opening device menu", [this]()
                    {
                        const QRect slot = m_player->microphoneSlotRect(2);
                        click(QPoint(slot.center().x(), slot.top() + 8));
                    },
                    [this]()
                    {
                        return m_player->m_selectedMicrophone == 2 && !QApplication::activePopupWidget();
                    });
            }
            add(63, "gain clamps at display500/actual5000 while drag top is display100/actual1000", [this]()
                {
                    move(m_player->microphoneGainTrackRect().center());
                    for (int i = 0; i < 60; ++i)
                    {
                        key(Qt::Key_Up);
                    }
                    m_results.check(63, m_player->currentMicrophone().m_gain == 500 && m_player->currentMicrophone().m_task.m_gainPercent.load() == 5000, "keyboard upper gain bound maps to 50x PCM");
                    click(m_player->microphoneGainTrackRect().topLeft());
                },
                [this]()
                {
                    return m_player->currentMicrophone().m_gain == 100 &&
                        m_player->currentMicrophone().m_task.m_gainPercent.load() == 1000 && m_player->m_microphones[0]->m_gain == 110;
                });
            add(63, "escape dismisses tool window without changing microphone states", [this]()
                {
                    key(Qt::Key_Escape);
                },
                [this]()
                {
                    return !m_player->m_microphonePopupVisible && !m_player->m_microphoneView;
                });
            add(63, "reopen retains independent mic controls", [this]()
                {
                    click(m_player->microphoneButtonRect().center());
                },
                [this]()
                {
                    if (m_player->m_microphoneView)
                    {
                        m_player->m_microphoneView->window()->grab().save(m_directory + "/three-microphones.png");
                    }
                    return m_player->m_microphoneView && m_player->m_microphoneView->isVisible() && m_player->m_microphones.size() == 3 &&
                        m_player->m_microphones[0]->m_gain == 110 && m_player->m_microphones[1]->m_gain == 90;
                });
            add(63, "recording session prevents removal without losing lane state", [this]()
                {
                    m_player->m_karaokeSessionStarted = true;
                    click(m_player->microphoneRemoveRect(1).center());
                    m_results.check(63, m_player->m_microphones.size() == 3 && m_player->m_microphones[1]->m_gain == 90, "unsaved recording locks removal");
                    m_player->m_karaokeSessionStarted = false;
                },
                []()
                {
                    return true;
                });
            add(63, "remove middle lane and preserve selected third lane identity", [this]()
                {
                    click(m_player->microphoneRemoveRect(1).center());
                },
                [this]()
                {
                    return m_player->m_microphones.size() == 2 && m_player->m_selectedMicrophone == 1 &&
                        m_player->currentMicrophone().m_slot == 2 && m_player->currentMicrophone().m_gain == 100 &&
                        !m_player->m_retiredMicrophones.empty() && m_player->m_retiredMicrophones[0]->m_retired.load();
                });
            add(63, "add reuses freed identity without renumbering other recording tracks", [this]()
                {
                    click(m_player->microphoneAddRect().center());
                },
                [this]()
                {
                    return m_player->m_microphones.size() == 3 && m_player->m_selectedMicrophone == 2 &&
                        m_player->currentMicrophone().m_slot == 1 && m_player->m_microphones[1]->m_slot == 2 &&
                        m_player->currentMicrophone().m_muted && m_player->microphoneRemoveRect(0).isEmpty();
                });
            add(63, "mouse bottom and keyboard lower bound both map to actual zero", [this]()
                {
                    click(m_player->microphoneGainTrackRect().bottomLeft());
                    move(m_player->microphoneGainTrackRect().center());
                    key(Qt::Key_Down);
                },
                [this]()
                {
                    return m_player->currentMicrophone().m_gain == 0 &&
                        m_player->currentMicrophone().m_task.m_gainPercent.load() == 0;
                });
            add(63, "normal window keeps popup aligned after resize and movement", [this]()
                {
                    const QRect screen = QApplication::desktop()->availableGeometry(m_player);
                    m_player->resize(640, 480);
                    m_player->move(screen.right() - 680, screen.top() + 100);
                },
                [this]()
                {
                    return m_player->m_microphoneView &&
                        m_player->m_microphoneView->window()->geometry() == m_player->microphonePanelGeometry() &&
                        m_player->microphonePopupRect().right() == m_player->volumePopupRect().right();
                });
            add(63, "dismiss covering popup before operating player title buttons", [this]()
                {
                    key(Qt::Key_Escape);
                },
                [this]()
                {
                    return !m_player->m_microphonePopupVisible && !m_player->m_microphoneView;
                });
            add(63, "unpin and resume playback for desktop popup interaction", [this]()
                {
                    click(m_player->pinButtonRect().center());
                    click(m_player->playButtonRect().center());
                },
                [this]()
                {
                    return !m_player->m_pinned && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(63, "playing unpinned window opens right-aligned tool window", [this]()
                {
                    click(m_player->microphoneButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_microphonePopupVisible && m_player->m_microphoneView &&
                        m_player->m_microphoneView->isVisible() && m_player->m_bottomVisibleHeight == m_player->bottomOverlayHeight() &&
                        m_player->microphonePopupRect().right() == m_player->volumePopupRect().right();
                });
            if (LumaPlayerMicrophoneCapture::availableDeviceNames().size() > 1)
            {
                add(63, "device dropdown remains selectable during unpinned playback", [this]()
                    {
                        selectMicrophoneDevice(0, 0);
                    },
                    [this]()
                    {
                        return m_position == 1 && m_player->currentMicrophone().m_device == 0 &&
                            m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying && m_player->m_microphonePopupVisible;
                    },
                    3000);
            }
            add(63, "capture aligned player and popup", [this]()
                {
                    const QRect bounds = m_player->geometry().united(m_player->microphonePanelGeometry());
                    QApplication::primaryScreen()->grabWindow(0, bounds.x(), bounds.y(), bounds.width(), bounds.height()).save(m_directory + "/popup-aligned.png");
                },
                []()
                {
                    return true;
                });
            break;
        }
        case CaseKaraokeSaveDropdown:
        {
            add(66, "play while save choices are open without capture or model", [this]()
                {
                    click(m_player->playButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(66, "open managed recording save choices", [this]()
                {
                    QTimer::singleShot(0, this, [this]()
                        {
                            CustomDialogParam prompt(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 2));
                            prompt.m_parent = m_player->windowHandle();
                            DialogManager::instance().makeDialog(prompt);
                            m_results.check(66, prompt.m_result == ERROR_RESULT, "cancel returns without saving");
                        });
                },
                []()
                {
                    QWidget* modal = QApplication::activeModalWidget();
                    return modal && modal->findChildren<QRadioButton*>().size() == 4;
                });
            add(66, "inline defaults are mixed audio and MP3, without popup windows", [this]()
                {
                    QWidget* modal = QApplication::activeModalWidget();
                    if (!modal)
                    {
                        return;
                    }
                    const QList<QRadioButton*> choices = modal->findChildren<QRadioButton*>();
                    m_results.check(66, modal->findChildren<QComboBox*>().isEmpty() && !QApplication::activePopupWidget(),
                        "save options do not create dropdown popup windows");
                    for (int i = 0; i < choices.size(); ++i)
                    {
                        m_results.check(66, choices[i]->isVisible() && choices[i]->window() == modal && choices[i]->isChecked() == (choices[i]->objectName() == "recordingAudio1" || choices[i]->objectName() == "recordingFormat0"), "inline option visibility and default selection");
                    }
                    modal->grab().save(m_directory + "/save-options-inline.png");
                },
                []()
                {
                    return true;
                });
            for (int turn = 0; turn < 4; ++turn)
            {
                const int choice = turn % 2;
                const int row = turn < 2 ? (choice == 1 ? 1 : 0) : (choice == 1 ? 0 : 1);
                add(66, "select inline option through native mouse before any save confirmation", [this, choice, row]()
                    {
                        QWidget* modal = QApplication::activeModalWidget();
                        QRadioButton* button = modal ? modal->findChild<QRadioButton*>(
                                                           QString("recording%1%2").arg(choice == 0 ? "Audio" : "Format").arg(row))
                                                     : nullptr;
                        if (!button)
                        {
                            return;
                        }
                        const QPoint target = button->mapToGlobal(QPoint(10, button->height() / 2));
                        m_results.check(66, QApplication::widgetAt(target) == button, "native hit target is the inline radio button");
                        m_results.check(66, clickDesktopPoint(target), "native radio click submitted");
                        QTimer::singleShot(500, this, [this]()
                            {
                                QWidget* modal = QApplication::activeModalWidget();
                                const QList<QRadioButton*> buttons = modal ? modal->findChildren<QRadioButton*>() : QList<QRadioButton*>();
                                for (int i = 0; i < buttons.size(); ++i)
                                {
                                    LOGINFO("Inline save choice %s checked=%d", buttons[i]->objectName().toUtf8().constData(), buttons[i]->isChecked() ? 1 : 0);
                                }
                                if (modal)
                                {
                                    modal->grab().save(m_directory + "/save-options-selected.png");
                                }
                            });
                    },
                    [this, turn]()
                    {
                        QWidget* modal = QApplication::activeModalWidget();
                        if (!modal || m_elapsed.elapsed() <= 1000 || QApplication::activePopupWidget() ||
                            m_player->m_snapshot.m_state != LumaPlayerCoreCStatePlaying)
                        {
                            return false;
                        }
                        const QList<QRadioButton*> choices = modal->findChildren<QRadioButton*>();
                        if (choices.size() != 4)
                        {
                            return false;
                        }
                        for (int i = 0; i < choices.size(); ++i)
                        {
                            const QString name = choices[i]->objectName();
                            const bool expected = name == QString("recordingAudio%1").arg(turn < 2 ? 0 : 1) ||
                                name == QString("recordingFormat%1").arg(turn == 1 || turn == 2 ? 1 : 0);
                            if (choices[i]->isChecked() != expected)
                            {
                                return false;
                            }
                        }
                        return true;
                    });
            }
            add(66, "cancel choices and keep playing", []()
                {
                    QWidget* modal = QApplication::activeModalWidget();
                    const QList<QPushButton*> buttons = modal ? modal->findChildren<QPushButton*>() : QList<QPushButton*>();
                    for (int i = 0; i < buttons.size(); ++i)
                    {
                        if (buttons[i]->text() == QStringLiteral("取消"))
                        {
                            QTimer::singleShot(0, buttons[i], SLOT(click()));
                        }
                    }
                },
                [this]()
                {
                    return !QApplication::activeModalWidget() && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            break;
        }
        case CaseKaraokeSaveLifecycle:
        {
            add(65, "open native media chooser before initializing model runtime", [this]()
                {
                    clickDesktopPoint(m_player->mapToGlobal(m_player->loadButtonRect().center()));
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 800 && m_player->findChild<QFileDialog*>() != nullptr;
                });
            add(65, "cancel native media chooser before model initialization", [this]()
                {
                    QFileDialog* chooser = m_player->findChild<QFileDialog*>();
                    if (chooser)
                    {
                        static_cast<QDialog*>(chooser)->reject();
                    }
                },
                [this]()
                {
                    return !m_player->findChild<QFileDialog*>() && !m_player->m_pendingMediaLoad &&
                        m_player->m_hasMedia && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            add(65, "unpin before opening karaoke", [this]()
                {
                    m_player->resize(640, 480);
                    click(m_player->pinButtonRect().center());
                },
                [this]()
                {
                    return !m_player->m_pinned && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            add(65, "open independent microphone window", [this]()
                {
                    move(QPoint(m_player->width() / 2, m_player->height() - 2));
                },
                [this]()
                {
                    return m_player->m_bottomVisibleHeight == m_player->bottomOverlayHeight();
                });
            add(65, "open karaoke controls", [this]()
                {
                    click(m_player->microphoneButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_microphoneView && m_player->m_microphoneView->isVisible();
                });
            add(65, "generate backing from native input before recording", [this]()
                {
                    clickDesktopPoint(m_player->mapToGlobal(m_player->microphoneRemovalSwitchRect().center()));
                },
                [this]()
                {
                    return m_player->m_karaokeBacking != nullptr && !m_player->m_karaokeFinishing &&
                        QApplication::activeModalWidget() == nullptr;
                },
                90000);
            add(65, "reopen microphone panel after separation", [this]()
                {
                    if (!m_player->m_microphonePopupVisible)
                    {
                        click(m_player->microphoneButtonRect().center());
                    }
                },
                [this]()
                {
                    return m_player->m_microphoneView && m_player->m_microphoneView->isVisible();
                });
            add(65, "start zero-gain capture without exporting user audio", [this]()
                {
                    click(m_player->microphoneGainTrackRect().bottomLeft());
                    click(m_player->microphoneMuteButtonRect().center());
                },
                [this]()
                {
                    return m_player->currentMicrophone().m_task.hasReceivedPcm() &&
                        m_player->currentMicrophone().m_gain == 0;
                },
                6000);
            add(65, "resume fixture before recording", [this]()
                {
                    click(m_player->playButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(65, "reopen microphone panel after playback click", [this]()
                {
                    if (!m_player->m_microphonePopupVisible)
                    {
                        click(m_player->microphoneButtonRect().center());
                    }
                },
                [this]()
                {
                    return m_player->m_microphoneView && m_player->m_microphoneView->isVisible();
                });
            add(65, "start recording from native mouse event", [this]()
                {
                    m_results.check(65, clickDesktopPoint(m_player->mapToGlobal(m_player->microphoneRecordSwitchRect().center())),
                        "native record click submitted");
                },
                [this]()
                {
                    return m_player->m_karaokeRecording && m_player->m_karaokeRecordingReady;
                });
            add(65, "record a short interval", []()
                {

                },
                [this]()
                {
                    uint64_t frames = 0;
                    return m_elapsed.elapsed() > 700 &&
                        m_player->m_core.karaokeCapturedFrameCount(&frames) == 0 && frames >= 4800;
                });
            add(65, "reveal toolbar while recording", [this]()
                {
                    move(QPoint(m_player->width() / 2, m_player->height() - 2));
                },
                [this]()
                {
                    return m_player->m_bottomVisibleHeight == m_player->bottomOverlayHeight();
                });
            add(65, "reopen microphone window while recording", [this]()
                {
                    if (!m_player->m_microphonePopupVisible)
                    {
                        click(m_player->microphoneButtonRect().center());
                    }
                },
                [this]()
                {
                    return m_player->m_microphoneView && m_player->m_microphoneView->isVisible();
                });
            for (int attempt = 0; attempt < 3; ++attempt)
            {
                add(65, "stop reaches save choices and survives popup retirement", [this, attempt]()
                    {
                        if (attempt == 1)
                        {
                            const std::shared_ptr<bool> returned(new bool(false));
                            const std::shared_ptr<bool> nested(new bool(false));
                            QTimer::singleShot(1500, this, [returned, nested]()
                                {
                                    if (*returned)
                                    {
                                        return;
                                    }
                                    *nested = true;
                                    QWidget* modal = QApplication::activeModalWidget();
                                    const QList<QPushButton*> buttons = modal ? modal->findChildren<QPushButton*>() : QList<QPushButton*>();
                                    for (int i = 0; i < buttons.size(); ++i)
                                    {
                                        if (buttons[i]->text() == QStringLiteral("取消"))
                                        {
                                            buttons[i]->click();
                                            break;
                                        }
                                    }
                                });
                            const QPointer<QWidget> sender = m_player->m_microphoneView;
                            click(m_player->microphoneRecordSwitchRect().center());
                            *returned = true;
                            const bool inputReturned = !*nested && sender && !QApplication::activeModalWidget() && !m_player->m_karaokeFinishing;
                            m_results.check(65, inputReturned,
                                "popup input returns before modal work can destroy its event receiver");
                            if (!inputReturned)
                            {
                                QApplication::exit(1);
                            }
                        }
                        else
                        {
                            m_results.check(65, clickDesktopPoint(m_player->mapToGlobal(m_player->microphoneRecordSwitchRect().center())),
                                "native stop click submitted");
                        }
                    },
                    [this]()
                    {
                        QWidget* modal = QApplication::activeModalWidget();
                        return m_elapsed.elapsed() >= 1200 && modal && modal->findChildren<QRadioButton*>().size() == 4 &&
                            !m_player->m_karaokeRecording && m_player->m_karaokeSessionStarted;
                    },
                    8000);
                for (int choice = 0; choice < 2; ++choice)
                {
                    add(65, "choose inline save option through native mouse while playback continues", [this, choice, attempt]()
                        {
                            QWidget* modal = QApplication::activeModalWidget();
                            const int row = choice == 1 || attempt == 2 ? 1 : 0;
                            QRadioButton* button = modal ? modal->findChild<QRadioButton*>(
                                                               QString("recording%1%2").arg(choice == 0 ? "Audio" : "Format").arg(row))
                                                         : nullptr;
                            if (!button)
                            {
                                return;
                            }
                            const QPoint target = button->mapToGlobal(QPoint(10, button->height() / 2));
                            m_results.check(65, QApplication::widgetAt(target) == button,
                                "native option target belongs to the managed dialog");
                            m_results.check(65, clickDesktopPoint(target), "native save option click submitted");
                            QTimer::singleShot(500, this, [this, choice, attempt]()
                                {
                                    QWidget* modal = QApplication::activeModalWidget();
                                    const QList<QRadioButton*> buttons = modal ? modal->findChildren<QRadioButton*>() : QList<QRadioButton*>();
                                    for (int i = 0; i < buttons.size(); ++i)
                                    {
                                        LOGINFO("Save attempt=%d choice=%d option=%s checked=%d active=%d state=%d",
                                            attempt, choice, buttons[i]->objectName().toUtf8().constData(),
                                            buttons[i]->isChecked() ? 1 : 0, modal->isActiveWindow() ? 1 : 0,
                                            m_player->m_snapshot.m_state);
                                    }
                                    if (modal)
                                    {
                                        const QRect bounds = modal->frameGeometry();
                                        QApplication::primaryScreen()->grabWindow(0, bounds.x(), bounds.y(), bounds.width(), bounds.height()).save(m_directory + QString("/save-desktop-%1-%2.png").arg(attempt).arg(choice));
                                    }
                                });
                        },
                        [this, choice, attempt]()
                        {
                            QWidget* modal = QApplication::activeModalWidget();
                            QRadioButton* button = modal ? modal->findChild<QRadioButton*>(
                                                               QString("recording%1%2").arg(choice == 0 ? "Audio" : "Format").arg(choice == 1 || attempt == 2 ? 1 : 0))
                                                         : nullptr;
                            return m_elapsed.elapsed() >= 1200 && !QApplication::activePopupWidget() && button && button->isChecked() &&
                                m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                        });
                }
                if (attempt != 1)
                {
                    add(65, "save opens native file chooser", []()
                        {
                            QWidget* modal = QApplication::activeModalWidget();
                            const QList<QPushButton*> buttons = modal ? modal->findChildren<QPushButton*>() : QList<QPushButton*>();
                            for (int i = 0; i < buttons.size(); ++i)
                            {
                                if (buttons[i]->text() == QStringLiteral("保存"))
                                {
                                    QTimer::singleShot(0, buttons[i], SLOT(click()));
                                    break;
                                }
                            }
                        },
                        [this]()
                        {
                            return m_player->findChild<QFileDialog*>() != nullptr;
                        },
                        8000);
                    add(65, attempt == 0 ? "cancel native path chooser retains recording" : "save mixed recording through native chooser", [this, attempt]()
                        {
                            QFileDialog* chooser = m_player->findChild<QFileDialog*>();
                            if (!chooser)
                            {
                                return;
                            }
                            if (attempt == 0)
                            {
                                static_cast<QDialog*>(chooser)->reject();
                            }
                            else
                            {
                                m_results.check(65, acceptDesktopSavePath(QDir::toNativeSeparators(QDir(m_directory).absoluteFilePath("native-recording.mp4"))),
                                    "submit native file name without changing the user clipboard");
                            }
                        },
                        [this, attempt]()
                        {
                            return !m_player->m_karaokeFinishing && m_player->m_karaokeSessionStarted == (attempt == 0) &&
                                (attempt == 0 || QFileInfo(m_directory + "/native-recording.mp4").size() > 1000);
                        },
                        30000);
                }
                else
                {
                    add(65, "cancel save options retains recording", [this, attempt]()
                        {
                            QWidget* modal = QApplication::activeModalWidget();
                            if (!modal)
                            {
                                return;
                            }
                            const QList<QPushButton*> buttons = modal->findChildren<QPushButton*>();
                            for (int i = 0; i < buttons.size(); ++i)
                            {
                                if (buttons[i]->text() == QStringLiteral("取消"))
                                {
                                    clickDesktopPoint(buttons[i]->mapToGlobal(buttons[i]->rect().center()));
                                    break;
                                }
                            }
                        },
                        [this, attempt]()
                        {
                            return QApplication::activeModalWidget() == nullptr && !m_player->m_karaokeFinishing &&
                                m_player->m_karaokeSessionStarted;
                        });
                }
                if (attempt != 2)
                {
                    add(65, "reveal controls after modal focus returns", [this]()
                        {
                            move(QPoint(m_player->width() / 2, m_player->height() - 2));
                        },
                        [this]()
                        {
                            return m_player->m_bottomVisibleHeight == m_player->bottomOverlayHeight();
                        });
                    add(65, "reopen retained recording controls", [this]()
                        {
                            if (!m_player->m_microphonePopupVisible)
                            {
                                click(m_player->microphoneButtonRect().center());
                            }
                        },
                        [this]()
                        {
                            return m_player->m_microphoneView && m_player->m_microphoneView->isVisible();
                        });
                }
            }
            add(65, "saved MP4 contains the complete song rather than the recorded excerpt", [this]()
                {
                    FFmpegCpp saved;
                    const bool opened = saved.open((m_directory + "/native-recording.mp4").toUtf8().constData());
                    m_results.check(65, opened && saved.hasAudio() && saved.hasVideo() && std::abs(saved.durationMilliseconds() - m_player->m_snapshot.m_duration100ns / 10000) < 200,
                        "native save output has audio/video and full source duration at 1x");
                    FFmpegCppImageData first, last;
                    m_results.check(65, opened && saved.extractFirstFrameToMemory(&first) && saved.extractFrameToMemory(m_player->m_snapshot.m_duration100ns / 10000 - 500, &last),
                        "native MP4 save decodes video at the beginning and near the end");
                },
                []()
                {
                    return true;
                });
            break;
        }
        case CaseKaraokeCompleteUi:
        {
            add(58, "prepare karaoke viewport", [this]()
                {
                    m_player->resize(640, 480);
                },
                [this]()
                {
                    return m_player->size() == QSize(640, 480);
                });
            add(58, "separation modal progresses then installs backing without resuming paused video", [this]()
                {
                    click(m_player->microphoneButtonRect().center());
                    QTimer::singleShot(700, this, [this]()
                        {
                            QWidget* modal = QApplication::activeModalWidget();
                            m_results.check(58, modal && modal->findChild<QProgressBar*>(), "DialogManager progress view has real progress control");
                            if (modal)
                            {
                                modal->grab().save(m_directory + "/separation-progress.png");
                            }
                        });
                    click(m_player->microphoneRemovalSwitchRect().center());
                },
                [this]()
                {
                    return m_player->m_vocalRemovalEnabled && m_player->m_karaokeBacking != nullptr &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                },
                90000);
            add(58, "switch to original track and retain cached backing", [this]()
                {
                    if (m_player->m_microphoneView)
                    {
                        m_player->m_microphoneView->window()->grab().save(m_directory + "/karaoke-panel.png");
                    }
                    const QRect original = m_player->microphoneOriginalTrackRect();
                    const QRect backing = m_player->microphoneBackingTrackRect();
                    m_results.check(58, original.right() + 1 == backing.left() && original.top() == backing.top() && original.height() == backing.height(), "original/backing segments share one straight seam with no gap");
                    click(m_player->microphoneOriginalTrackRect().center());
                },
                [this]()
                {
                    return !m_player->m_vocalRemovalEnabled && m_player->m_karaokeBacking != nullptr;
                });
            add(58, "switch back to cached backing without rerunning separation", [this]()
                {
                    click(m_player->microphoneBackingTrackRect().center());
                },
                [this]()
                {
                    return m_player->m_vocalRemovalEnabled && QApplication::activeModalWidget() == nullptr;
                });
            add(58, "save options report microphone-only MP4 without using native path dialog", [this]()
                {
                    QTimer::singleShot(250, this, [this]()
                        {
                            QWidget* modal = QApplication::activeModalWidget();
                            if (!modal)
                            {
                                return;
                            }
                            modal->grab().save(m_directory + "/save-options.png");
                            QRadioButton* audio = modal->findChild<QRadioButton*>("recordingAudio0");
                            QRadioButton* format = modal->findChild<QRadioButton*>("recordingFormat1");
                            if (audio && format)
                            {
                                audio->setChecked(true);
                                format->setChecked(true);
                            }
                            const QList<QPushButton*> buttons = modal->findChildren<QPushButton*>();
                            for (int i = 0; i < buttons.size(); ++i)
                            {
                                if (buttons[i]->text() == QStringLiteral("保存"))
                                {
                                    buttons[i]->click();
                                }
                            }
                        });
                    CustomDialogParam prompt(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 2));
                    prompt.m_parent = m_player->windowHandle();
                    DialogManager::instance().makeDialog(prompt);
                    m_position = prompt.m_result == ACCEPT_BUTTON && prompt.m_userResult == 2 ? 1 : -1;
                },
                [this]()
                {
                    return m_position == 1;
                });
            add(58, "atomic MP3 save and pre-cancel preserve existing destination", [this, media]()
                {
                    m_position = 0;
                    std::vector<int16_t> input(48000, 2000), monitor(48000);
                    m_player->m_core.initKaraokeRecording(48000, 1, 1024 * 1024);
                    m_player->m_core.processKaraokeMicrophone(0, 10000000, 1000, input.data(), input.size(), 100,
                        false, false, true, true, monitor.data());
                    const QString target = m_directory + "/atomic-recording.mp3";
                    LumaPlayerKaraokeJob task(&m_player->m_core, media, target, false, false, 0, 100, nullptr);
                    bool started = task.start();
                    if (started)
                    {
                        LumaPlayerKaraokeProgressParam param(&task);
                        param.m_parent = m_player->windowHandle();
                        DialogManager::instance().makeDialog(param);
                    }
                    QFile saved(target);
                    bool valid = started && task.succeeded() && saved.open(QIODevice::ReadOnly);
                    const QByteArray bytes = valid ? saved.readAll() : QByteArray();
                    saved.close();
                    m_results.check(58, valid && bytes.size() > 1000, "export task commits encoded MP3 through QSaveFile: " + task.error());
                    LumaPlayerKaraokeJob canceled(&m_player->m_core, media, target, false, false, 0, 100, nullptr);
                    canceled.StopTask();
                    if (canceled.start())
                    {
                        LumaPlayerKaraokeProgressParam param(&canceled);
                        param.m_parent = m_player->windowHandle();
                        DialogManager::instance().makeDialog(param);
                    }
                    m_results.check(58, !canceled.succeeded() && saved.open(QIODevice::ReadOnly) && saved.readAll() == bytes,
                        "cancel leaves existing file byte-for-byte unchanged");
                    saved.close();
                    m_results.check(58, QDir(m_directory).entryList(QStringList() << ".LumaPlayer-recording-*", QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty(),
                        "both completed and canceled tasks clean temporary media");
                    m_player->m_core.clearKaraokeRecording();
                    m_position = 1;
                },
                [this]()
                {
                    return m_position == 1;
                },
                30000);
            break;
        }
        case CaseKaraokeDeviceMonitor:
        {
            add(53, "prepare viewport for microphone panel", [this]()
                {
                    m_player->resize(640, 480);
                },
                [this]()
                {
                    return m_player->width() == 640 && m_player->height() == 480;
                });
            add(53, "microphone defaults off and opening panel does not start capture", [this]()
                {
                    click(m_player->microphoneButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_microphonePopupVisible && m_player->currentMicrophone().m_muted &&
                        !m_player->currentMicrophone().m_capture.isRunning() && !m_player->m_karaokeRecording;
                });
            add(53, "zero monitor gain prevents acoustic feedback during device test", [this]()
                {
                    click(m_player->microphoneGainTrackRect().bottomLeft());
                },
                [this]()
                {
                    return m_player->currentMicrophone().m_gain == 0;
                });
            add(53, "unmute alone starts actual capture and speaker writes while paused", [this]()
                {
                    click(m_player->microphoneMuteButtonRect().center());
                },
                [this]()
                {
                    return !m_player->currentMicrophone().m_muted && !m_player->m_karaokeRecording &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused &&
                        m_player->currentMicrophone().m_task.hasReceivedPcm() &&
                        m_player->currentMicrophone().m_task.monitorBytesWritten() >= 48000;
                },
                5000);
            const int32_t deviceCount = LumaPlayerMicrophoneCapture::availableDeviceNames().size();
            for (int32_t device = 1; device < deviceCount; ++device)
            {
                add(53, QStringLiteral("switch input device %1 and receive new PCM").arg(device), [this, device]()
                    {
                        LOGINFO("Test microphone device selection index=%d panel=%d", device,
                            m_player->m_microphonePopupVisible ? 1 : 0);
                        selectMicrophoneDevice(0, device, 53);
                    },
                    [this, device]()
                    {
                        return m_position == 1 && m_player->currentMicrophone().m_device == device &&
                            m_player->currentMicrophone().m_task.hasReceivedPcm() &&
                            m_player->currentMicrophone().m_task.monitorBytesWritten() > 24000;
                    },
                    5000);
            }
            add(53, "monitor off stops output but leaves capture active", [this]()
                {
                    click(m_player->microphoneMonitorSwitchRect().center());
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 300 && !m_player->m_microphoneMonitoring &&
                        m_player->currentMicrophone().m_capture.isRunning();
                });
            add(53, "monitor remains stopped across observation interval", [this]()
                {
                    m_position = m_player->currentMicrophone().m_task.monitorBytesWritten();
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 400 &&
                        m_player->currentMicrophone().m_task.monitorBytesWritten() == static_cast<uint64_t>(m_position);
                });
            add(53, "monitor resumes without recording", [this]()
                {
                    click(m_player->microphoneMonitorSwitchRect().center());
                },
                [this]()
                {
                    return !m_player->m_karaokeRecording &&
                        m_player->currentMicrophone().m_task.monitorBytesWritten() > static_cast<uint64_t>(m_position) + 24000;
                });
            add(53, "mute microphone stops speaker output", [this]()
                {
                    click(m_player->microphoneMuteButtonRect().center());
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 300 && m_player->currentMicrophone().m_muted;
                });
            add(53, "muted microphone remains silent", [this]()
                {
                    m_position = m_player->currentMicrophone().m_task.monitorBytesWritten();
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 400 &&
                        m_player->currentMicrophone().m_task.monitorBytesWritten() == static_cast<uint64_t>(m_position);
                });
            add(53, "unmute restores speaker without toggling recording", [this]()
                {
                    click(m_player->microphoneMuteButtonRect().center());
                },
                [this]()
                {
                    return !m_player->currentMicrophone().m_muted && !m_player->m_karaokeRecording &&
                        m_player->currentMicrophone().m_task.monitorBytesWritten() > static_cast<uint64_t>(m_position) + 24000;
                });
            add(53, "monitoring alone has not written recording timeline", []()
                {

                },
                [this]()
                {
                    uint64_t frames = 1;
                    return m_player->m_core.karaokeCapturedFrameCount(&frames) ==
                        LumaPlayerCoreCResultSuccess &&
                        frames == 0 && m_player->currentMicrophone().m_task.lastError() == 0;
                });
            const std::shared_ptr<uint64_t> duringUiStall(new uint64_t(0));
            add(53, "capture and speaker keep advancing while UI is busy for 120ms", [this, duringUiStall]()
                {
                    const uint64_t before = m_player->currentMicrophone().m_task.monitorBytesWritten();
                    std::this_thread::sleep_for(std::chrono::milliseconds(120));
                    *duringUiStall = m_player->currentMicrophone().m_task.monitorBytesWritten() - before;
                    LOGINFO("Microphone bytes delivered during 120ms UI stall=%llu", static_cast<unsigned long long>(*duringUiStall));
                },
                [this, duringUiStall]()
                {
                    return *duringUiStall >=
                        static_cast<uint64_t>(m_player->currentMicrophone().m_task.monitorBytesPerSecond()) * 120 * 7 / 10000;
                });
            const std::shared_ptr<int32_t> maxMonitorBuffer(new int32_t(0));
            const std::shared_ptr<uint64_t> observedBytes(new uint64_t(0));
            const std::shared_ptr<int64_t> observedMs(new int64_t(0));
            add(53, "observe live monitor buffering for 1500ms without growing delay", [this]()
                {
                    m_position = m_player->currentMicrophone().m_task.monitorBytesWritten();
                },
                [this, maxMonitorBuffer, observedBytes, observedMs]()
                {
                    *maxMonitorBuffer = (std::max)(*maxMonitorBuffer, m_player->currentMicrophone().m_task.monitorPeriodMs());
                    if (m_elapsed.elapsed() < 1500)
                    {
                        return false;
                    }
                    *observedMs = m_elapsed.elapsed();
                    *observedBytes = m_player->currentMicrophone().m_task.monitorBytesWritten() - m_position;
                    return true;
                });
            add(53, "native callback period stays below 100ms with sustained PCM throughput", [maxMonitorBuffer, observedBytes, observedMs]()
                {
                    LOGINFO("Native microphone check: maxCallbackPeriodMs=%d bytesWritten=%llu observedMs=%lld",
                        *maxMonitorBuffer, static_cast<unsigned long long>(*observedBytes), static_cast<long long>(*observedMs));
                },
                [this, maxMonitorBuffer, observedBytes, observedMs]()
                {
                    const uint64_t expected = static_cast<uint64_t>(*observedMs) *
                        m_player->currentMicrophone().m_task.monitorBytesPerSecond() / 1000;
                    return expected > 0 && *maxMonitorBuffer > 0 && *maxMonitorBuffer <= 100 &&
                        *observedBytes >= expected * 9 / 10 && *observedBytes <= expected * 11 / 10;
                });
            add(53, "recording enabled while paused still monitors without writing timeline", [this]()
                {
                    m_position = m_player->currentMicrophone().m_task.monitorBytesWritten();
                    click(m_player->microphoneRecordSwitchRect().center());
                },
                [this]()
                {
                    uint64_t frames = 1;
                    return m_elapsed.elapsed() >= 400 && m_player->m_karaokeRecording &&
                        m_player->m_core.karaokeCapturedFrameCount(&frames) == LumaPlayerCoreCResultSuccess &&
                        frames == 0 && m_player->currentMicrophone().m_task.monitorBytesWritten() > static_cast<uint64_t>(m_position);
                });
            add(53, "resume writes actual captured PCM timeline", [this]()
                {
                    click(m_player->playButtonRect().center());
                },
                [this]()
                {
                    uint64_t frames = 0;
                    return m_player->m_karaokeRecording && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying &&
                        m_player->m_core.karaokeCapturedFrameCount(&frames) == LumaPlayerCoreCResultSuccess && frames > 4800;
                });
            const std::shared_ptr<uint64_t> recordedFrames(new uint64_t(0));
            const std::shared_ptr<int64_t> recordedMs(new int64_t(0));
            add(53, "observe one second of recording with small capture blocks", [this]()
                {
                    uint64_t frames = 0;
                    m_player->m_core.karaokeCapturedFrameCount(&frames);
                    m_position = frames;
                },
                [this, recordedFrames, recordedMs]()
                {
                    if (m_elapsed.elapsed() < 1000)
                    {
                        return false;
                    }
                    uint64_t frames = 0;
                    if (m_player->m_core.karaokeCapturedFrameCount(&frames) != LumaPlayerCoreCResultSuccess ||
                        frames < static_cast<uint64_t>(m_position))
                    {
                        return false;
                    }
                    *recordedFrames = frames - m_position;
                    *recordedMs = m_elapsed.elapsed();
                    return true;
                });
            add(53, "recording advances at the capture sample rate without repeated snapshot overwrites", [recordedFrames, recordedMs]()
                {
                    LOGINFO("Microphone recording continuity frames=%llu observedMs=%lld",
                        static_cast<unsigned long long>(*recordedFrames), static_cast<long long>(*recordedMs));
                },
                [this, recordedFrames, recordedMs]()
                {
                    const uint64_t expected = static_cast<uint64_t>(*recordedMs) *
                        m_player->currentMicrophone().m_capture.sampleRate() / 1000;
                    return expected > 0 && *recordedFrames >= expected * 8 / 10 &&
                        *recordedFrames <= expected * 12 / 10;
                });
            add(53, "pause recording before stop", [this]()
                {
                    click(m_player->playButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            add(53, "reopen panel without changing microphone state", [this]()
                {
                    click(m_player->microphoneButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_microphonePopupVisible && !m_player->currentMicrophone().m_muted;
                });
            add(53, "stop recording through save decision, retain live monitoring", [this]()
                {
                    m_position = m_player->currentMicrophone().m_task.monitorBytesWritten();
                    QTimer::singleShot(200, this, []()
                        {
                            QWidget* dialog = QApplication::activeModalWidget();
                            if (dialog != nullptr)
                            {
                                const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
                                for (int32_t i = 0; i < buttons.size(); ++i)
                                {
                                    if (buttons[i]->text() == QStringLiteral("放弃"))
                                    {
                                        buttons[i]->click();
                                        break;
                                    }
                                }
                            }
                        });
                    click(m_player->microphoneRecordSwitchRect().center());
                },
                [this]()
                {
                    return !m_player->m_karaokeRecording && !m_player->m_karaokeSessionStarted &&
                        m_player->currentMicrophone().m_task.monitorBytesWritten() > static_cast<uint64_t>(m_position) + 24000;
                });
            add(53, "PCM processing does not introduce a long synchronous stall", [this]()
                {
                    LOGINFO("Microphone maxProcessingMs=%d", m_player->currentMicrophone().m_task.maxProcessingMs());
                },
                [this]()
                {
                    return m_player->currentMicrophone().m_task.maxProcessingMs() < 50;
                });
            if (deviceCount >= 3)
            {
                add(53, "reopen panel for simultaneous device validation", [this]()
                    {
                        if (!m_player->m_microphonePopupVisible)
                        {
                            click(m_player->microphoneButtonRect().center());
                        }
                    },
                    [this]()
                    {
                        return m_player->m_microphoneView && m_player->m_microphoneView->isVisible();
                    });
                for (int slot = 1; slot < 3; ++slot)
                {
                    add(53, QString("add live microphone lane %1 at zero gain").arg(slot + 1), [this, slot]()
                        {
                            click(m_player->microphoneAddRect().center());
                            click(m_player->microphoneGainTrackRect().bottomLeft());
                            // Two Realtek endpoints can share mutually exclusive hardware; use the separate USB input.
                            if (slot == 2)
                            {
                                const QStringList devices = LumaPlayerMicrophoneCapture::availableDeviceNames();
                                for (int i = 0; i < devices.size(); ++i)
                                {
                                    if (devices[i].contains(QStringLiteral("USB PnP")) && !m_player->microphoneDeviceUsed(i, slot))
                                    {
                                        QTimer::singleShot(100, this, [this, i]()
                                            {
                                                chooseMenu(i);
                                            });
                                        click(m_player->microphoneDeviceChoiceRect(slot).center());
                                        break;
                                    }
                                }
                            }
                            click(m_player->microphoneMuteButtonRect().center());
                        },
                        [this, slot]()
                        {
                            return m_player->m_microphones.size() == static_cast<size_t>(slot + 1) &&
                                m_player->currentMicrophone().m_gain == 0 && m_player->currentMicrophone().m_task.monitorBytesWritten() > 24000;
                        },
                        8000);
                }
                add(53, "three live lanes have unique devices and independent monitor streams", []()
                    {

                    },
                    [this]()
                    {
                        bool valid = true;
                        for (int i = 0; i < 3; ++i)
                        {
                            valid = valid && m_player->m_microphones[i]->m_capture.isRunning() &&
                                m_player->m_microphones[i]->m_task.lastError() == 0 &&
                                !m_player->microphoneDeviceUsed(m_player->m_microphones[i]->m_device, i);
                        }
                        return valid;
                    });
                add(53, "mute selected third mic leaves first two running", [this]()
                    {
                        click(m_player->microphoneMuteButtonRect().center());
                    },
                    [this]()
                    {
                        return m_player->m_microphones[2]->m_muted && !m_player->m_microphones[0]->m_muted &&
                            !m_player->m_microphones[1]->m_muted && m_player->m_microphones[0]->m_capture.isRunning();
                    });
                add(53, "actual 5000 percent diagnostics with monitor off avoids acoustic feedback", [this]()
                    {
                        click(m_player->microphoneMonitorSwitchRect().center());
                        click(m_player->microphoneMuteButtonRect().center());
                        move(m_player->microphoneGainTrackRect().center());
                        for (int i = 0; i < 50; ++i)
                        {
                            key(Qt::Key_Up);
                        }
                        m_position = m_player->currentMicrophone().m_task.monitorBytesWritten();
                    },
                    [this]()
                    {
                        return m_elapsed.elapsed() > 6100 && m_player->currentMicrophone().m_gain == 500 &&
                            !m_player->m_microphoneMonitoring && m_player->currentMicrophone().m_capture.isRunning() &&
                            m_player->currentMicrophone().m_task.monitorBytesWritten() == static_cast<uint64_t>(m_position);
                    },
                    8000);
                add(53, "three microphone recording starts paused without captured frames", [this]()
                    {
                        click(m_player->microphoneGainTrackRect().bottomLeft());
                        click(m_player->microphoneRecordSwitchRect().center());
                    },
                    [this]()
                    {
                        uint64_t frames = 1;
                        return m_elapsed.elapsed() > 200 && m_player->m_karaokeRecordingReady &&
                            m_player->m_core.karaokeCapturedFrameCount(&frames) == 0 && frames == 0;
                    });
                add(53, "all three zero-gain lanes record concurrently after resume", [this]()
                    {
                        click(m_player->playButtonRect().center());
                    },
                    [this]()
                    {
                        uint64_t frames = 0;
                        return m_elapsed.elapsed() > 500 &&
                            m_player->m_core.karaokeCapturedFrameCount(&frames) == 0 && frames > 48000 &&
                            m_player->m_microphones[0]->m_task.lastError() == 0 && m_player->m_microphones[1]->m_task.lastError() == 0 &&
                            m_player->m_microphones[2]->m_task.lastError() == 0;
                    });
                add(53, "pause all recording lanes", [this]()
                    {
                        click(m_player->playButtonRect().center());
                    },
                    [this]()
                    {
                        return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                    });
                add(53, "reopen multi microphone panel before ending recording", [this]()
                    {
                        click(m_player->microphoneButtonRect().center());
                    },
                    [this]()
                    {
                        return m_player->m_microphoneView && m_player->m_microphoneView->isVisible();
                    });
                add(53, "stop joins all three recording lanes before discard", [this]()
                    {
                        QTimer::singleShot(300, this, []()
                            {
                                QWidget* dialog = QApplication::activeModalWidget();
                                if (!dialog)
                                {
                                    return;
                                }
                                const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
                                for (int i = 0; i < buttons.size(); ++i)
                                {
                                    if (buttons[i]->text() == QStringLiteral("放弃"))
                                    {
                                        buttons[i]->click();
                                    }
                                }
                            });
                        click(m_player->microphoneRecordSwitchRect().center());
                    },
                    [this]()
                    {
                        return !m_player->m_karaokeSessionStarted &&
                            m_player->m_microphones[0]->m_task.recordingSettled() && m_player->m_microphones[1]->m_task.recordingSettled() &&
                            m_player->m_microphones[2]->m_task.recordingSettled();
                    });
                add(53, "remove live middle microphone off UI thread and release only its device", [this]()
                    {
                        if (!m_player->m_microphonePopupVisible)
                        {
                            click(m_player->microphoneButtonRect().center());
                        }
                        click(m_player->microphoneRemoveRect(1).center());
                    },
                    [this]()
                    {
                        return m_player->m_microphones.size() == 2 &&
                            !m_player->m_retiredMicrophones.empty() && m_player->m_retiredMicrophones[0]->m_retired.load() &&
                            !m_player->m_retiredMicrophones[0]->m_capture.isRunning() &&
                            m_player->m_microphones[0]->m_capture.isRunning() && m_player->m_microphones[1]->m_capture.isRunning() &&
                            m_player->m_microphones[1]->m_slot == 2;
                    });
            }
            break;
        }
        case CaseHelpUi:
        {
            for (int32_t closeKind = 0; closeKind < 2; ++closeKind)
            {
                add(15, "open modal via help", [this]()
                    {
                        g_config.m_helpWidth = m_originalHelpWidth + (m_originalHelpWidth % 2 == 0 ? 1 : 0);
                        click(m_player->helpButtonRect().center());
                    },
                    []()
                    {
                        return QApplication::activeModalWidget() != nullptr;
                    });
                add(15, "modal title fills edge with level-2 shadow and no outline", [this]()
                    {
                        QWidget* dialog = QApplication::activeModalWidget();
                        if (dialog != nullptr)
                        {
                            dialog->grab().save(m_directory + "/help.png");
                            QWidget* title = dialog->findChild<QWidget*>(QStringLiteral("helpTitleBar"));
                            if (title != nullptr)
                            {
                                LOGINFO("Help title style=%s class=%s", title->styleSheet().toUtf8().constData(), title->metaObject()->className());
                                const QList<QLabel*> labels = title->findChildren<QLabel*>();
                                for (int32_t i = 0; i < labels.size(); ++i)
                                {
                                    LOGINFO("Help label style=%s color=%s", labels[i]->styleSheet().toUtf8().constData(),
                                        labels[i]->palette().color(QPalette::WindowText).name().toUtf8().constData());
                                }
                            }
                        }
                    },
                    [this]()
                    {
                        QWidget* dialog = QApplication::activeModalWidget();
                        if (dialog == nullptr || (dialog->geometry().center() - m_player->geometry().center()).manhattanLength() > 2)
                        {
                            return false;
                        }
                        DialogBase* shell = qobject_cast<DialogBase*>(dialog);
                        if (shell == nullptr || shell->windowBorderEnabled() ||
                            !shell->windowShadowEnabled() || shell->windowShadowSize() != 2)
                        {
                            return false;
                        }
                        QWidget* shadow = shell->findChild<QWidget*>(QStringLiteral("qtControlsDialogShadow"),
                            Qt::FindDirectChildrenOnly);
                        if (shadow == nullptr || !shadow->isVisible() ||
                            shadow->width() <= dialog->width() || shadow->height() <= dialog->height())
                        {
                            return false;
                        }
                        QWidget* titleBar = dialog->findChild<QWidget*>(QStringLiteral("helpTitleBar"));
                        if (titleBar == nullptr || titleBar->parentWidget() == nullptr)
                        {
                            return false;
                        }
                        const int32_t titleHeight = g_config.m_topOverlayHeight + (g_config.m_topOverlayHeight % 2);
                        const int32_t titleInset = (titleHeight - g_config.m_titleButtonSize) / 2;
                        const int32_t oddHelpWidth = m_originalHelpWidth + (m_originalHelpWidth % 2 == 0 ? 1 : 0);
                        const QPoint titleInDialog = titleBar->mapTo(dialog, QPoint(0, 0));
                        const bool titleBandValid = dialog->width() == oddHelpWidth + 1 && titleHeight % 2 == 0 &&
                            titleBar->geometry() == QRect(0, 0, titleBar->parentWidget()->width(), titleHeight) &&
                            titleInDialog == QPoint(0, 0) && titleBar->width() == dialog->width();
                        const QImage shellImage = dialog->grab().toImage();
                        if (shellImage.isNull())
                        {
                            return false;
                        }
                        const QColor titleCenter(shellImage.pixel(shellImage.width() / 2, 0));
                        QImage expectedTitle(1, 1, QImage::Format_ARGB32);
                        expectedTitle.fill(g_config.m_helpBackground.rgba());
                        {
                            QPainter painter(&expectedTitle);
                            painter.fillRect(expectedTitle.rect(), g_config.m_overlayColor);
                        }
                        const QColor expectedColor(expectedTitle.pixel(0, 0));
                        const bool titleColorPreserved = std::abs(titleCenter.red() - expectedColor.red()) <= 1 &&
                            std::abs(titleCenter.green() - expectedColor.green()) <= 1 &&
                            std::abs(titleCenter.blue() - expectedColor.blue()) <= 1;
                        const bool topEdgeHasNoOutline =
                            QColor(shellImage.pixel(0, 0)) == titleCenter &&
                            QColor(shellImage.pixel(shellImage.width() - 1, 0)) == titleCenter;
                        const QList<QLabel*> labels = dialog->findChildren<QLabel*>();
                        bool titleTextInset = false;
                        bool versionPositionPreserved = false;
                        QLabel* productLabel = nullptr;
                        for (int32_t i = 0; i < labels.size(); ++i)
                        {
                            if (labels[i]->text() == g_config.m_helpTitle)
                            {
                                titleTextInset = labels[i]->parentWidget() == titleBar &&
                                    labels[i]->geometry().left() == titleInset &&
                                    labels[i]->palette().color(QPalette::WindowText) == g_config.m_textColor;
                            }
                            if (labels[i]->text() == g_config.m_versionLabel)
                            {
                                versionPositionPreserved = (labels[i]->alignment() & Qt::AlignLeft) &&
                                    labels[i]->font().pointSize() == g_config.m_fontSize &&
                                    labels[i]->geometry().left() == g_config.m_helpMargin;
                            }
                            if (labels[i]->text() == g_config.m_windowTitle)
                            {
                                productLabel = labels[i];
                            }
                        }
                        if (productLabel != nullptr)
                        {
                            for (int32_t i = 0; i < labels.size(); ++i)
                            {
                                if (labels[i]->text() == g_config.m_versionLabel)
                                {
                                    const int32_t expectedVersionTop = titleHeight + g_config.m_helpSpacing +
                                        g_config.m_titleButtonTop + productLabel->height() + g_config.m_helpSpacing;
                                    versionPositionPreserved = versionPositionPreserved &&
                                        labels[i]->geometry().top() == expectedVersionTop;
                                }
                            }
                        }
                        bool closeButtonCentered = false;
                        const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
                        for (int32_t i = 0; i < buttons.size(); ++i)
                        {
                            if (buttons[i]->accessibleName() == g_config.m_closeText && buttons[i]->parentWidget() == titleBar)
                            {
                                closeButtonCentered = titleBar->width() - buttons[i]->geometry().right() - 1 == titleInset &&
                                    buttons[i]->geometry().top() == (titleHeight - g_config.m_titleButtonSize) / 2;
                                break;
                            }
                        }
                        return titleBandValid && titleColorPreserved && topEdgeHasNoOutline && titleTextInset &&
                            closeButtonCentered && versionPositionPreserved;
                    });
                add(15, "title mouse drag", [this]()
                    {
                        QWidget* dialog = QApplication::activeModalWidget();
                        if (dialog == nullptr)
                        {
                            return;
                        }
                        m_normal = QRect(dialog->pos(), dialog->size());
                        const QList<QLabel*> labels = dialog->findChildren<QLabel*>();
                        for (int32_t i = 0; i < labels.size(); ++i)
                        {
                            if (labels[i]->text() == g_config.m_helpTitle && labels[i]->isVisible() && labels[i]->parentWidget() != dialog)
                            {
                                QWidget* title = labels[i];
                                // DialogManager外壳有同名标题；只操作自定义标题栏内实际显示的标签。
                                const QPoint local = title->rect().center();
                                const QPoint global = title->mapToGlobal(local);
                                QMouseEvent press(QEvent::MouseButtonPress, local, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                                QApplication::sendEvent(title, &press);
                                QMouseEvent motion(QEvent::MouseMove, local + QPoint(30, 20), global + QPoint(30, 20), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                                QApplication::sendEvent(title, &motion);
                                QMouseEvent release(QEvent::MouseButtonRelease, local, global + QPoint(30, 20), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                                QApplication::sendEvent(title, &release);
                                break;
                            }
                        }
                    },
                    [this]()
                    {
                        QWidget* dialog = QApplication::activeModalWidget();
                        return dialog != nullptr && dialog->pos() == m_normal.topLeft() + QPoint(30, 20);
                    });
                add(15, "modal button closes asynchronously", [this, closeKind]()
                    {
                        QWidget* dialog = QApplication::activeModalWidget();
                        if (dialog == nullptr)
                        {
                            return;
                        }
                        const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
                        for (int32_t i = 0; i < buttons.size(); ++i)
                        {
                            if ((closeKind == 0 && buttons[i]->text() == g_config.m_confirmText) ||
                                (closeKind == 1 && buttons[i]->accessibleName() == g_config.m_closeText))
                            {
                                buttons[i]->click();
                                break;
                            }
                        }
                        if (closeKind == 1)
                        {
                            g_config.m_helpWidth = m_originalHelpWidth;
                        }
                    },
                    []()
                    {
                        return QApplication::activeModalWidget() == nullptr;
                    });
            }
            break;
        }
        case CaseQueuedClose:
        {
            add(16, "close during queued reset", [this]()
                {
                    key(Qt::Key_Up, Qt::ControlModifier);
                    click(m_player->resetButtonRect().center());
                    m_player->close();
                },
                [this]()
                {
                    return m_player->m_closeReady;
                },
                10000);
            break;
        }
        case CaseButtonVisuals:
        {
            add(CaseButtonVisuals, "QtControls item padding preserves content insets", [this]()
                {
                    Menu menu;
                    menu.setItemPadding(3, 4, 5, 6, true);
                    const QString style = menu.styleSheet();
                    m_results.check(CaseButtonVisuals, style.contains(".Menu::item{") && style.contains("padding-left:3px") && style.contains("padding-top:4px") && style.contains("padding-right:5px") && style.contains("padding-bottom:6px"), QStringLiteral("菜单项内边距不混用外边距"));
                    menu.setItemPadding(-1, -2, -3, -4, true);
                    const QString clamped = menu.styleSheet();
                    m_results.check(CaseButtonVisuals, clamped.contains("padding-left:0px") && clamped.contains("padding-top:0px") && clamped.contains("padding-right:0px") && clamped.contains("padding-bottom:0px"), QStringLiteral("负内边距归零"));
                },
                []()
                {
                    return true;
                });
            add(4, "played color AB colors and layout", [this]()
                {
                    m_player->grab().save(m_directory + "/layout.png");
                },
                [this]()
                {
                    return g_config.m_loopAColor != g_config.m_themeColor && g_config.m_loopBColor != g_config.m_themeColor &&
                        m_player->playButtonRect().center().x() == m_player->rect().center().x() &&
                        m_player->playButtonRect().top() > m_player->progressTrackRect().bottom();
                });
            for (int32_t button = 0; button < 4; ++button)
            {
                add(CaseButtonVisuals, "button tooltip " + QString::number(button), [this, button]()
                    {
                        // 素材加载会调整窗口大小，必须在动作执行时取按钮矩形。
                        const QRect buttons[] = {m_player->loadButtonRect(), m_player->resetButtonRect(), m_player->pinButtonRect(), m_player->helpButtonRect()};
                        move(buttons[button].center());
                    },
                    [this]()
                    {
                        return m_elapsed.elapsed() >= 100 && QToolTip::isVisible();
                    });
            }
            add(CaseButtonVisuals, "capture knob without hover", [this]()
                {
                    move(m_player->videoViewportRect().center());
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    const int32_t radius = g_config.m_positionDragRadius + 2;
                    m_image = m_player->grab(QRect(point - QPoint(radius, radius), QSize(radius * 2 + 1, radius * 2 + 1))).toImage();
                },
                []()
                {
                    return true;
                });
            add(CaseButtonVisuals, "hover knob enlarges pixels", [this]()
                {
                    move(QPoint(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y()));
                },
                [this]()
                {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    const int32_t radius = g_config.m_positionDragRadius + 2;
                    return m_player->grab(QRect(point - QPoint(radius, radius), m_image.size())).toImage() != m_image;
                });
            add(CaseButtonVisuals, "press knob restores radius and overlays blue", [this]()
                {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                },
                [this]()
                {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    const QImage pixels = m_player->grab().toImage();
                    const QColor center(pixels.pixel(point));
                    const QColor outside(pixels.pixel(point + QPoint(0, -g_config.m_positionRadius - 2)));
                    return m_player->m_progressPressPending && !m_player->m_isDraggingProgress && center != QColor(m_image.pixel(m_image.width() / 2, m_image.height() / 2)) && outside == QColor(m_image.pixel(m_image.width() / 2, m_image.height() / 2 - g_config.m_positionRadius - 2));
                });
            add(CaseButtonVisuals, "knob release restores normal color", [this]()
                {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &release);
                },
                [this]()
                {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    return !m_player->m_isDraggingProgress && !m_player->m_hasDragPosition &&
                        QColor(m_player->grab().toImage().pixel(point)) == QColor(m_image.pixel(m_image.width() / 2, m_image.height() / 2));
                });
            add(37, "pin pressed overlay", [this]()
                {
                    QPoint p = m_player->pinButtonRect().center();
                    move(p);
                    m_image = m_player->grab(m_player->pinButtonRect()).toImage();
                    QMouseEvent press(QEvent::MouseButtonPress, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                },
                [this]()
                {
                    return m_player->m_leftPressed && m_player->grab(m_player->pinButtonRect()).toImage() != m_image;
                });
            add(37, "pin release clears pressed", [this]()
                {
                    QPoint p = m_player->pinButtonRect().center();
                    QMouseEvent release(QEvent::MouseButtonRelease, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &release);
                },
                [this]()
                {
                    return !m_player->m_leftPressed;
                });
            break;
        }
        case CaseInputCancel:
        {
            const QEvent::Type cancelEvents[] = {QEvent::FocusOut, QEvent::WindowDeactivate};
            for (size_t i = 0; i < sizeof(cancelEvents) / sizeof(cancelEvents[0]); ++i)
            {
                const QEvent::Type type = cancelEvents[i];
                add(id, "cancel pending click without seek", [this, type]()
                    {
                        const QPoint point(m_player->progressTrackRect().center());
                        move(point);
                        m_position = m_player->m_lastSeekInput;
                        QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(m_player, &press);
                        m_results.check(CaseInputCancel, m_player->m_progressPressPending, "timeline press pending before cancel");
                        if (type == QEvent::FocusOut)
                        {
                            QFocusEvent cancel(type);
                            QApplication::sendEvent(m_player, &cancel);
                        }
                        else
                        {
                            QEvent cancel(type);
                            QApplication::sendEvent(m_player, &cancel);
                        }
                        QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                        QApplication::sendEvent(m_player, &release);
                    },
                    [this]()
                    {
                        return m_elapsed.elapsed() >= 150 && !m_player->m_progressPressPending &&
                            !m_player->m_isDraggingProgress && !m_player->m_hasDragPosition &&
                            !m_player->m_leftPressed && m_player->m_lastSeekInput == static_cast<uint64_t>(m_position) &&
                            m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                    });
            }
            add(id, "start real progress drag", [this]()
                {
                    const QPoint point = m_player->progressTrackRect().center();
                    move(point);
                    QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                    move(point + QPoint(QApplication::startDragDistance() + 10, 0), Qt::LeftButton);
                },
                [this]()
                {
                    return m_player->m_isDraggingProgress && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            add(id, "focus loss finishes drag paused and stops previews", [this]()
                {
                    m_position = m_player->m_dragPosition100ns;
                    QFocusEvent cancel(QEvent::FocusOut);
                    QApplication::sendEvent(m_player, &cancel);
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 150 && !m_player->m_isDraggingProgress &&
                        !m_player->m_progressPressPending && !m_player->m_hasDragPosition &&
                        m_player->m_lastPreviewRequestPosition100ns == -1 &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused &&
                        std::abs(m_player->m_snapshot.m_position100ns - m_position) <= 400000;
                });
            add(id, "old timeline release cannot seek replacement media", [this, media]()
                {
                    const QPoint point = m_player->progressTrackRect().center();
                    move(point);
                    m_position = m_player->m_lastSeekInput;
                    QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                    m_results.check(CaseInputCancel, m_player->m_progressPressPending, "timeline press pending before load");
                    m_player->loadMedia(media);
                    QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &release);
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 500 && m_player->m_hasMedia && !m_player->m_cachedFrame.isNull() &&
                        !m_player->m_progressPressPending && !m_player->m_isDraggingProgress &&
                        !m_player->m_hasDragPosition && !m_player->m_leftPressed &&
                        m_player->m_lastSeekInput == static_cast<uint64_t>(m_position);
                });
            break;
        }
        case CasePlayPauseUi:
        {
            add(22, "space plays", [this]()
                {
                    key(Qt::Key_Space);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(22, "button pauses", [this]()
                {
                    click(m_player->playButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            add(22, "video single click plays", [this]()
                {
                    click(m_player->videoViewportRect().center());
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(22, "playing double click never pauses", [this]()
                {
                    m_unexpectedPause = false;
                    doubleClick();
                },
                [this]()
                {
                    m_unexpectedPause = m_unexpectedPause || m_player->m_core.snapshot().m_state != LumaPlayerCoreCStatePlaying;
                    return m_elapsed.elapsed() >= QApplication::doubleClickInterval() + 150 &&
                        m_player->isFullScreen() && !m_player->m_clickTimer.isActive() && !m_unexpectedPause;
                });
            add(22, "pause before reverse double click", [this]()
                {
                    key(Qt::Key_Space);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            add(22, "paused double click never plays or advances a frame", [this]()
                {
                    m_unexpectedPause = false;
                    m_position = m_player->m_cachedFrameStart;
                    doubleClick();
                },
                [this]()
                {
                    m_unexpectedPause = m_unexpectedPause || m_player->m_core.snapshot().m_state != LumaPlayerCoreCStatePaused ||
                        m_player->m_cachedFrameStart != m_position;
                    return m_elapsed.elapsed() >= QApplication::doubleClickInterval() + 150 &&
                        !m_player->isFullScreen() && !m_player->m_clickTimer.isActive() && !m_unexpectedPause;
                });
            break;
        }
        case CaseDragUi:
        {
            add(6, "drag starts", [this]()
                {
                    const QPoint p = m_player->progressTrackRect().center();
                    move(p);
                    QMouseEvent event(QEvent::MouseButtonPress, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &event);
                    move(p + QPoint(QApplication::startDragDistance() + 1, 0), Qt::LeftButton);
                    m_dragMatches = 0;
                },
                [this]()
                {
                    return m_player->m_isDraggingProgress;
                });
            for (int32_t sample = 1; sample <= 32; ++sample)
            {
                add(6, "drag immediate progress sample=" + QString::number(sample), [this, sample]()
                    {
                        QRect track = m_player->progressTrackRect();
                        QPoint p(track.left() + sample * (track.width() - 1) / 40, track.center().y());
                        move(p, Qt::LeftButton);
                        if (m_player->displayPosition100ns() == m_player->progressPointToTime100ns(p))
                        {
                            ++m_dragMatches;
                        }
                    },
                    [this, sample]()
                    {
                        return m_dragMatches == sample;
                    });
            }
            add(6, "final seek frame covers target", [this]()
                {
                    QRect track = m_player->progressTrackRect();
                    QPoint p(track.left() + 32 * (track.width() - 1) / 40, track.center().y());
                    m_position = m_player->progressPointToTime100ns(p);
                    QMouseEvent event(QEvent::MouseButtonRelease, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &event);
                },
                [this]()
                {
                    return !m_player->m_hasDragPosition && !m_player->m_isDraggingProgress &&
                        m_player->m_cachedFrameStart <= m_position && m_player->m_cachedFrameEnd > m_position;
                },
                8000);
            break;
        }
        case CaseSeekKeys:
        {
            add(0, "prepare position", [this]()
                {
                    menuAt(50000000, -1);
                },
                [this]()
                {
                    return !m_player->m_hasDragPosition && m_player->m_cachedFrameStart > 49000000;
                });
            add(7, "left key default 2sec", [this]()
                {
                    move(m_player->videoViewportRect().center());
                    m_position = m_player->m_snapshot.m_position100ns;
                    key(Qt::Key_Left);
                },
                [this]()
                {
                    return m_player->m_cachedFrameStart <= m_position - 20000000 &&
                        m_player->m_cachedFrameEnd > m_position - 20000000;
                });
            break;
        }
        case CaseFractionalKeys:
        {
            add(0, "prepare position", [this]()
                {
                    menuAt(50000000, -1);
                },
                [this]()
                {
                    return !m_player->m_hasDragPosition && m_player->m_cachedFrameStart > 49000000;
                });
            add(7, "left key configured 0.5sec", [this]()
                {
                    move(m_player->videoViewportRect().center());
                    m_position = m_player->m_snapshot.m_position100ns;
                    key(Qt::Key_Left);
                },
                [this]()
                {
                    return m_player->m_cachedFrameStart <= m_position - 5000000 &&
                        m_player->m_cachedFrameEnd > m_position - 5000000;
                });
            break;
        }
        case CaseAbMenu:
        {
            add(9, "playing before AB edit", [this]()
                {
                    m_player->m_core.playAsync();
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(9, "right-click A preserves playing", [this]()
                {
                    menuAt(10000000, 0);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopA && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(9, "right-click B preserves playing", [this]()
                {
                    menuAt(20000000, 1);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopB && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            break;
        }
        case CasePausedMenu:
        {
            add(6, "paused right click synchronizes actual frame and progress", [this]()
                {
                    menuAt(30000000, -1);
                },
                [this]()
                {
                    return !m_player->m_hasDragPosition && m_player->m_cachedFrameStart > 29000000 &&
                        m_player->m_cachedFrameStart < 31000000 && m_player->displayPosition100ns() == m_player->m_cachedFrameStart;
                });
            break;
        }
        case CaseFrameKey:
        {
            add(11, "hover A then one frame key", [this]()
                {
                    m_position = m_player->m_snapshot.m_loopAStart100ns;
                    move(QPoint(m_player->progressTimeToX(m_position), m_player->progressTrackRect().center().y()));
                    key(Qt::Key_Right);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_loopAStart100ns > m_position &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            break;
        }
        case CaseResetUi:
        {
            add(12, "AB alone does not dirty reset", []()
                {

                },
                [this]()
                {
                    return !m_player->m_resetEnabled;
                });
            add(CaseResetUi, "start video pan", [this]()
                {
                    m_position = m_player->m_snapshot.m_position100ns;
                    const QPoint point = m_player->videoViewportRect().center();
                    move(point);
                    QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                    move(point + QPoint(40, 0), Qt::LeftButton);
                },
                [this]()
                {
                    return m_player->m_panOffset == QPointF(40, 0) && m_player->m_resetEnabled;
                });
            add(CaseResetUi, "manual pan return stays dirty", [this]()
                {
                    move(m_player->videoViewportRect().center(), Qt::LeftButton);
                },
                [this]()
                {
                    return m_player->m_panOffset.isNull() && m_player->m_resetEnabled;
                });
            add(CaseResetUi, "release pan then zoom", [this]()
                {
                    const QPoint point = m_player->videoViewportRect().center();
                    QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &release);
                    QWheelEvent wheel(point, m_player->mapToGlobal(point), QPoint(), QPoint(0, 120), 120, Qt::Vertical, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &wheel);
                },
                [this]()
                {
                    return m_player->m_zoomPercent > 100 && m_player->m_resetEnabled;
                });
            add(12, "rate modifies actual state", [this]()
                {
                    move(m_player->videoViewportRect().center());
                    key(Qt::Key_Up, Qt::ControlModifier);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_ratePermille > 1000 && m_player->m_resetEnabled;
                });
            add(12, "reset waits result and preserves AB", [this]()
                {
                    click(m_player->resetButtonRect().center());
                },
                [this]()
                {
                    return !m_player->m_resetEnabled && m_player->m_snapshot.m_ratePermille == 1000 && m_player->m_snapshot.m_hasLoopA && m_player->m_snapshot.m_hasLoopB &&
                        m_player->m_panOffset.isNull() && m_player->m_zoomPercent == 100 &&
                        m_player->m_snapshot.m_position100ns == m_position && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            break;
        }
        case CaseWindowState:
        {
            add(13, "set ordinary geometry", [this]()
                {
                    m_player->setGeometry(140, 150, 680, 450);
                },
                [this]()
                {
                    return m_player->geometry() == QRect(140, 150, 680, 450);
                });
            add(13, "ordinary Escape is inert", [this]()
                {
                    key(Qt::Key_Escape);
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 250 && !m_player->isFullScreen() && !m_player->isMaximized() && m_player->geometry() == QRect(140, 150, 680, 450);
                });
            for (int32_t sequence = 0; sequence < 3; ++sequence)
            {
                add(13, "maximize", [this]()
                    {
                        m_normal = m_player->geometry();
                        click(m_player->maximizeButtonRect().center());
                    },
                    [this]()
                    {
                        return m_player->isMaximized() && !m_player->isFullScreen();
                    });
                add(13, "full screen keeps pin", [this]()
                    {
                        doubleClick();
                    },
                    [this]()
                    {
                        return m_player->isFullScreen() && m_player->m_pinned && m_player->m_bottomVisibleHeight == g_config.m_bottomOverlayHeight;
                    });
                add(13, "fullscreen max button only changes future state", [this]()
                    {
                        click(m_player->maximizeButtonRect().center());
                    },
                    [this]()
                    {
                        return m_player->isFullScreen() && !m_player->isMaximizedOutsideFullScreen();
                    });
                add(13, "double click or Escape restores exact ordinary rectangle", [this, sequence]()
                    {
                        if (sequence == 0)
                        {
                            doubleClick();
                        }
                        else
                        {
                            key(Qt::Key_Escape);
                        }
                    },
                    [this]()
                    {
                        return !m_player->isFullScreen() && !m_player->isMaximized() && m_player->geometry() == m_normal;
                    });
                add(13, "new user ordinary rectangle", [this, sequence]()
                    {
                        m_player->setGeometry(170 + sequence * 10, 160, 640 + sequence * 10, 430);
                    },
                    [this, sequence]()
                    {
                        return m_player->geometry() == QRect(170 + sequence * 10, 160, 640 + sequence * 10, 430);
                    });
            }
            add(13, "ordinary enters fullscreen", [this]()
                {
                    m_normal = m_player->geometry();
                    m_position = m_player->m_snapshot.m_position100ns;
                    doubleClick();
                },
                [this]()
                {
                    return m_player->isFullScreen();
                });
            add(13, "repeated Escape exits once preserving pause position and pin", [this]()
                {
                    for (int32_t i = 0; i < 12; ++i)
                    {
                        key(Qt::Key_Escape);
                        QKeyEvent repeat(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier, QString(), true, 1);
                        QApplication::sendEvent(m_player, &repeat);
                    }
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 300 && !m_player->isFullScreen() && !m_player->isMaximized() && m_player->geometry() == m_normal && m_player->m_pinned && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused && m_player->m_snapshot.m_position100ns == m_position;
                });
            add(13, "maximize before Escape return", [this]()
                {
                    click(m_player->maximizeButtonRect().center());
                },
                [this]()
                {
                    return m_player->isMaximized() && !m_player->isFullScreen();
                });
            add(13, "maximized enters fullscreen", [this]()
                {
                    doubleClick();
                },
                [this]()
                {
                    return m_player->isFullScreen();
                });
            add(13, "Escape restores maximized target", [this]()
                {
                    key(Qt::Key_Escape);
                },
                [this]()
                {
                    return !m_player->isFullScreen() && m_player->isMaximized() && m_player->m_pinned;
                });
            add(13, "maximized Escape is inert", [this]()
                {
                    key(Qt::Key_Escape);
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 250 && !m_player->isFullScreen() && m_player->isMaximized();
                });
            break;
        }
        case CaseInitialMediaFit:
        {
            add(43, "fixture settles before size-policy checks", [this]()
                {
                    if (m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying)
                    {
                        m_player->m_core.pauseAsync();
                    }
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused ||
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStateError;
                });
            add(43, "unpin before checking full video viewport", [this]()
                {
                    if (m_player->m_pinned)
                    {
                        click(m_player->pinButtonRect().center());
                    }
                },
                [this]()
                {
                    return !m_player->m_pinned;
                });

            const QRect screen = QApplication::desktop()->screenGeometry(m_player);
            const int32_t maxVideoWidth = (std::max)(1, static_cast<int32_t>(screen.width() * g_config.m_initialDesktopFraction));
            const int32_t maxVideoHeight = (std::max)(1, static_cast<int32_t>(screen.height() * g_config.m_initialDesktopFraction));
            const int32_t sourceWidths[] = {screen.width() * 2, screen.width() * 4, screen.width(), (std::max)(1, maxVideoWidth / 2)};
            const int32_t sourceHeights[] = {screen.height() * 2, screen.height(), screen.height() * 4, (std::max)(1, maxVideoHeight / 2)};
            const char* scenarios[] = {"screen aspect reaches both half-screen bounds", "ultrawide source limits width only",
                "tall source limits height only", "small source is not enlarged"};
            for (size_t index = 0; index < sizeof(sourceWidths) / sizeof(sourceWidths[0]); ++index)
            {
                const int32_t sourceWidth = sourceWidths[index];
                const int32_t sourceHeight = sourceHeights[index];
                const double widthScale = static_cast<double>(maxVideoWidth) / sourceWidth;
                const double heightScale = static_cast<double>(maxVideoHeight) / sourceHeight;
                const double expectedScale = (std::min)(1.0, (std::min)(widthScale, heightScale));
                const int32_t expectedWidth = (std::max)(1, static_cast<int32_t>(sourceWidth * expectedScale + 0.5));
                const int32_t expectedHeight = (std::max)(1, static_cast<int32_t>(sourceHeight * expectedScale + 0.5));
                const int32_t expectedWindowWidth = (std::max)(g_config.m_minWindowWidth, expectedWidth);
                const int32_t expectedWindowHeight = (std::max)(g_config.m_minWindowHeight, expectedHeight);
                add(43, scenarios[index], [this, sourceWidth, sourceHeight]()
                    {
                        LumaPlayerCoreCVideoFormat format = {};
                        format.m_width = sourceWidth;
                        format.m_height = sourceHeight;
                        m_player->m_videoRender.openVideo(format);
                        m_player->fitWindowToMedia();
                    },
                    [this, screen, maxVideoWidth, maxVideoHeight, expectedScale, expectedWidth, expectedHeight, expectedWindowWidth, expectedWindowHeight]()
                    {
                        const QRect draw = m_player->videoDrawRect();
                        const QPoint imageCenter = m_player->mapToGlobal(draw.center());
                        return QApplication::desktop()->screenGeometry(m_player) == screen &&
                            qAbs(m_player->m_baseDisplayScale - expectedScale) < 0.000001 &&
                            draw.size() == QSize(expectedWidth, expectedHeight) &&
                            draw.width() <= maxVideoWidth && draw.height() <= maxVideoHeight &&
                            qAbs(imageCenter.x() - screen.center().x()) <= 1 &&
                            qAbs(imageCenter.y() - screen.center().y()) <= 1 &&
                            m_player->geometry().size() == QSize(expectedWindowWidth, expectedWindowHeight);
                    });
            }
            add(43, "reveal ordinary window controls", [this]()
                {
                    move(QPoint(100, 5));
                },
                [this]()
                {
                    return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight;
                });
            add(43, "fitted window can maximize", [this]()
                {
                    m_normal = m_player->geometry();
                    click(m_player->maximizeButtonRect().center());
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 150 && m_player->isMaximized() && !m_player->isFullScreen() &&
                        m_player->geometry().size() == QApplication::desktop()->availableGeometry(m_player).size();
                });
            add(43, "maximized window can enter fullscreen", [this]()
                {
                    m_player->toggleFullScreen();
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= QApplication::doubleClickInterval() + 150 && m_player->isFullScreen();
                });
            add(43, "Escape restores maximized target", [this]()
                {
                    key(Qt::Key_Escape);
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 150 && !m_player->isFullScreen() && m_player->isMaximized();
                });
            add(43, "reveal maximized title controls after Escape", [this]()
                {
                    move(QPoint(100, 5));
                },
                [this]()
                {
                    return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight;
                });
            add(43, "restoring maximized window returns to fitted rectangle", [this]()
                {
                    click(m_player->maximizeButtonRect().center());
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 150 && !m_player->isMaximized() &&
                        !m_player->isFullScreen() && m_player->geometry() == m_normal;
                });
            break;
        }
        case CasePinnedUi:
        {
            add(14, "pinned layout excludes both bars", []()
                {

                },
                [this]()
                {
                    return m_player->m_bottomVisibleHeight == g_config.m_bottomOverlayHeight &&
                        m_player->videoViewportRect().top() >= m_player->topOverlayRect().bottom() &&
                        m_player->videoViewportRect().bottom() <= m_player->bottomOverlayRect().top();
                });
            add(14, "unpin hide bars", [this]()
                {
                    click(m_player->pinButtonRect().center());
                    move(m_player->videoViewportRect().center());
                },
                [this]()
                {
                    return !m_player->m_pinned && m_player->m_topVisibleHeight == 0 && m_player->m_bottomVisibleHeight == 0;
                },
                6000);
            add(14, "unpinned enters fullscreen", [this]()
                {
                    m_normal = m_player->geometry();
                    doubleClick();
                },
                [this]()
                {
                    return m_player->isFullScreen() && !m_player->m_pinned;
                });
            add(14, "Escape keeps unpinned ordinary rectangle", [this]()
                {
                    key(Qt::Key_Escape);
                },
                [this]()
                {
                    return !m_player->isFullScreen() && !m_player->isMaximized() && !m_player->m_pinned && m_player->geometry() == m_normal;
                });
            add(14, "reveal and repin", [this]()
                {
                    move(QPoint(100, 5));
                },
                [this]()
                {
                    return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight;
                });
            add(14, "pin restored", [this]()
                {
                    click(m_player->pinButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_pinned;
                });
            break;
        }
        case CaseRateSeekLoopRace:
        {
            add(42, "wait for playing controls after media resize", [this]()
                {
                    move(QPoint(m_player->width() / 2, m_player->height() - 5));
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying &&
                        m_player->m_bottomVisibleHeight == m_player->bottomOverlayHeight();
                });
            add(42, "prepare playing AB start", [this]()
                {
                    menuAt(m_player->m_snapshot.m_duration100ns / 10, 0);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopA != 0;
                });
            add(42, "prepare playing AB end", [this]()
                {
                    menuAt(m_player->m_snapshot.m_duration100ns / 3, 1);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopB != 0;
                });
            const int32_t rates[] = {2200, 700, 5000};
            const int32_t positions[] = {36, 79, 54, 45, 87, 45, 39, 6};
            for (size_t round = 0; round < 3; ++round)
            {
                const int32_t rate = rates[round];
                add(42, "change rate with AB active " + QString::number(rate), [this, rate]()
                    {
                        move(QPoint(m_player->width() / 2, m_player->height() / 2));
                        if (rate == 2200)
                        {
                            for (int32_t index = 0; index < 12; ++index)
                            {
                                key(Qt::Key_Up, Qt::ControlModifier);
                            }
                        }
                        else
                        {
                            m_player->m_core.setPlaybackRatePermilleAsync(rate);
                        }
                    },
                    [this, rate]()
                    {
                        return m_player->m_snapshot.m_ratePermille == rate &&
                            m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                    });
                for (size_t index = 0; index < 8; ++index)
                {
                    const int32_t percent = positions[index];
                    add(42, "seek and keep decoding rate=" + QString::number(rate) + " percent=" + QString::number(percent), [this, percent]()
                        {
                            const QRect track = m_player->progressTrackRect();
                            m_observedFrame = m_player->m_cachedFrameStart;
                            m_unexpectedPause = false;
                            click(QPoint(track.left() + track.width() * percent / 100, track.center().y()));
                        },
                        [this]()
                        {
                            const int32_t state = m_player->m_core.snapshot().m_state;
                            m_unexpectedPause = m_unexpectedPause || state == LumaPlayerCoreCStatePaused ||
                                state == LumaPlayerCoreCStateError;
                            return m_elapsed.elapsed() >= 700 && !m_unexpectedPause &&
                                state == LumaPlayerCoreCStatePlaying && !m_player->m_hasDragPosition &&
                                m_player->m_cachedFrameStart != m_observedFrame;
                        },
                        5000);
                }
            }
            break;
        }
        case CaseProgressClick:
        {
            const int32_t rates[] = {1000, 2000, 5000};
            for (size_t rateIndex = 0; rateIndex < 3; ++rateIndex)
            {
                const int32_t rate = rates[rateIndex];
                add(40, "prepare playback rate " + QString::number(rate), [this, rate]()
                    {
                        m_player->m_core.setPlaybackRatePermilleAsync(rate);
                        m_player->m_core.playAsync();
                    },
                    [this, rate]()
                    {
                        return m_player->m_snapshot.m_ratePermille == rate &&
                            m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                    });
                add(40, "track press without motion does not pause", [this]()
                    {
                        const QPoint point = m_player->progressTrackRect().center();
                        move(point);
                        m_unexpectedPause = false;
                        QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(m_player, &press);
                    },
                    [this]()
                    {
                        m_unexpectedPause = m_unexpectedPause || m_player->m_core.snapshot().m_state != LumaPlayerCoreCStatePlaying;
                        return m_elapsed.elapsed() >= 200 && !m_unexpectedPause &&
                            m_player->m_progressPressPending && !m_player->m_isDraggingProgress;
                    });
                add(40, "release stationary track press seeks without drag", [this]()
                    {
                        const QPoint point = m_player->progressTrackRect().center();
                        QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                        QApplication::sendEvent(m_player, &release);
                    },
                    [this]()
                    {
                        return !m_player->m_hasDragPosition && !m_player->m_progressPressPending &&
                            m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                    });
                for (int32_t target = 1; target <= 2; ++target)
                {
                    add(40, "click seek and actual continuing frames rate=" + QString::number(rate) + " target=" + QString::number(target), [this, target]()
                        {
                            const QRect track = m_player->progressTrackRect();
                            const QPoint point(track.left() + track.width() * target / 4, track.center().y());
                            m_position = m_player->progressPointToTime100ns(point);
                            m_unexpectedPause = false;
                            click(point);
                        },
                        [this]()
                        {
                            m_unexpectedPause = m_unexpectedPause ||
                                m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                            return !m_unexpectedPause && !m_player->m_isDraggingProgress && !m_player->m_hasDragPosition &&
                                m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying &&
                                m_player->m_cachedFrameStart >= m_position + 800000 &&
                                m_player->m_cachedFrameStart < m_position + 15000000 && m_elapsed.elapsed() < 800;
                        },
                        5000);
                }
                add(40, "rapid consecutive track clicks keep playing rate=" + QString::number(rate), [this]()
                    {
                        const QRect track = m_player->progressTrackRect();
                        m_unexpectedPause = false;
                        click(QPoint(track.left() + track.width() / 3, track.center().y()));
                        QTimer::singleShot(20, this, [this]()
                            {
                                const QRect currentTrack = m_player->progressTrackRect();
                                const QPoint target(currentTrack.left() + currentTrack.width() / 2, currentTrack.center().y());
                                m_position = m_player->progressPointToTime100ns(target);
                                click(target);
                            });
                    },
                    [this]()
                    {
                        m_unexpectedPause = m_unexpectedPause || m_player->m_core.snapshot().m_state == LumaPlayerCoreCStatePaused;
                        return m_elapsed.elapsed() >= 100 && !m_unexpectedPause && !m_player->m_hasDragPosition &&
                            m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying &&
                            m_player->m_cachedFrameStart >= m_position && m_player->m_cachedFrameStart < m_position + 20000000;
                    },
                    5000);
            }
            break;
        }
        case CaseAbRateLatency:
        {
            add(41, "play at 2x before AB edits", [this]()
                {
                    m_player->m_core.setPlaybackRatePermilleAsync(2000);
                    m_player->m_core.playAsync();
                },
                [this]()
                {
                    return m_player->m_snapshot.m_ratePermille == 2000 &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            for (int32_t edit = 0; edit < 3; ++edit)
            {
                add(41, "playing 2x actual AB edit=" + QString::number(edit), [this, edit]()
                    {
                        m_position = m_player->m_snapshot.m_duration100ns * (edit == 1 ? 4 : edit == 2 ? 3
                                                                                                       : 2) /
                            8;
                        m_position = m_player->progressPointToTime100ns(QPoint(m_player->progressTimeToX(m_position),
                            m_player->progressTrackRect().center().y()));
                        m_monitorFrames = true;
                        m_observedFrame = m_player->m_cachedFrameStart;
                        m_lastFrameMs = 0;
                        m_maxFrameGapMs = 0;
                        m_unexpectedPause = false;
                        menuAt(m_position, edit == 1 ? 1 : 0);
                    },
                    [this, edit]()
                    {
                        const LumaPlayerCoreCSnapshot snapshot = m_player->m_snapshot;
                        const int64_t point = edit == 1 ? snapshot.m_loopBEnd100ns : snapshot.m_loopAStart100ns;
                        return (edit == 1 ? snapshot.m_hasLoopB : snapshot.m_hasLoopA) &&
                            std::abs(point - m_position) < 1000000 &&
                            snapshot.m_state == LumaPlayerCoreCStatePlaying && m_elapsed.elapsed() < 800 &&
                            !m_unexpectedPause && m_maxFrameGapMs < 200;
                    },
                    5000);
            }
            add(41, "clear before paused AB checks", [this]()
                {
                    m_player->m_core.clearLoopAsync();
                },
                [this]()
                {
                    return !m_player->m_snapshot.m_hasLoopA && !m_player->m_snapshot.m_hasLoopB;
                });
            add(41, "clear cancels in-flight AB preparation without late marker", [this]()
                {
                    m_player->postCore(LumaPlayerCoreCOperationSetA, m_player->m_snapshot.m_duration100ns * 3 / 4);
                    m_player->postCore(LumaPlayerCoreCOperationClearLoop);
                },
                [this]()
                {
                    return m_elapsed.elapsed() >= 800 && !m_player->m_snapshot.m_hasLoopA &&
                        !m_player->m_snapshot.m_hasLoopB && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying;
                });
            add(41, "pause at 5x before actual AB menu", [this]()
                {
                    m_player->m_core.setPlaybackRatePermilleAsync(5000);
                    m_player->m_core.pauseAsync();
                },
                [this]()
                {
                    return m_player->m_snapshot.m_ratePermille == 5000 &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused;
                });
            add(41, "paused right-click A: actual Core result within 800ms", [this]()
                {
                    menuAt(m_player->m_snapshot.m_duration100ns / 4, 0);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopA && !m_player->m_hasDragPosition &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused && m_elapsed.elapsed() < 800;
                },
                5000);
            add(41, "paused right-click B: actual Core result within 800ms", [this]()
                {
                    menuAt(m_player->m_snapshot.m_duration100ns / 2, 1);
                },
                [this]()
                {
                    return m_player->m_snapshot.m_hasLoopB && !m_player->m_hasDragPosition &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused && m_elapsed.elapsed() < 800;
                },
                5000);
            add(41, "resume at 5x after AB: advancing actual frames", [this]()
                {
                    m_position = m_player->m_cachedFrameStart;
                    click(m_player->playButtonRect().center());
                },
                [this]()
                {
                    return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying &&
                        m_player->m_cachedFrameStart != m_position && m_elapsed.elapsed() < 800;
                },
                5000);
            break;
        }
        default:
        {
            m_results.check(id, false, "missing GUI implementation");
            break;
        }
        }
    }
    add(0, "cleanup close", [this]()
        {
            m_player->close();
        },
        [this]()
        {
            return m_player->m_closeReady;
        },
        10000);
}

void GuiTestRunner::add(int32_t id, const QString& detail, const std::function<void()>& action,
    const std::function<bool()>& ready, int32_t timeout)
{
    GuiTestStep step;
    step.m_id = id;
    step.m_detail = detail;
    step.m_action = action;
    step.m_ready = ready;
    step.m_timeout = timeout;
    m_steps.push_back(step);
}

void GuiTestRunner::start()
{
    m_timer.start(20);
}

bool GuiTestRunner::completed() const
{
    return !m_aborting && m_index == m_steps.size() && m_results.failures() == 0;
}

void GuiTestRunner::tick()
{
    if (m_monitorFrames)
    {
        const int64_t nowMs = m_elapsed.elapsed();
        m_maxFrameGapMs = (std::max)(m_maxFrameGapMs, nowMs - m_lastFrameMs);
        if (m_player->m_cachedFrameStart != m_observedFrame)
        {
            m_observedFrame = m_player->m_cachedFrameStart;
            m_lastFrameMs = nowMs;
        }
        m_unexpectedPause = m_unexpectedPause || m_player->m_core.snapshot().m_state != LumaPlayerCoreCStatePlaying;
    }
    if (m_aborting)
    {
        // Retire our test dialogs before closing the player, including unsaved synthetic recordings.
        QWidget* modal = QApplication::activeModalWidget();
        if (modal)
        {
            QFileDialog* file = qobject_cast<QFileDialog*>(modal);
            if (file)
            {
                file->reject();
            }
            else
            {
                const QList<QPushButton*> buttons = modal->findChildren<QPushButton*>();
                QPushButton* exit = nullptr;
                for (int i = 0; i < buttons.size(); ++i)
                {
                    if (buttons[i]->text() == QStringLiteral("放弃"))
                    {
                        exit = buttons[i];
                        break;
                    }
                    if (buttons[i]->text() == QStringLiteral("取消") || buttons[i]->text() == g_config.m_confirmText)
                    {
                        exit = buttons[i];
                    }
                }
                if (exit)
                {
                    exit->click();
                }
                else
                {
                    modal->close();
                }
            }
            return;
        }
        if (!m_player->m_closeReady)
        {
            m_player->close();
        }
        if (m_player->m_closeReady)
        {
            m_timer.stop();
            QApplication::exit(1);
        }
        return;
    }
    if (m_index >= m_steps.size())
    {
        m_timer.stop();
        m_results.check(0, m_results.failures() == 0, "GUI sequence complete");
        QApplication::exit(m_results.failures() == 0 ? 0 : 1);
        return;
    }
    const size_t index = m_index;
    if (!m_started)
    {
        m_started = true;
        m_elapsed.restart();
        m_steps[index].m_action();
        return;
    }
    const bool ready = m_steps[index].m_ready();
    if (ready || m_elapsed.elapsed() >= m_steps[index].m_timeout)
    {
        m_results.check(m_steps[index].m_id, ready, m_steps[index].m_detail + " ms=" + QString::number(m_elapsed.elapsed()) + (m_monitorFrames ? " maxFrameGapMs=" + QString::number(m_maxFrameGapMs) : QString()));
        m_monitorFrames = false;
        if (!ready)
        {
            m_aborting = true;
            QWidget* modal = QApplication::activeModalWidget();
            if (modal != nullptr)
            {
                const QList<QPushButton*> buttons = modal->findChildren<QPushButton*>();
                for (int32_t i = 0; i < buttons.size(); ++i)
                {
                    if (buttons[i]->text() == g_config.m_confirmText)
                    {
                        buttons[i]->click();
                        break;
                    }
                }
            }
            m_player->close();
            return;
        }
        ++m_index;
        m_started = false;
    }
}

void GuiTestRunner::move(const QPoint& point, Qt::MouseButtons buttons)
{
    QCursor::setPos(m_player->mapToGlobal(point));
    QEvent enter(QEvent::Enter);
    QApplication::sendEvent(m_player, &enter);
    QMouseEvent event(QEvent::MouseMove, point, m_player->mapToGlobal(point), Qt::NoButton, buttons, Qt::NoModifier);
    QApplication::sendEvent(m_player, &event);
}

void GuiTestRunner::click(const QPoint& point, Qt::MouseButton button)
{
    move(point);
    QPointer<QWidget> target = m_player;
    if (m_player->m_microphonePopupVisible && m_player->m_microphoneView && m_player->microphonePopupRect().contains(point))
    {
        target = m_player->m_microphoneView;
    }
    const QPoint global = m_player->mapToGlobal(point);
    const QPoint local = target->mapFromGlobal(global);
    QMouseEvent press(QEvent::MouseButtonPress, local, global, button, button, Qt::NoModifier);
    QApplication::sendEvent(target, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, global, button, Qt::NoButton, Qt::NoModifier);
    if (target)
    {
        QApplication::sendEvent(target, &release);
    }
}

void GuiTestRunner::key(int32_t keyValue, Qt::KeyboardModifiers modifiers)
{
    QWidget* target = m_player;
    if (m_player->m_microphoneView && m_player->microphonePopupRect().contains(m_player->mapFromGlobal(QCursor::pos())))
    {
        target = m_player->m_microphoneView;
    }
    QKeyEvent press(QEvent::KeyPress, keyValue, modifiers);
    QApplication::sendEvent(target, &press);
    QKeyEvent release(QEvent::KeyRelease, keyValue, modifiers);
    QApplication::sendEvent(target, &release);
}

void GuiTestRunner::doubleClick()
{
    const QPoint point = m_player->videoViewportRect().center();
    click(point);
    QTimer::singleShot((std::min)(80, (std::max)(1, QApplication::doubleClickInterval() / 3)), this, [this, point]()
        {
            QMouseEvent event(QEvent::MouseButtonDblClick, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(m_player, &event);
            QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(m_player, &release);
        });
}

void GuiTestRunner::chooseMenu(int32_t index)
{
    QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    if (menu == nullptr)
    {
        return;
    }
    if (index < 0 || index >= menu->actions().size())
    {
        menu->close();
        return;
    }
    const QPoint point = menu->actionGeometry(menu->actions()[index]).center();
    QMouseEvent press(QEvent::MouseButtonPress, point, menu->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(menu, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, point, menu->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(menu, &release);
}

void GuiTestRunner::menuAt(int64_t position, int32_t index)
{
    QTimer::singleShot(100, this, [this, index]()
        {
            chooseMenu(index);
        });
    click(QPoint(m_player->progressTimeToX(position), m_player->progressTrackRect().center().y()), Qt::RightButton);
}

void GuiTestRunner::selectMicrophoneDevice(int32_t slot, int32_t device, int32_t caseId)
{
    m_position = 0;
    QTimer::singleShot(400, this, [this, device, caseId]()
        {
            QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
            m_results.check(caseId, menu != nullptr, "desktop click opens microphone device menu");
            if (!menu || device < 0 || device >= menu->actions().size())
            {
                if (menu)
                {
                    menu->close();
                }
                m_position = -1;
                return;
            }
            menu->grab().save(m_directory + "/device-menu.png");
            const QPoint global = menu->mapToGlobal(menu->actionGeometry(menu->actions()[device]).center());
            QWidget* hit = QApplication::widgetAt(global);
            m_results.check(caseId, hit && hit->window() == menu, "device menu receives mouse hit instead of tool window");
            m_results.check(caseId, clickDesktopPoint(global), "system accepts device selection mouse input");
            QTimer::singleShot(400, this, [this, caseId]()
                {
                    QMenu* remaining = qobject_cast<QMenu*>(QApplication::activePopupWidget());
                    m_results.check(caseId, !remaining, "native menu selection closes dropdown");
                    if (remaining)
                    {
                        remaining->close();
                    }
                    m_position = 1;
                });
        });
    const QRect area = m_player->microphoneSlotRect(slot);
    m_results.check(caseId, clickDesktopPoint(m_player->mapToGlobal(QPoint(area.center().x(), area.bottom() - 8))),
        "system accepts device dropdown mouse input");
}