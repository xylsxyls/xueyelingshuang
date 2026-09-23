#include "SplitViewerFileAssociationHelper.h"
#include <QtCore/QSettings>
#include <QtCore/QString>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

bool SplitViewerFileAssociationHelper::registerSvFileAssociation(const QString& applicationPath)
{
#ifdef Q_OS_WIN
    QSettings extension(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\.sv"), QSettings::NativeFormat);
    extension.setValue(QStringLiteral("."), QStringLiteral("SplitViewer.sv"));
    extension.setValue(QStringLiteral("Content Type"), QStringLiteral("image/png"));
    extension.setValue(QStringLiteral("PerceivedType"), QStringLiteral("image"));
    extension.setValue(QStringLiteral("PersistentHandler/."), QStringLiteral("{098f2470-bae0-11cd-b579-08002b30bfeb}"));
    extension.setValue(QStringLiteral("ShellEx/{8895b1c6-b41f-4c1c-a562-0d564250836f}/."), QStringLiteral("{d7ac9243-9d84-4498-8192-d7ad263b63c8}"));
    extension.setValue(QStringLiteral("ShellEx/{e357fccd-a995-4576-b01f-234630154e96}/."), QStringLiteral("{C7657C4A-9F68-40fa-A4DF-96BC08EB3551}"));
    QSettings type(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\SplitViewer.sv"), QSettings::NativeFormat);
    type.setValue(QStringLiteral("."), QStringLiteral("SplitViewer configuration"));
    type.setValue(QStringLiteral("DefaultIcon/."), applicationPath + QStringLiteral(",0"));
    type.setValue(QStringLiteral("ShellEx/{8895b1c6-b41f-4c1c-a562-0d564250836f}/."), QStringLiteral("{d7ac9243-9d84-4498-8192-d7ad263b63c8}"));
    type.setValue(QStringLiteral("ShellEx/{e357fccd-a995-4576-b01f-234630154e96}/."), QStringLiteral("{C7657C4A-9F68-40fa-A4DF-96BC08EB3551}"));
    type.setValue(QStringLiteral("shell/open/command/."), QStringLiteral("\"%1\" \"%2\"").arg(applicationPath, QStringLiteral("%1")));
#ifdef Q_OS_WIN
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
#endif
    extension.sync();
    type.sync();
    return extension.status() == QSettings::NoError && type.status() == QSettings::NoError;
#else
    Q_UNUSED(applicationPath);
    return false;
#endif
}

bool SplitViewerFileAssociationHelper::registerSvThumbnailHandler(const QString& applicationPath)
{
#ifdef Q_OS_WIN
    Q_UNUSED(applicationPath);
    // 与旧版相同，.sv的PNG预览及缩略图处理器由文件关联函数注册。
    return SplitViewerFileAssociationHelper::registerSvFileAssociation(applicationPath);
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_MACX)
    Q_UNUSED(applicationPath);
    return false;
#else
    Q_UNUSED(applicationPath);
    return false;
#endif
}