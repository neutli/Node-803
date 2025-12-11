#ifndef CONNECTIONGRAPHICSITEM_H
#define CONNECTIONGRAPHICSITEM_H

#include <QGraphicsPathItem>
#include <QPen>

class NodeGraphicsSocket;

class ConnectionGraphicsItem : public QGraphicsPathItem {
public:
    ConnectionGraphicsItem(NodeGraphicsSocket* from, NodeGraphicsSocket* to, QGraphicsItem* parent = nullptr);
    ~ConnectionGraphicsItem() override;
    
    void updatePath();
    
    NodeGraphicsSocket* fromSocket() const { return m_from; }
    NodeGraphicsSocket* toSocket() const { return m_to; }

    // ドラッグ中の更新用
    void setEndPoint(const QPointF& endPoint);

    // Socket deletion notification
    void onSocketDeleted(const NodeGraphicsSocket* socket);

private:
    NodeGraphicsSocket* m_from;
    NodeGraphicsSocket* m_to;
    QPointF m_endPoint;
    bool m_isDragging;
};

#endif // CONNECTIONGRAPHICSITEM_H
