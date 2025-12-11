#include "connectiongraphicsitem.h"
#include "nodegraphicsitem.h"
#include <QPainterPath>

ConnectionGraphicsItem::ConnectionGraphicsItem(NodeGraphicsSocket* from, NodeGraphicsSocket* to, QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , m_from(from)
    , m_to(to)
    , m_isDragging(to == nullptr)
{
    setPen(QPen(QColor(200, 200, 200), 2));
    setZValue(-1); // ノードの下に描画
    
    // 接続を選択可能にする
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setAcceptHoverEvents(true);
    
    if (m_from) m_from->addConnection(this);
    if (m_to) m_to->addConnection(this);
    
    updatePath();
}

ConnectionGraphicsItem::~ConnectionGraphicsItem() {
    if (m_from) {
        m_from->removeConnection(this);
    }
    if (m_to) {
        m_to->removeConnection(this);
    }
}

void ConnectionGraphicsItem::setEndPoint(const QPointF& endPoint) {
    m_endPoint = endPoint;
    updatePath();
}

void ConnectionGraphicsItem::onSocketDeleted(const NodeGraphicsSocket* socket) {
    if (m_from == socket) {
        m_from = nullptr;
    }
    if (m_to == socket) {
        m_to = nullptr;
    }
    updatePath();
}

void ConnectionGraphicsItem::updatePath() {
    QPointF startPos;
    QPointF endPos;
    
    if (m_from) {
        startPos = m_from->centerPos();
        // シーン座標に変換
        if (m_from->parentItem()) {
            startPos = m_from->parentItem()->mapToScene(startPos);
        }
    }
    
    if (m_to) {
        endPos = m_to->centerPos();
        // シーン座標に変換
        if (m_to->parentItem()) {
            endPos = m_to->parentItem()->mapToScene(endPos);
        }
    } else if (m_isDragging) {
        endPos = m_endPoint;
    } else {
        return;
    }
    
    QPainterPath path;
    path.moveTo(startPos);
    
    qreal dx = endPos.x() - startPos.x();
    qreal dy = endPos.y() - startPos.y();
    
    QPointF ctrl1(startPos.x() + dx * 0.5, startPos.y());
    QPointF ctrl2(endPos.x() - dx * 0.5, endPos.y());
    
    path.cubicTo(ctrl1, ctrl2, endPos);
    
    setPath(path);
}
