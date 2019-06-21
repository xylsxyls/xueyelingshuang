#ifndef CEXTERNALTEXTEDIT_H
#define CEXTERNALTEXTEDIT_H

#include <QTextEdit>
#include <QTimer>
#include "QtControlsMacro.h"

class QtControlsAPI CExternalTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    CExternalTextEdit(QWidget* parent = NULL);
    ~CExternalTextEdit();

private:
    QMap<QString, QMovie*> mGifResourceMapForExpression;   //[url,  movie]
    QMap<QString, QString> mMapedExpression;               //[shortcut , url]
    QString mExpressionTag;                                //url : expression/yun ���������ľ��Ǳ���[/yun]
	int     mExpressionSize;
    QTimer mRefresgGifTimer;

    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);
    void keyPressEvent(QKeyEvent* e);
	void mousePressEvent(QMouseEvent *e);

signals:
    void enterPressed();
    void ctrlEnterPressed();
    void altCPressed();
    void altSPressed();

private slots:
    void updateGifResourceForExpression();

    void insertFromMimeData(const QMimeData *source);
    QMimeData* createMimeDataFromSelection() const;

public :
    void loadExpressions(const QString &emotionPath);       //* ����Emotions�ļ�������Ŀ¼

    void insertExpressionShortcut(const QString& shortcut); //* �������shortcut����ʾ����
    void appendText(const QString& text);                   //* ���벻����ʽ�����ݣ�����б���shortcut����ת��
    void clear();                                           //* �������

	QString filterExpressionToShortcut(QString html) const ;
	QString parseShortcutToExpression(QString text) const;
	QString parseShortcutToHTMLExpression(QString text) const; 
    QString message();                             //* ��ȡ��������ݣ�������ʽ
    int     testExpressionCount(QString msg);      //* ��ȡ�������
    int     testWordCount(QString msg);            //* ��ȡ������������
    QString removeExpressionShortcut(QString msg); //* ��ȥ����shortcut
};

#endif // CEXTERNALTEXTEDIT_H
