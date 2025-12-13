#ifndef COLORRAMPWIDGET_H
#define COLORRAMPWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#include "colorrampnode.h"
#include "uicomponents.h"

// Blender-style Color Ramp Widget
class ColorRampWidget : public QWidget {
    Q_OBJECT
public:
    explicit ColorRampWidget(ColorRampNode* node, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onAddStop();
    void onRemoveStop();
    void onPositionChanged(double pos);
    void onInterpolationChanged(int index);

private:
    ColorRampNode* m_node;
    int m_selectedStopIndex = -1;
    bool m_isDragging = false;
    
    // UI Controls
    QPushButton* m_addBtn;
    QPushButton* m_removeBtn;
    PopupAwareComboBox* m_interpolationCombo;
    QDoubleSpinBox* m_positionSpin;
    QPushButton* m_colorBtn;
    QLabel* m_indexLabel;
    
    // Geometry helpers
    QRect gradientRect() const;
    int stopToX(double pos) const;
    double xToStop(int x) const;
    QRect stopRect(int x) const;
    
    void updateUIState();

signals:
    void rampChanged();
};

#endif // COLORRAMPWIDGET_H
