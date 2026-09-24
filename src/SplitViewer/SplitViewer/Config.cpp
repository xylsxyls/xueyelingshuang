#include "Config.h"

QString Config::windowStyle()
{
    return QString::fromLatin1("QMainWindow { background:#eeeeee; } "
        "QToolBar { background:#f6f6f6; border:0; spacing:6px; padding:6px; } "
        "QToolButton { color:#202020; background:#f6f6f6; border:1px solid #bebebe; border-radius:2px; padding:5px 10px; } "
        "QToolButton:hover { background:#d2e2f6; border-color:#4e84be; } "
        "QStatusBar { color:#222222; background:#f6f6f6; } "
        "QLabel#zoomTip { background:#fffcd6; color:#222222; border:1px solid #5c5c46; padding:5px; } "
        "QMenu { background:#f6f6f6; color:#202020; border:1px solid #bebebe; } "
        "QMenu::item { padding:6px 20px; } QMenu::item:selected { background:#d2e2f6; } "
        "QMenu::item:disabled { color:#909090; }");
}

QString Config::fileDialogStyle()
{
    return QString::fromLatin1("QWidget#splitViewerFileView { background:#f6f6f6; color:#202020; } "
        "QLabel#fileDialogTitle { font-weight:bold; padding:4px; } "
        "QPushButton { padding:5px 14px; } QLineEdit { padding:4px; }");
}

QString Config::aboutDialogStyle()
{
    return QString::fromLatin1(
        "QWidget#splitViewerAboutView { background:#f7faff; color:#162b48; border:1px solid #cad8e8; } "
        "QLabel#aboutLogo { background:#ffffff; border:1px solid #dce9f5; border-radius:18px; padding:5px; } "
        "QLabel#aboutTitle { color:#102a4a; font-family:'Microsoft YaHei'; font-size:24px; font-weight:bold; } "
        "QLabel#aboutVersion { color:#7187a3; font-family:'Microsoft YaHei'; font-size:13px; } "
        "QLabel#aboutSeparator { background:#dce6f0; } "
        "QLabel#aboutMessage { color:#263f5c; font-family:'Microsoft YaHei'; font-size:14px; } "
        "QPushButton#aboutCloseButton { color:#ffffff; background:#3f9fca; border:1px solid #328db7; border-radius:9px; padding:8px 30px; font-family:'Microsoft YaHei'; font-weight:bold; } "
        "QPushButton#aboutCloseButton:hover { background:#52add5; border-color:#3f9fca; } "
        "QPushButton#aboutCloseButton:pressed { background:#2f83aa; border-color:#287595; } ");
}

QSize Config::fileDialogSize()
{
    return QSize(760, 520);
}

QSize Config::aboutDialogSize()
{
    return QSize(520, 340);
}