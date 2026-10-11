#include "SplitViewerMediaManager.h"
#include "SplitViewerTaskManager.h"
#include "SplitViewerVideoSession.h"
#include "SplitViewerForeignWindowHelper.h"
#include "SplitViewerImageHelper.h"
#include "Config.h"
#include "LogManager/LogManagerAPI.h"
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>
#include <QtWidgets/QWidget>
#include <algorithm>

SplitViewerMediaManager::SplitViewerMediaManager(SplitViewerCoreDocument* document, QMap<QString, QImage>* images,
    QMap<SplitViewerCoreNode*, QWidget*>* embedded, QWidget* canvas, SplitViewerTaskManager* tasks, QObject* parent) :
QObject(parent),
m_document(document),
m_images(images),
m_embedded(embedded),
m_canvas(canvas),
m_session(new SplitViewerVideoSession(this)),
m_tasks(tasks),
m_revision(0),
m_stopping(false)
{
    connect(m_session, &SplitViewerVideoSession::windowReady, this, &SplitViewerMediaManager::windowReady);
    connect(m_session, &SplitViewerVideoSession::idle, this, &SplitViewerMediaManager::idle);
    connect(m_session, &SplitViewerVideoSession::failed, this,
        [this](quint64 id, const QString& message)
        {
            const bool relevant = m_targets.remove(id) > 0;
            ++m_revision;
            emit changed();
            if (!m_stopping && (relevant || id == 0))
            {
                emit error(message);
            }
        });
}

SplitViewerMediaManager::~SplitViewerMediaManager()
{
    shutdown();
}

QString SplitViewerMediaManager::playerPath()
{
    QString value = QProcessEnvironment::systemEnvironment().value(g_config.m_playerEnvironmentName).trimmed();
    if (value.size() >= 2 && value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
    {
        value = value.mid(1, value.size() - 2);
    }
    const QFileInfo file(value);
    return !value.isEmpty() && file.isFile() ? file.absoluteFilePath() : QString();
}

QString SplitViewerMediaManager::fileFilter() const
{
    return playerPath().isEmpty() ? g_config.m_imageFilter : g_config.m_mediaFilter;
}

QString SplitViewerMediaManager::selectionTitle() const
{
    return playerPath().isEmpty() ? g_config.m_selectImageText : g_config.m_selectMediaText;
}

QString SplitViewerMediaManager::loadActionText() const
{
    return playerPath().isEmpty() ? g_config.m_loadImageText : g_config.m_loadMediaText;
}

bool SplitViewerMediaManager::validLeaf(SplitViewerCoreNode* leaf) const
{
    if (!m_document || !leaf)
    {
        return false;
    }
    std::vector<SplitViewerCoreNode*> leaves;
    SplitViewerCoreNode::collectLeaves(m_document->baseRoot(), leaves);
    for (int i = 0; i < m_document->layerCount(); ++i)
    {
        SplitViewerCoreNode::collectLeaves(m_document->layerAt(i)->root, leaves);
    }
    return std::find(leaves.begin(), leaves.end(), leaf) != leaves.end();
}

bool SplitViewerMediaManager::pending(SplitViewerCoreNode* leaf) const
{
    foreach (const SplitViewerVideoTarget& target, m_targets)
    {
        if (target.m_node == leaf)
        {
            return true;
        }
    }
    return false;
}

void SplitViewerMediaManager::cancelPending(SplitViewerCoreNode* leaf)
{
    const QList<quint64> ids = m_targets.keys();
    foreach (quint64 id, ids)
    {
        if (m_targets.value(id).m_node == leaf)
        {
            m_targets.remove(id);
            m_session->cancel(id);
        }
    }
}

void SplitViewerMediaManager::load(SplitViewerCoreNode* leaf, const QString& path, bool restoring)
{
    if (m_stopping || !validLeaf(leaf) || path.isEmpty())
    {
        return;
    }
    const QFileInfo file(path);
    const bool video =
        restoring || g_config.m_videoExtensions.find(file.suffix().toLower()) != g_config.m_videoExtensions.end();
    if (video)
    {
        const QString program = playerPath();
        if (program.isEmpty() || !file.isFile())
        {
            emit error(
                program.isEmpty() ? g_config.m_videoPlayerMissingError : g_config.m_videoFileMissingError + path);
            return;
        }
        cancelPending(leaf);
        const quint64 id = m_session->start(program, file.absoluteFilePath());
        if (!id)
        {
            emit error(g_config.m_videoBusyError);
            return;
        }
        m_targets.insert(id, SplitViewerVideoTarget(leaf, file.absoluteFilePath()));
        ++m_revision;
        emit changed();
        return;
    }
    const std::shared_ptr<QImage> image = std::make_shared<QImage>();
    const quint64 revision = m_revision;
    const QString readError = g_config.m_readImageError;
    const bool accepted = m_tasks->submit(
        true,
        [image, path](const std::atomic<bool>& exit)
        {
            if (exit.load())
            {
                return false;
            }
            image->load(path);
            return !exit.load() && !image->isNull();
        },
        [this, image, leaf, path, readError, revision](bool success)
        {
            if (m_stopping || revision != m_revision || !validLeaf(leaf))
            {
                LOGINFO("Discard stale image result; revision=%llu", revision);
                return;
            }
            if (!success)
            {
                emit error(readError + path);
                return;
            }
            release(leaf);
            m_images->insert(path, *image);
            leaf->view.clear();
            leaf->view.path = path.toStdWString();
            leaf->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
            leaf->view.hasImage = true;
            SplitViewerImageHelper::pruneUnusedImages(*m_document, *m_images);
            ++m_revision;
            emit changed();
        });
    if (!accepted)
    {
        emit error(g_config.m_taskBusyError);
    }
}

bool SplitViewerMediaManager::embedWindow(SplitViewerCoreNode* leaf, WId windowId, const QString& videoPath)
{
    if (m_stopping || !m_canvas || !windowId || !validLeaf(leaf) ||
        (videoPath.isEmpty() && (leaf->view.hasContent() || pending(leaf))))
    {
        return false;
    }
    QWidget* container = SplitViewerForeignWindowHelper::embedForeignWindow(windowId, m_canvas);
    if (!container)
    {
        return false;
    }
    if (!m_session->trackWindow(windowId))
    {
        SplitViewerForeignWindowHelper::detachForeignWindow(container);
        return false;
    }
    release(leaf);
    m_embedded->insert(leaf, container);
    leaf->view.clear();
    leaf->view.contentKind = videoPath.isEmpty() ? SPLITVIEWER_CORE_CONTENT_EMBEDDED : SPLITVIEWER_CORE_CONTENT_VIDEO;
    leaf->view.path = videoPath.toStdWString();
    SplitViewerImageHelper::pruneUnusedImages(*m_document, *m_images);
    ++m_revision;
    LOGINFO("Window embedded; video=%d", !videoPath.isEmpty());
    emit changed();
    return true;
}

void SplitViewerMediaManager::windowReady(quint64 id, quintptr windowId)
{
    if (m_stopping || !m_targets.contains(id))
    {
        m_session->cancel(id);
        return;
    }
    const SplitViewerVideoTarget target = m_targets.take(id);
    if (validLeaf(target.m_node) && embedWindow(target.m_node, static_cast<WId>(windowId), target.m_path))
    {
        m_session->accept(id);
    }
    else
    {
        m_session->cancel(id);
        if (!m_stopping)
        {
            emit error(g_config.m_videoEmbedError + target.m_path);
        }
    }
}

bool SplitViewerMediaManager::release(SplitViewerCoreNode* leaf)
{
    cancelPending(leaf);
    QWidget* container = m_embedded->take(leaf);
    const bool restored = container ? m_session->releaseWindow(container) : false;
    ++m_revision;
    return restored;
}

bool SplitViewerMediaManager::windowValid(QWidget* container) const
{
    return m_session->windowValid(container);
}

void SplitViewerMediaManager::releaseAll()
{
    const QList<quint64> ids = m_targets.keys();
    m_targets.clear();
    foreach (quint64 id, ids)
    {
        m_session->cancel(id);
    }
    const QList<SplitViewerCoreNode*> nodes = m_embedded->keys();
    foreach (SplitViewerCoreNode* node, nodes)
    {
        release(node);
    }
    ++m_revision;
}

void SplitViewerMediaManager::rebind(SplitViewerCoreNode* previous, SplitViewerCoreNode* replacement)
{
    if (previous == replacement || !replacement)
    {
        return;
    }
    if (m_embedded->contains(previous))
    {
        m_embedded->insert(replacement, m_embedded->take(previous));
    }
    for (auto it = m_targets.begin(); it != m_targets.end(); ++it)
    {
        if (it.value().m_node == previous)
        {
            it.value().m_node = replacement;
        }
    }
    ++m_revision;
}

void SplitViewerMediaManager::restoreVideos()
{
    std::vector<SplitViewerCoreNode*> leaves;
    SplitViewerCoreNode::collectLeaves(m_document->baseRoot(), leaves);
    for (int i = 0; i < m_document->layerCount(); ++i)
    {
        SplitViewerCoreNode::collectLeaves(m_document->layerAt(i)->root, leaves);
    }
    QStringList failures;
    foreach (SplitViewerCoreNode* leaf, leaves)
    {
        if (leaf->view.contentKind != SPLITVIEWER_CORE_CONTENT_VIDEO)
        {
            continue;
        }
        const QString path = QString::fromStdWString(leaf->view.path);
        const bool missingPlayer = playerPath().isEmpty();
        if (missingPlayer || !QFileInfo(path).isFile())
        {
            failures.append(missingPlayer ? g_config.m_videoPlayerMissingError + QLatin1Char(' ') + path
                                          : g_config.m_videoFileMissingError + path);
            continue;
        }
        load(leaf, path, true);
    }
    if (!failures.isEmpty())
    {
        emit error(failures.join(QLatin1Char('\n')));
    }
}

quint64 SplitViewerMediaManager::revision() const
{
    return m_revision;
}

void SplitViewerMediaManager::shutdown()
{
    if (!m_stopping)
    {
        m_stopping = true;
        releaseAll();
        m_session->shutdown();
    }
}

bool SplitViewerMediaManager::busy() const
{
    return m_session->busy();
}