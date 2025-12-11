#ifndef OUTPUTVIEWERWIDGET_H
#define OUTPUTVIEWERWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPoint>

// 出力ビューアーウィジェット
// 端をドラッグしてビューポート範囲を変更可能
class OutputViewerWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit OutputViewerWidget(QWidget* parent = nullptr);
    
    void setImage(const QImage& image);
    void setSourceImage(const QImage& image);  // Source image shown as background
    QImage image() const { return m_image; }
    
signals:
    void viewportChanged();  // Emitted when viewport range is changed by drag

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;  // Reset on double-click
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    enum DragEdge {
        None = 0,
        Left = 1,
        Right = 2,
        Top = 4,
        Bottom = 8
    };
    
    DragEdge hitTest(const QPoint& pos);
    void updateCursor(DragEdge edge);
    QRect imageRect() const;  // Get the rect where image is drawn
    
    QImage m_image;
    QImage m_sourceImage;  // Original source image for background
    QPoint m_dragStart;
    DragEdge m_dragEdge;
    bool m_isDragging;
    
    // Panning support
    bool m_isPanning;
    QPoint m_panStart;
    QPointF m_panOffset;  // Current pan offset in UV space
    
    // Zoom
    double m_zoom;
    
    // Edge detection margin
    static const int EDGE_MARGIN = 10;
};

#endif // OUTPUTVIEWERWIDGET_H
