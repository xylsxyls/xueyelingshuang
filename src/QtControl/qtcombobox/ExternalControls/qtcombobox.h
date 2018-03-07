#ifndef QTCOMBOBOX_H
#define QTCOMBOBOX_H

#include <QtWidgets/QMainWindow>
#include "ui_qtcombobox.h"
#include <stdint.h>

class HtmlComboBox;
class qtcombobox : public QMainWindow
{
	Q_OBJECT

public:
	qtcombobox(QWidget *parent = 0);
	~qtcombobox();

private slots:
	void currentTextChanged6(const QString& str);
	void idComboBoxItemChanged(qint64 id, const QString& str);
	void htmlItemPressed(int index);
	void modalPop();
	void modalFriendPop();
	void onTestButton();
	void testDialog();
	void onDialogDone(int32_t dialogId, int32_t result, int32_t userType);

	void onChangeToBack(int32_t dialogId);
	void onDownloadAgain(int32_t dialogId);
	void onCancelDownload(int32_t dialogId);
	void onUseOtherDownload(int32_t dialogId);
	void onCopyDownloadAddr(int32_t dialogId, const QString& addr);
	void onCopyPath(int32_t dialogId, const QString& path);
private:
	Ui::qtcomboboxClass ui;
	HtmlComboBox* box;
	int32_t m_dialogId8;
};

#endif // QTCOMBOBOX_H
