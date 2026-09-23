#pragma once
#include <QtCore/QPoint>
#include <QtGui/qwindowdefs.h>
#ifdef Q_OS_WIN
/** Mouse click held until it can be distinguished from a host double click */
struct SplitViewerNativeClick
{
    WId target;
    QPoint screenPoint;
    QPoint clientPoint;
    quint32 modifiers;
    bool released;
    quint64 generation;
};
#endif