#include "PdfReaderTest.h"
#include "PdfReaderRegression.h"
#include "../../PdfReader/PdfReader/PdfReaderDialogRuntime.h"
#include <QtWidgets/QApplication>
#include <stdint.h>

int32_t main(int argc, char* argv[])
{
	QApplication app(argc, argv);
    PdfReaderDialogRuntime dialogs;
    const QStringList args = app.arguments();
    if (args.contains(QStringLiteral("--regression")))
    {
        const int at = args.indexOf(QStringLiteral("--cases"));
        const int report = args.indexOf(QStringLiteral("--reports"));
        return PdfReaderRegression::run(at >= 0 ? args.value(at + 1) : QString(),
            report >= 0 ? args.value(report + 1) : QString());
    }
	PdfReaderTest window;
	window.show();
	return app.exec();
}
