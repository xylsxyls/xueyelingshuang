#include "PdfReaderConfigurationTests.h"
#include "PdfReaderTestHelper.h"
#include "PdfReaderTestUiHelper.h"
#include "../../PdfReader/PdfReader/PdfReader.h"
#include "CStringManager/CStringManagerAPI.h"
#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QToolButton>
#include <QMessageBox>
#include <memory>
#include <stdexcept>

void PdfReaderConfigurationTests::run(int id, const QString& input, const QString& directory)
{
    if (id == 15)
    {
        PdfReaderTestHelper::require(PdfReaderCppConfigSmoke(),"C++ API configuration and lifecycle");
        PdfReaderCoreCConfig config;
        pdfReaderCoreDefaultConfig(&config);
        PdfReaderTestHelper::require(config.exportNumberWidth == 3 && config.maxRenderPixels == 64000000, "C default configuration");
        std::shared_ptr<PdfReaderCoreCContext> core(pdfReaderCoreCreate(),pdfReaderCoreDestroy);
        config.exportNumberWidth = 0;
        PdfReaderTestHelper::require(pdfReaderCoreInitWithConfig(core.get(),&config) == PdfReaderCoreCResultInvalidParam,"invalid configuration rejected");
        PdfReaderTestHelper::require(pdfReaderCoreOpen(core.get(),input.toUtf8().constData(),"") == PdfReaderCoreCResultNotInit,"failed init leaves engine closed");
        config.exportNumberWidth = 2;
        config.maxRenderPixels = 400;
        PdfReaderTestHelper::require(pdfReaderCoreInitWithConfig(core.get(),&config) == 0,"custom configuration init");
        PdfReaderTestHelper::require(pdfReaderCoreOpen(core.get(),input.toUtf8().constData(),"") == 0,"custom core open");
        config.maxRenderPixels = 10000;
        PdfReaderTestHelper::require(pdfReaderCoreInitWithConfig(core.get(),&config) != 0,"running configuration cannot be replaced");
        unsigned char pixels[1600];
        int32_t w=0,h=0,stride=0; size_t bytes=0;
        PdfReaderTestHelper::require(pdfReaderCoreRenderPage(core.get(),0,20,20,pixels,sizeof(pixels),&w,&h,&stride,&bytes) == 0,"configured render limit inclusive");
        PdfReaderTestHelper::require(pdfReaderCoreRenderPage(core.get(),0,21,20,pixels,sizeof(pixels),&w,&h,&stride,&bytes) != 0,"configuration copied and render over limit rejected");
        PdfReaderTestHelper::require(pdfReaderCoreSaveEachPage(core.get(),directory.toUtf8().constData(),"custom") == 0,"custom export");
        const QStringList files=QDir(directory).entryList(QDir::Files);
        PdfReaderTestHelper::require(files.contains("custom_01.pdf",Qt::CaseSensitive) && !files.contains("custom_01.PDF",Qt::CaseSensitive),"configured padding and exact lowercase extension");
        std::shared_ptr<PdfReaderCoreCContext> other(pdfReaderCoreCreate(),pdfReaderCoreDestroy);
        PdfReaderTestHelper::require(pdfReaderCoreInit(other.get()) == 0 && pdfReaderCoreOpen(other.get(),input.toUtf8().constData(),"") == 0,"independent default instance");
        PdfReaderTestHelper::require(pdfReaderCoreSaveEachPage(other.get(),directory.toUtf8().constData(),"default") == 0 && QDir(directory).entryList(QDir::Files).contains("default_001.pdf",Qt::CaseSensitive),"default instance unaffected");
        pdfReaderCoreUninit(core.get());
        PdfReaderTestHelper::require(pdfReaderCoreInitWithConfig(core.get(),&config) == 0,"configuration can change after uninit");
        return;
    }
    if (id == 16)
    {
        Config config;
        PdfReaderTestHelper::require(config.log.m_fileId == 0 && config.log.m_path.empty(),
            "default logging uses the executable directory and default file id");
        PdfReaderTestHelper::require(config.log.m_maxFileBytes == 20LL * 1024 * 1024 &&
            config.log.m_maxFileCount == 8 && config.log.m_checkFileSizeInterval == 1,
            "default rolling log policy matches LumaPlayer");
        PdfReaderTestHelper::require(config.log.m_archiveOldLog && !config.log.m_outputConsole,
            "archive old logs without console output");
        config.windowSize=QSize(1040,700);
        config.initialZoom=0.5; config.minimumZoom=0.4; config.maximumZoom=0.6; config.zoomStep=0.2;
        config.thumbnailWidth=90; config.sidebarWidth=320;
        PdfReader window(nullptr,config);
        window.show();
        PdfReaderTestHelper::require(window.openFile(input),"configured window open");
        PdfReaderTestUiHelper::wait(60);
        PdfReaderTestHelper::require(window.size()==QSize(1040,700),"configured initial window size");
        PdfReaderTestHelper::require(!window.windowIcon().isNull(),"runtime application icon");
        const QList<QLabel*> pages=window.findChildren<QLabel*>(QStringLiteral("pageLabel"));
        PdfReaderTestHelper::require(!pages.isEmpty() && pages.first()->width()==102,"custom initial zoom");
        QMetaObject::invokeMethod(&window,"zoomIn",Qt::DirectConnection);
        PdfReaderTestHelper::require(window.findChildren<QLabel*>(QStringLiteral("pageLabel")).first()->width()==122,"custom maximum zoom");
        QMetaObject::invokeMethod(&window,"zoomOut",Qt::DirectConnection);
        QMetaObject::invokeMethod(&window,"zoomOut",Qt::DirectConnection);
        PdfReaderTestHelper::require(window.findChildren<QLabel*>(QStringLiteral("pageLabel")).first()->width()==82,"custom minimum zoom");
        QToolBar* toolbar=window.findChild<QToolBar*>();
        QAction* about=window.findChild<QAction*>(QStringLiteral("aboutAction"));
        PdfReaderTestHelper::require(about && about->text().isEmpty() && !about->icon().isNull(),"question icon replaces Help text");
        QWidget* button=toolbar->widgetForAction(about);
        PdfReaderTestHelper::require(button && button->geometry().right()>toolbar->width()-80,"question button at right edge");
        bool shown=false;
        QTimer timer;
        QObject::connect(&timer,&QTimer::timeout,[&shown]() {
            if (QWidget* box=PdfReaderTestUiHelper::dialogView())
            {
                QLabel* message=box->findChild<QLabel*>(QStringLiteral("dialogMessage"));
                shown=message && !message->text().isEmpty(); PdfReaderTestUiHelper::answerDialog(true);
            }
        });
        timer.start(20); about->trigger(); timer.stop();
        PdfReaderTestHelper::require(shown,"about button opens populated dialog");
        window.grab().save(directory+"/configured-window.png");
        return;
    }
    throw std::runtime_error("unsupported configuration test id/platform");
}