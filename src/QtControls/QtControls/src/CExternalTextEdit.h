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
    QString mExpressionTag;                                //url : expression/yun 类似这样的就是表情[/yun]
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
    void loadExpressions(const QString &emotionPath);       //* 传入Emotions文件夹所在目录

    void insertExpressionShortcut(const QString& shortcut); //* 传入表情shortcut来显示表情
    void appendText(const QString& text);                   //* 插入不带格式的数据，如果有表情shortcut会做转换
    void clear();                                           //* 清空数据

	QString filterExpressionToShortcut(QString html) const ;
	QString parseShortcutToExpression(QString text) const;
	QString parseShortcutToHTMLExpression(QString text) const; 
    QString message();                             //* 获取输入的内容，不带格式
    int     testExpressionCount(QString msg);      //* 获取表情个数
    int     testWordCount(QString msg);            //* 获取表情除外的字数
    QString removeExpressionShortcut(QString msg); //* 除去表情shortcut
};

#endif // CEXTERNALTEXTEDIT_H
