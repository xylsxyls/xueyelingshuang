#include "DownloadOperateDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "ProgressBar.h"
#include <Windows.h>
#include "DownloadErrorDialog.h"
#include "DialogManager.h"
#include "LineEdit.h"

std::wstring DownloadOperateDialog::s_fileString = L"文件名：%s";
QString DownloadOperateDialog::s_slowString = QString::fromStdWString(L"如果下载失败。您可以尝试手动下载文件，或者");
QString DownloadOperateDialog::s_handString = QString::fromStdWString(L"使用其他下载渠道");

int32_t DownloadOperateDialog::popDownloadOperateDialog(int32_t& dialogId,
														int32_t taskId,
														const QString& title,
														const QString& fileName,
														const QString& downloadSpeed,
														const QString& hasDownloaded,
														const QString& downloadTime,
														int32_t rate,
														bool backEnable,
														const QString& downloadAddr,
														const QString& path,
														QWindow* parent)
{
	DownloadOperateDialog* dlg = new DownloadOperateDialog(taskId, title, fileName, downloadSpeed, hasDownloaded, downloadTime, rate, backEnable, downloadAddr, path);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(DOWNLOAD_OPERATE_DIALOG);
	int32_t result = dlg->exec(dialogId);
	delete dlg;
	return result;

}

DownloadOperateDialog::DownloadOperateDialog(int32_t taskId,
											 const QString& title,
											 const QString& fileName,
											 const QString& downloadSpeed,
											 const QString& hasDownloaded,
											 const QString& downloadTime,
											 int32_t rate,
											 bool backEnable,
											 const QString& downloadAddr,
											 const QString& path)
{
	initForExec(420, 180);

	m_taskId = taskId;

	m_title->setText(title);
	setWindowTitle(title);
	m_title->setFontSize(12);
	m_title->resize(width() - 30, 35);
	m_title->setTextColor(QColor("#cdd5e1"));

	m_separator->setGeometry(16, 41, width() - 16 * 2, 2);

	m_file = addLabel(fileName, QRect(16, 63, width() / 2 - 16, 30), QColor("#8592bf"));
	m_file->setFontSize(12);
	m_file->setAlignment(Qt::AlignLeft);

	m_downloadSpeed = addLabel(downloadSpeed, QRect(212, 63, 50, 20), QColor("#8592bf"));
	m_downloadSpeed->setAlignment(Qt::AlignLeft);
	m_downloadSpeed->setFontSize(12);
	m_downloaded = addLabel(hasDownloaded, QRect(271, 63, 80, 20), QColor("#8592bf"));
	m_downloaded->setAlignment(Qt::AlignLeft);
	m_downloaded->setFontSize(12);
	m_downloadTime = addLabel(downloadTime, QRect(357, 63, 56, 20), QColor("#8592bf"));
	m_downloadTime->setAlignment(Qt::AlignLeft);
	m_downloadTime->setFontSize(12);

	m_error = addLabel(QString::fromStdWString(L"下载出错！"), QRect(212, 63, width() / 2 - 12, 20), QColor("#ff4747"));
	m_error->setFontSize(12);
	m_error->setVisible(false);

	m_persent = addLabel(QString(rate) + "%", QRect(width() - 41, 108, 27, 13), QColor("#96abff"));
	m_persent->setFontSize(10);
	m_persent->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	m_persent->setVisible(false);

	m_back = new COriginalButton(this);
	m_back->installEventFilter(this);
	m_back->setGeometry(62, 126, 102, 30);
	m_back->setFontFace(CGeneralStyle::instance()->font().family());
	m_back->setFontSize(12);
	m_back->setFontColor(QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#545566"));
	m_back->setText(QString::fromStdWString(L"转到后台下载"));
	m_back->setBorderRadius(3);
	m_back->setBkgColor(QColor("#5a5ea2"), QColor("#4a6fff"), QColor("#5a5ea2"), QColor("#888994"));
	m_back->setEnabled(backEnable);

	m_again = new COriginalButton(this);
	m_again->installEventFilter(this);
	m_again->setGeometry(62, 126, 102, 30);
	m_again->setFontFace(CGeneralStyle::instance()->font().family());
	m_again->setFontSize(12);
	m_again->setFontColor(QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#545566"));
	m_again->setText(QString::fromStdWString(L"重试"));
	m_again->setBorderRadius(3);
	m_again->setBkgColor(QColor("#5a5ea2"), QColor("#4a6fff"), QColor("#5a5ea2"), QColor("#888994"));
	m_again->setVisible(false);

	m_cancel = new COriginalButton(this);
	m_cancel->installEventFilter(this);
	m_cancel->setGeometry(258, 126, 102, 30);
	m_cancel->setFontFace(CGeneralStyle::instance()->font().family());
	m_cancel->setFontSize(12);
	m_cancel->setFontColor(QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#545566"));
	m_cancel->setText(QString::fromStdWString(L"取消下载"));
	m_cancel->setBorderRadius(3);
	m_cancel->setBkgColor(QColor("#5a5ea2"), QColor("#4a6fff"), QColor("#5a5ea2"), QColor("#888994"));

	m_progressBar = new ProgressBar(this);
	m_progressBar->setOrientation(Qt::Horizontal);
	//当前进度
	setRate(rate);
	m_progressBar->setRange(0, 100);
	m_progressBar->setGeometry(16, 92, width() - 16 * 2, 4);
	m_progressBar->setBorderWidth(0);
	m_progressBar->setBorderRadius(2);
	m_progressBar->setBackgroundColor(QColor(38, 45, 73, 255));
	m_progressBar->setItemBorderWidth(0);
	m_progressBar->setItemBorderRadius(2);
	m_progressBar->setItemBackgroundColor(QColor(150, 171, 255, 255));
	m_progressBar->setFormat("");

	m_downloadSlow = addLabel(s_slowString, QRect(16, 179, 255, 18), QColor("#8491bd"));
	m_downloadSlow->setFontSize(12);

	m_hand = new COriginalButton(this);
	m_hand->installEventFilter(this);
	m_hand->setGeometry(258, 179, 120, 18);
	m_hand->setFontFace(CGeneralStyle::instance()->font().family());
	m_hand->setFontSize(12);
	m_hand->setFontColor(QColor("#4a6fff"));
	m_hand->setText(s_handString);
	m_hand->setUnderline(true);

	m_time->move(width() - 139, height() - 36);

	m_downloadAddr = addLabel(QString::fromStdWString(L"下载地址"), QRect(16, 215, 100, 20), QColor("#8d8e91"));
	m_downloadAddr->setFontSize(12);
	m_downloadAddr->setAlignment(Qt::AlignLeft);
	m_downloadAddrEdit = new LineEdit(this);
	m_downloadAddrEdit->setBackgroundColor(QColor("#0b0d15"));
	m_downloadAddrEdit->setGeometry(72, 210, 252, 24);
	m_downloadAddrEdit->setBorderRadius(2);
	m_downloadAddrEdit->setTextColor(QColor("#cdd5e1"));
	m_downloadAddrEdit->setFontSize(12);
	m_downloadAddrEdit->setBorderWidth(0);
	m_downloadAddrEdit->setText(downloadAddr);
	m_downloadAddrEdit->setReadOnly(true);
	QObject::connect(this, &DownloadOperateDialog::editDownloadAddr, m_downloadAddrEdit, &LineEdit::setText);

	m_downloadButton = new COriginalButton(this);
	m_downloadButton->setGeometry(332, 210, 72, 24);
	m_downloadButton->setBkgColor(QColor("#5a5ea2"), QColor("#4a6fff"), QColor("#5a5ea2"), QColor("#888994"));
	m_downloadButton->setFontSize(12);
	m_downloadButton->setText(QString::fromStdWString(L"复制地址"));
	m_downloadButton->setBorderRadius(2);
	m_downloadButton->setFontColor(QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#545566"));

	m_path = addLabel(QString::fromStdWString(L"本地路径"), QRect(16, 247, 100, 20), QColor("#8d8e91"));
	m_path->setFontSize(12);
	m_path->setAlignment(Qt::AlignLeft);
	m_pathEdit = new LineEdit(this);
	m_pathEdit->setBackgroundColor(QColor("#0b0d15"));
	m_pathEdit->setGeometry(72, 242, 252, 24);
	m_pathEdit->setBorderRadius(2);
	m_pathEdit->setTextColor(QColor("#cdd5e1"));
	m_pathEdit->setFontSize(12);
	m_pathEdit->setBorderWidth(0);
	m_pathEdit->setText(path);
	m_pathEdit->setReadOnly(true);
	QObject::connect(this, &DownloadOperateDialog::editPath, m_pathEdit, &LineEdit::setText);
	m_pathButton = new COriginalButton(this);
	m_pathButton->setGeometry(332, 242, 72, 24);
	m_pathButton->setBkgColor(QColor("#5a5ea2"), QColor("#4a6fff"), QColor("#5a5ea2"), QColor("#888994"));
	m_pathButton->setFontSize(12);
	m_pathButton->setText(QString::fromStdWString(L"复制地址"));
	m_pathButton->setBorderRadius(2);
	m_pathButton->setFontColor(QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#b5c2f3"), QColor("#545566"));

	QObject::connect(this, &DownloadOperateDialog::rateChanged, m_progressBar, &ProgressBar::setValue);
	QObject::connect(this, &DownloadOperateDialog::persentChanged, m_persent, &Label::setText);
	QObject::connect(this, &DownloadOperateDialog::changeErrorStatus, this, &DownloadOperateDialog::onChangeErrorStatus, Qt::UniqueConnection);
	QObject::connect(this, &DownloadOperateDialog::changeNormalStatus, this, &DownloadOperateDialog::onChangeNormalStatus, Qt::UniqueConnection);

	QObject::connect(this, &DownloadOperateDialog::downloadComplete, this, &DownloadOperateDialog::reject);
	QObject::connect(this, &DownloadOperateDialog::downloadSpeed, m_downloadSpeed, &Label::setText);
	QObject::connect(this, &DownloadOperateDialog::downloaded, m_downloaded, &Label::setText);
	QObject::connect(this, &DownloadOperateDialog::downloadTime, m_downloadTime, &Label::setText);
	QObject::connect(this, &DownloadOperateDialog::backEnable, m_back, &COriginalButton::setEnabled);

	QObject::connect(m_cancel, &COriginalButton::clicked, this, &DownloadOperateDialog::onCancelDownload);
	QObject::connect(m_back, &COriginalButton::clicked, this, &DownloadOperateDialog::onBack);
	QObject::connect(m_again, &COriginalButton::clicked, this, &DownloadOperateDialog::onAgain);
	QObject::connect(m_hand, &COriginalButton::clicked, this, &DownloadOperateDialog::onUseOtherDownload);
	QObject::connect(m_downloadButton, &COriginalButton::clicked, this, &DownloadOperateDialog::onCopyDownloadAddr);
	QObject::connect(m_pathButton, &COriginalButton::clicked, this, &DownloadOperateDialog::onCopyPath);

	setExitVisible(false);
	//error();
	normal();
}

DownloadOperateDialog::~DownloadOperateDialog()
{

}

void DownloadOperateDialog::setClipboardData(HWND hWnd, const std::string& str)
{
	//打开剪贴板
	if (::OpenClipboard(hWnd))
	{
		HANDLE hClip;
		char* pBuf;
		//清空剪贴板
		::EmptyClipboard();

		//写入数据
		hClip = ::GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
		pBuf = (char*)::GlobalLock(hClip);
		::strcpy(pBuf, str.c_str());
		//解锁
		::GlobalUnlock(hClip);
		//设置格式
		::SetClipboardData(CF_TEXT, hClip);

		//关闭剪贴板
		::CloseClipboard();
	}
}

void DownloadOperateDialog::setDownloadSpeed(const QString& speed)
{
	emit downloadSpeed(speed);
}

void DownloadOperateDialog::setDownloaded(const QString& download)
{
	emit downloaded(download);
}

void DownloadOperateDialog::setDownloadTime(const QString& time)
{
	emit downloadTime(time);
}

void DownloadOperateDialog::setRate(int32_t persent)
{
	emit rateChanged(persent);
	emit persentChanged(QString::fromStdWString(CStringManager::Format(L"%d%%", persent)));
}

void DownloadOperateDialog::setEditDownloadAddr(const QString& addr)
{
	emit editDownloadAddr(addr);
}

void DownloadOperateDialog::setEditPath(const QString& path)
{
	emit editPath(path);
}

void DownloadOperateDialog::setBackEnable(bool enable)
{
	emit backEnable(enable);
}

void DownloadOperateDialog::error()
{
	emit changeErrorStatus();
}

void DownloadOperateDialog::normal()
{
	emit changeNormalStatus();
}

void DownloadOperateDialog::setTaskId(int32_t taskId)
{
	m_taskId = taskId;
}

int32_t DownloadOperateDialog::getTaskId()
{
	return m_taskId;
}

void DownloadOperateDialog::onChangeErrorStatus()
{
	resize(420, 280);
	m_downloadSpeed->setVisible(false);
	m_downloaded->setVisible(false);
	m_downloadTime->setVisible(false);
	m_error->setVisible(true);
	m_back->setText(QString::fromStdWString(L"重试"));
	m_back->setVisible(false);
	m_again->setVisible(true);
	return;
}

void DownloadOperateDialog::onChangeNormalStatus()
{
	resize(420, 180);
	m_downloadSpeed->setVisible(true);
	m_downloaded->setVisible(true);
	m_downloadTime->setVisible(true);
	m_error->setVisible(false);
	m_back->setText(QString::fromStdWString(L"转到后台下载"));
	m_back->setVisible(true);
	m_again->setVisible(false);
	return;
}

void DownloadOperateDialog::downloadAccept(QObject* tar, Qt::Key key)
{
	m_hand->setFocus();
	m_hand->click();
}

void DownloadOperateDialog::onBack()
{
	emit changeToBack();
	emit DialogManager::instance().changeToBack(DialogManager::instance().taskId(this));
}

void DownloadOperateDialog::onAgain()
{
	emit downloadAgain();
	emit DialogManager::instance().downloadAgain(DialogManager::instance().taskId(this));
}

void DownloadOperateDialog::onCancelDownload()
{
	emit cancelDownload();
	emit DialogManager::instance().cancelDownload(DialogManager::instance().taskId(this));
}

void DownloadOperateDialog::onUseOtherDownload()
{
	emit useOtherDownload();
	emit DialogManager::instance().useOtherDownload(DialogManager::instance().taskId(this));
}

void DownloadOperateDialog::onCopyDownloadAddr()
{
	setClipboardData((HWND)winId(), CStringManager::UnicodeToAnsi(m_downloadAddrEdit->text().toStdWString()));
	emit copyDownloadAddr(m_downloadAddrEdit->text());
	emit DialogManager::instance().copyDownloadAddr(DialogManager::instance().taskId(this), m_downloadAddrEdit->text());
}

void DownloadOperateDialog::onCopyPath()
{
	setClipboardData((HWND)winId(), CStringManager::UnicodeToAnsi(m_pathEdit->text().toStdWString()));
	emit copyPath(m_pathEdit->text());
	emit DialogManager::instance().copyPath(DialogManager::instance().taskId(this), m_pathEdit->text());
}
