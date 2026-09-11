#ifndef CEXTERNALTEXTEDIT_H
#define CEXTERNALTEXTEDIT_H

#include "ExpressionLoadResult.h"
#include <QTextEdit>
#include <QMap>
#include <QTimer>
#include "QtControlsMacro.h"

class QMovie;

/** 支持表情快捷码和GIF资源刷新的文本编辑控件，负责表情HTML转换和快捷键输入信号
*/
class QtControlsAPI CExternalTextEdit : public QTextEdit
{
    Q_OBJECT
public:
	/** 构造表情文本编辑控件
	@param [in] parent 父窗口指针
	*/
    CExternalTextEdit(QWidget* parent = nullptr);

	/** 析构函数，释放表情动图资源
	*/
    ~CExternalTextEdit();

private:
	// 表情图片路径和动图对象的映射
    QMap<QString, QMovie*> m_gifResourceMapForExpression;
	// 表情快捷码和图片路径的映射
    QMap<QString, QString> m_mappedExpression;
	// 表情资源路径标记，例如expression/yun对应[/yun]
    QString m_expressionTag;
	// 表情显示尺寸
	qint32 m_expressionSize;
	// GIF表情刷新计时器
    QTimer m_refreshGifTimer;

private:
	/** 控件显示时启动GIF表情刷新
	@param [in] e Qt显示事件
	*/
    void showEvent(QShowEvent *e);

	/** 控件隐藏时停止GIF表情刷新
	@param [in] e Qt隐藏事件
	*/
    void hideEvent(QHideEvent *e);

	/** 处理回车、Ctrl+回车和Alt组合快捷键
	@param [in] e Qt键盘事件
	*/
    void keyPressEvent(QKeyEvent* e);

	/** 处理鼠标按下事件，用于保持文本编辑焦点和选择行为
	@param [in] e Qt鼠标事件
	*/
	void mousePressEvent(QMouseEvent *e);

signals:
	/** 按下回车时发送信号
	*/
    void enterPressed();

	/** 按下Ctrl+回车时发送信号
	*/
    void ctrlEnterPressed();

	/** 按下Alt+C时发送信号
	*/
    void altCPressed();

	/** 按下Alt+S时发送信号
	*/
    void altSPressed();

private slots:
	/** 定时刷新GIF表情资源，让动图在文本框中持续更新
	*/
    void updateGifResourceForExpression();

	/** 插入剪贴板或拖拽数据，并把可识别表情快捷码转成表情HTML
	@param [in] source MIME数据源
	*/
    void insertFromMimeData(const QMimeData *source);

	/** 从当前选择内容创建MIME数据，保留表情快捷码语义
	@return 返回新创建的MIME数据对象
	*/
    QMimeData* createMimeDataFromSelection() const;

public :
	/** 加载表情资源目录
	@param [in] emotionPath Emotions文件夹所在目录
	*/
    void loadExpressions(const QString &emotionPath);

    /** 加载表情资源并返回具体错误，不在基础控件内写日志
    @param [in] emotionPath 表情资源根目录
    @param [out] detail 可空，接收解析动态详情
    @return 返回ExpressionLoadResult，中文原因见共享const map
    */
    ExpressionLoadResult loadExpressionsResult(const QString& emotionPath, QString* detail = nullptr);

	/** 按表情快捷码插入对应表情
	@param [in] shortcut 表情快捷码
	*/
    void insertExpressionShortcut(const QString& shortcut);

	/** 追加纯文本，文本内的表情快捷码会转换成表情HTML
	@param [in] text 要追加的文本
	*/
    void appendText(const QString& text);

	/** 清空文本和表情显示内容
	*/
    void clear();

	/** 把HTML中的表情图片转换回表情快捷码
	@param [in] html 文本框HTML内容
	@return 返回替换后的快捷码文本
	*/
	QString filterExpressionToShortcut(QString html) const ;

	/** 把纯文本中的表情快捷码转换成表情占位文本
	@param [in] text 原始文本
	@return 返回替换后的文本
	*/
	QString parseShortcutToExpression(QString text) const;

	/** 把纯文本中的表情快捷码转换成可显示的HTML表情
	@param [in] text 原始文本
	@return 返回替换后的HTML文本
	*/
	QString parseShortcutToHTMLExpression(QString text) const;

	/** 获取输入内容，不包含HTML格式
	@return 返回文本内容
	*/
    QString message();

	/** 统计文本中表情快捷码数量
	@param [in] msg 待统计文本
	@return 返回表情数量
	*/
    int     testExpressionCount(QString msg);

	/** 统计文本中除表情外的字符数量
	@param [in] msg 待统计文本
	@return 返回非表情字符数量
	*/
    int     testWordCount(QString msg);

	/** 移除文本中的表情快捷码
	@param [in] msg 待处理文本
	@return 返回移除表情快捷码后的文本
	*/
    QString removeExpressionShortcut(QString msg);
};

#endif // CEXTERNALTEXTEDIT_H