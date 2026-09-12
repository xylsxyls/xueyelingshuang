#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "GuiTestRunner.h"
#include "TestCaseRegistry.h"
#include "LumaPlayerAudioRender.h"
#include "LumaPlayerVideoRender.h"
#include "LumaPlayerCoreBridge.h"
#include "LumaPlayerLogicController.h"
#include "Config.h"
#include "LogManager/LogManagerAPI.h"
#include <QtWidgets>
#include <QElapsedTimer>
// 仅测试观察器读取现有成员，不修改其状态，不影响产品编译与布局
#define private public
#include "LumaPlayer.h"
#undef private
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

GuiTestRunner::GuiTestRunner(LumaPlayer* player, const QString& media, const QString& directory, const QString& mode) :
QObject(nullptr),
m_player(player),
m_results(directory, mode.toInt()),
m_directory(directory),
m_index(0),
m_started(false),
m_aborting(false),
m_position(0),
m_dragMatches(0)
{
    QObject::connect(&m_timer, &QTimer::timeout, this, &GuiTestRunner::tick);
    const int32_t id = mode.toInt();
    if (id == CaseEmptyUi)
    {
        add(4, "empty window top reveal and bottom absent", [this]() { move(QPoint(100, 5)); },
            [this]() { return m_player->m_topVisibleHeight > 0 && m_player->m_bottomVisibleHeight == 0; });
        add(4, "central plus has no tooltip", [this]() { move(m_player->plusButtonRect().center()); },
            [this]() { return m_elapsed.elapsed() > 200 && !QToolTip::isVisible(); });
        add(14, "pin empty window keeps bottom hidden", [this]() { move(QPoint(100, 5)); },
            [this]() { return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight; });
        add(14, "pin via real button", [this]() { click(m_player->pinButtonRect().center()); },
            [this]() { return m_player->m_pinned && m_player->m_bottomVisibleHeight == 0; });
    }
    else
    {
        add(0, "prepare top overlay", [this]() { move(QPoint(100, 5)); },
            [this]() { return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight; });
        add(0, "prepare fixed controls", [this]() { click(m_player->pinButtonRect().center()); },
            [this]() { return m_player->m_pinned; });
        add(3, "load fixture", [this, media]() { m_player->loadMedia(media); },
            [this]() { return m_player->m_hasMedia && !m_player->m_cachedFrame.isNull(); }, 15000);
        add(0, "prepare pause", [this]() { if (m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying) { key(Qt::Key_Space); } },
            [this]() { return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused && m_player->m_bottomVisibleHeight == g_config.m_bottomOverlayHeight; });
        if (id == CaseAbMenu || id == CasePausedMenu || id == CaseFrameKey || id == CaseResetUi)
        {
            add(0, "prepare A", [this]() { menuAt(10000000, 0); }, [this]() { return m_player->m_snapshot.m_hasLoopA && !m_player->m_hasDragPosition; });
            add(0, "prepare B", [this]() { menuAt(20000000, 1); }, [this]() { return m_player->m_snapshot.m_hasLoopB && !m_player->m_hasDragPosition; });
        }
        switch (id)
        {
            case CaseHelpUi:
            {
                for (int32_t closeKind = 0; closeKind < 2; ++closeKind)
                {
                    add(15, "open modal via help", [this]() { click(m_player->helpButtonRect().center()); },
                        []() { return QApplication::activeModalWidget() != nullptr; });
                    add(15, "modal centered and version left aligned", [this]() {
                        QWidget* dialog = QApplication::activeModalWidget();
                        if (dialog != nullptr)
                        {
                            dialog->grab().save(m_directory + "/help.png");
                        }
                        }, [this]() {
                        QWidget* dialog = QApplication::activeModalWidget();
                        if (dialog == nullptr || (dialog->geometry().center() - m_player->geometry().center()).manhattanLength() > 2)
                        {
                            return false;
                        }
                        const QList<QLabel*> labels = dialog->findChildren<QLabel*>();
                        for (int32_t i = 0; i < labels.size(); ++i)
                        {
                            if (labels[i]->text() == g_config.m_versionLabel)
                            {
                                return (labels[i]->alignment() & Qt::AlignLeft) && labels[i]->font().pointSize() == g_config.m_fontSize;
                            }
                        }
                        return false;
                        });
                    add(15, "title mouse drag", [this]() {
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
                        }, [this]() { QWidget* dialog = QApplication::activeModalWidget(); return dialog != nullptr && dialog->pos() == m_normal.topLeft() + QPoint(30, 20); });
                    add(15, "modal button closes asynchronously", [this, closeKind]() {
                        QWidget* dialog = QApplication::activeModalWidget();
                        if (dialog == nullptr)
                        {
                            return;
                        }
                        const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
                        for (int32_t i = 0; i < buttons.size(); ++i)
                        {
                            if (buttons[i]->text() == (closeKind == 0 ? g_config.m_confirmText : g_config.m_closeText))
                            {
                                buttons[i]->click();
                                break;
                            }
                        }
                        }, []() { return QApplication::activeModalWidget() == nullptr; });
                }
                break;
            }
            case CaseQueuedClose:
            {
                add(16, "close during queued reset", [this]() { key(Qt::Key_Up, Qt::ControlModifier); click(m_player->resetButtonRect().center()); m_player->close(); },
                    [this]() { return m_player->m_closeReady; }, 10000);        break;
            }
            case CaseButtonVisuals:
            {
                add(4, "played color AB colors and layout", [this]() { m_player->grab().save(m_directory + "/layout.png"); },
                    [this]() { return g_config.m_loopAColor != g_config.m_themeColor && g_config.m_loopBColor != g_config.m_themeColor &&
                        m_player->playButtonRect().center().x() == m_player->rect().center().x() &&
                            m_player->playButtonRect().top() > m_player->progressTrackRect().bottom(); });
                for (int32_t button = 0; button < 4; ++button)
                {
                    add(CaseButtonVisuals, "button tooltip " + QString::number(button), [this, button]() {
                        // 素材加载会调整窗口大小，必须在动作执行时取按钮矩形。
                        const QRect buttons[] = {m_player->loadButtonRect(), m_player->resetButtonRect(), m_player->pinButtonRect(), m_player->helpButtonRect()};
                        move(buttons[button].center());
                        }, [this]() { return m_elapsed.elapsed() >= 100 && QToolTip::isVisible(); });
                }
                add(CaseButtonVisuals, "capture knob without hover", [this]() {
                    move(m_player->videoViewportRect().center());
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    const int32_t radius = g_config.m_positionDragRadius + 2;
                    m_image = m_player->grab(QRect(point - QPoint(radius, radius), QSize(radius * 2 + 1, radius * 2 + 1))).toImage();
                    }, []() { return true; });
                add(CaseButtonVisuals, "hover knob enlarges pixels", [this]() {
                    move(QPoint(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y()));
                    }, [this]() {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    const int32_t radius = g_config.m_positionDragRadius + 2;
                    return m_player->grab(QRect(point - QPoint(radius, radius), m_image.size())).toImage() != m_image;
                    });
                add(CaseButtonVisuals, "press knob restores radius and overlays blue", [this]() {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                    }, [this]() {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    const QImage pixels = m_player->grab().toImage();
                    const QColor center(pixels.pixel(point));
                    const QColor outside(pixels.pixel(point + QPoint(0, -g_config.m_positionRadius - 2)));
                    return m_player->m_dragProgress && center != QColor(m_image.pixel(m_image.width() / 2, m_image.height() / 2)) && outside == QColor(m_image.pixel(m_image.width() / 2, m_image.height() / 2 - g_config.m_positionRadius - 2));
                    });
                add(CaseButtonVisuals, "knob release restores normal color", [this]() {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &release);
                    }, [this]() {
                    const QPoint point(m_player->progressTimeToX(m_player->displayPosition100ns()), m_player->progressTrackRect().center().y());
                    return !m_player->m_dragProgress && !m_player->m_hasDragPosition &&
                        QColor(m_player->grab().toImage().pixel(point)) == QColor(m_image.pixel(m_image.width() / 2, m_image.height() / 2));
                    });
                add(37, "pin pressed overlay", [this]() {
                    QPoint p = m_player->pinButtonRect().center();
                    move(p);
                    m_image = m_player->grab(m_player->pinButtonRect()).toImage();
                    QMouseEvent press(QEvent::MouseButtonPress, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                    }, [this]() { return m_player->m_leftPressed && m_player->grab(m_player->pinButtonRect()).toImage() != m_image; });
                add(37, "pin release clears pressed", [this]() {
                    QPoint p = m_player->pinButtonRect().center();
                    QMouseEvent release(QEvent::MouseButtonRelease, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &release);
                    }, [this]() { return !m_player->m_leftPressed; });
                break;
            }
            case CasePlayPauseUi:
            {
                add(22, "space plays", [this]() { key(Qt::Key_Space); }, [this]() { return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying; });
                add(22, "button pauses", [this]() { click(m_player->playButtonRect().center()); }, [this]() { return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused; });
                add(22, "video single click plays", [this]() { click(m_player->videoViewportRect().center()); }, [this]() { return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying; });
                break;
            }
            case CaseDragUi:
            {
                add(6, "drag starts", [this]() {
                    const QPoint p = m_player->progressTrackRect().center();
                    move(p);
                    QMouseEvent event(QEvent::MouseButtonPress, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &event);
                    m_dragMatches = 0;
                    }, [this]() { return m_player->m_dragProgress; });
                for (int32_t sample = 1; sample <= 32; ++sample)
                {
                    add(6, "drag immediate progress sample=" + QString::number(sample), [this, sample]() {
                        QRect track = m_player->progressTrackRect();
                        QPoint p(track.left() + sample * (track.width() - 1) / 40, track.center().y());
                        move(p, Qt::LeftButton);
                        if (m_player->displayPosition100ns() == m_player->progressPointToTime100ns(p))
                        {
                            ++m_dragMatches;
                        }
                        }, [this, sample]() { return m_dragMatches == sample; });
                }
                add(6, "final seek frame covers target", [this]() {
                    QRect track = m_player->progressTrackRect();
                    QPoint p(track.left() + 32 * (track.width() - 1) / 40, track.center().y());
                    m_position = m_player->progressPointToTime100ns(p);
                    QMouseEvent event(QEvent::MouseButtonRelease, p, m_player->mapToGlobal(p), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &event);
                    }, [this]() { return !m_player->m_hasDragPosition && !m_player->m_dragProgress &&
                        m_player->m_cachedFrameStart <= m_position && m_player->m_cachedFrameEnd > m_position; }, 8000);
                break;
            }
            case CaseSeekKeys:
            {
                add(0, "prepare position", [this]() { menuAt(50000000, -1); }, [this]() { return !m_player->m_hasDragPosition && m_player->m_cachedFrameStart > 49000000; });
                add(7, "left key default 2sec", [this]() {
                    move(m_player->videoViewportRect().center());
                    m_position = m_player->m_snapshot.m_position100ns;
                    key(Qt::Key_Left);
                    }, [this]() { return m_player->m_cachedFrameStart <= m_position - 20000000 &&
                        m_player->m_cachedFrameEnd > m_position - 20000000; });
                break;
            }
            case CaseFractionalKeys:
            {
                add(0, "prepare position", [this]() { menuAt(50000000, -1); }, [this]() { return !m_player->m_hasDragPosition && m_player->m_cachedFrameStart > 49000000; });
                add(7, "left key configured 0.5sec", [this]() {
                    move(m_player->videoViewportRect().center());
                    m_position = m_player->m_snapshot.m_position100ns;
                    key(Qt::Key_Left);
                    }, [this]() { return m_player->m_cachedFrameStart <= m_position - 5000000 &&
                        m_player->m_cachedFrameEnd > m_position - 5000000; });
                break;
            }
            case CaseAbMenu:
            {
                add(9, "playing before AB edit", [this]() { key(Qt::Key_Space); },
                    [this]() { return m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying; });
                add(9, "right-click A preserves playing", [this]() { menuAt(10000000, 0); },
                    [this]() { return m_player->m_snapshot.m_hasLoopA && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying; });
                add(9, "right-click B preserves playing", [this]() { menuAt(20000000, 1); },
                    [this]() { return m_player->m_snapshot.m_hasLoopB && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePlaying; });
                break;
            }
            case CasePausedMenu:
            {
                add(6, "paused right click synchronizes actual frame and progress", [this]() { menuAt(30000000, -1); },
                    [this]() { return !m_player->m_hasDragPosition && m_player->m_cachedFrameStart > 29000000 &&
                        m_player->m_cachedFrameStart < 31000000 && m_player->displayPosition100ns() == m_player->m_cachedFrameStart; });
                break;
            }
            case CaseFrameKey:
            {
                add(11, "hover A then one frame key", [this]() {
                    m_position = m_player->m_snapshot.m_loopAStart100ns;
                    move(QPoint(m_player->progressTimeToX(m_position), m_player->progressTrackRect().center().y()));
                    key(Qt::Key_Right);
                    }, [this]() { return m_player->m_snapshot.m_loopAStart100ns > m_position &&
                        m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused; });
                break;
            }
            case CaseResetUi:
            {
                add(12, "AB alone does not dirty reset", []() {}, [this]() { return !m_player->m_resetEnabled; });
                add(CaseResetUi, "start video pan", [this]() {
                    m_position = m_player->m_snapshot.m_position100ns;
                    const QPoint point = m_player->videoViewportRect().center();
                    move(point);
                    QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &press);
                    move(point + QPoint(40, 0), Qt::LeftButton);
                    }, [this]() { return m_player->m_panOffset == QPointF(40, 0) && m_player->m_resetEnabled; });
                add(CaseResetUi, "manual pan return stays dirty", [this]() {
                    move(m_player->videoViewportRect().center(), Qt::LeftButton);
                    }, [this]() { return m_player->m_panOffset.isNull() && m_player->m_resetEnabled; });
                add(CaseResetUi, "release pan then zoom", [this]() {
                    const QPoint point = m_player->videoViewportRect().center();
                    QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &release);
                    QWheelEvent wheel(point, m_player->mapToGlobal(point), QPoint(), QPoint(0, 120), 120, Qt::Vertical, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(m_player, &wheel);
                    }, [this]() { return m_player->m_zoomPercent > 100 && m_player->m_resetEnabled; });
                add(12, "rate modifies actual state", [this]() { move(m_player->videoViewportRect().center()); key(Qt::Key_Up, Qt::ControlModifier); },
                    [this]() { return m_player->m_snapshot.m_ratePermille > 1000 && m_player->m_resetEnabled; });
                add(12, "reset waits result and preserves AB", [this]() { click(m_player->resetButtonRect().center()); },
                    [this]() { return !m_player->m_resetEnabled && m_player->m_snapshot.m_ratePermille == 1000 && m_player->m_snapshot.m_hasLoopA && m_player->m_snapshot.m_hasLoopB &&
                        m_player->m_panOffset.isNull() && m_player->m_zoomPercent == 100 &&
                            m_player->m_snapshot.m_position100ns == m_position && m_player->m_snapshot.m_state == LumaPlayerCoreCStatePaused; });
                break;
            }
            case CaseWindowState:
            {
                add(13, "set ordinary geometry", [this]() { m_player->setGeometry(140, 150, 680, 450); },
                    [this]() { return m_player->geometry() == QRect(140, 150, 680, 450); });
                for (int32_t sequence = 0; sequence < 3; ++sequence)
                {
                    add(13, "maximize", [this]() { m_normal = m_player->geometry(); click(m_player->maximizeButtonRect().center()); },
                        [this]() { return m_player->isMaximized() && !m_player->isFullScreen(); });
                    add(13, "full screen keeps pin", [this]() { doubleClick(); },
                        [this]() { return m_player->isFullScreen() && m_player->m_pinned && m_player->m_bottomVisibleHeight == g_config.m_bottomOverlayHeight; });
                    add(13, "fullscreen max button only changes future state", [this]() { click(m_player->maximizeButtonRect().center()); },
                        [this]() { return m_player->isFullScreen() && !m_player->isMaximizedOutsideFullScreen(); });
                    add(13, "exit full restores exact ordinary rectangle", [this]() { doubleClick(); },
                        [this]() { return !m_player->isFullScreen() && !m_player->isMaximized() && m_player->geometry() == m_normal; });
                    add(13, "new user ordinary rectangle", [this, sequence]() { m_player->setGeometry(170 + sequence * 10, 160, 640 + sequence * 10, 430); },
                        [this, sequence]() { return m_player->geometry() == QRect(170 + sequence * 10, 160, 640 + sequence * 10, 430); });
                }
                break;
            }
            case CasePinnedUi:
            {
                add(14, "pinned layout excludes both bars", []() {}, [this]() {
                    return m_player->m_bottomVisibleHeight == g_config.m_bottomOverlayHeight &&
                        m_player->videoViewportRect().top() >= m_player->topOverlayRect().bottom() &&
                            m_player->videoViewportRect().bottom() <= m_player->bottomOverlayRect().top(); });
                add(14, "unpin hide bars", [this]() { click(m_player->pinButtonRect().center()); move(m_player->videoViewportRect().center()); },
                    [this]() { return !m_player->m_pinned && m_player->m_topVisibleHeight == 0 && m_player->m_bottomVisibleHeight == 0; }, 6000);
                add(14, "reveal and repin", [this]() { move(QPoint(100, 5)); },
                    [this]() { return m_player->m_topVisibleHeight == g_config.m_topOverlayHeight; });
                add(14, "pin restored", [this]() { click(m_player->pinButtonRect().center()); },
                    [this]() { return m_player->m_pinned; });
                break;
            }
            default:
            {
                m_results.check(id, false, "missing GUI implementation");
                break;
            }
        }
    }
    add(0, "cleanup close", [this]() { m_player->close(); }, [this]() { return m_player->m_closeReady; }, 10000);
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

void GuiTestRunner::tick()
{
    if (m_aborting)
    {
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
        m_results.check(m_steps[index].m_id, ready, m_steps[index].m_detail + " ms=" + QString::number(m_elapsed.elapsed()));
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
    QEvent enter(QEvent::Enter);
    QApplication::sendEvent(m_player, &enter);
    QMouseEvent event(QEvent::MouseMove, point, m_player->mapToGlobal(point), Qt::NoButton, buttons, Qt::NoModifier);
    QApplication::sendEvent(m_player, &event);
}

void GuiTestRunner::click(const QPoint& point, Qt::MouseButton button)
{
    move(point);
    QMouseEvent press(QEvent::MouseButtonPress, point, m_player->mapToGlobal(point), button, button, Qt::NoModifier);
    QApplication::sendEvent(m_player, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), button, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(m_player, &release);
}

void GuiTestRunner::key(int32_t keyValue, Qt::KeyboardModifiers modifiers)
{
    QKeyEvent press(QEvent::KeyPress, keyValue, modifiers);
    QApplication::sendEvent(m_player, &press);
    QKeyEvent release(QEvent::KeyRelease, keyValue, modifiers);
    QApplication::sendEvent(m_player, &release);
}

void GuiTestRunner::doubleClick()
{
    const QPoint point = m_player->videoViewportRect().center();
    move(point);
    QMouseEvent event(QEvent::MouseButtonDblClick, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_player, &event);
    QMouseEvent release(QEvent::MouseButtonRelease, point, m_player->mapToGlobal(point), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(m_player, &release);
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
    QTimer::singleShot(100, this, [this, index]() { chooseMenu(index); });
    click(QPoint(m_player->progressTimeToX(position), m_player->progressTrackRect().center().y()), Qt::RightButton);
}