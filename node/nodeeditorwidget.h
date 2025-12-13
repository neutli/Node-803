#ifndef NODEEDITORWIDGET_H
#define NODEEDITORWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QUndoStack>
#include "node.h"
#include "nodegraphicsitem.h"

// ノードエディタのメインウィジェット
class NodeEditorWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit NodeEditorWidget(QWidget* parent = nullptr);
    ~NodeEditorWidget() override;
    
    // ノード管理
    void addNode(Node* node, const QPointF& position = QPointF(0, 0));
    void removeNode(Node* node);
    void detachNode(Node* node); // Remove from scene but don't delete
    void updateNodePosition(Node* node); // Update graphics item position
    
    void createConnection(NodeSocket* from, NodeSocket* to);
    void removeConnection(NodeSocket* from, NodeSocket* to);
    void clear();
    
    QList<Node*> nodes() const { return m_nodes; }
    QList<NodeConnection*> connections() const { return m_connections; }

    // Undo/Redo
    QUndoStack* undoStack() const { return m_undoStack; }

    // Serialization
    void saveToFile(const QString& filename);
    void loadFromFile(const QString& filename);
    void loadFromData(const QByteArray& data);
    
    // FPS Control
    void setShowFPS(bool show) { m_showFPS = show; update(); }
    
    // Theme Control
    void updateTheme();
    
    // Batch Import
    // Batch Import
    void addMultipleImageNodes(const QStringList& filePaths);
    void showBulkNodeAddDialog(); // Dialog to select multiple node types to add
    void addBulkNodes(const QStringList& nodeNames, const QPointF& startPos);

signals:
    void parameterChanged(); // Emitted when any parameter changes

protected:
    // イベントハンドラ
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void setupScene();
    void drawGrid(QPainter* painter, const QRectF& rect);
    NodeGraphicsSocket* socketAt(const QPoint& pos);
    void tryAutoConnect(Node* node);
    void showNodeSearchMenu(const QPoint& pos);
    void showNodeCategoryMenu(const QPoint& pos);
    QString showNodeSearchMenuForConnection(const QPoint& pos, NodeSocket* dragSocket);
    
    QGraphicsScene* m_scene;
    QList<Node*> m_nodes;
    QList<NodeConnection*> m_connections;
    QList<NodeGraphicsItem*> m_nodeItems;
    QUndoStack* m_undoStack;
    
    // Interaction state
    class ConnectionGraphicsItem* m_tempConnection;
    NodeGraphicsSocket* m_dragSourceSocket;
    bool m_isPanning;
    bool m_spacePressed;
    QPoint m_lastPanPoint;
    qreal m_zoomFactor;
    const qreal MIN_ZOOM = 0.1;
    const qreal MAX_ZOOM = 2.0;

    // FPS
    bool m_showFPS = false;
    int m_frameCount = 0;
    qint64 m_lastFrameTime = 0;
    double m_fps = 0.0;

    // Undo/Redo state
    QMap<Node*, QPointF> m_initialNodePositions;
    
    // Socket quick-connect
    NodeGraphicsSocket* m_selectedSocket = nullptr;
};

#endif // NODEEDITORWIDGET_H
