#include "FileDialog.h"
#include <QVariant>

FileDialog::FileDialog(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setProperty("class", QVariant(QString("FileDialog")));
	setOption(QFileDialog::DontUseNativeDialog, true);
	setOption(QFileDialog::DontConfirmOverwrite, true);
}
