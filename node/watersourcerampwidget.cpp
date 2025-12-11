#include "watersourcerampwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QColorDialog>
#include <QLinearGradient>
#include <QStyle>
#include <QStyleOption>
#include <QDebug>

// ============================================================================
// GradientDisplayWidget Implementation
// ============================================================================

GradientDisplayWidget::GradientDisplayWidget(WaterSourceNode* node, QWidget* parent)
    : QWidget(parent), m_node(node)
{
    setFixedHeight(24); // Taller for better visibility
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void GradientDisplayWidget::setSelectedIndex(int index) {
    if (m_selectedStopIndex != index) {
        m_selectedStopIndex = index;
        update();
        emit stopSelected(index);
    }
}

int GradientDisplayWidget::stopToX(double pos) const {
    return static_cast<int>(pos * (width() - 10)) + 5;
}

double GradientDisplayWidget::xToStop(int x) const {
    return static_cast<double>(x - 5) / (width() - 10);
}

QRect GradientDisplayWidget::stopRect(int x) const {
    // Arrow-like shape area at the bottom
    return QRect(x - 5, height() - 10, 10, 10);
}

void GradientDisplayWidget::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Ensure we have stops to draw
    auto stops = m_node->stops();
    
    // 1. Draw Checkerboard Background (for alpha)
    QRect barRect(5, 2, width() - 10, height() - 14);
    
    // Draw checkerboard manually or fill with dark gray for now
    p.fillRect(barRect, QColor(40, 40, 40)); 
    
    // 2. Draw Gradient
    QLinearGradient gradient(barRect.left(), 0, barRect.right(), 0);
    if (stops.isEmpty()) {
        gradient.setColorAt(0, Qt::black);
        gradient.setColorAt(1, Qt::black);
    } else {
        for (const auto& stop : stops) {
            gradient.setColorAt(stop.position, stop.color);
        }
    }
    p.setBrush(gradient);
    p.setPen(QPen(QColor(20, 20, 20), 1));
    p.drawRect(barRect);

    // 3. Draw Stops
    for (int i = 0; i < stops.size(); ++i) {
        int x = stopToX(stops[i].position);
        
        // Draw the stop indicator
        // Blender style: A small triangle/arrow pointing up to the bar
        
        QPainterPath path;
        // Triangle pointing up
        path.moveTo(x, barRect.bottom() + 1); 
        path.lineTo(x - 5, height() - 2);
        path.lineTo(x + 5, height() - 2);
        path.closeSubpath();
        
        // Selection highlight
        if (i == m_selectedStopIndex) {
            p.setBrush(Qt::white); // Active stop is lighter
            p.setPen(QPen(Qt::black, 1));
        } else {
            p.setBrush(QColor(120, 120, 120)); // Inactive stop
            p.setPen(QPen(Qt::black, 1));
        }
        p.drawPath(path);
        
        // Draw small color rect inside the indicator showing the stop color
        QRectF colorRect(x - 2, height() - 8, 4, 4);
         // p.fillRect(colorRect, stops[i].color); // Optional: small color dot
    }
}

void GradientDisplayWidget::mousePressEvent(QMouseEvent* event) {
    auto stops = m_node->stops();
    
    // Check if clicked on a stop handle
    // Iterate in reverse to select top-most if overlapping
    for (int i = stops.size() - 1; i >= 0; --i) {
        int x = stopToX(stops[i].position);
        QRect hitArea(x - 6, height() - 12, 12, 12); // Slightly larger hit area
        
        if (hitArea.contains(event->pos())) {
            m_selectedStopIndex = i;
            m_isDragging = true;
            update();
            emit stopSelected(i);
            return;
        }
    }
    
    // If clicked on the bar, maybe add a new stop?
    // Blender behavior: Ctrl+Click to add stop
    // Standard Qt behavior usually allows clicking empty space to add
    QRect barRect(5, 2, width() - 10, height() - 14);
    if (barRect.contains(event->pos())) {
         // Deselect or maybe add stop logic handled by parent via double click?
         // For now just deselect
         // m_selectedStopIndex = -1;
         // update();
         // emit stopSelected(-1);
    }
}

void GradientDisplayWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && m_selectedStopIndex != -1) {
        double pos = xToStop(event->pos().x());
        // Clamp 0..1
        pos = std::max(0.0, std::min(1.0, pos));
        
        emit stopMoved(m_selectedStopIndex, pos);
    }
}

void GradientDisplayWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isDragging) {
        m_isDragging = false;
        emit rampChanged(); // Finalize change
    }
}

void GradientDisplayWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    // Double click to open color dialog is handled by parent connected to this signal
    // Or check if stop was hit
    auto stops = m_node->stops();
    for (int i = stops.size() - 1; i >= 0; --i) {
        int x = stopToX(stops[i].position);
        QRect hitArea(x - 6, height() - 12, 12, 12);
        if (hitArea.contains(event->pos())) {
             emit doubleClicked(i);
             return;
        }
    }
    
    // If double click on bar, add stop
    QRect barRect(5, 2, width() - 10, height() - 14);
    if (barRect.contains(event->pos())) {
        double pos = xToStop(event->pos().x());
        pos = std::max(0.0, std::min(1.0, pos));
        
        // Add stop logic needs color interpolation ideally
        m_node->addStop(pos, Qt::gray); // Parent should handle smarts, but here we just add gray
        
        // Need to find the new index
        auto newStops = m_node->stops();
        int bestIdx = -1;
        double minD = 100.0;
        for(int k=0; k<newStops.size(); ++k){
            if(std::abs(newStops[k].position - pos) < minD){
                minD = std::abs(newStops[k].position - pos);
                bestIdx = k;
            }
        }
        
        m_selectedStopIndex = bestIdx;
        update();
        emit stopSelected(bestIdx);
        emit rampChanged();
    }
}


// ============================================================================
// WaterSourceRampWidget Implementation (Container)
// ============================================================================

WaterSourceRampWidget::WaterSourceRampWidget(WaterSourceNode* node, QWidget* parent)
    : QWidget(parent), m_node(node)
{
    // Fix layout margin/spacing
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    // === 1. Toolbar (Add/Remove) ===
    auto toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(2);
    
    m_btnAdd = new QPushButton("+", this);
    m_btnAdd->setFixedSize(20, 20);
    m_btnAdd->setToolTip("Add Stop");
    
    m_btnRemove = new QPushButton("-", this);
    m_btnRemove->setFixedSize(20, 20);
    m_btnRemove->setToolTip("Delete Selected Stop");
    
    /* Interpolation combo (Future)
    m_comboInterpolation = new QComboBox(this);
    m_comboInterpolation->addItem("Linear");
    m_comboInterpolation->addItem("Constant"); */
    
    toolbarLayout->addWidget(m_btnAdd);
    toolbarLayout->addWidget(m_btnRemove);
    // toolbarLayout->addWidget(m_comboInterpolation);
    toolbarLayout->addStretch();
    
    mainLayout->addLayout(toolbarLayout);

    // === 2. Gradient Display ===
    m_display = new GradientDisplayWidget(node, this);
    mainLayout->addWidget(m_display);

    // === 3. Properties (Pos, Color) ===
    auto propsLayout = new QHBoxLayout();
    propsLayout->setSpacing(4);
    
    // Position
    auto lblPos = new QLabel("Pos:", this);
    lblPos->setStyleSheet("color: #cccccc; font-size: 8pt;");
    m_spinPos = new QDoubleSpinBox(this);
    m_spinPos->setRange(0.0, 1.0);
    m_spinPos->setSingleStep(0.01);
    m_spinPos->setDecimals(3);
    m_spinPos->setButtonSymbols(QAbstractSpinBox::NoButtons); // Blender Compact style
    m_spinPos->setFixedHeight(20);
    // Style spinbox to overlap arrow buttons or just plain
    
    // Color
    auto lblColor = new QLabel("Color:", this);
    lblColor->setStyleSheet("color: #cccccc; font-size: 8pt;");
    m_btnColor = new QPushButton(this);
    m_btnColor->setFixedSize(40, 20);
    m_btnColor->setStyleSheet("border: 1px solid #555; background-color: #000;");
    
    propsLayout->addWidget(lblPos);
    propsLayout->addWidget(m_spinPos);
    propsLayout->addSpacing(8);
    propsLayout->addWidget(lblColor);
    propsLayout->addWidget(m_btnColor);
    propsLayout->addStretch();
    
    mainLayout->addLayout(propsLayout);

    // Connect Signals
    connect(m_btnAdd, &QPushButton::clicked, this, &WaterSourceRampWidget::onAddStop);
    connect(m_btnRemove, &QPushButton::clicked, this, &WaterSourceRampWidget::onRemoveStop);
    
    connect(m_display, &GradientDisplayWidget::stopSelected, this, &WaterSourceRampWidget::onStopSelected);
    connect(m_display, &GradientDisplayWidget::stopMoved, this, &WaterSourceRampWidget::onStopMoved);
    connect(m_display, &GradientDisplayWidget::rampChanged, this, [this](){ emit rampChanged(); });
    connect(m_display, &GradientDisplayWidget::doubleClicked, this, [this](int idx){ 
        m_currentSelection = idx; 
        onColorBtnClicked(); 
    });
    
    connect(m_spinPos, &QDoubleSpinBox::valueChanged, this, &WaterSourceRampWidget::onPosSpinChanged);
    connect(m_btnColor, &QPushButton::clicked, this, &WaterSourceRampWidget::onColorBtnClicked);
    
    // Initial State
    updateUI();
    
    // Adjust total height
    // Toolbar(20) + Display(24) + Props(20) + Spacing ~ 70-80
    setFixedHeight(80); 
}

void WaterSourceRampWidget::onAddStop() {
    m_node->addStop(0.5, Qt::gray);
    emit rampChanged();
    m_display->update();
    // Ideally select the new stop, simplify logic later
}

void WaterSourceRampWidget::onRemoveStop() {
    if (m_currentSelection >= 0 && m_node->stops().size() > 1) {
        m_node->removeStop(m_currentSelection);
        m_currentSelection = -1;
        m_display->setSelectedIndex(-1);
        emit rampChanged();
        m_display->update();
        updateUI();
    }
}

void WaterSourceRampWidget::onStopSelected(int index) {
    m_currentSelection = index;
    updateUI();
}

void WaterSourceRampWidget::onStopMoved(int index, double pos) {
    m_currentSelection = index; // Ensure selection matches drag
    
    // Update node
    m_node->setStopPosition(index, pos);
    
    // Re-sync index since stops are sorted by position
    // Simple heuristic: find stop with closest pos
    auto stops = m_node->stops();
    int newIndex = -1;
    double minD = 100.0;
    for(int i=0; i<stops.size(); ++i){
        if(std::abs(stops[i].position - pos) < minD){
            minD = std::abs(stops[i].position - pos);
            newIndex = i;
        }
    }
    m_currentSelection = newIndex;
    m_display->setSelectedIndex(newIndex);
    
    updateUI(); // Updates spinbox visual only
    m_display->update(); // Redraw gradient
    emit rampChanged();
}

void WaterSourceRampWidget::onPosSpinChanged(double val) {
    if (m_currentSelection >= 0 && m_currentSelection < m_node->stops().size()) {
        // Only update if differ to verify loop
        double currentPos = m_node->stops()[m_currentSelection].position;
        if (std::abs(currentPos - val) > 0.0001) {
            onStopMoved(m_currentSelection, val);
        }
    }
}

void WaterSourceRampWidget::onColorBtnClicked() {
    if (m_currentSelection >= 0 && m_currentSelection < m_node->stops().size()) {
        QColor initial = m_node->stops()[m_currentSelection].color;
        
        // **BUG FIX**: Using DontUseNativeDialog may help with white screen issues
        QColor color = QColorDialog::getColor(initial, nullptr, "Select Stop Color", 
                                              QColorDialog::DontUseNativeDialog);
        
        if (color.isValid()) {
            m_node->setStopColor(m_currentSelection, color);
            updateUI(); // Update button color
            m_display->update();
            emit rampChanged();
        }
    }
}

void WaterSourceRampWidget::updateUI() {
    auto stops = m_node->stops();
    bool hasSelection = (m_currentSelection >= 0 && m_currentSelection < stops.size());
    
    m_btnRemove->setEnabled(hasSelection && stops.size() > 1);
    m_spinPos->setEnabled(hasSelection);
    m_btnColor->setEnabled(hasSelection);
    
    if (hasSelection) {
        const auto& stop = stops[m_currentSelection];
        
        // Block signals to prevent feedback loop
        m_spinPos->blockSignals(true);
        m_spinPos->setValue(stop.position);
        m_spinPos->blockSignals(false);
        
        // Update color button style
        QString style = QString("border: 1px solid #555; background-color: %1;").arg(stop.color.name());
        m_btnColor->setStyleSheet(style);
    } else {
        m_spinPos->clear();
        m_btnColor->setStyleSheet("border: 1px solid #555; background-color: #333;");
    }
}
