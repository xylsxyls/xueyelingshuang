#include "SplitViewerTest.h"
#include <QtWidgets/QApplication>
#include "SplitViewerUiTests.h"
#include "../../SplitViewer/SplitViewer/SplitViewerDialogSession.h"
#include <QtCore/QTimer>
#include <stdint.h>

int32_t main(int argc, char* argv[])
{
	QApplication app(argc, argv);
    SplitViewerDialogSession dialogs;
	if (app.arguments().contains(QStringLiteral("--ui-audit")))
	{
		const QStringList args=app.arguments();
        const int index=args.indexOf(QStringLiteral("--ui-audit"));
        if (index+1>=args.size()) return 2;
        int selected=0;
        foreach (const QString& arg,args)
        {
            if (arg.startsWith(QStringLiteral("--case=")))
            {
                bool ok=false;
                selected=arg.mid(7).toInt(&ok);
                if (!ok || selected<101 || selected>183) return 2;
            }
        }
        return SplitViewerRunUiTests(args.at(index+1),selected);
	}
	SplitViewerTest window;
	window.show();
	if (app.arguments().contains(QStringLiteral("--autotest")))
	{
		QTimer::singleShot(0, &window, SLOT(runTests()));
		QTimer::singleShot(150, &app, SLOT(quit()));
	}
	const int result = app.exec();
	return app.arguments().contains(QStringLiteral("--autotest")) && !window.allTestsPassed() ? 1 : result;
}