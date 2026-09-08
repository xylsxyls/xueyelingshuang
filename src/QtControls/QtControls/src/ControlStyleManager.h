#ifndef CONTROLSTYLEMANAGER_H
#define CONTROLSTYLEMANAGER_H

#include <QColor>
#include <QFont>
#include <QRect>
#include <QString>
#include <Qt>
#include "QtControlsMacro.h"

class Label;
class QPainter;
class QTextDocument;

/** QtControls通用样式管理器，保存控件库级别的默认字体、颜色、资源路径和HTML绘制辅助对象
*/
class QtControlsAPI ControlStyleManager
{
public:
	/** 获取通用样式管理器单例
	@return 返回进程内唯一的通用样式管理器引用
	*/
	static ControlStyleManager& instance();

public:
	/** 设置默认字体
	@param [in] font 默认字体对象
	*/
	void setDefaultFont(const QFont& font);

	/** 获取默认字体
	@return 返回默认字体对象
	*/
	QFont defaultFont() const;

	/** 设置默认文字颜色
	@param [in] color 默认文字颜色
	*/
	void setDefaultTextColor(const QColor& color);

	/** 获取默认文字颜色
	@return 返回默认文字颜色
	*/
	QColor defaultTextColor() const;

	/** 设置资源根目录
	@param [in] rootPath 资源根目录，允许使用系统原生路径分隔符
	*/
	void setResourceRootPath(const QString& rootPath);

	/** 获取资源根目录
	@return 返回标准化后的资源根目录
	*/
	QString resourceRootPath() const;

	/** 清理内部HTML绘制缓存，通常由Qt退出流程自动调用
	*/
	void clearRenderCache();

	/** 根据资源根目录和相对路径拼接完整资源路径
	@param [in] relativePath 资源相对路径，允许旧代码传入以/开头的资源路径，盘符或UNC绝对路径会原样标准化返回
	@return 返回标准化后的完整资源路径
	*/
	QString resourcePath(const QString& relativePath) const;

	/** 通过Label控件绘制HTML文本，使用painter当前字体和画笔颜色
	@param [in] painter 绘图对象，传入空指针时不绘制
	@param [in] rect 绘制区域
	@param [in] html HTML文本内容
	@param [in] alignment 文本对齐方式
	*/
	void drawHtmlTextByLabel(QPainter* painter,
							 const QRect& rect,
							 const QString& html,
							 Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop);

	/** 通过QTextDocument绘制HTML文本，使用painter当前字体和画笔颜色
	@param [in] painter 绘图对象，传入空指针时不绘制
	@param [in] rect 绘制区域
	@param [in] html HTML文本内容
	*/
	void drawHtmlTextByQTextDocument(QPainter* painter,
									 const QRect& rect,
									 const QString& html);

private:
	/** 构造通用样式管理器，初始化默认字体、颜色、资源根目录和HTML绘制缓存
	*/
	ControlStyleManager();

	/** 析构通用样式管理器，释放HTML绘制缓存
	*/
	~ControlStyleManager();

private:
	/** 禁止拷贝构造，保证样式管理器只有一个实例
	@param [in] other 被拷贝对象
	*/
	ControlStyleManager(const ControlStyleManager& other);

	/** 禁止赋值，保证样式管理器只有一个实例
	@param [in] other 被赋值对象
	@return 返回当前对象引用
	*/
	ControlStyleManager& operator=(const ControlStyleManager& other);

private:
	/** 将路径转换为Qt统一分隔符并去掉尾部多余分隔符
	@param [in] path 待标准化路径
	@return 返回标准化后的路径
	*/
	static QString normalizePath(const QString& path);

	/** 在QCoreApplication退出前释放缓存的Qt绘制对象。
	*/
	static void clearRenderCacheForApplicationExit();

	/** 延迟创建Label方式的HTML绘制控件
	@return true表示创建成功或缓存已存在，false表示创建失败
	*/
	bool ensureHtmlRender();

	/** 延迟创建QTextDocument方式的HTML绘制文档
	@return true表示创建成功或缓存已存在，false表示创建失败
	*/
	bool ensureTextDocument();

	/** 判断当前调用是否位于Qt GUI线程，避免跨线程创建或绘制Widget。
	@return true表示当前线程允许访问内部Qt绘制对象
	*/
	static bool isGuiThread();

private:
	// 默认字体
	QFont m_defaultFont;

	// 默认文字颜色
	QColor m_defaultTextColor;

	// 控件库资源根目录
	QString m_resourceRootPath;

	// Label方式绘制HTML时复用的临时控件
	Label* m_htmlRender;

	// QTextDocument方式绘制HTML时复用的临时文档
	QTextDocument* m_doc;
};

#endif // CONTROLSTYLEMANAGER_H