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

QSize Config::fileDialogSize()
{
    return QSize(760, 520);
}