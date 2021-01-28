#ifndef FINDTEXTLINUX_H
#define FINDTEXTLINUX_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>

class COriginalButton;
class PushButton;
class Label;
class LineEdit;
class CheckBox;

class FindTextLinux : public QDialog
{
	Q_OBJECT
public:
	FindTextLinux(QWidget* parent = nullptr);
	~FindTextLinux();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);

private slots:
	void onButtonClicked();

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
};

#endif // FINDTEXTLINUX_H