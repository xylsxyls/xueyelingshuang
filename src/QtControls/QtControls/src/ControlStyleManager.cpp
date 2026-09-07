#include "ControlStyleManager.h"
#include "Label.h"
#include <QAbstractTextDocumentLayout>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QTextDocument>
#include <QTextOption>
#include <stdint.h>

ControlStyleManager& ControlStyleManager::instance()
{
	static ControlStyleManager s_manager;
	return s_manager;
}

void ControlStyleManager::setDefaultFont(const QFont& font)
{
	m_defaultFont = font;
}

QFont ControlStyleManager::defaultFont() const
{
	return m_defaultFont;
}

void ControlStyleManager::setDefaultTextColor(const QColor& color)
{
	m_defaultTextColor = color;
}

QColor ControlStyleManager::defaultTextColor() const
{
	return m_defaultTextColor;
}

void ControlStyleManager::setResourceRootPath(const QString& rootPath)
{
	m_resourceRootPath = normalizePath(rootPath);
}

QString ControlStyleManager::resourceRootPath() const
{
	return m_resourceRootPath;
}

void ControlStyleManager::clearRenderCache()
{
	delete m_htmlRender;
	m_htmlRender = nullptr;
	delete m_doc;
	m_doc = nullptr;
}

QString ControlStyleManager::resourcePath(const QString& relativePath) const
{
	QString path = normalizePath(relativePath);
	if (path.isEmpty())
	{
		return m_resourceRootPath;
	}

	bool isLegacyRootRelativePath = path.startsWith('/') && !path.startsWith("//");
	QFileInfo pathInfo(path);
	if (pathInfo.isAbsolute() && !isLegacyRootRelativePath)
	{
		return path;
	}

	while (path.startsWith('/'))
	{
		path.remove(0, 1);
	}

	if (m_resourceRootPath.isEmpty())
	{
		return path;
	}
	return m_resourceRootPath + "/" + path;
}

void ControlStyleManager::drawHtmlTextByLabel(QPainter* painter,
											  const QRect& rect,
											  const QString& html,
											  Qt::Alignment alignment)
{
	if (painter == nullptr || !ensureHtmlRender())
	{
		return;
	}

	int32_t fontSize = painter->font().pixelSize();
	if (fontSize <= 0)
	{
		fontSize = painter->font().pointSize();
	}
	if (fontSize <= 0)
	{
		fontSize = m_defaultFont.pixelSize();
	}
	if (fontSize <= 0)
	{
		fontSize = 14;
	}

	QString htmlString = QStringLiteral("<div style=\"color:%1;font-size:%2px;font-family:'%3'\">%4</div>")
		.arg(painter->pen().color().name())
		.arg(fontSize)
		.arg(painter->font().family())
		.arg(html);

	painter->save();
	m_htmlRender->setText(htmlString);
	m_htmlRender->setAlignment(alignment);
	m_htmlRender->resize(rect.size());
	painter->translate(rect.topLeft());
	m_htmlRender->render(painter);
	painter->restore();
}

void ControlStyleManager::drawHtmlTextByQTextDocument(QPainter* painter,
													  const QRect& rect,
													  const QString& html)
{
	if (painter == nullptr || !ensureTextDocument())
	{
		return;
	}

	QString htmlString = QStringLiteral("<div style=\"color:%1;\">%2</div>")
		.arg(painter->pen().color().name())
		.arg(html);

	painter->save();
	m_doc->setHtml(htmlString);
	m_doc->setDocumentMargin(0);
	m_doc->setDefaultFont(painter->font());

	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	m_doc->setDefaultTextOption(textOption);

	painter->translate(rect.topLeft());

	QAbstractTextDocumentLayout::PaintContext context;
	m_doc->documentLayout()->draw(painter, context);
	painter->restore();
}

ControlStyleManager::ControlStyleManager() :
m_defaultFont(),
m_defaultTextColor("#abb3d3"),
m_resourceRootPath(),
m_htmlRender(nullptr),
m_doc(nullptr)
{
	m_defaultFont.setFamily(QStringLiteral("微软雅黑"));
	m_defaultFont.setPixelSize(14);
	m_resourceRootPath = normalizePath(QCoreApplication::applicationDirPath());
	qAddPostRoutine(ControlStyleManager::clearRenderCacheForApplicationExit);
}

ControlStyleManager::~ControlStyleManager()
{
	clearRenderCache();
}

ControlStyleManager::ControlStyleManager(const ControlStyleManager& other)
{
	Q_UNUSED(other);
}

ControlStyleManager& ControlStyleManager::operator=(const ControlStyleManager& other)
{
	Q_UNUSED(other);
	return *this;
}

QString ControlStyleManager::normalizePath(const QString& path)
{
	QString normalizedPath = QDir::fromNativeSeparators(path).trimmed();
	while (normalizedPath.length() > 1 && normalizedPath.endsWith('/'))
	{
		normalizedPath.chop(1);
	}
	return normalizedPath;
}

void ControlStyleManager::clearRenderCacheForApplicationExit()
{
	ControlStyleManager::instance().clearRenderCache();
}

bool ControlStyleManager::ensureHtmlRender()
{
	if (m_htmlRender != nullptr)
	{
		return true;
	}

	m_htmlRender = new Label;
	if (m_htmlRender == nullptr)
	{
		return false;
	}
	m_htmlRender->setAttribute(Qt::WA_TranslucentBackground);
	return true;
}

bool ControlStyleManager::ensureTextDocument()
{
	if (m_doc != nullptr)
	{
		return true;
	}

	m_doc = new QTextDocument;
	return m_doc != nullptr;
}

