#ifndef NODEGRAPHICSITEM_H
#define NODEGRAPHICSITEM_H

#include <QGraphicsObject>

#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include "node.h"

class NodeGraphicsSocket;
class ConnectionGraphicsItem;
class QGraphicsProxyWidget;
class QSlider;
class QDoubleSpinBox;

// ノードのビジュアル表現
class NodeGraphicsItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit NodeGraphicsItem(Node* node, QGraphicsItem* parent = nullptr);
    ~NodeGraphicsItem() override;
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    Node* node() const { return m_node; }
    
    // ソケットの位置を取得
    QPointF getSocketPosition(NodeSocket* socket) const;
    
    // プレビュー更新
    void updatePreview();

    // レイアウト更新
    void updateLayout();

signals:
    void parameterChanged();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    bool sceneEvent(QEvent *event) override;

private:
    void updateGeometry();
    
    Node* m_node;
    QGraphicsTextItem* m_titleItem;
    
    QVector<NodeGraphicsSocket*> m_inputSocketItems;
    QVector<NodeGraphicsSocket*> m_outputSocketItems;
    
    // パラメータウィジェット
    QVector<QGraphicsProxyWidget*> m_parameterWidgets;
    
    // プレビュー画像
    QPixmap m_previewPixmap;
    
    // サイズ
    qreal m_width;
    qreal m_height;
    qreal m_titleHeight;
    qreal m_socketSpacing;
    qreal m_previewSize;
    
    // Update debounce flag
    bool m_updatePending = false;
};

// ソケットのビジュアル表現
class NodeGraphicsSocket : public QGraphicsItem {
public:
    NodeGraphicsSocket(NodeSocket* socket, QGraphicsItem* parent = nullptr);
    ~NodeGraphicsSocket() override;
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    NodeSocket* socket() const { return m_socket; }
    QPointF centerPos() const;
    
    void addConnection(ConnectionGraphicsItem* connection);
    void removeConnection(ConnectionGraphicsItem* connection);
    void updateConnectionPositions();

private:
    NodeSocket* m_socket;
    QVector<ConnectionGraphicsItem*> m_connections;
    const int SOCKET_RADIUS = 6;
};

#endif // NODEGRAPHICSITEM_H
