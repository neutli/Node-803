#include "outputviewerwidget.h"
#include "appsettings.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <cmath>

OutputViewerWidget::OutputViewerWidget(QWidget* parent)
    : QWidget(parent)
    , m_dragEdge(None)
    , m_isDragging(false)
    , m_isPanning(false)
    , m_zoom(1.0)
{
    setMouseTracking(true);
    setMinimumSize(100, 100);
    setFocusPolicy(Qt::StrongFocus);
    
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30, 30, 30));
    setPalette(pal);
}

void OutputViewerWidget::setImage(const QImage& image) {
    m_image = image;
    update();
}

void OutputViewerWidget::setSourceImage(const QImage& image) {
    m_sourceImage = image;
    update();
}

QRect OutputViewerWidget::imageRect() const {
    if (m_image.isNull()) return QRect();
    
    // Fit image to widget while maintaining aspect ratio
    double widgetAspect = (double)width() / height();
    double imageAspect = (double)m_image.width() / m_image.height();
    
    int imgW, imgH;
    if (imageAspect > widgetAspect) {
        // Image is wider - fit to width
        imgW = width() - 40;  // Leave margin for handles
        imgH = imgW / imageAspect;
    } else {
        // Image is taller - fit to height
        imgH = height() - 40;
        imgW = imgH * imageAspect;
    }
    
    imgW *= m_zoom;
    imgH *= m_zoom;
    
    int x = (width() - imgW) / 2;
    int y = (height() - imgH) / 2;
    
    return QRect(x, y, imgW, imgH);
}

void OutputViewerWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    
    if (m_image.isNull()) {
        p.setPen(QColor(100, 100, 100));
        p.drawText(rect(), Qt::AlignCenter, "No output\nConnect nodes and run");
        return;
    }
    
    QRect imgRect = imageRect();
    
    // Draw source image as background if available (darker)
    if (!m_sourceImage.isNull()) {
        p.setOpacity(0.3);
        p.drawImage(rect(), m_sourceImage);
        p.setOpacity(1.0);
    }
    
    // Draw the rendered output image
    p.drawImage(imgRect, m_image);
    
    // Draw border around the image (this is what you can drag)
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    DragEdge hoverEdge = hitTest(mousePos);
    
    const int handleSize = 8;
    QColor normalColor(100, 150, 255);
    QColor hoverColor(255, 180, 50);
    
    // Left edge
    QPen leftPen((hoverEdge & Left) ? hoverColor : normalColor, handleSize);
    p.setPen(leftPen);
    p.drawLine(imgRect.left(), imgRect.top(), imgRect.left(), imgRect.bottom());
    
    // Right edge
    QPen rightPen((hoverEdge & Right) ? hoverColor : normalColor, handleSize);
    p.setPen(rightPen);
    p.drawLine(imgRect.right(), imgRect.top(), imgRect.right(), imgRect.bottom());
    
    // Top edge
    QPen topPen((hoverEdge & Top) ? hoverColor : normalColor, handleSize);
    p.setPen(topPen);
    p.drawLine(imgRect.left(), imgRect.top(), imgRect.right(), imgRect.top());
    
    // Bottom edge
    QPen bottomPen((hoverEdge & Bottom) ? hoverColor : normalColor, handleSize);
    p.setPen(bottomPen);
    p.drawLine(imgRect.left(), imgRect.bottom(), imgRect.right(), imgRect.bottom());
    
    // Draw info
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 9));
    QString info = QString("UV: [%1,%2]-[%3,%4] | ダブルクリックでリセット")
        .arg(AppSettings::instance().viewportMinU(), 0, 'f', 2)
        .arg(AppSettings::instance().viewportMinV(), 0, 'f', 2)
        .arg(AppSettings::instance().viewportMaxU(), 0, 'f', 2)
        .arg(AppSettings::instance().viewportMaxV(), 0, 'f', 2);
    
    QRect infoRect(5, height() - 22, width() - 10, 18);
    p.fillRect(infoRect, QColor(0, 0, 0, 180));
    p.drawText(infoRect, Qt::AlignVCenter | Qt::AlignLeft, info);
}

OutputViewerWidget::DragEdge OutputViewerWidget::hitTest(const QPoint& pos) {
    QRect imgRect = imageRect();
    if (imgRect.isEmpty()) return None;
    
    DragEdge edge = None;
    const int margin = 15;  // Larger margin for easier grabbing
    
    // Only detect edges near the image rect, not the whole widget
    bool nearLeft = std::abs(pos.x() - imgRect.left()) < margin;
    bool nearRight = std::abs(pos.x() - imgRect.right()) < margin;
    bool nearTop = std::abs(pos.y() - imgRect.top()) < margin;
    bool nearBottom = std::abs(pos.y() - imgRect.bottom()) < margin;
    
    bool inVerticalRange = pos.y() >= imgRect.top() - margin && pos.y() <= imgRect.bottom() + margin;
    bool inHorizontalRange = pos.x() >= imgRect.left() - margin && pos.x() <= imgRect.right() + margin;
    
    if (nearLeft && inVerticalRange) edge = (DragEdge)(edge | Left);
    if (nearRight && inVerticalRange) edge = (DragEdge)(edge | Right);
    if (nearTop && inHorizontalRange) edge = (DragEdge)(edge | Top);
    if (nearBottom && inHorizontalRange) edge = (DragEdge)(edge | Bottom);
    
    return edge;
}

void OutputViewerWidget::updateCursor(DragEdge edge) {
    if (edge == None) {
        setCursor(Qt::ArrowCursor);
    } else if ((edge & Left) && (edge & Top)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if ((edge & Right) && (edge & Bottom)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if ((edge & Left) && (edge & Bottom)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if ((edge & Right) && (edge & Top)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if ((edge & Left) || (edge & Right)) {
        setCursor(Qt::SizeHorCursor);
    } else if ((edge & Top) || (edge & Bottom)) {
        setCursor(Qt::SizeVerCursor);
    }
}

void OutputViewerWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragEdge = hitTest(event->pos());
        if (m_dragEdge != None) {
            m_isDragging = true;
            m_dragStart = event->pos();
        }
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_panStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void OutputViewerWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    // Double-click to reset viewport to 0-1
    AppSettings::instance().setViewportMinU(0.0);
    AppSettings::instance().setViewportMinV(0.0);
    AppSettings::instance().setViewportMaxU(1.0);
    AppSettings::instance().setViewportMaxV(1.0);
    emit viewportChanged();
    update();
}

void OutputViewerWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        QPoint delta = event->pos() - m_dragStart;
        
        double sensitivity = 0.003;
        double uvDeltaX = (double)delta.x() * sensitivity;
        double uvDeltaY = (double)delta.y() * sensitivity;
        
        double minU = AppSettings::instance().viewportMinU();
        double maxU = AppSettings::instance().viewportMaxU();
        double minV = AppSettings::instance().viewportMinV();
        double maxV = AppSettings::instance().viewportMaxV();
        
        if (m_dragEdge & Left) {
            double newMin = minU - uvDeltaX;
            if (newMin < maxU - 0.05) {
                AppSettings::instance().setViewportMinU(newMin);
            }
        }
        if (m_dragEdge & Right) {
            double newMax = maxU + uvDeltaX;
            if (newMax > minU + 0.05) {
                AppSettings::instance().setViewportMaxU(newMax);
            }
        }
        if (m_dragEdge & Top) {
            double newMin = minV - uvDeltaY;
            if (newMin < maxV - 0.05) {
                AppSettings::instance().setViewportMinV(newMin);
            }
        }
        if (m_dragEdge & Bottom) {
            double newMax = maxV + uvDeltaY;
            if (newMax > minV + 0.05) {
                AppSettings::instance().setViewportMaxV(newMax);
            }
        }
        
        m_dragStart = event->pos();
        
        // Real-time update - emit signal during drag
        emit viewportChanged();
        update();
    } else if (m_isPanning) {
        QPoint delta = event->pos() - m_panStart;
        
        double sensitivity = 0.002;
        double uvDeltaX = -(double)delta.x() * sensitivity;
        double uvDeltaY = -(double)delta.y() * sensitivity;
        
        double minU = AppSettings::instance().viewportMinU();
        double maxU = AppSettings::instance().viewportMaxU();
        double minV = AppSettings::instance().viewportMinV();
        double maxV = AppSettings::instance().viewportMaxV();
        
        AppSettings::instance().setViewportMinU(minU + uvDeltaX);
        AppSettings::instance().setViewportMaxU(maxU + uvDeltaX);
        AppSettings::instance().setViewportMinV(minV + uvDeltaY);
        AppSettings::instance().setViewportMaxV(maxV + uvDeltaY);
        
        m_panStart = event->pos();
        
        // Real-time update
        emit viewportChanged();
        update();
    } else {
        DragEdge edge = hitTest(event->pos());
        updateCursor(edge);
        update();
    }
}

void OutputViewerWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_dragEdge = None;
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void OutputViewerWidget::wheelEvent(QWheelEvent* event) {
    double delta = event->angleDelta().y() / 120.0;
    double factor = 1.0 - delta * 0.1;
    
    double minU = AppSettings::instance().viewportMinU();
    double maxU = AppSettings::instance().viewportMaxU();
    double minV = AppSettings::instance().viewportMinV();
    double maxV = AppSettings::instance().viewportMaxV();
    
    double centerU = (minU + maxU) / 2.0;
    double centerV = (minV + maxV) / 2.0;
    double rangeU = (maxU - minU) * factor;
    double rangeV = (maxV - minV) * factor;
    
    AppSettings::instance().setViewportMinU(centerU - rangeU / 2.0);
    AppSettings::instance().setViewportMaxU(centerU + rangeU / 2.0);
    AppSettings::instance().setViewportMinV(centerV - rangeV / 2.0);
    AppSettings::instance().setViewportMaxV(centerV + rangeV / 2.0);
    
    emit viewportChanged();
    update();
}

void OutputViewerWidget::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event);
    update();
}
