#include "DownloadDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "ProgressBar.h"
#include <Windows.h>
#include "DownloadErrorDialog.h"
#include "DialogManager.h"

std::wstring DownloadDialog::s_fileString = L"文件名：%s";
QString DownloadDialog::s_slowString = QString::fromStdWString(L"下载太慢？点击");
QString DownloadDialog::s_handString = QString::fromStdWString(L"手动下载");

int32_t DownloadDialog::popDownloadDialog(int32_t& dialogId,
										  const QString& title,
										  const QString& fileName,
										  const QString& tip,
										  const QString& buttonText,
										  int32_t done,
										  QWindow* parent,
										  int32_t timeOut,
										  bool isCountDownVisible)
{
	DownloadDialog* dlg = new DownloadDialog(title, fileName, tip, buttonText, done);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(DOWNLOAD_DIALOG);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	delete dlg;
	return result;
}

DownloadDialog::DownloadDialog(const QString& title,
							   const QString& fileName,
							   const QString& tip,
							   const QString& buttonText,
							   int32_t done)
{
	initForExec(411, 165);
	m_title->setText(title);
	setWindowTitle(title);
	m_title->setFontSize(12);
	m_tip = addLabel(tip, QRect(43, 33, width() - 43 * 2, 47), QColor("#cdd5e1"));
	m_tip->setFontSize(12);
	m_file = addLabel(QString::fromStdWString(CStringManager::Format(s_fileString.c_str(),fileName.toStdWString().c_str())),
					  QRect(43, 50, width() - 43 * 2, 63),
					  QColor("#8592bf"));
	m_file->setFontSize(12);
	m_persent = addLabel("0%", QRect(width() - 41, 108, 27, 13), QColor("#96abff"));
	m_persent->setFontSize(10);
	m_persent->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	m_time->move(width() - 139, height() - 36);
	m_downloadSlow = addLabel(s_slowString, QRect(13, 137, 83, 18),QColor("#8491bd"));
	m_downloadSlow->setFontSize(12);
	m_hand = new COriginalButton(this);
	m_hand->installEventFilter(this);
	m_hand->setGeometry(100, 137, 48, 18);
	m_hand->setFontFace(CGeneralStyle::instance()->font().family());
	m_hand->setFontSize(12);
	m_hand->setFontColor(QColor("#465695"), QColor("#c9d3fc"), QColor("#313c6b"), QColor("#465695"));
	m_hand->setText(s_handString);
	m_hand->setUnderline(true);

	QObject::connect(this, SIGNAL(keyboardAccept()), this, SLOT(tipAccept()));

	progressBar = new ProgressBar(this);
	progressBar->setOrientation(Qt::Horizontal);
	//当前进度
	setRate(80);
	progressBar->setRange(0, 100);
	progressBar->setGeometry(13, 101, width() - 13 * 2, 4);
	progressBar->setBorderWidth(0);
	progressBar->setBorderRadius(2);
	progressBar->setBackgroundColor(QColor(38, 45, 73, 255));
	progressBar->setItemBorderWidth(0);
	progressBar->setItemBorderRadius(2);
	progressBar->setItemBackgroundColor(QColor(150, 171, 255, 255));
	progressBar->setFormat("");
	QObject::connect(this, SIGNAL(rateChanged(int)), progressBar, SLOT(setValue(int)));
	QObject::connect(this, SIGNAL(persentChanged(const QString&)), m_persent, SLOT(setText(const QString&)));
	QObject::connect(this, SIGNAL(downloadComplete()), this, SLOT(reject()));
	QObject::connect(this, SIGNAL(showError()), this, SLOT(showDownloadErrorDialog()), Qt::UniqueConnection);
}

DownloadDialog::~DownloadDialog()
{
	int x = 3;
}

void DownloadDialog::setRate(int32_t persent)
{
	emit rateChanged(persent);
	emit persentChanged(QString::fromStdWString(CStringManager::Format(L"%d%%", persent)));
}

void DownloadDialog::complete()
{
	emit downloadComplete();
}

void DownloadDialog::error()
{
	emit showError();
}

void DownloadDialog::showDownloadErrorDialog()
{
	setVisible(false);
	int32_t dialogId = 0;
	std::wstring fileName = m_file->text().toStdWString();
	CStringManager::Replace(fileName, L"文件名：", L"");
	DownloadErrorDialog::popDownloadErrorDialog(dialogId,
												QString::fromStdWString(fileName),
												m_title->text(),
												QString::fromStdWString(L"确定"),
												ACCEPT_BUTTON,
												windowHandle());
	//setVisible(true);
	//reject();
	return;
	//m_persent->setText("123123123");
	//setVisible(false);
	QDialog::done(0);
	//int32_t dialogId = 0;
	//std::wstring fileName = m_file->text().toStdWString();
	//CStringManager::Replace(fileName, L"文件名：", L"");
	//DownloadErrorDialog::popDownloadErrorDialog(dialogId,
	//	QString::fromStdWString(fileName),
	//	m_title->text(),
	//	QString::fromStdWString(L"确定"),
	//	ACCEPT_BUTTON,
	//	windowHandle());
	//m_persent->setText("123123123");
	//setVisible(true);
	//reject();
}

void DownloadDialog::downloadAccept()
{
	m_hand->setFocus();
	m_hand->click();
}
