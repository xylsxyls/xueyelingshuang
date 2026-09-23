#include "PdfReaderRegression.h"
#include "PdfReaderTestHelper.h"
#include "PdfReaderTestUiHelper.h"
#include "PdfReaderConfigurationTests.h"
#include "PdfReaderDialogTests.h"
#include "../../PdfReader/PdfReader/PdfReader.h"

#define PDFREADERCORE_USE_C_API
#include "PdfReaderCore/PdfReaderCoreAPI.h"

#include <QEventLoop>
#include <QKeyEvent>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QImage>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QAbstractButton>
#include <QInputDialog>
#include <QLineEdit>
#include <QSet>
#include <QMap>
#include <functional>
#include <memory>
#include <stdexcept>

int PdfReaderRegression::run(const QString& selection, const QString& reportRoot)
{
    const QMap<int, QString> names = {
        {1, QStringLiteral("缩略图始终单列")}, {2, QStringLiteral("短按移动不拖拽不双选")},
        {3, QStringLiteral("长按重排及正文颜色一致")}, {4, QStringLiteral("拖动取消与原位置不变")},
        {5, QStringLiteral("缩略图比例和独立缩放")}, {6, QStringLiteral("正文点击选择与缩放边界")},
        {7, QStringLiteral("前后插入选择与顺序")}, {8, QStringLiteral("Core中文路径覆盖保存及重开")},
        {9, QStringLiteral("分页命名和冲突前置检查")}, {10, QStringLiteral("范围导出与非法输入隔离")},
        {11, QStringLiteral("多页可见渲染与边缘拖动滚动")}, {12, QStringLiteral("失败打开与覆盖取消保留状态")},
        {13, QStringLiteral("保存失败保留原件和恢复文件")}, {14, QStringLiteral("范围先验证再选择输出")},
        {15, QStringLiteral("Core配置校验快照与实例隔离")}, {16, QStringLiteral("桌面配置问号按钮与图标")},
        {19, QStringLiteral("DialogManager生命周期与QtControls控件")}
    };
    QSet<int> ids;
    if (selection.isEmpty()) { foreach (int id, names.keys()) { ids.insert(id); } }
    else
    {
        foreach (const QString& token, selection.split(','))
        {
            const QStringList range = token.split('-');
            bool firstOk = false, lastOk = false;
            const int first = range.first().toInt(&firstOk);
            const int last = range.last().toInt(&lastOk);
            if (range.size() > 2 || !firstOk || !lastOk || first < 1 || last < first || last > names.lastKey()) { return 2; }
            for (int id = first; id <= last; ++id)
            {
                if (!names.contains(id)) return 2;
                ids.insert(id);
            }
        }
    }
    if (ids.isEmpty()) { return 2; }
    const QString root = reportRoot.isEmpty() ? QApplication::applicationDirPath() + "/reports" : reportRoot;
    const QString batch = root + "/" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz");
    if (!QDir().mkpath(batch)) { return 3; }
    QFile report(batch + "/results.txt");
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text)) { return 3; }
    QTextStream log(&report);
    log.setCodec("UTF-8");
    log << "PdfReader Windows x64 Release regression\nEXE=" << QApplication::applicationFilePath()
        << "\nSHA256=" << QCryptographicHash::hash(PdfReaderTestHelper::bytes(QApplication::applicationFilePath()), QCryptographicHash::Sha256).toHex() << "\n";
    int passed = 0, failed = 0;
    foreach (int id, names.keys())
    {
        if (!ids.contains(id)) { continue; }
        QElapsedTimer elapsed;
        elapsed.start();
        log << "RUN " << id << " " << names[id] << "\n";
        log.flush();
        try
        {
            const QString dir = batch + "/case" + QString::number(id);
            PdfReaderTestHelper::require(QDir().mkpath(dir), "case directory");
            const QString input = PdfReaderTestHelper::fixture(dir, QStringLiteral("中文样本.pdf"), id == 11 ? 120 : 3);
            if (id == 19)
            {
                PdfReaderDialogTests::run(input, dir);
            }
            else if (id >= 15)
            {
                PdfReaderConfigurationTests::run(id, input, dir);
            }
            else if ((id >= 8 && id <= 10) || id == 13)
            {
                std::shared_ptr<PdfReaderCoreCContext> core = PdfReaderTestHelper::core(input);
                if (id == 13)
                {
                    const QByteArray before = PdfReaderTestHelper::bytes(input);
                    PdfReaderTestHelper::require(pdfReaderCoreMovePage(core.get(), 0, 2) == 0, "move before failed export");
                    const QString destination = dir + "/blocked.pdf";
                    PdfReaderTestHelper::require(QDir().mkpath(destination), "prepare directory conflict");
                    PdfReaderTestHelper::require(pdfReaderCoreSaveTo(core.get(), destination.toUtf8().constData()) == PdfReaderCoreCResultSaveFailed, "replace must fail on directory");
                    PdfReaderTestHelper::require(PdfReaderTestHelper::bytes(input) == before && QFileInfo(destination).isDir(), "failed replace preserves original and destination");
                    PdfReaderTestHelper::widths(core.get(), QList<int>() << 300 << 400 << 200);
                    const QStringList recovered = QDir(dir).entryList(QStringList() << "blocked.pdf.pdfreader-*.tmp", QDir::Files);
                    PdfReaderTestHelper::require(recovered.size() == 1, "one recoverable output retained");
                    PdfReaderTestHelper::widths(PdfReaderTestHelper::core(dir + "/" + recovered.first()).get(), QList<int>() << 300 << 400 << 200);
                }
                if (id == 8)
                {
                    PdfReaderTestHelper::require(pdfReaderCoreMovePage(core.get(), 0, 2) == 0, "core move");
                    PdfReaderTestHelper::require(pdfReaderCoreSaveToMain(core.get()) == 0, "replace existing Unicode source");
                    PdfReaderTestHelper::widths(core.get(), QList<int>() << 300 << 400 << 200);
                    PdfReaderTestHelper::require(pdfReaderCoreSaveToMain(core.get()) == 0, "repeat save");
                    core = PdfReaderTestHelper::core(input);
                    PdfReaderTestHelper::widths(core.get(), QList<int>() << 300 << 400 << 200);
                    const QString output = PdfReaderTestHelper::fixture(dir, "existing.pdf", 1);
                    PdfReaderTestHelper::require(pdfReaderCoreSaveTo(core.get(), output.toUtf8().constData()) == 0, "save as replace existing");
                    PdfReaderTestHelper::widths(PdfReaderTestHelper::core(output).get(), QList<int>() << 300 << 400 << 200);
                }
                if (id == 9)
                {
                    const QString conflict = PdfReaderTestHelper::fixture(dir, QStringLiteral("分页_003.pdf"), 1);
                    const QByteArray before = PdfReaderTestHelper::bytes(conflict);
                    const QByteArray prefix = QStringLiteral("分页").toUtf8();
                    PdfReaderTestHelper::require(pdfReaderCoreSaveEachPage(core.get(), dir.toUtf8().constData(), prefix.constData()) == PdfReaderCoreCResultFileExists, "conflict result");
                    PdfReaderTestHelper::require(!QFile::exists(dir + QStringLiteral("/分页_001.pdf")) && PdfReaderTestHelper::bytes(conflict) == before, "conflict changes no files");
                    PdfReaderTestHelper::require(pdfReaderCoreSaveEachPageEx(core.get(), dir.toUtf8().constData(), prefix.constData(), 1) == 0, "explicit overwrite export");
                    PdfReaderTestHelper::widths(PdfReaderTestHelper::core(dir + QStringLiteral("/分页_001.pdf")).get(), QList<int>() << 200);
                    PdfReaderTestHelper::widths(PdfReaderTestHelper::core(conflict).get(), QList<int>() << 400);
                }
                if (id == 10)
                {
                    const QString output = dir + "/range.pdf";
                    PdfReaderTestHelper::require(pdfReaderCoreSavePageRange(core.get(), "3,1-2", output.toUtf8().constData()) == 0, "range save");
                    // 原工程按当前页面顺序输出选集，重复范围合并。
                    PdfReaderTestHelper::widths(PdfReaderTestHelper::core(output).get(), QList<int>() << 200 << 300 << 400);
                    const QByteArray before = PdfReaderTestHelper::bytes(output);
                    PdfReaderTestHelper::require(pdfReaderCoreSavePageRange(core.get(), "4", output.toUtf8().constData()) != 0, "reject out of range");
                    PdfReaderTestHelper::require(PdfReaderTestHelper::bytes(output) == before, "invalid range preserves existing file");
                    PdfReaderTestHelper::require(pdfReaderCoreInsertDocument(core.get(), input.toUtf8().constData(), "", 1) == 0, "insert source");
                    PdfReaderTestHelper::widths(core.get(), QList<int>() << 200 << 200 << 300 << 400 << 300 << 400);
                }
            }
            else
            {
                PdfReader window;
                window.show();
                PdfReaderTestHelper::require(window.openFile(input), "UI open fixture");
                PdfReaderTestUiHelper::wait(80);
                PdfReaderThumbnailList* list = window.findChild<PdfReaderThumbnailList*>();
                QScrollArea* scroll = window.findChild<QScrollArea*>(QStringLiteral("documentScroll"));
                PdfReaderTestHelper::require(list && scroll && list->count() >= 3, "UI children and count");
                if (id == 1)
                {
                    foreach (int width, QList<int>() << 190 << 270 << 600)
                    {
                        window.findChild<QSplitter*>()->setSizes(QList<int>() << width << 700);
                        PdfReaderTestUiHelper::wait(50);
                        QRect previous = list->visualItemRect(list->item(0));
                        if (qAbs(previous.center().x() - list->viewport()->rect().center().x()) > 2)
                        {
                            const QByteArray message = QString("thumbnail item follows resized sidebar center: item=%1,%2,%3,%4 viewport=%5,%6,%7,%8")
                                .arg(previous.x()).arg(previous.y()).arg(previous.width()).arg(previous.height())
                                .arg(list->viewport()->rect().x()).arg(list->viewport()->rect().y())
                                .arg(list->viewport()->rect().width()).arg(list->viewport()->rect().height()).toLocal8Bit();
                            throw std::runtime_error(message.constData());
                        }
                        for (int i = 1; i < list->count(); ++i)
                        {
                            const QRect next = list->visualItemRect(list->item(i));
                            PdfReaderTestHelper::require(next.top() > previous.bottom() && next.left() == previous.left(), "single column at every splitter width");
                            previous = next;
                        }
                        const QImage thumbnail = list->viewport()->grab().toImage();
                        int minRed = thumbnail.width();
                        int maxRed = -1;
                        for (int y = 0; y < thumbnail.height(); ++y)
                        {
                            for (int x = 0; x < thumbnail.width(); ++x)
                            {
                                if ((thumbnail.pixel(x, y) & 0xffffff) == 0xff0000)
                                {
                                    minRed = qMin(minRed, x);
                                    maxRed = qMax(maxRed, x);
                                }
                            }
                        }
                        PdfReaderTestHelper::require(maxRed >= minRed, "thumbnail page pixels rendered");
                        PdfReaderTestHelper::require(qAbs((minRed + maxRed) / 2 - list->viewport()->rect().center().x()) <= 2,
                                "thumbnail page pixels follow resized sidebar center");
                    }
                }
                if (id == 2 || id == 3 || id == 4)
                {
                    const QPoint source = list->visualItemRect(list->item(0)).center();
                    const QRect target = list->visualItemRect(list->item(2));
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonPress, source);
                    if (id != 2) { PdfReaderTestUiHelper::wait(1050); }
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseMove, QPoint(target.center().x(), target.bottom() - 3));
                    PdfReaderTestHelper::require(list->selectedItems().size() == 1 && list->currentRow() == 0, "drag never selects hovered row");
                    if (id == 4) { PdfReaderTestUiHelper::escape(list); }
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonRelease, QPoint(target.center().x(), target.bottom() - 3));
                    PdfReaderTestUiHelper::wait(80);
                    PdfReaderTestHelper::require(list->count() == 3 && list->selectedItems().size() == 1, "no duplicate/drop loss");
                    PdfReaderTestHelper::require(list->currentRow() == (id == 3 ? 2 : 0), "selection follows moved source only");
                    PdfReaderTestUiHelper::color(window, 0, id == 3 ? qRgb(0,255,0) : qRgb(255,0,0));
                    if (id == 3)
                    {
                        PdfReaderTestUiHelper::color(window, 2, qRgb(255,0,0));
                        for (int i=0;i<3;++i) { PdfReaderTestHelper::require(list->item(i)->text() == QString::number(i+1), "renumber all thumbnails"); }
                        const QString output = dir + "/ui-reordered.pdf";
                        PdfReaderTestUiHelper::dialogAction(window, "saveAs", dir + "/ui-reordered.PDF");
                        PdfReaderTestHelper::require(QDir(dir).entryList(QDir::Files).contains("ui-reordered.pdf", Qt::CaseSensitive), "Save As normalizes uppercase extension on disk");
                        PdfReaderTestHelper::widths(PdfReaderTestHelper::core(output).get(), QList<int>() << 300 << 400 << 200);
                        PdfReaderTestUiHelper::dialogAction(window, "saveMain", QString());
                        PdfReaderTestHelper::widths(PdfReaderTestHelper::core(input).get(), QList<int>() << 300 << 400 << 200);
                        // 再反向拖回第一位，验证向上插入间隙及保存后的继续编辑。
                        list->scrollToItem(list->item(2));
                        PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonPress, list->visualItemRect(list->item(2)).center());
                        PdfReaderTestUiHelper::wait(1050);
                        const QPoint top = list->visualItemRect(list->item(0)).topLeft() + QPoint(20, 2);
                        PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseMove, top);
                        PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonRelease, top);
                        PdfReaderTestUiHelper::wait(40);
                        PdfReaderTestHelper::require(list->currentRow() == 0 && list->selectedItems().size() == 1, "reverse move follows source");
                        PdfReaderTestUiHelper::color(window, 0, qRgb(255,0,0));
                        PdfReaderTestUiHelper::color(window, 1, qRgb(0,255,0));
                    }
                }
                if (id == 4)
                {
                    // 原位释放、横向离开视口释放、窗口失活，都不能重排。
                    for (int mode = 0; mode < 3; ++mode)
                    {
                        const QPoint start = list->visualItemRect(list->item(0)).center();
                        PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonPress, start);
                        PdfReaderTestUiHelper::wait(1050);
                        QPoint end = start;
                        if (mode == 1) { end = QPoint(list->viewport()->width() + 10, 450); }
                        if (mode == 2)
                        {
                            QEvent deactivate(QEvent::WindowDeactivate);
                            QApplication::sendEvent(list, &deactivate);
                            end = list->visualItemRect(list->item(2)).bottomLeft() + QPoint(20, -2);
                        }
                        PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseMove, end);
                        PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonRelease, end);
                        PdfReaderTestHelper::require(list->currentRow() == 0 && list->selectedItems().size() == 1, "cancel/no-op retains single source");
                        PdfReaderTestUiHelper::color(window, 0, qRgb(255,0,0));
                    }
                }
                if (id == 5)
                {
                    list->setCurrentRow(1);
                    const QSize size = list->item(1)->data(Qt::UserRole + 1).toSize();
                    const QRect before = list->visualItemRect(list->item(1));
                    PdfReaderTestHelper::require(qAbs(size.width() / 2.0 - size.height()) <= 1, "landscape thumbnail aspect ratio");
                    const QImage selected = list->item(1)->icon().pixmap(size, QIcon::Selected).toImage();
                    PdfReaderTestHelper::require(!selected.isNull() && (selected.pixel(selected.width()/2, selected.height()/2) & 0xffffff) == 0x00ff00, "selected thumbnail preserves PDF colors");
                    const QSize body = PdfReaderTestUiHelper::page(window, 1)->size();
                    PdfReaderTestUiHelper::wheel(list->viewport(), 120);
                    PdfReaderTestHelper::require(list->currentRow() == 1 && list->selectedItems().size() == 1, "thumbnail zoom keeps selection");
                    const QRect after = list->visualItemRect(list->item(1));
                    PdfReaderTestHelper::require(list->item(1)->data(Qt::UserRole + 1).toSize() == size, "thumbnail page keeps base size");
                    PdfReaderTestHelper::require(after.height() > before.height(), "thumbnail zoom only expands gray item background");
                    const QPoint gray = after.bottomLeft() + QPoint(after.width() / 2, -4);
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonPress, gray);
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonRelease, gray);
                    PdfReaderTestHelper::require(list->currentRow() == 1 && list->selectedItems().size() == 1, "gray thumbnail background selects its page");
                    PdfReaderTestHelper::require(PdfReaderTestUiHelper::page(window, 1)->size() == body, "thumbnail zoom does not change body");
                }
                if (id == 6)
                {
                    scroll->verticalScrollBar()->setValue(0);
                    const QPoint pageTwoPoint = list->visualItemRect(list->item(2)).center();
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonPress, pageTwoPoint);
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonRelease, pageTwoPoint);
                    PdfReaderTestUiHelper::wait(60);
                    const QRect selectedPage = PdfReaderTestUiHelper::page(window, 2)->rect().translated(PdfReaderTestUiHelper::page(window, 2)->mapTo(scroll->viewport(), QPoint(0, 0)));
                    PdfReaderTestHelper::require(selectedPage.intersects(scroll->viewport()->rect()), "thumbnail click shows corresponding body page");
                    PdfReaderTestUiHelper::mouse(PdfReaderTestUiHelper::page(window, 1), QEvent::MouseButtonPress, QPoint(10,10));
                    PdfReaderTestUiHelper::mouse(PdfReaderTestUiHelper::page(window, 1), QEvent::MouseButtonRelease, QPoint(10,10));
                    PdfReaderTestHelper::require(list->currentRow() == 1, "body click selects matching thumbnail");
                    PdfReaderTestUiHelper::wheel(scroll->viewport(), 120);
                    PdfReaderTestHelper::require(PdfReaderTestUiHelper::page(window, 1)->width() == 317 && list->currentRow() == 1, "body zoom step is five percent");
                    PdfReaderTestUiHelper::wheel(scroll->viewport(), 120 * 100);
                    PdfReaderTestHelper::require(PdfReaderTestUiHelper::page(window, 1)->width() == 1202, "body zoom upper boundary");
                    PdfReaderTestUiHelper::wheel(scroll->viewport(), -120 * 100);
                    PdfReaderTestHelper::require(PdfReaderTestUiHelper::page(window, 1)->width() == 77, "body zoom lower boundary");
                }
                if (id == 7)
                {
                    const QString inserted = PdfReaderTestHelper::fixture(dir, "insert.pdf", 1);
                    list->setCurrentRow(1);
                    PdfReaderTestUiHelper::dialogAction(window, "insertBefore", inserted);
                    PdfReaderTestHelper::require(list->count() == 4 && list->currentRow() == 1, "insert before selects first inserted page");
                    PdfReaderTestUiHelper::color(window, 1, qRgb(255,0,0));
                    PdfReaderTestUiHelper::color(window, 2, qRgb(0,255,0));
                    list->setCurrentRow(2);
                    PdfReaderTestUiHelper::dialogAction(window, "insertAfter", inserted);
                    PdfReaderTestHelper::require(list->count() == 5 && list->currentRow() == 3, "insert after selects first inserted page");
                    PdfReaderTestUiHelper::color(window, 3, qRgb(255,0,0));
                }
                if (id == 11)
                {
                    int cached = 0;
                    for (int i = 0; i < 120; ++i) { if (PdfReaderTestUiHelper::page(window, i)->pixmap()) { ++cached; } }
                    PdfReaderTestHelper::require(cached > 0 && cached < 10, "only visible pages allocate bitmaps");
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseButtonPress, list->visualItemRect(list->item(0)).center());
                    PdfReaderTestUiHelper::wait(1050);
                    PdfReaderTestUiHelper::mouse(list->viewport(), QEvent::MouseMove, QPoint(50, list->viewport()->height()-2));
                    PdfReaderTestUiHelper::wait(120);
                    PdfReaderTestHelper::require(list->verticalScrollBar()->value() > 0 && list->currentRow() == 0, "drag edge auto scroll retains source");
                    PdfReaderTestUiHelper::escape(list);
                    window.grab().save(dir + "/reader.png");
                }
                if (id == 12)
                {
                    list->setCurrentRow(1);
                    const QByteArray before = PdfReaderTestHelper::bytes(input);
                    PdfReaderTestUiHelper::dialogAction(window, "saveMain", QString(), false);
                    PdfReaderTestHelper::require(PdfReaderTestHelper::bytes(input) == before && list->currentRow() == 1, "cancel save preserves document/view");
                    QTimer timer;
                    QObject::connect(&timer, &QTimer::timeout, []() {
                        PdfReaderTestUiHelper::answerDialog(true);
                    });
                    timer.start(20);
                    PdfReaderTestHelper::require(!window.openFile(dir + "/missing.pdf"), "missing document rejected");
                    timer.stop();
                    PdfReaderTestHelper::require(list->currentRow() == 1 && list->count() == 3, "failed open preserves old view");
                }
                if (id == 14)
                {
                    bool inputSeen = false, errorSeen = false, fileSeen = false, cancel = false;
                    QTimer timer;
                    QObject::connect(&timer, &QTimer::timeout, [&]() {
                        if (QWidget* dialog = PdfReaderTestUiHelper::dialogView())
                        {
                            const int mode = dialog->property("dialogMode").toInt();
                            if (mode >= 3) { fileSeen = true; if (QFileDialog* files = PdfReaderTestUiHelper::fileDialog()) files->reject(); return; }
                            if (mode != 2) { errorSeen = true; PdfReaderTestUiHelper::answerDialog(true); return; }
                            inputSeen = true;
                            if (cancel) { PdfReaderTestUiHelper::answerDialog(false); }
                            else
                            {
                                dialog->findChild<QLineEdit*>(QStringLiteral("dialogInput"))->setText(QStringLiteral("4"));
                                PdfReaderTestUiHelper::answerDialog(true);
                            }
                        }
                        else if (QFileDialog* dialog = PdfReaderTestUiHelper::fileDialog())
                        {
                            fileSeen = true;
                            dialog->reject();
                        }
                        else if (QMessageBox* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
                        {
                            errorSeen = true;
                            box->accept();
                        }
                    });
                    timer.start(20);
                    PdfReaderTestHelper::require(QMetaObject::invokeMethod(&window, "savePageRange", Qt::DirectConnection), "range action exists");
                    timer.stop();
                    PdfReaderTestHelper::require(inputSeen && errorSeen && !fileSeen, "invalid range rejected before file dialog");
                    cancel = true;
                    inputSeen = errorSeen = fileSeen = false;
                    timer.start(20);
                    PdfReaderTestHelper::require(QMetaObject::invokeMethod(&window, "savePageRange", Qt::DirectConnection), "range cancel action exists");
                    timer.stop();
                    PdfReaderTestHelper::require(inputSeen && !errorSeen && !fileSeen, "cancel range does not reach output dialog");
                }
                window.grab().save(dir + "/reader.png");
                window.close();
            }
            ++passed;
            log << "PASS " << id << " " << elapsed.elapsed() << "ms\n";
        }
        catch (const std::exception& error)
        {
            ++failed;
            log << "FAIL " << id << " " << error.what() << " " << elapsed.elapsed() << "ms\n";
        }
        log.flush();
    }
    log << "TOTAL=" << ids.size() << " PASS=" << passed << " FAIL=" << failed << "\n";
    log.flush();
    return failed ? 1 : 0;
}
