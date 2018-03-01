#include "DownloadOperateDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "ProgressBar.h"
#include <Windows.h>
#include "DownloadErrorDialog.h"
#include "DialogManager.h"

std::wstring DownloadOperateDialog::s_fileString = L"文件名：%s";
QString DownloadOperateDialog::s_slowString = QString::fromStdWString(L"下载太慢？点击");
QString DownloadOperateDialog::s_handString = QString::fromStdWString(L"手动下载");

int32_t DownloadOperateDialog::popDownloadOperateDialog(int32_t& dialogId,
														const QString& title,
														const QString& fileName,
														const QString& buttonText,
														int32_t done,
														QWindow* parent)
{
	DownloadOperateDialog* dlg = new DownloadOperateDialog(title, fileName, buttonText, done);
	dlg->setParentWindow(parent);
	return dlg->exec(dialogId);
}

DownloadOperateDialog::DownloadOperateDialog(const QString& title,
											 const QString& fileName,
											 const QString& buttonText,
											 int32_t done)
{
	initForExec(420, 180);
	m_title->setText(title);
	setWindowTitle(title);
	m_title->setFontSize(12);

	m_separator->setGeometry(16, 40, width() - 16 * 2, 2);

	m_file = addLabel(fileName, QRect(16, 63, width() / 2 - 16, 30), QColor("#8592bf"));
	m_file->setFontSize(12);
	m_file->setAlignment(Qt::AlignLeft);

	m_persent = addLabel("0%", QRect(width() - 41, 108, 27, 13), QColor("#96abff"));
	m_persent->setFontSize(10);
	m_persent->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	m_persent->setVisible(false);

	m_change = new COriginalButton(this);
	m_change->installEventFilter(this);
	m_change->setGeometry(62, 126, 102, 30);
	m_change->setFontFace(CGeneralStyle::instance()->font().family());
	m_change->setFontSize(12);
	m_change->setFontColor(QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"));
	m_change->setText(QString::fromStdWString(L"转到后台下载"));
	m_change->setBorderRadius(3);
	m_change->setBkgColor(QColor("#5a5ea2"), QColor("#4a6fff"), QColor("#5a5ea2"), QColor("#5a5ea2"));

	m_cancel = new COriginalButton(this);
	m_cancel->installEventFilter(this);
	m_cancel->setGeometry(258, 126, 102, 30);
	m_cancel->setFontFace(CGeneralStyle::instance()->font().family());
	m_cancel->setFontSize(12);
	m_cancel->setFontColor(QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"));
	m_cancel->setText(QString::fromStdWString(L"取消下载"));
	m_cancel->setBorderRadius(3);
	m_cancel->setBkgColor(QColor("#5a5ea2"), QColor("#4a6fff"), QColor("#5a5ea2"), QColor("#5a5ea2"));

	QObject::connect(this, SIGNAL(keyboardAccept()), this, SLOT(tipAccept()));

	m_progressBar = new ProgressBar(this);
	m_progressBar->setOrientation(Qt::Horizontal);
	//当前进度
	setRate(80);
	m_progressBar->setRange(0, 100);
	m_progressBar->setGeometry(13, 92, width() - 13 * 2, 4);
	m_progressBar->setBorderWidth(0);
	m_progressBar->setBorderRadius(2);
	m_progressBar->setBackgroundColor(QColor(38, 45, 73, 255));
	m_progressBar->setItemBorderWidth(0);
	m_progressBar->setItemBorderRadius(2);
	m_progressBar->setItemBackgroundColor(QColor(150, 171, 255, 255));
	m_progressBar->setFormat("");

	m_downloadSlow = addLabel(s_slowString, QRect(16, 179, 83, 18), QColor("#8491bd"));
	m_downloadSlow->setFontSize(12);

	m_hand = new COriginalButton(this);
	m_hand->installEventFilter(this);
	m_hand->setGeometry(103, 179, 48, 18);
	m_hand->setFontFace(CGeneralStyle::instance()->font().family());
	m_hand->setFontSize(12);
	m_hand->setFontColor(QColor("#465695"), QColor("#c9d3fc"), QColor("#313c6b"), QColor("#465695"));
	m_hand->setText(s_handString);
	m_hand->setUnderline(true);

	m_time->move(width() - 139, height() - 36);

	QObject::connect(this, SIGNAL(rateChanged(int)), m_progressBar, SLOT(setValue(int)));
	QObject::connect(this, SIGNAL(persentChanged(const QString&)), m_persent, SLOT(setText(const QString&)));
	QObject::connect(this, SIGNAL(downloadComplete()), this, SLOT(reject()));
	QObject::connect(this, SIGNAL(changeErrorStatus()), this, SLOT(onChangeErrorStatus()), Qt::UniqueConnection);
}

DownloadOperateDialog::~DownloadOperateDialog()
{

}

void DownloadOperateDialog::setRate(int32_t persent)
{
	emit rateChanged(persent);
	emit persentChanged(QString::fromStdWString(CStringManager::Format(L"%d%%", persent)));
}

void DownloadOperateDialog::error()
{
	emit changeErrorStatus();
}

void DownloadOperateDialog::onChangeErrorStatus()
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

void DownloadOperateDialog::downloadAccept()
{
	m_hand->setFocus();
	m_hand->click();
}
