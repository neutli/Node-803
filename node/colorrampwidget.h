#ifndef COLORRAMPWIDGET_H
#define COLORRAMPWIDGET_H

#include <QWidget>
#include "colorrampnode.h"

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

private:
    ColorRampNode* m_node;
    int m_selectedStopIndex = -1;
    bool m_isDragging = false;
    
    int stopToX(double pos) const;
    double xToStop(int x) const;
    QRect stopRect(int x) const;

signals:
    void rampChanged();
};

#endif // COLORRAMPWIDGET_H
