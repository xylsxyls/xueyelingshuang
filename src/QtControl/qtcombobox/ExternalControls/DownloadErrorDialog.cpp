#include "DownloadErrorDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "ProgressBar.h"
#include <Windows.h>
#include "DialogManager.h"

std::wstring DownloadErrorDialog::s_fileString = L"文件名：%s";
QString DownloadErrorDialog::s_tipString = QString::fromStdWString(L"下载错误请稍后再试");

int32_t DownloadErrorDialog::popDownloadErrorDialog(int32_t& dialogId,
													const QString& fileName,
													const QString& title,
													const QString& buttonText,
													int32_t done,
													QWindow* parent,
													int32_t timeOut,
													bool isCountDownVisible)
{
	DownloadErrorDialog* dlg = new DownloadErrorDialog(fileName, title, buttonText, done);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(DOWNLOAD_ERROR_DIALOG);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	delete dlg;
	return result;
}

DownloadErrorDialog::DownloadErrorDialog(const QString& fileName,
										 const QString& title,
										 const QString& buttonText,
										 int32_t done)
{
	initForExec(340, 165);
	m_title->setText(title);
	setWindowTitle(title);
	m_title->setFontSize(12);
	m_tip = addLabel(s_tipString, QRect(100, 61, width() - 61, 20), QColor("#cdd5e1"));
	m_tip->setFontSize(12);
	m_tip->setAlignment(Qt::AlignLeft);
	m_file = addLabel(QString::fromStdWString(CStringManager::Format(s_fileString.c_str(),fileName.toStdWString().c_str())),
					  QRect(100, 80, width() - 80, 20),
					  QColor("#8592bf"));
	m_file->setFontSize(12);
	m_file->setAlignment(Qt::AlignLeft);
	m_accept = addButton(buttonText, QRect((width() - 116) / 2, 127, 116, 22), done);
	m_accept->installEventFilter(this);

	m_error = addLabel("", QRect(58, 62, 33, 33), QColor(0, 0, 0, 0));
	m_error->setBorderImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/DownloadView/DownloadErrorIcon.png", 1, 1, 1, 1);

	//m_time->move(width() - 139, height() - 36);

}