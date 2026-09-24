﻿#include "SplitViewerRenderer.h"
#include "SplitViewerLayoutHelper.h"
#include "SplitViewerImageHelper.h"
#include "SplitViewerPlatform.h"
#include <algorithm>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

SplitViewerRenderer::SplitViewerRenderer(SplitViewerCoreDocument& document, const QMap<QString,QImage>& images,
    QMap<SplitViewerCoreNode*,QWidget*>& embedded, const QPoint& mouse, bool exporting, bool includeEmbedded) :
    m_document(document), m_imageCache(images), m_embedded(embedded), m_mouse(mouse),
    m_exporting(exporting), m_includeEmbedded(includeEmbedded)
{
}

void SplitViewerRenderer::drawStage(QPainter& painter, const QRectF& stage)
{
    painter.save();
    painter.setClipRect(stage, Qt::IntersectClip);
    painter.fillRect(stage, QColor(128,128,128));
    drawNode(painter, m_document.baseRoot(), SplitViewerLayoutHelper::contentRect(stage, m_document.borderVisible()), -1);
    if (m_document.borderVisible())
    {
        painter.setPen(QPen(QColor(32,32,32), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(stage.adjusted(0.5,0.5,-0.5,-0.5));
    }
    for (int i=0; i<m_document.layerCount(); ++i)
    {
        SplitViewerCoreLayer* layer=m_document.layerAt(i);
        const QRectF owner=SplitViewerLayoutHelper::normalizedToPixel(layer->rect, stage);
        painter.save();
        painter.setClipRect(owner, Qt::IntersectClip);
        painter.fillRect(owner, QColor(128,128,128));
        drawNode(painter, layer->root, SplitViewerLayoutHelper::contentRect(owner, m_document.borderVisible()), i);
        painter.restore();
        if (m_document.borderVisible())
        {
            painter.setPen(QPen(i == m_document.selectedLayer() ? QColor(255,0,0) : QColor(32,32,32), 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(owner.adjusted(0.5,0.5,-0.5,-0.5));
        }
    }
    painter.restore();
}

void SplitViewerRenderer::drawNode(QPainter& painter, SplitViewerCoreNode* node, const QRectF& rect, int layerIndex)
{
    if (!node || rect.width() <= 0 || rect.height() <= 0)
    {
        return;
    }
    if (node->isLeaf())
    {
        painter.save();
        painter.setClipRect(rect, Qt::IntersectClip);
        drawLeaf(painter, node, rect, layerIndex);
        painter.restore();
        return;
    }
    QRectF first, splitter, second;
    SplitViewerLayoutHelper::nodeRects(rect, node, first, splitter, second, m_document.borderVisible());
    drawNode(painter, node->first, first, layerIndex);
    drawNode(painter, node->second, second, layerIndex);
    if (m_document.borderVisible())
    {
        const QPoint mouse=m_mouse;
        const bool hover=!m_exporting && splitter.adjusted(-4,-4,4,4).contains(mouse);
        painter.fillRect(splitter, hover ? QColor(214,214,214) : QColor(188,188,188));
    }
}

void SplitViewerRenderer::drawLeaf(QPainter& painter, SplitViewerCoreNode* node, const QRectF& rect, int layerIndex)
{
    Q_UNUSED(layerIndex);
    painter.fillRect(rect, QColor(128,128,128));
    if (node->view.hasImage)
    {
        const QImage image=m_imageCache.value(SplitViewerImageHelper::path(node->view.path));
        if (!image.isNull())
        {
            const double scale=node->view.autoFit ? SplitViewerCoreFitScale(image.width(), image.height(), rect.width(), rect.height()) : node->view.scale;
            const QSizeF size(image.width()*scale, image.height()*scale);
            const QPointF center=rect.center()+QPointF(node->view.offsetX,node->view.offsetY);
            if (scale < 1.0)
            {
                // Pre-filter large images before the final paint. QPainter's
                // bilinear transform alone can leave high-contrast edges
                // jagged after a strong reduction.
                const QSize filteredSize((std::max)(1, qRound(size.width())), (std::max)(1, qRound(size.height())));
                const QImage filtered=image.scaled(filteredSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                const QSizeF actualSize(filtered.size());
                painter.drawImage(QRectF(center-QPointF(actualSize.width()/2.0,actualSize.height()/2.0),actualSize), filtered);
            }
            else
            {
                painter.drawImage(QRectF(center-QPointF(size.width()/2,size.height()/2),size), image);
            }
            return;
        }
    }
    if (m_embedded.contains(node) && !SplitViewerForeignWindowValid(m_embedded.value(node)))
    {
        SplitViewerDetachForeignWindow(m_embedded.take(node));
        node->view.clear();
    }
    if (m_embedded.contains(node))
    {
        QWidget* container=m_embedded.value(node);
        if (!m_exporting)
        {
            container->show();
            container->raise();
            SplitViewerSyncForeignWindow(container, rect.toAlignedRect());
            return;
        }
        if (m_includeEmbedded)
        {
            painter.drawImage(rect, SplitViewerForeignWindowSnapshot(container));
            return;
        }
    }
    const QRectF plus=SplitViewerLayoutHelper::plusButtonRect(rect);
    if (plus.width() <= 8)
    {
        return;
    }
    painter.fillRect(plus, QColor(238,238,238));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(96,96,96),1));
    painter.drawRect(plus.adjusted(0.5,0.5,-0.5,-0.5));
    painter.setPen(QPen(QColor(70,70,70),3));
    painter.drawLine(plus.center()+QPointF(-10,0),plus.center()+QPointF(10,0));
    painter.drawLine(plus.center()+QPointF(0,-10),plus.center()+QPointF(0,10));
}