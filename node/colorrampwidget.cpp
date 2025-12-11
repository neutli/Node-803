#include "colorrampwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QColorDialog>
#include <QLinearGradient>

ColorRampWidget::ColorRampWidget(ColorRampNode* node, QWidget* parent)
    : QWidget(parent), m_node(node)
{
    setFixedSize(180, 40); // Standard size for node widgets
    setMouseTracking(true);
}

int ColorRampWidget::stopToX(double pos) const {
    return static_cast<int>(pos * (width() - 10)) + 5;
}

double ColorRampWidget::xToStop(int x) const {
    return static_cast<double>(x - 5) / (width() - 10);
}

QRect ColorRampWidget::stopRect(int x) const {
    return QRect(x - 4, height() - 12, 8, 12);
}

void ColorRampWidget::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Draw Gradient Bar
    QRect barRect(5, 5, width() - 10, height() - 20);
    
    QLinearGradient gradient(barRect.left(), 0, barRect.right(), 0);
    auto stops = m_node->stops();
    if (stops.isEmpty()) {
        gradient.setColorAt(0, Qt::black);
        gradient.setColorAt(1, Qt::black);
    } else {
        for (const auto& stop : stops) {
            gradient.setColorAt(stop.position, stop.color);
        }
    }
    
    // Checkerboard background for alpha
    p.fillRect(barRect, QColor(60, 60, 60)); // Dark background
    
    p.setBrush(gradient);
    p.setPen(QPen(QColor(30, 30, 30), 1));
    p.drawRect(barRect);

    // Draw Stops
    for (int i = 0; i < stops.size(); ++i) {
        int x = stopToX(stops[i].position);
        QRect r = stopRect(x);
        
        // Triangle shape
        QPainterPath path;
        path.moveTo(x, r.top());
        path.lineTo(r.right(), r.bottom());
        path.lineTo(r.left(), r.bottom());
        path.closeSubpath();
        
        p.setBrush(stops[i].color);
        if (i == m_selectedStopIndex) {
            p.setPen(QPen(Qt::white, 2)); // Highlight selected
        } else {
            p.setPen(QPen(Qt::black, 1));
        }
        p.drawPath(path);
    }
}

void ColorRampWidget::mousePressEvent(QMouseEvent* event) {
    auto stops = m_node->stops();
    
    // Check if clicked on a stop
    for (int i = 0; i < stops.size(); ++i) {
        int x = stopToX(stops[i].position);
        if (stopRect(x).contains(event->pos())) {
            m_selectedStopIndex = i;
            m_isDragging = true;
            update();
            return;
        }
    }
    
    // Check if clicked on bar to add stop
    QRect barRect(5, 5, width() - 10, height() - 20);
    if (barRect.contains(event->pos())) {
        double pos = xToStop(event->pos().x());
        // Interpolate color at this position
        // For now, just duplicate nearest or default
        m_node->addStop(pos, Qt::gray); 
        m_selectedStopIndex = m_node->stops().size() - 1; // Select new stop (it might be reordered though)
        // Find the new index after sorting
        auto newStops = m_node->stops();
        for(int i=0; i<newStops.size(); ++i) {
             if(std::abs(newStops[i].position - pos) < 0.001) {
                 m_selectedStopIndex = i;
                 break;
             }
        }
        m_isDragging = true;
        update();
        emit rampChanged();
    }
}

void ColorRampWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && m_selectedStopIndex != -1) {
        double pos = xToStop(event->pos().x());
        m_node->setStopPosition(m_selectedStopIndex, pos);
        
        // Re-find index because sorting might have changed it
        // This is tricky. For now, let's assume the user drags slowly.
        // Ideally ColorRampNode should return the new index.
        // Or we re-search based on position.
        auto stops = m_node->stops();
        int bestIndex = -1;
        double minDiff = 1.0;
        for(int i=0; i<stops.size(); ++i) {
            double diff = std::abs(stops[i].position - pos);
            if(diff < minDiff) {
                minDiff = diff;
                bestIndex = i;
            }
        }
        m_selectedStopIndex = bestIndex;
        
        update();
        emit rampChanged();
    }
}

void ColorRampWidget::mouseReleaseEvent(QMouseEvent* event) {
    m_isDragging = false;
    emit rampChanged();
}

void ColorRampWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    auto stops = m_node->stops();
    for (int i = 0; i < stops.size(); ++i) {
        int x = stopToX(stops[i].position);
        if (stopRect(x).contains(event->pos())) {
            QColor newColor = QColorDialog::getColor(stops[i].color, nullptr, "Select Color", QColorDialog::DontUseNativeDialog);
            if (newColor.isValid()) {
                m_node->setStopColor(i, newColor);
                update();
                emit rampChanged();
            }
            return;
        }
    }
}
