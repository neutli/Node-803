#ifndef WATERSOURCERAMPWIDGET_H
#define WATERSOURCERAMPWIDGET_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include "watersourcenode.h"

// Forward declaration
class GradientDisplayWidget;

class WaterSourceRampWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaterSourceRampWidget(WaterSourceNode* node, QWidget* parent = nullptr);

private slots:
    void onAddStop();
    void onRemoveStop();
    void onStopSelected(int index); // Called when user clicks a stop
    void onStopMoved(int index, double pos); // Called when stop is dragged
    void onPosSpinChanged(double val);
    void onColorBtnClicked();
    
    void updateUI(); // Update spinbox/buttons based on selection

private:
    WaterSourceNode* m_node;
    GradientDisplayWidget* m_display;
    
    // UI Elements
    QPushButton* m_btnAdd;
    QPushButton* m_btnRemove;
    QComboBox* m_comboInterpolation;
    QDoubleSpinBox* m_spinPos;
    QPushButton* m_btnColor;
    
    int m_currentSelection = -1;

signals:
    void rampChanged();
};

// Internal widget responsible for drawing the ramp and stops
class GradientDisplayWidget : public QWidget {
    Q_OBJECT
public:
    explicit GradientDisplayWidget(WaterSourceNode* node, QWidget* parent = nullptr);
    
    int selectedIndex() const { return m_selectedStopIndex; }
    void setSelectedIndex(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    WaterSourceNode* m_node;
    int m_selectedStopIndex = -1;
    bool m_isDragging = false;
    
    int stopToX(double pos) const;
    double xToStop(int x) const;
    QRect stopRect(int x) const;

signals:
    void stopSelected(int index);
    void stopMoved(int index, double pos);
    void rampChanged();
    void doubleClicked(int index);
};

#endif // WATERSOURCERAMPWIDGET_H
