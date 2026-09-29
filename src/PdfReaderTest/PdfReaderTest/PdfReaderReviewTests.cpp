#include "PdfReaderReviewTests.h"
#include "PdfReaderTestConfigGuard.h"
#include "PdfReaderTestWorkerGate.h"
#include "PdfReaderTestHelper.h"
#include "PdfReaderTestUiHelper.h"
#include "../../PdfReader/PdfReader/PdfReader.h"
#include "../../PdfReader/PdfReader/PdfReaderDialogHelper.h"
#include "QtControls/DialogBase.h"
#include "QtControls/Menu.h"
#include <QApplication>
#include <QPointer>
#include <QAbstractButton>
#include <QElapsedTimer>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <limits>
#include <stdexcept>
#include <cstring>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

void PdfReaderReviewTests::run(int32_t id, const QString& input, const QString& directory)
{
    PdfReaderTestConfigGuard guard;
    g_config.m_useNativeFileDialog = false;
    if (id == 33 || id == 34)
    {
        libraryBoundaries(id, input, directory);
    }
    else if (id == 35)
    {
        callbackDestruction(input);
    }
    else if (id == 21)
    {
        configuration(directory);
    }
    else if (id == 22)
    {
        shadowAndMenu(input, directory);
    }
    else
    {
        asynchronous(id, input, directory);
    }
}

void PdfReaderReviewTests::libraryBoundaries(int32_t id, const QString& input, const QString& directory)
{
    std::shared_ptr<PdfReaderCoreCContext> core = PdfReaderTestHelper::core(input);
    if (id == 33)
    {
        char output[6] = {'x', 'x', 'x', 'x', 'x', 'x'};
        PdfReaderTestHelper::require(CStringManager::CopyToBuffer("abc", nullptr, 0) == 4, "query includes NUL");
        PdfReaderTestHelper::require(CStringManager::CopyToBuffer("abc", output, 0) == 4 && output[0] == 'x', "zero capacity writes nothing");
        PdfReaderTestHelper::require(CStringManager::CopyToBuffer("abc", output, 1) == 4 && output[0] == 0 && output[1] == 'x', "NUL-only capacity preserves sentinel");
        PdfReaderTestHelper::require(CStringManager::CopyToBuffer("abc", output, 3) == 4 && std::strcmp(output, "ab") == 0 && output[3] == 'x', "truncated text terminated without overrun");
        PdfReaderTestHelper::require(CStringManager::CopyToBuffer("abc", output, 4) == 4 && std::strcmp(output, "abc") == 0 && output[4] == 'x', "exact capacity includes full text");
        PdfReaderTestHelper::require(CStringManager::CopyToBuffer("", output, 1) == 1 && output[0] == 0, "empty string writes only NUL");
        const QByteArray path = input.toUtf8();
        PdfReaderTestHelper::require(pdfReaderCoreGetFilePath(core.get(), nullptr, 0) == static_cast<size_t>(path.size() + 1), "C API UTF8 path byte count");
        output[2] = 'x';
        PdfReaderTestHelper::require(pdfReaderCoreGetFilePath(core.get(), output, 2) == static_cast<size_t>(path.size() + 1) &&
            output[0] == path[0] && output[1] == 0 && output[2] == 'x', "C API path uses bounded protocol");
        PdfReaderTestHelper::require(CSystem::joinPath("a", "b.pdf") == "a/b.pdf" &&
            CSystem::joinPath("a/", "b.pdf") == "a/b.pdf" && CSystem::joinPath("a\\", "b.pdf") == "a\\b.pdf" &&
            CSystem::joinPath("", "b.pdf") == "b.pdf", "path joins retain delimiters and relative names");
        PdfReaderTestHelper::require(CSystem::ensureFileExtension("", ".pdf").empty() &&
            CSystem::ensureFileExtension("a.PdF", ".pdf") == "a.pdf" &&
            CSystem::ensureFileExtension("a.txt", ".pdf") == "a.txt.pdf" &&
            CSystem::ensureFileExtension("folder.pdf/a", ".pdf") == "folder.pdf/a.pdf", "extension normalization independent cases");
        int32_t width = 99, height = 99, stride = 99;
        size_t bytes = 99;
        unsigned char pixels[4] = {1, 2, 3, 4};
        PdfReaderTestHelper::require(pdfReaderCoreRenderPage(core.get(), 0, -1, 2, pixels, sizeof(pixels),
            &width, &height, &stride, &bytes) == PdfReaderCoreCResultInvalidParam && width == 0 && height == 0 && stride == 0 && bytes == 0 &&
            pixels[0] == 1 && pixels[3] == 4, "invalid render clears metadata without touching pixels");
        return;
    }
    PdfReaderTestHelper::require(pdfReaderCoreMovePage(core.get(), 0, 3) == PdfReaderCoreCResultInvalidParam, "destination out of range is invalid parameter");
    PdfReaderTestHelper::widths(core.get(), QList<int>() << 200 << 300 << 400);
    PdfReaderTestHelper::require(pdfReaderCoreInsertDocument(core.get(), (directory + "/missing.pdf").toUtf8().constData(), "", 1) != 0, "missing insertion fails");
    PdfReaderTestHelper::widths(core.get(), QList<int>() << 200 << 300 << 400);
    const QStringList before = QDir(directory).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    const QByteArray original = PdfReaderTestHelper::bytes(input);
    const char* prefixes[] = {"../escape", "child\\escape", "C:escape"};
    for (size_t index = 0; index < sizeof(prefixes) / sizeof(prefixes[0]); ++index)
    {
        PdfReaderTestHelper::require(pdfReaderCoreSaveEachPageEx(core.get(), directory.toUtf8().constData(), prefixes[index], 1) ==
            PdfReaderCoreCResultInvalidParam, "export prefix must stay a file name");
    }
    PdfReaderTestHelper::require(QDir(directory).entryList(QDir::AllEntries | QDir::NoDotAndDotDot) == before &&
        PdfReaderTestHelper::bytes(input) == original, "rejected exports change no case files");
    PdfReaderTestHelper::require(pdfReaderCoreInsertDocument(core.get(), input.toUtf8().constData(), "", 1) == 0 &&
        pdfReaderCoreMovePage(core.get(), 0, 5) == 0, "editing remains usable after failures");
    PdfReaderTestHelper::widths(core.get(), QList<int>() << 200 << 300 << 400 << 300 << 400 << 200);
}

void PdfReaderReviewTests::callbackDestruction(const QString& input)
{
    QObject connectionScope;
    std::unique_ptr<PdfReader> owner(new PdfReader);
    const QPointer<PdfReader> window(owner.get());
    int completed = 0;
    bool succeeded = false;
    QObject::connect(owner.get(), &PdfReader::operationFinished, &connectionScope, [&](quint64, bool success) {
        ++completed;
        succeeded = success;
        owner.reset();
    });
    owner->show();
    PdfReaderTestHelper::require(owner->openFile(input), "callback destruction request accepted");
    PdfReaderTestUiHelper::waitUntil([&]() { return window.isNull(); }, "window deleted inside actual completion");
    PdfReaderTestHelper::require(completed == 1 && succeeded, "one successful completion before deletion");
    PdfReaderTestWorkerGate fence;
    PdfReaderTestUiHelper::waitUntil([&]() { return fence.m_entered->load(); }, "destroyed session cleanup reaches worker fence");
    PdfReaderTestHelper::require(!fence.m_timedOut->load(), "cleanup fence did not fail");
    fence.release();
}

void PdfReaderReviewTests::configuration(const QString& directory)
{
    PdfReaderTestHelper::require(&Config::instance() == &g_config, "one desktop configuration instance");
    const double originalZoom = g_config.m_initialZoom;
    {
        PdfReaderTestConfigGuard restore;
        g_config.m_initialZoom = std::numeric_limits<double>::quiet_NaN();
        bool rejected = false;
        try
        {
            g_config.validate();
        }
        catch (const std::invalid_argument&)
        {
            rejected = true;
        }
        PdfReaderTestHelper::require(rejected, "nonfinite zoom rejected before creating product resources");
    }
    PdfReaderTestHelper::require(g_config.m_initialZoom == originalZoom, "configuration restored after scoped test");
    g_config.m_titleCloseHeight = 48;
    g_config.m_dialogTitleBarHeight = 30;
    PdfReader window;
    window.show();
    bool seen = false;
    bool valid = false;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        DialogBase* dialog = dynamic_cast<DialogBase*>(QApplication::activeModalWidget());
        if (dialog)
        {
            seen = true;
            QAbstractButton* close = dialog->findChild<QAbstractButton*>(QStringLiteral("dialogCloseButton"));
            valid = dialog->customerTitleBarHeight() == 48 && close && close->isVisible() &&
                close->geometry().top() == g_config.m_titleCloseTop &&
                close->size() == QSize(g_config.m_titleCloseSize, g_config.m_titleCloseSize);
            dialog->grab().save(directory + "/custom-title.png");
            if (close)
            {
                close->click();
            }
            else
            {
                dialog->close();
            }
        }
    });
    timer.start(10);
    PdfReaderDialogHelper::message(&window, QStringLiteral("自定义说明标题"), QStringLiteral("布局不能依赖标题字符串"), true);
    timer.stop();
    PdfReaderTestHelper::require(seen && valid, "explicit about options control title height and close button for arbitrary text");
    window.close();
    PdfReaderTestUiHelper::waitIdle(window);
}

void PdfReaderReviewTests::shadowAndMenu(const QString& input, const QString& directory)
{
    QPointer<QWidget> shadow;
    {
        DialogBase dialog;
        dialog.resize(400, 240);
        dialog.move(180, 180);
        dialog.setWindowShadow(true, 0);
        PdfReaderTestHelper::require(!dialog.findChild<QWidget*>(QStringLiteral("qtControlsDialogShadow")), "zero shadow creates no window");
        dialog.show();
        const QRect body = dialog.geometry();
        dialog.setWindowShadow(true, 2);
        shadow = dialog.findChild<QWidget*>(QStringLiteral("qtControlsDialogShadow"));
        PdfReaderTestHelper::require(shadow && shadow->isVisible() && dialog.geometry() == body, "shadow outside unchanged client geometry");
        const QImage image = shadow->grab().toImage();
        image.save(directory + "/shadow.png");
        const int ratio = shadow->devicePixelRatio();
        const QPoint offset = shadow->mapFromGlobal(dialog.mapToGlobal(QPoint())) * ratio;
        const int cx = offset.x() + dialog.width() * ratio / 2;
        const int cy = offset.y() + dialog.height() * ratio / 2;
        const int nearAlpha = qAlpha(image.pixel(offset.x() + dialog.width() * ratio + ratio, cy));
        const int farAlpha = qAlpha(image.pixel(offset.x() + dialog.width() * ratio + 16 * ratio, cy));
        PdfReaderTestHelper::require(qAlpha(image.pixel(cx, cy)) == 0 && nearAlpha > farAlpha && nearAlpha < 128 && farAlpha >= 0,
            "shadow center transparent with real decreasing outer alpha");
        const QImage opaque = dialog.grab().toImage();
        PdfReaderTestHelper::require(qAlpha(opaque.pixel(opaque.width()/2, 5)) == 255, "dialog title background opaque");
        QEvent screenChange(QEvent::ScreenChangeInternal);
        QApplication::sendEvent(&dialog, &screenChange);
        PdfReaderTestHelper::require(shadow->grab().toImage() == image, "same-DPI screen refresh stable");
        dialog.hide();
        PdfReaderTestHelper::require(!shadow->isVisible(), "hidden dialog has no visible shadow");
        dialog.show();
        dialog.setWindowShadow(false, 2);
        PdfReaderTestHelper::require(!shadow->isVisible() && dialog.geometry() == body, "disable retains client geometry");
        dialog.setWindowShadow(true, 2);
        PdfReaderTestHelper::require(shadow->isVisible(), "shadow enabled again");
    }
    PdfReaderTestHelper::require(shadow.isNull(), "shadow destroyed with dialog");
    PdfReader window;
    window.show();
    PdfReaderTestHelper::require(window.openFile(input), "menu fixture accepted");
    PdfReaderTestUiHelper::waitIdle(window);
    PdfReaderThumbnailList* list = window.findChild<PdfReaderThumbnailList*>();
    bool seen = false;
    bool opaque = false;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (menu)
        {
            seen = true;
            const QImage image = menu->grab().toImage();
            opaque = !menu->testAttribute(Qt::WA_TranslucentBackground) && qAlpha(image.pixel(image.width()/2, image.height()/2)) == 255;
            image.save(directory + "/thumbnail-menu.png");
            menu->close();
        }
    });
    timer.start(10);
    QMetaObject::invokeMethod(&window, "onThumbnailContextMenu", Qt::DirectConnection,
        Q_ARG(QPoint, list->visualItemRect(list->item(0)).center()));
    timer.stop();
    PdfReaderTestHelper::require(seen && opaque, "real thumbnail context menu is opaque");
    window.close();
    PdfReaderTestUiHelper::waitIdle(window);
}

void PdfReaderReviewTests::asynchronous(int32_t id, const QString& input, const QString& directory)
{
    if (id == 28)
    {
        g_config.m_core.maxRenderPixels = 10000;
    }
    PdfReader window;
    window.show();
    QList<quint64> identities;
    QList<bool> successes;
    QObject::connect(&window, &PdfReader::operationFinished, [&](quint64 request, bool success) {
        identities.push_back(request);
        successes.push_back(success);
    });
    if (id != 23)
    {
        PdfReaderTestHelper::require(window.openFile(input), "initial open accepted");
        PdfReaderTestUiHelper::waitIdle(window);
        PdfReaderTestHelper::require(window.lastOperationSucceeded(), "initial open completed");
    }
    PdfReaderThumbnailList* list = window.findChild<PdfReaderThumbnailList*>();
    if (id == 28)
    {
        PdfReaderTestHelper::require(!PdfReaderTestUiHelper::page(window,0)->pixmap(), "over-budget render rejected without pixel allocation");
        PdfReaderTestHelper::require(!PdfReaderTestUiHelper::page(window,0)->text().isEmpty() && window.idle(), "render failure displays an error and has a terminal state");
        for (int32_t step = 0; step < 15; ++step)
        {
            QMetaObject::invokeMethod(&window, "zoomOut", Qt::DirectConnection);
        }
        PdfReaderTestUiHelper::waitIdle(window);
        PdfReaderTestUiHelper::color(window,0,qRgb(255,0,0));
        PdfReaderTestHelper::require(PdfReaderTestUiHelper::page(window,0)->width() == 52, "new small view renders after prior size-limit failure");
        window.close();
        PdfReaderTestUiHelper::waitIdle(window);
        return;
    }
    if (id == 27)
    {
        window.close();
        PdfReaderTestUiHelper::waitIdle(window);
#ifdef Q_OS_WIN
        std::shared_ptr<PdfReaderCoreCContext> core = PdfReaderTestHelper::core(input);
        const QByteArray before = PdfReaderTestHelper::bytes(input);
        PdfReaderTestHelper::require(pdfReaderCoreMovePage(core.get(), 0, 2) == 0, "reorder before locked main save");
        const std::wstring path = input.toStdWString();
        HANDLE raw = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        PdfReaderTestHelper::require(raw != INVALID_HANDLE_VALUE, "hold independent handle denying delete/replace");
        std::unique_ptr<void, decltype(&CloseHandle)> lock(raw, &CloseHandle);
        PdfReaderTestHelper::require(pdfReaderCoreSaveToMain(core.get()) == PdfReaderCoreCResultSaveFailed, "locked main replace returns failure");
        PdfReaderTestHelper::require(PdfReaderTestHelper::bytes(input) == before, "failed main replacement preserves original bytes");
        PdfReaderTestHelper::widths(core.get(), QList<int>() << 300 << 400 << 200);
        char savedPath[1024] = {};
        pdfReaderCoreGetFilePath(core.get(), savedPath, sizeof(savedPath));
        PdfReaderTestHelper::require(QString::fromUtf8(savedPath) == input, "recovery keeps original save destination");
        lock.reset();
        PdfReaderTestHelper::require(pdfReaderCoreSaveToMain(core.get()) == 0, "retry after releasing conflict succeeds");
        core.reset();
        PdfReaderTestHelper::widths(PdfReaderTestHelper::core(input).get(), QList<int>() << 300 << 400 << 200);
#else
        throw std::runtime_error("case 27 requires Windows file sharing conflict; not validated on this platform");
#endif
        return;
    }
    if (id == 26)
    {
        list->setCurrentRow(1);
        PdfReaderTestUiHelper::waitIdle(window);
        const QByteArray original = PdfReaderTestHelper::bytes(input);
        QTimer responses;
        QObject::connect(&responses, &QTimer::timeout, []() { PdfReaderTestUiHelper::answerDialog(true); });
        responses.start(10);
        PdfReaderTestHelper::require(window.openFile(directory + "/missing.pdf"), "failing open accepted asynchronously");
        PdfReaderTestUiHelper::waitIdle(window);
        responses.stop();
        PdfReaderTestHelper::require(!window.lastOperationSucceeded() && list->count() == 3 && list->currentRow() == 1,
            "failure terminal keeps old document and selected page");
        PdfReaderTestHelper::require(PdfReaderTestHelper::bytes(input) == original, "failed open does not modify original bytes");
        const QString blocked = directory + "/blocked.pdf";
        PdfReaderTestHelper::require(QDir().mkpath(blocked), "output conflict prepared");
        // C entry-point failures after another error must not expose that old error text.
        window.close();
        PdfReaderTestUiHelper::waitIdle(window);
        std::shared_ptr<PdfReaderCoreCContext> core = PdfReaderTestHelper::core(input);
        PdfReaderTestHelper::require(pdfReaderCoreSaveTo(core.get(), blocked.toUtf8().constData()) != 0, "save conflict fails");
        PdfReaderTestHelper::require(pdfReaderCoreGetPageInfo(core.get(), 0, nullptr) == PdfReaderCoreCResultInvalidParam, "null output rejects");
        char error[512] = {};
        pdfReaderCoreGetLastError(core.get(), error, sizeof(error));
        PdfReaderTestHelper::require(std::string(error).find("replace") == std::string::npos, "invalid argument does not reuse prior save error");
        return;
    }
    PdfReader other;
    if (id == 25)
    {
        other.show();
        PdfReaderTestHelper::require(other.openFile(input), "second window accepted");
        PdfReaderTestUiHelper::waitIdle(other);
    }
    PdfReaderTestWorkerGate gate;
    PdfReaderTestUiHelper::waitUntil([&]() { return gate.m_entered->load(); }, "controlled worker entered");
    int pulses = 0;
    QTimer heartbeat;
    QObject::connect(&heartbeat, &QTimer::timeout, [&]() { ++pulses; });
    heartbeat.start(5);
    if (id == 23)
    {
        QElapsedTimer elapsed;
        elapsed.start();
        PdfReaderTestHelper::require(window.openFile(input), "first request accepted");
        PdfReaderTestHelper::require(elapsed.elapsed() < 250, "open submission does not wait for blocked core");
        PdfReaderTestHelper::require(!window.openFile(input), "second business request explicitly rejected");
        PdfReaderTestUiHelper::waitUntil([&]() { return pulses >= 3; }, "GUI heartbeat while core blocked");
        PdfReaderTestHelper::require(identities.isEmpty() && !window.idle() && list->count() == 0, "no premature success or page snapshot");
        gate.release();
        PdfReaderTestUiHelper::waitIdle(window);
        PdfReaderTestHelper::require(identities.size() == 1 && identities.first() != 0 && successes.first() && list->count() == 3,
            "exactly one actual completion for accepted request");
        PdfReaderTestUiHelper::color(window, 0, qRgb(255,0,0));
    }
    else if (id == 24)
    {
        QMetaObject::invokeMethod(&window, "zoomIn", Qt::DirectConnection);
        QApplication::processEvents();
        QMetaObject::invokeMethod(&window, "zoomIn", Qt::DirectConnection);
        QApplication::processEvents();
        const QString second = PdfReaderTestHelper::fixture(directory, "second.pdf", 1);
        QByteArray blue = PdfReaderTestHelper::bytes(second);
        blue.replace("1 0 0 rg", "0 0 1 rg");
        QFile blueFile(second);
        PdfReaderTestHelper::require(blueFile.open(QIODevice::WriteOnly) && blueFile.write(blue) == blue.size(), "independent blue replacement fixture");
        blueFile.close();
        PdfReaderTestHelper::require(window.openFile(second), "document replacement accepted with previews queued");
        PdfReaderTestUiHelper::waitUntil([&]() { return pulses >= 3; }, "GUI responds during stale previews");
        gate.release();
        PdfReaderTestUiHelper::waitIdle(window);
        PdfReaderTestHelper::require(list->count() == 1 && window.windowTitle().endsWith("second.pdf"), "new document owns final view");
        PdfReaderTestHelper::require(PdfReaderTestUiHelper::page(window,0)->width() == 222, "latest zoom preserved after stale requests");
        PdfReaderTestUiHelper::color(window,0,qRgb(0,0,255));
        PdfReaderTestHelper::require(identities.size() == 2 && identities[1] > identities[0] && successes.last(), "replacement has distinct successful request identity");
    }
    else if (id == 25)
    {
        QMetaObject::invokeMethod(&window, "zoomIn", Qt::DirectConnection);
        QApplication::processEvents();
        window.close();
        window.close();
        PdfReaderTestUiHelper::waitUntil([&]() { return pulses >= 3; }, "GUI heartbeat during asynchronous close");
        PdfReaderTestHelper::require(window.isVisible() && !window.idle(), "close waits for Core release without blocking GUI");
        gate.release();
        PdfReaderTestUiHelper::waitIdle(window);
        PdfReaderTestHelper::require(!window.isVisible(), "close completes after resource release");
        PdfReaderTestHelper::require(other.isVisible() && other.findChild<PdfReaderThumbnailList*>()->count() == 3, "independent window preserved");
        PdfReaderTestUiHelper::color(other,2,qRgb(0,0,255));
    }
    heartbeat.stop();
    PdfReaderTestHelper::require(!gate.m_timedOut->load(), "controlled task released before budget");
    window.grab().save(directory + "/async-final.png");
    window.close();
    other.close();
    PdfReaderTestUiHelper::waitIdle(window);
    PdfReaderTestUiHelper::waitIdle(other);
}