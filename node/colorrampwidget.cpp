#include "colorrampwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QColorDialog>
#include <QLinearGradient>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyleFactory>

ColorRampWidget::ColorRampWidget(ColorRampNode* node, QWidget* parent)
    : QWidget(parent), m_node(node)
{
    setFixedSize(220, 90);
    setMouseTracking(true);
    
    // Create controls
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(3);
    
    // Top row: +/- buttons and interpolation
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(3);
    
    m_addBtn = new QPushButton("+");
    m_addBtn->setFixedSize(24, 20);
    m_addBtn->setStyleSheet("QPushButton { background-color: #444; color: white; border: 1px solid #555; font-weight: bold; }");
    connect(m_addBtn, &QPushButton::clicked, this, &ColorRampWidget::onAddStop);
    
    m_removeBtn = new QPushButton("-");
    m_removeBtn->setFixedSize(24, 20);
    m_removeBtn->setStyleSheet("QPushButton { background-color: #444; color: white; border: 1px solid #555; font-weight: bold; }");
    connect(m_removeBtn, &QPushButton::clicked, this, &ColorRampWidget::onRemoveStop);
    
    // Use PopupAwareComboBox and Fusion style for better popup handling in GraphicsView
    m_interpolationCombo = new PopupAwareComboBox();
    m_interpolationCombo->setStyle(QStyleFactory::create("Fusion"));
    m_interpolationCombo->addItems({"Linear", "Constant", "Ease", "Cardinal"});
    m_interpolationCombo->setFixedHeight(20);
    m_interpolationCombo->setStyleSheet("QComboBox { background-color: #383838; color: white; border: 1px solid #555; font-size: 9pt; }");
    connect(m_interpolationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ColorRampWidget::onInterpolationChanged);
    
    topRow->addWidget(m_addBtn);
    topRow->addWidget(m_removeBtn);
    topRow->addWidget(m_interpolationCombo);
    topRow->addStretch();
    
    mainLayout->addLayout(topRow);
    
    // Gradient bar area (painted in paintEvent)
    mainLayout->addSpacing(30); // Space for gradient bar
    
    // Bottom row: index, position, color button
    QHBoxLayout* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(3);
    
    m_indexLabel = new QLabel("0");
    m_indexLabel->setFixedWidth(20);
    m_indexLabel->setStyleSheet("color: #aaa; font-size: 9pt;");
    
    QLabel* posLabel = new QLabel("Pos:");
    posLabel->setStyleSheet("color: #888; font-size: 8pt;");
    
    m_positionSpin = new QDoubleSpinBox();
    m_positionSpin->setRange(0.0, 1.0);
    m_positionSpin->setSingleStep(0.01);
    m_positionSpin->setDecimals(3);
    m_positionSpin->setFixedWidth(60);
    m_positionSpin->setStyleSheet("QDoubleSpinBox { background-color: #333; color: white; border: 1px solid #555; font-size: 9pt; }");
    connect(m_positionSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ColorRampWidget::onPositionChanged);
    
    m_colorBtn = new QPushButton();
    m_colorBtn->setFixedSize(40, 18);
    m_colorBtn->setStyleSheet("background-color: #808080; border: 1px solid #555;");
    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        if (m_selectedStopIndex >= 0 && m_selectedStopIndex < m_node->stops().size()) {
            QColor current = m_node->stops()[m_selectedStopIndex].color;
            QColor newColor = QColorDialog::getColor(current, nullptr, "Select Color", QColorDialog::DontUseNativeDialog);
            if (newColor.isValid()) {
                m_node->setStopColor(m_selectedStopIndex, newColor);
                updateUIState();
                update();
                emit rampChanged();
            }
        }
    });
    
    bottomRow->addWidget(m_indexLabel);
    bottomRow->addWidget(posLabel);
    bottomRow->addWidget(m_positionSpin);
    bottomRow->addWidget(m_colorBtn);
    bottomRow->addStretch();
    
    mainLayout->addLayout(bottomRow);
    
    updateUIState();
}

QRect ColorRampWidget::gradientRect() const {
    return QRect(5, 28, width() - 10, 22);
}

int ColorRampWidget::stopToX(double pos) const {
    QRect r = gradientRect();
    return r.left() + static_cast<int>(pos * r.width());
}

double ColorRampWidget::xToStop(int x) const {
    QRect r = gradientRect();
    return qBound(0.0, static_cast<double>(x - r.left()) / r.width(), 1.0);
}

QRect ColorRampWidget::stopRect(int x) const {
    QRect gr = gradientRect();
    return QRect(x - 5, gr.bottom() + 1, 10, 10);
}

void ColorRampWidget::updateUIState() {
    auto stops = m_node->stops();
    bool hasSelection = m_selectedStopIndex >= 0 && m_selectedStopIndex < stops.size();
    
    m_removeBtn->setEnabled(hasSelection && stops.size() > 2);
    m_positionSpin->setEnabled(hasSelection);
    m_colorBtn->setEnabled(hasSelection);
    
    if (hasSelection) {
        m_indexLabel->setText(QString::number(m_selectedStopIndex + 1));
        m_positionSpin->blockSignals(true);
        m_positionSpin->setValue(stops[m_selectedStopIndex].position);
        m_positionSpin->blockSignals(false);
        m_colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(stops[m_selectedStopIndex].color.name()));
    } else {
        m_indexLabel->setText("-");
        m_positionSpin->blockSignals(true);
        m_positionSpin->setValue(0.0);
        m_positionSpin->blockSignals(false);
        m_colorBtn->setStyleSheet("background-color: #404040; border: 1px solid #555;");
    }
}

void ColorRampWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    QRect barRect = gradientRect();
    
    // Draw gradient
    QLinearGradient gradient(barRect.left(), 0, barRect.right(), 0);
    auto stops = m_node->stops();
    if (stops.isEmpty()) {
        gradient.setColorAt(0, Qt::black);
        gradient.setColorAt(1, Qt::white);
    } else {
        for (const auto& stop : stops) {
            gradient.setColorAt(stop.position, stop.color);
        }
    }
    
    p.fillRect(barRect, QColor(40, 40, 40));
    p.setBrush(gradient);
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.drawRect(barRect);
    
    // Draw stop handles
    for (int i = 0; i < stops.size(); ++i) {
        int x = stopToX(stops[i].position);
        QRect r = stopRect(x);
        
        // Diamond shape
        QPainterPath path;
        path.moveTo(x, r.top());
        path.lineTo(r.right(), r.center().y());
        path.lineTo(x, r.bottom());
        path.lineTo(r.left(), r.center().y());
        path.closeSubpath();
        
        p.setBrush(stops[i].color);
        if (i == m_selectedStopIndex) {
            p.setPen(QPen(Qt::white, 2));
        } else {
            p.setPen(QPen(Qt::black, 1));
        }
        p.drawPath(path);
    }
}

void ColorRampWidget::mousePressEvent(QMouseEvent* event) {
    auto stops = m_node->stops();
    QRect gr = gradientRect();
    
    // Check stop handles
    for (int i = 0; i < stops.size(); ++i) {
        int x = stopToX(stops[i].position);
        if (stopRect(x).contains(event->pos())) {
            m_selectedStopIndex = i;
            m_isDragging = true;
            updateUIState();
            update();
            return;
        }
    }
    
    // Click on gradient bar adds new stop
    if (gr.contains(event->pos())) {
        double pos = xToStop(event->pos().x());
        m_node->addStop(pos, Qt::gray);
        
        // Find and select new stop
        auto newStops = m_node->stops();
        for (int i = 0; i < newStops.size(); ++i) {
            if (std::abs(newStops[i].position - pos) < 0.01) {
                m_selectedStopIndex = i;
                break;
            }
        }
        m_isDragging = true;
        updateUIState();
        update();
        emit rampChanged();
    }
}

void ColorRampWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && m_selectedStopIndex >= 0) {
        double pos = xToStop(event->pos().x());
        m_node->setStopPosition(m_selectedStopIndex, pos);
        
        // Re-find index after sorting
        auto stops = m_node->stops();
        for (int i = 0; i < stops.size(); ++i) {
            if (std::abs(stops[i].position - pos) < 0.001) {
                m_selectedStopIndex = i;
                break;
            }
        }
        
        updateUIState();
        update();
        emit rampChanged();
    }
}

void ColorRampWidget::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    m_isDragging = false;
}

void ColorRampWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    auto stops = m_node->stops();
    for (int i = 0; i < stops.size(); ++i) {
        int x = stopToX(stops[i].position);
        if (stopRect(x).contains(event->pos())) {
            QColor newColor = QColorDialog::getColor(stops[i].color, nullptr, "Select Color", QColorDialog::DontUseNativeDialog);
            if (newColor.isValid()) {
                m_node->setStopColor(i, newColor);
                updateUIState();
                update();
                emit rampChanged();
            }
            return;
        }
    }
}

void ColorRampWidget::onAddStop() {
    // Add at center between selected and next, or at 0.5
    auto stops = m_node->stops();
    double newPos = 0.5;
    if (m_selectedStopIndex >= 0 && m_selectedStopIndex < stops.size() - 1) {
        newPos = (stops[m_selectedStopIndex].position + stops[m_selectedStopIndex + 1].position) / 2.0;
    } else if (m_selectedStopIndex == stops.size() - 1 && stops.size() > 1) {
        newPos = (stops[m_selectedStopIndex - 1].position + stops[m_selectedStopIndex].position) / 2.0;
    }
    
    m_node->addStop(newPos, Qt::gray);
    
    // Select new stop
    auto newStops = m_node->stops();
    for (int i = 0; i < newStops.size(); ++i) {
        if (std::abs(newStops[i].position - newPos) < 0.01) {
            m_selectedStopIndex = i;
            break;
        }
    }
    
    updateUIState();
    update();
    emit rampChanged();
}

void ColorRampWidget::onRemoveStop() {
    if (m_selectedStopIndex >= 0 && m_node->stops().size() > 2) {
        m_node->removeStop(m_selectedStopIndex);
        m_selectedStopIndex = qMin(m_selectedStopIndex, m_node->stops().size() - 1);
        updateUIState();
        update();
        emit rampChanged();
    }
}

void ColorRampWidget::onPositionChanged(double pos) {
    if (m_selectedStopIndex >= 0 && m_selectedStopIndex < m_node->stops().size()) {
        m_node->setStopPosition(m_selectedStopIndex, pos);
        
        // Re-find after sorting
        auto stops = m_node->stops();
        for (int i = 0; i < stops.size(); ++i) {
            if (std::abs(stops[i].position - pos) < 0.001) {
                m_selectedStopIndex = i;
                break;
            }
        }
        
        updateUIState();
        update();
        emit rampChanged();
    }
}

void ColorRampWidget::onInterpolationChanged(int index) {
    // Future: store interpolation mode in node
    Q_UNUSED(index);
    emit rampChanged();
}
