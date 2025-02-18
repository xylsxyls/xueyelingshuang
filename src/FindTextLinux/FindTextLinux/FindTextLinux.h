#ifndef FINDTEXTLINUX_H
#define FINDTEXTLINUX_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>

class COriginalButton;
class PushButton;
class Label;
class LineEdit;
class CheckBox;
class ComboBox;
class SearchEdit;
class Menu;

class FindTextLinux : public QDialog
{
	Q_OBJECT
public:
	friend class SearchPathTask;
	friend class EveryFilePath;
	friend class SearchResultTask;

public:
	FindTextLinux(QWidget* parent = nullptr);
	~FindTextLinux();

Q_SIGNALS:
    void searchEnd();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onCurrentIndexChanged(int index);
	void onFormatButtonClicked();
	void onSearchButtonClicked();
	void onSearchEnd();
	void onShowSearchEditMenu();

private:
	Label* m_pathLabel;
	LineEdit* m_path;
	PushButton* m_formatButton;
	bool m_searchFormat;
	LineEdit* m_format;
	Label* m_searchLabel;
	LineEdit* m_search;
	CheckBox* m_suffix;
	CheckBox* m_matchCase;
	CheckBox* m_searchName;
	PushButton* m_searchButton;
	Label* m_charsetLabel;
	ComboBox* m_charset;
	SearchEdit* m_searchText;
	uint32_t m_searchPathThreadId;
	int32_t m_threadCount;
	QString m_text;
	Menu* m_searchEditMenu;
};

#endif // FINDTEXTLINUX_H