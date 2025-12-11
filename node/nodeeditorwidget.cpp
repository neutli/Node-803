#include "nodeeditorwidget.h"
#include "appsettings.h"
#include "connectiongraphicsitem.h"
#include "nodegraphicsitem.h"
#include "noisetexturenode.h"
#include "rivernode.h"
#include "invertnode.h"
#include "mappingnode.h"
#include "texturecoordinatenode.h"
#include "sliderspinbox.h"
#include "sliderspinbox.h"
#include "noderegistry.h"
#include "commands.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QApplication>
#include <QLineEdit>
#include <QWidgetAction>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGraphicsProxyWidget>
#include <QGraphicsProxyWidget>
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QListWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>

NodeEditorWidget::NodeEditorWidget(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_tempConnection(nullptr)
    , m_dragSourceSocket(nullptr)
    , m_isPanning(false)
    , m_spacePressed(false)
    , m_zoomFactor(1.0)
    , m_undoStack(new QUndoStack(this))
{
    setFocusPolicy(Qt::StrongFocus);  // Enable keyboard events
    setupScene();
}

NodeEditorWidget::~NodeEditorWidget() {
    qDeleteAll(m_nodes);
    qDeleteAll(m_connections);
}

void NodeEditorWidget::setupScene() {
    // シーンの設定
    m_scene->setSceneRect(-5000, -5000, 10000, 10000);
    setScene(m_scene);
    
    // ビューの設定
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    // 背景色
    setBackgroundBrush(QBrush(QColor(40, 40, 40)));
    
    // ドラッグモード
    setDragMode(QGraphicsView::RubberBandDrag);
}

void NodeEditorWidget::addNode(Node* node, const QPointF& position) {
    node->setPosition(position);
    m_nodes.append(node);
    
    NodeGraphicsItem* item = new NodeGraphicsItem(node);
    item->setPos(position);
    m_scene->addItem(item);
    m_nodeItems.append(item);
    
    connect(item, &NodeGraphicsItem::parameterChanged, this, &NodeEditorWidget::parameterChanged);
}

void NodeEditorWidget::removeNode(Node* node) {
    // Disconnect all connections first
    QList<NodeConnection*> connectionsToRemove;
    for (NodeConnection* conn : m_connections) {
        if (conn->from()->parentNode() == node || conn->to()->parentNode() == node) {
            connectionsToRemove.append(conn);
        }
    }
    
    for (NodeConnection* conn : connectionsToRemove) {
        removeConnection(conn->from(), conn->to());
    }

    detachNode(node);
    delete node;
}

void NodeEditorWidget::detachNode(Node* node) {
    // ノードアイテムを削除
    for (int i = 0; i < m_nodeItems.size(); i++) {
        if (m_nodeItems[i]->node() == node) {
            m_scene->removeItem(m_nodeItems[i]);
            delete m_nodeItems[i];
            m_nodeItems.removeAt(i);
            break;
        }
    }
    
    // ノードをリストから削除（所有権は呼び出し元へ）
    m_nodes.removeAll(node);
}

void NodeEditorWidget::updateNodePosition(Node* node) {
    for (auto item : m_nodeItems) {
        if (item->node() == node) {
            item->setPos(node->position());
            break;
        }
    }
}

void NodeEditorWidget::createConnection(NodeSocket* from, NodeSocket* to) {
    if (!from || !to) return;
    
    // Ensure direction is correct (Output -> Input)
    if (from->direction() == SocketDirection::Input && to->direction() == SocketDirection::Output) {
        std::swap(from, to);
    }
    
    // Validate connection
    if (from->direction() == to->direction()) return;
    
    NodeConnection* connection = new NodeConnection(from, to);
    if (connection->isValid()) {
        m_connections.append(connection);
        
        // Find graphics items
        NodeGraphicsItem* fromItem = nullptr;
        NodeGraphicsItem* toItem = nullptr;
        
        for (auto item : m_nodeItems) {
            if (item->node() == from->parentNode()) fromItem = item;
            if (item->node() == to->parentNode()) toItem = item;
        }
        
        if (fromItem && toItem) {
            // Helper to find socket item
            auto findSocketItem = [](NodeGraphicsItem* nodeItem, NodeSocket* s) -> NodeGraphicsSocket* {
                for (QGraphicsItem* child : nodeItem->childItems()) {
                    if (NodeGraphicsSocket* gs = dynamic_cast<NodeGraphicsSocket*>(child)) {
                        if (gs->socket() == s) return gs;
                    }
                }
                return nullptr;
            };
            
            NodeGraphicsSocket* fromSocketItem = findSocketItem(fromItem, from);
            NodeGraphicsSocket* toSocketItem = findSocketItem(toItem, to);
            
            if (fromSocketItem && toSocketItem) {
                ConnectionGraphicsItem* item = new ConnectionGraphicsItem(fromSocketItem, toSocketItem);
                m_scene->addItem(item);
            }
        }
        
        to->parentNode()->setDirty(true);
        to->parentNode()->evaluate();
    } else {
        delete connection;
    }
}

void NodeEditorWidget::removeConnection(NodeSocket* from, NodeSocket* to) {
    // Find connection
    NodeConnection* targetConn = nullptr;
    for (NodeConnection* conn : m_connections) {
        if (conn->from() == from && conn->to() == to) {
            targetConn = conn;
            break;
        }
    }
    
    if (!targetConn) return;
    
    // Remove from data
    from->removeConnection(to);
    to->removeConnection(from);
    m_connections.removeAll(targetConn);
    
    // Remove graphics
    // We need to find the ConnectionGraphicsItem
    for (QGraphicsItem* item : m_scene->items()) {
        if (ConnectionGraphicsItem* connItem = dynamic_cast<ConnectionGraphicsItem*>(item)) {
            if (connItem->fromSocket()->socket() == from && connItem->toSocket()->socket() == to) {
                // Explicitly remove from sockets to prevent dangling pointers
                if (connItem->fromSocket()) connItem->fromSocket()->removeConnection(connItem);
                if (connItem->toSocket()) connItem->toSocket()->removeConnection(connItem);
                
                m_scene->removeItem(connItem);
                delete connItem;
                break;
            }
        }
    }
    
    delete targetConn;
}

void NodeEditorWidget::clear() {
    // Clear connections first (before scene clear which deletes graphics items)
    m_connections.clear();
    m_nodeItems.clear();
    
    // Scene clear will delete all QGraphicsItems (including NodeGraphicsItem proxies)
    m_scene->clear();
    
    // Delete the Node objects (data model, not graphics)
    qDeleteAll(m_nodes);
    m_nodes.clear();
    
    m_tempConnection = nullptr;
    m_dragSourceSocket = nullptr;
    
    // Re-setup scene
    setupScene();
}

void NodeEditorWidget::wheelEvent(QWheelEvent* event) {
    // ズーム処理
    const qreal zoomInFactor = 1.15;
    const qreal zoomOutFactor = 1.0 / zoomInFactor;
    
    if (event->angleDelta().y() > 0) {
        // ズームイン
        if (m_zoomFactor * zoomInFactor <= MAX_ZOOM) {
            scale(zoomInFactor, zoomInFactor);
            m_zoomFactor *= zoomInFactor;
        }
    } else {
        // ズームアウト
        if (m_zoomFactor * zoomOutFactor >= MIN_ZOOM) {
            scale(zoomOutFactor, zoomOutFactor);
            m_zoomFactor *= zoomOutFactor;
        }
    }
}

void NodeEditorWidget::mousePressEvent(QMouseEvent* event) {
    // Ensure this widget has focus for keyboard events
    setFocus();
    
    // Space + Left click or Middle click for panning
    if ((event->button() == Qt::LeftButton && m_spacePressed) || event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    
    // Ctrl+Shift+Click: Connect clicked node's first output to Material Output
    if (event->button() == Qt::LeftButton && 
        (event->modifiers() & Qt::ControlModifier) && 
        (event->modifiers() & Qt::ShiftModifier)) {
        
        QGraphicsItem* item = itemAt(event->pos());
        NodeGraphicsItem* nodeItem = nullptr;
        QGraphicsItem* parent = item;
        while (parent) {
            if (NodeGraphicsItem* ni = dynamic_cast<NodeGraphicsItem*>(parent)) {
                nodeItem = ni;
                break;
            }
            parent = parent->parentItem();
        }
        
        if (nodeItem && nodeItem->node()->name() != "Material Output") {
            // Find Material Output node
            Node* outputNode = nullptr;
            for (Node* node : m_nodes) {
                if (node->name() == "Material Output") {
                    outputNode = node;
                    break;
                }
            }
            
            if (outputNode && !nodeItem->node()->outputSockets().isEmpty()) {
                NodeSocket* fromSocket = nodeItem->node()->outputSockets().first();
                NodeSocket* toSocket = outputNode->findInputSocket("Surface");
                
                if (toSocket) {
                    // Disconnect existing connection to Surface if any
                    if (toSocket->isConnected()) {
                        for (NodeSocket* conn : toSocket->connections()) {
                            removeConnection(conn, toSocket);
                        }
                    }
                    createConnection(fromSocket, toSocket);
                    emit parameterChanged();
                }
            }
        }
        event->accept();
        return;
    }
    
    // Left click for socket operation
    if (event->button() == Qt::LeftButton) {
        // Check for items under cursor to manage drag mode
        QGraphicsItem* item = itemAt(event->pos());
        if (item) {
            setDragMode(QGraphicsView::NoDrag);
            
            // If clicking on a node (or child), store initial positions for Undo
            // We need to check if it's a NodeGraphicsItem or child
            NodeGraphicsItem* nodeItem = nullptr;
            QGraphicsItem* parent = item;
            while (parent) {
                if (NodeGraphicsItem* ni = dynamic_cast<NodeGraphicsItem*>(parent)) {
                    nodeItem = ni;
                    break;
                }
                parent = parent->parentItem();
            }
            
            if (nodeItem) {
                // Store positions of ALL selected nodes (since they move together)
                // If the clicked node is not selected, it will become selected (handled by base class)
                // But base class runs AFTER this? No, we call base class at end.
                // Actually, if we click a node, QGraphicsView handles selection.
                // But we need to know BEFORE move starts.
                
                // If we are just clicking, selection might change.
                // But if we drag, selection is preserved/updated.
                // Let's store positions of currently selected items + the one under cursor if not selected?
                // Simpler: Just store positions of all nodes. Or just selected ones.
                // If we click an unselected node, it becomes the only selection (usually).
                // If we Shift+Click, it adds.
                
                // Let's clear and store on press.
                m_initialNodePositions.clear();
                
                // We can't rely on scene->selectedItems() because selection updates in base implementation.
                // But we can capture "before move" state.
                // Actually, a better place might be to detect start of drag?
                // But QGraphicsView handles dragging internally.
                
                // Let's store ALL node positions. It's not that expensive (few nodes).
                for (Node* node : m_nodes) {
                    m_initialNodePositions[node] = node->position();
                }
            }
        } else {
            setDragMode(QGraphicsView::RubberBandDrag);
        }

        NodeGraphicsSocket* socket = socketAt(event->pos());
        if (socket) {
            m_dragSourceSocket = socket;
            m_tempConnection = new ConnectionGraphicsItem(socket, nullptr);
            m_scene->addItem(m_tempConnection);
            m_tempConnection->setEndPoint(mapToScene(event->pos()));
            event->accept();
            return;
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void NodeEditorWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        // パン処理
        QPointF delta = mapToScene(event->pos()) - mapToScene(m_lastPanPoint);
        m_lastPanPoint = event->pos();
        
        // ビューを移動
        setTransformationAnchor(QGraphicsView::NoAnchor);
        translate(delta.x(), delta.y());
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        
        event->accept();
        return;
    }
    
    if (m_tempConnection) {
        m_tempConnection->setEndPoint(mapToScene(event->pos()));
        event->accept();
        return;
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void NodeEditorWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isPanning) {
        if (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton) {
            // パン終了
            m_isPanning = false;
            setCursor(m_spacePressed ? Qt::OpenHandCursor : Qt::ArrowCursor);
            event->accept();
            return;
        }
    }
    
    if (m_tempConnection && event->button() == Qt::LeftButton) {
        // 接続完了処理
        NodeGraphicsSocket* targetSocket = socketAt(event->pos());
        
        if (targetSocket && targetSocket != m_dragSourceSocket) {
            // 接続の妥当性チェック
            NodeSocket* from = m_dragSourceSocket->socket();
            NodeSocket* to = targetSocket->socket();
            
            // 方向を正規化（Output -> Input）
            if (from->direction() == SocketDirection::Input && to->direction() == SocketDirection::Output) {
                std::swap(from, to);
                std::swap(m_dragSourceSocket, targetSocket);
            }
            
            // Explicitly block Input-Input and Output-Output connections
            if (from->direction() == to->direction()) {
                // Invalid connection
                delete m_tempConnection;
                m_tempConnection = nullptr;
                m_dragSourceSocket = nullptr;
                event->accept();
                return;
            }
            
            NodeConnection* connection = new NodeConnection(from, to);
            if (connection->isValid()) {
                delete connection; // We just checked validity, command will recreate
                
                // Use UndoStack
                m_undoStack->push(new ConnectCommand(this, from, to));
            } else {
                delete connection;
            }
        } else if (!targetSocket) {
            // Connection dropped on empty space - show search and auto-connect
            NodeSocket* dragSocket = m_dragSourceSocket->socket();
            QPoint dropPos = event->pos();
            QPointF dropScenePos = mapToScene(dropPos);
            
            // Show search dialog
            QString selectedNodeName = showNodeSearchMenuForConnection(dropPos, dragSocket);
            
            if (!selectedNodeName.isEmpty()) {
                Node* newNode = NodeRegistry::instance().createNode(selectedNodeName);
                if (newNode) {
                    addNode(newNode, dropScenePos);
                    
                    // Auto-connect based on socket direction
                    if (dragSocket->direction() == SocketDirection::Output) {
                        // Find compatible input on new node
                        for (NodeSocket* input : newNode->inputSockets()) {
                            if (NodeConnection::isValid(dragSocket, input)) {
                                createConnection(dragSocket, input);
                                break;
                            }
                        }
                    } else {
                        // Find compatible output on new node
                        for (NodeSocket* output : newNode->outputSockets()) {
                            if (NodeConnection::isValid(output, dragSocket)) {
                                createConnection(output, dragSocket);
                                break;
                            }
                        }
                    }
                    emit parameterChanged();
                }
            }
        }
        
        // Remove temporary connection line
        m_scene->removeItem(m_tempConnection);
        delete m_tempConnection;
        m_tempConnection = nullptr;
        m_dragSourceSocket = nullptr;
        
        event->accept();
        return;
    }
    
    if (m_tempConnection && event->button() == Qt::LeftButton) {
        // ... (Connection logic handled above)
    }
    
    // Check for moved nodes
    if (!m_initialNodePositions.isEmpty() && event->button() == Qt::LeftButton) {
        QList<Node*> movedNodes;
        QList<QPointF> oldPos;
        QList<QPointF> newPos;
        
        for (auto it = m_initialNodePositions.begin(); it != m_initialNodePositions.end(); ++it) {
            Node* node = it.key();
            if (node->position() != it.value()) {
                movedNodes.append(node);
                oldPos.append(it.value());
                newPos.append(node->position());
            }
        }
        
        if (!movedNodes.isEmpty()) {
            m_undoStack->push(new MoveNodeCommand(this, movedNodes, oldPos, newPos));
            
            // Try auto-connect for the single moved node (if only one moved)
            // If multiple moved, maybe too complex? Let's do it for single node move.
            if (movedNodes.size() == 1) {
                tryAutoConnect(movedNodes.first());
            }
        }
        
        m_initialNodePositions.clear();
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

void NodeEditorWidget::keyPressEvent(QKeyEvent* event) {
    // Shift+Q: Category Menu
    if (event->modifiers() & Qt::ShiftModifier && event->key() == Qt::Key_Q) {
        showNodeCategoryMenu(mapFromGlobal(QCursor::pos()));
        event->accept();
        return;
    }

    // Shift+A: Search Menu
    if (event->modifiers() & Qt::ShiftModifier && event->key() == Qt::Key_A) {
        showNodeSearchMenu(mapFromGlobal(QCursor::pos()));
        event->accept();
        return;
    }

    // Delete key - ノードまたは接続を削除
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // テキスト入力ウィジェットにフォーカスがある場合は、そちらを優先
        QWidget* focusWidget = QApplication::focusWidget();
        if (focusWidget) {
            // QLineEdit, QSpinBox, QDoubleSpinBoxなどの編集可能ウィジェットの場合
            if (qobject_cast<QLineEdit*>(focusWidget) ||
                qobject_cast<QSpinBox*>(focusWidget) ||
                qobject_cast<QDoubleSpinBox*>(focusWidget)) {
                // ウィジェットにイベントを渡して、ノード削除は行わない
                QGraphicsView::keyPressEvent(event);
                return;
            }
            
            // SliderSpinBoxの子ウィジェットかもチェック
            QWidget* parent = focusWidget->parentWidget();
            while (parent) {
                if (parent->objectName() == "SliderSpinBox" || 
                    qobject_cast<SliderSpinBox*>(parent)) {
                    // SliderSpinBoxの子なのでノード削除しない
                    QGraphicsView::keyPressEvent(event);
                    return;
                }
                parent = parent->parentWidget();
            }
        }

        // Check if a graphics item (like a proxy widget) has focus
        if (m_scene->focusItem() && m_scene->focusItem()->type() == QGraphicsProxyWidget::Type) {
            QGraphicsView::keyPressEvent(event);
            return;
        }

        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        QList<ConnectionGraphicsItem*> connections;
        QList<NodeGraphicsItem*> nodes;

        // Separate items to avoid double deletion issues (Node deletion deletes connections)
        for (QGraphicsItem* item : selected) {
            if (ConnectionGraphicsItem* conn = dynamic_cast<ConnectionGraphicsItem*>(item)) {
                connections.append(conn);
            } else if (NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item)) {
                nodes.append(nodeItem);
            }
        }

        // Delete connections first
        for (ConnectionGraphicsItem* conn : connections) {
            // Check if still valid (though should be if we process first)
            if (!m_scene->items().contains(conn)) continue;

            // データモデルから接続を削除
            NodeGraphicsSocket* fromSocket = conn->fromSocket();
            NodeGraphicsSocket* toSocket = conn->toSocket();
            
            if (fromSocket && toSocket) {
                NodeSocket* from = fromSocket->socket();
                NodeSocket* to = toSocket->socket();
                
                // Use UndoStack
                m_undoStack->push(new DisconnectCommand(this, from, to));
            }
        }

        // Delete nodes
        for (NodeGraphicsItem* nodeItem : nodes) {
            // Check if still valid
            if (!m_scene->items().contains(nodeItem)) continue;

            Node* node = nodeItem->node();
            
            // OutputNodeは削除不可
            if (node->name() == "Material Output") {
                continue;
            }
            
            // Use UndoStack
            m_undoStack->push(new DeleteNodeCommand(this, node));
        }
        
        // プレビュー更新
        for (auto item : m_nodeItems) {
            item->updatePreview();
        }
        
        event->accept();
        return;
    }
    
    // Ctrl+T: Add Texture Coordinate + Mapping nodes before selected texture node
    if (event->key() == Qt::Key_T && (event->modifiers() & Qt::ControlModifier)) {
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        for (QGraphicsItem* item : selected) {
            if (NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item)) {
                Node* node = nodeItem->node();
                
                // Check if node has Vector input
                NodeSocket* vectorInput = nullptr;
                for (NodeSocket* socket : node->inputSockets()) {
                    if (socket->type() == SocketType::Vector && socket->name() == "Vector") {
                        vectorInput = socket;
                        break;
                    }
                }
                
                if (vectorInput && !vectorInput->isConnected()) {
                    QPointF nodePos = nodeItem->pos();
                    
                    // Create Texture Coordinate node
                    TextureCoordinateNode* texCoordNode = new TextureCoordinateNode();
                    addNode(texCoordNode, nodePos + QPointF(-400, 0));
                    
                    // Create Mapping node
                    MappingNode* mappingNode = new MappingNode();
                    addNode(mappingNode, nodePos + QPointF(-200, 0));
                    
                    // Connect: TexCoord -> Mapping -> Node
                    createConnection(texCoordNode->outputSockets().first(), mappingNode->inputSockets().first());
                    createConnection(mappingNode->outputSockets().first(), vectorInput);
                    
                    emit parameterChanged();
                }
            }
        }
        event->accept();
        return;
    }
    
    // Ctrl+D: Duplicate selected nodes
    if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) {
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        QList<Node*> newNodes;
        QMap<Node*, Node*> nodeMapping;  // old -> new
        
        // First pass: create duplicates
        for (QGraphicsItem* item : selected) {
            if (NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item)) {
                Node* oldNode = nodeItem->node();
                
                // Skip Output node
                if (oldNode->name() == "Material Output") continue;
                
                // Create a copy by saving and restoring
                QJsonObject json = oldNode->save();
                Node* newNode = NodeRegistry::instance().createNode(oldNode->name());
                if (newNode) {
                    newNode->restore(json);
                    QPointF newPos = nodeItem->pos() + QPointF(50, 50);
                    addNode(newNode, newPos);
                    nodeMapping[oldNode] = newNode;
                    newNodes.append(newNode);
                }
            }
        }
        
        // Deselect old, select new
        m_scene->clearSelection();
        for (NodeGraphicsItem* nodeItem : m_nodeItems) {
            if (newNodes.contains(nodeItem->node())) {
                nodeItem->setSelected(true);
            }
        }
        
        emit parameterChanged();
        event->accept();
        return;
    }
    
    // M: Mute/bypass selected node (toggle)
    if (event->key() == Qt::Key_M && !(event->modifiers() & Qt::ControlModifier)) {
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        for (QGraphicsItem* item : selected) {
            if (NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item)) {
                Node* node = nodeItem->node();
                node->setMuted(!node->isMuted());
                nodeItem->update();
            }
        }
        emit parameterChanged();
        event->accept();
        return;
    }
    
    // Shift+A or Tab: Open node search menu
    if ((event->key() == Qt::Key_A && (event->modifiers() & Qt::ShiftModifier)) ||
        event->key() == Qt::Key_Tab) {
        showNodeSearchMenu(mapFromGlobal(QCursor::pos()));
        event->accept();
        return;
    }
    
    // S: Scale selected nodes (toggle between 100% and 75%)
    if (event->key() == Qt::Key_S && !(event->modifiers() & Qt::ControlModifier)) {
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        for (QGraphicsItem* item : selected) {
            if (NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item)) {
                qreal currentScale = nodeItem->scale();
                if (currentScale < 0.9) {
                    nodeItem->setScale(1.0);
                } else {
                    nodeItem->setScale(0.75);
                }
            }
        }
        event->accept();
        return;
    }
    
    // R: Rotate selected nodes (toggle 0, 90, 180, 270 degrees)
    if (event->key() == Qt::Key_R && !(event->modifiers() & Qt::ControlModifier)) {
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        for (QGraphicsItem* item : selected) {
            if (NodeGraphicsItem* nodeItem = dynamic_cast<NodeGraphicsItem*>(item)) {
                qreal currentRotation = nodeItem->rotation();
                qreal newRotation = fmod(currentRotation + 90, 360);
                nodeItem->setRotation(newRotation);
            }
        }
        event->accept();
        return;
    }
    
    // Ctrl+G: Group selected nodes (visual only - frame box)
    if (event->key() == Qt::Key_G && (event->modifiers() & Qt::ControlModifier)) {
        // TODO: Implement proper node grouping
        // For now, just show a message that grouping is not yet implemented
        event->accept();
        return;
    }
    
    // Ctrl+Shift+Click: Connect to output (handled in mousePressEvent)
    
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        if (!m_isPanning) {
            setCursor(Qt::OpenHandCursor);
        }
    }
    QGraphicsView::keyPressEvent(event);
}

void NodeEditorWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = false;
        if (!m_isPanning) {
            setCursor(Qt::ArrowCursor);
        }
    }
    QGraphicsView::keyReleaseEvent(event);
}

void NodeEditorWidget::drawBackground(QPainter* painter, const QRectF& rect) {
    QGraphicsView::drawBackground(painter, rect);
    drawGrid(painter, rect);
}

void NodeEditorWidget::drawGrid(QPainter* painter, const QRectF& rect) {
    const int gridSize = 20;
    const int gridSquares = 5;
    
    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);
    
    QVector<QLineF> linesLight;
    QVector<QLineF> linesDark;
    
    // 縦線
    for (qreal x = left; x < rect.right(); x += gridSize) {
        if (int(x) % (gridSize * gridSquares) == 0) {
            linesDark.append(QLineF(x, rect.top(), x, rect.bottom()));
        } else {
            linesLight.append(QLineF(x, rect.top(), x, rect.bottom()));
        }
    }
    
    // 横線
    for (qreal y = top; y < rect.bottom(); y += gridSize) {
        if (int(y) % (gridSize * gridSquares) == 0) {
            linesDark.append(QLineF(rect.left(), y, rect.right(), y));
        } else {
            linesLight.append(QLineF(rect.left(), y, rect.right(), y));
        }
    }
    
    // 描画
    auto theme = AppSettings::instance().theme();
    QColor lightColor, darkColor;
    
    if (theme == AppSettings::Theme::Light) {
        lightColor = QColor(200, 200, 200);
        darkColor = QColor(180, 180, 180);
    } else if (theme == AppSettings::Theme::Colorful) {
        lightColor = QColor(60, 60, 80);
        darkColor = QColor(70, 70, 90);
    } else { // Dark
        lightColor = QColor(50, 50, 50);
        darkColor = QColor(60, 60, 60);
    }
    
    painter->setPen(QPen(lightColor, 1));
    painter->drawLines(linesLight);
    
    painter->setPen(QPen(darkColor, 1));
    painter->drawLines(linesDark);
}

void NodeEditorWidget::drawForeground(QPainter* painter, const QRectF& rect) {
    QGraphicsView::drawForeground(painter, rect);

    if (m_showFPS) {
        m_frameCount++;
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        
        if (m_lastFrameTime == 0) {
            m_lastFrameTime = currentTime;
        }
        
        qint64 elapsed = currentTime - m_lastFrameTime;
        if (elapsed >= 1000) {
            m_fps = m_frameCount * 1000.0 / elapsed;
            m_frameCount = 0;
            m_lastFrameTime = currentTime;
        }
        
        // Draw FPS in top-left corner (relative to view)
        // We need to map from viewport to scene or just draw in viewport coordinates?
        // drawForeground draws in SCENE coordinates.
        // To draw fixed on screen, we should use mapToScene on the viewport rect.
        
        painter->save();
        painter->setWorldMatrixEnabled(false); // Draw in viewport coordinates
        
        QString fpsText = QString("FPS: %1").arg(m_fps, 0, 'f', 1);
        painter->setPen(Qt::white);
        painter->setFont(QFont("Arial", 10, QFont::Bold));
        painter->drawText(10, 20, fpsText);
        
        painter->restore();
        
        // Force update to keep counting FPS if we want continuous update
        // But that wastes CPU. Only update if something changes or if we want to measure idle FPS?
        // For now, let's just update when painting happens naturally.
        // If user wants to see "idle" FPS, we'd need a timer.
        // Let's add a small timer-based update if FPS is shown?
        // Or just rely on interaction.
        // User asked for FPS display, usually implies continuous.
        update(); 
    }
}

NodeGraphicsSocket* NodeEditorWidget::socketAt(const QPoint& pos) {
    QList<QGraphicsItem*> items = this->items(pos);
    for (QGraphicsItem* item : items) {
        if (NodeGraphicsSocket* socket = dynamic_cast<NodeGraphicsSocket*>(item)) {
            return socket;
        }
    }
    return nullptr;
}

void NodeEditorWidget::contextMenuEvent(QContextMenuEvent* event) {
    // Use unified search menu for right-click
    showNodeSearchMenu(event->pos());
}

void NodeEditorWidget::updateTheme() {
    auto theme = AppSettings::instance().theme();
    QColor bgColor;
    
    if (theme == AppSettings::Theme::Light) {
        bgColor = QColor(240, 240, 240);
    } else if (theme == AppSettings::Theme::Colorful) {
        bgColor = QColor(40, 40, 60);
    } else { // Dark
        bgColor = QColor(40, 40, 40);
    }
    
    setBackgroundBrush(QBrush(bgColor));
    update();
}

void NodeEditorWidget::saveToFile(const QString& filename) {
    QJsonObject root;
    
    // Save Nodes
    QJsonArray nodesArray;
    for (int i = 0; i < m_nodes.size(); ++i) {
        Node* node = m_nodes[i];
        QJsonObject nodeJson = node->save();
        nodeJson["type"] = node->name(); // Use name as type identifier (assuming unique names in registry)
        nodesArray.append(nodeJson);
    }
    root["nodes"] = nodesArray;
    
    // Save Connections
    QJsonArray connectionsArray;
    for (NodeConnection* conn : m_connections) {
        QJsonObject connJson;
        
        // Find indices
        int fromIndex = m_nodes.indexOf(conn->from()->parentNode());
        int toIndex = m_nodes.indexOf(conn->to()->parentNode());
        
        if (fromIndex != -1 && toIndex != -1) {
            connJson["fromNode"] = fromIndex;
            connJson["fromSocket"] = conn->from()->name();
            connJson["toNode"] = toIndex;
            connJson["toSocket"] = conn->to()->name();
            connectionsArray.append(connJson);
        }
    }
    root["connections"] = connectionsArray;
    
    // Ensure directory exists
    QFileInfo fileInfo(filename);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Write to file
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(root);
        file.write(doc.toJson());
    }
}

void NodeEditorWidget::loadFromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    
    // Clear current scene
    // Note: We need to be careful about deleting items that might be in use or have signals
    // Safest way is to remove everything from scene first
    m_scene->clear(); 
    qDeleteAll(m_nodes);
    qDeleteAll(m_connections);
    m_nodes.clear();
    m_connections.clear();
    m_nodeItems.clear();
    m_tempConnection = nullptr;
    m_dragSourceSocket = nullptr;
    
    // Load Nodes
    QJsonArray nodesArray = root["nodes"].toArray();
    for (const QJsonValue& val : nodesArray) {
        QJsonObject nodeJson = val.toObject();
        QString type = nodeJson["type"].toString();
        
        Node* node = NodeRegistry::instance().createNode(type);
        if (node) {
            node->restore(nodeJson);
            addNode(node, node->position());
        } else {
            // Create a placeholder or skip?
            // For now, if node type not found, we skip (might break connections)
            // To keep indices aligned, we might need a null placeholder in m_nodes if we used index-based connections
            // But m_nodes is appended to in addNode.
            // If we skip, indices shift.
            // Ideally we should have a "Missing Node" type.
            // For simplicity, we assume all types exist.
        }
    }
    
    // Load Connections
    QJsonArray connectionsArray = root["connections"].toArray();
    for (const QJsonValue& val : connectionsArray) {
        QJsonObject connJson = val.toObject();
        
        int fromIndex = connJson["fromNode"].toInt();
        int toIndex = connJson["toNode"].toInt();
        QString fromSocketName = connJson["fromSocket"].toString();
        QString toSocketName = connJson["toSocket"].toString();
        
        if (fromIndex >= 0 && fromIndex < m_nodes.size() &&
            toIndex >= 0 && toIndex < m_nodes.size()) {
            
            Node* fromNode = m_nodes[fromIndex];
            Node* toNode = m_nodes[toIndex];
            
            NodeSocket* fromSocket = fromNode->findOutputSocket(fromSocketName);
            NodeSocket* toSocket = toNode->findInputSocket(toSocketName);
            
            if (fromSocket && toSocket) {
                NodeConnection* connection = new NodeConnection(fromSocket, toSocket);
                if (connection->isValid()) {
                    m_connections.append(connection);
                    
                    // Visual connection
                    // We need to find the graphics items for these sockets
                    NodeGraphicsItem* fromItem = m_nodeItems[fromIndex];
                    NodeGraphicsItem* toItem = m_nodeItems[toIndex];
                    
                    // This is tricky because NodeGraphicsItem doesn't expose sockets easily by name
                    // But we can iterate its child items or use a helper
                    // Let's assume we can get them via getSocketPosition logic or similar
                    // Actually, NodeGraphicsItem creates NodeGraphicsSocket children.
                    // We can iterate children of NodeGraphicsItem.
                    
                    auto findSocketItem = [](NodeGraphicsItem* nodeItem, NodeSocket* s) -> NodeGraphicsSocket* {
                        for (QGraphicsItem* child : nodeItem->childItems()) {
                            if (NodeGraphicsSocket* gs = dynamic_cast<NodeGraphicsSocket*>(child)) {
                                if (gs->socket() == s) return gs;
                            }
                        }
                        return nullptr;
                    };
                    
                    NodeGraphicsSocket* fromSocketItem = findSocketItem(fromItem, fromSocket);
                    NodeGraphicsSocket* toSocketItem = findSocketItem(toItem, toSocket);
                    
                    if (fromSocketItem && toSocketItem) {
                        ConnectionGraphicsItem* connItem = new ConnectionGraphicsItem(fromSocketItem, toSocketItem);
                        m_scene->addItem(connItem);
                    }
                } else {
                    delete connection;
                }
            }
        }
    }
    
    // Trigger update
    emit parameterChanged();
}

void NodeEditorWidget::tryAutoConnect(Node* node) {
    // Find the graphics item for this node
    NodeGraphicsItem* nodeItem = nullptr;
    for (auto item : m_nodeItems) {
        if (item->node() == node) {
            nodeItem = item;
            break;
        }
    }
    if (!nodeItem) return;

    // Check for collision with connections
    QRectF nodeRect = nodeItem->sceneBoundingRect();
    QList<QGraphicsItem*> collidingItems = m_scene->items(nodeRect);
    
    for (QGraphicsItem* item : collidingItems) {
        if (ConnectionGraphicsItem* connItem = dynamic_cast<ConnectionGraphicsItem*>(item)) {
            NodeConnection* conn = nullptr;
            // Find the connection object
            NodeSocket* fromSocket = connItem->fromSocket()->socket();
            NodeSocket* toSocket = connItem->toSocket()->socket();
            
            for (NodeConnection* c : m_connections) {
                if (c->from() == fromSocket && c->to() == toSocket) {
                    conn = c;
                    break;
                }
            }
            
            if (!conn) continue;
            
            // Check if we should split this connection
            // We need a compatible Input on the Node for 'fromSocket'
            // And a compatible Output on the Node for 'toSocket'
            
            NodeSocket* candidateInput = nullptr;
            NodeSocket* candidateOutput = nullptr;
            
            // Find first compatible input
            for (NodeSocket* input : node->inputSockets()) {
                // Check type compatibility using static helper
                if (NodeConnection::isValid(fromSocket, input)) {
                    candidateInput = input;
                    break;
                }
            }
            
            // Find first compatible output
            for (NodeSocket* output : node->outputSockets()) {
                if (NodeConnection::isValid(output, toSocket)) {
                    candidateOutput = output;
                    break;
                }
            }
            
            if (candidateInput && candidateOutput) {
                // Perform Auto-Connect
                m_undoStack->beginMacro("Auto-Connect");
                
                // 1. Disconnect original
                m_undoStack->push(new DisconnectCommand(this, fromSocket, toSocket));
                
                // 2. Connect From -> Node Input
                m_undoStack->push(new ConnectCommand(this, fromSocket, candidateInput));
                
                // 3. Connect Node Output -> To
                m_undoStack->push(new ConnectCommand(this, candidateOutput, toSocket));
                
                m_undoStack->endMacro();
                
                // Only do one auto-connect per drop
                return;
            }
        }
    }
}

// Category-based node menu
void NodeEditorWidget::showNodeCategoryMenu(const QPoint& pos) {
    QMenu menu;
    menu.setStyleSheet("QMenu { background-color: #383838; color: white; border: 1px solid #555; } QMenu::item:selected { background-color: #4a90d9; }");
    
    QPointF scenePos = mapToScene(pos);
    
    QStringList categories = NodeRegistry::instance().getCategories();
    for (const QString& category : categories) {
        QMenu* subMenu = menu.addMenu(AppSettings::instance().translate(category));
        subMenu->setStyleSheet("QMenu { background-color: #383838; color: white; border: 1px solid #555; } QMenu::item:selected { background-color: #4a90d9; }");
        
        QStringList nodeNames = NodeRegistry::instance().getNodesByCategory(category);
        for (const QString& nodeName : nodeNames) {
            QAction* action = subMenu->addAction(AppSettings::instance().translate(nodeName));
            connect(action, &QAction::triggered, [this, nodeName, scenePos]() {
                Node* newNode = NodeRegistry::instance().createNode(nodeName);
                if (newNode) {
                    addNode(newNode, scenePos);
                    emit parameterChanged();
                }
            });
        }
    }
    
    menu.exec(mapToGlobal(pos));
}

// Node search menu with filtering
void NodeEditorWidget::showNodeSearchMenu(const QPoint& pos) {
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(AppSettings::instance().translate("Add Node"));
    dialog->setWindowFlags(Qt::Popup);
    dialog->setStyleSheet("QDialog { background-color: #404040; border: 1px solid #555; }");
    dialog->setMinimumSize(250, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);
    
    // Search input
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(AppSettings::instance().translate("Search..."));
    searchEdit->setStyleSheet("QLineEdit { background: #333; color: white; border: 1px solid #555; padding: 5px; }");
    layout->addWidget(searchEdit);
    
    // Node list
    QListWidget* listWidget = new QListWidget();
    listWidget->setStyleSheet(
        "QListWidget { background: #383838; color: white; border: none; }"
        "QListWidget::item { padding: 5px; }"
        "QListWidget::item:hover { background: #505050; }"
        "QListWidget::item:selected { background: #4a90d9; }"
    );
    layout->addWidget(listWidget);
    
    // Store node data
    QMap<QListWidgetItem*, QString> itemMap;
    
    // Populate list with all nodes
    QStringList categories = NodeRegistry::instance().getCategories();
    for (const QString& category : categories) {
        QStringList nodeNames = NodeRegistry::instance().getNodesByCategory(category);
        for (const QString& nodeName : nodeNames) {
            QString translatedName = AppSettings::instance().translate(nodeName);
            QString translatedCategory = AppSettings::instance().translate(category);
            QListWidgetItem* item = new QListWidgetItem(translatedName + " [" + translatedCategory + "]");
            item->setData(Qt::UserRole, nodeName);  // Store original name
            listWidget->addItem(item);
            itemMap[item] = nodeName;
        }
    }
    
    // Filter function
    connect(searchEdit, &QLineEdit::textChanged, [listWidget](const QString& text) {
        QString filter = text.toLower();
        for (int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem* item = listWidget->item(i);
            bool visible = filter.isEmpty() || item->text().toLower().contains(filter);
            item->setHidden(!visible);
        }
    });
    
    QPointF scenePos = mapToScene(pos);
    QString selectedNodeName;
    
    // Single click or double-click to select
    connect(listWidget, &QListWidget::itemClicked, [&selectedNodeName, dialog](QListWidgetItem* item) {
        selectedNodeName = item->data(Qt::UserRole).toString();
        dialog->accept();
    });
    
    connect(searchEdit, &QLineEdit::returnPressed, [listWidget, &selectedNodeName, dialog]() {
        // Select first visible item
        for (int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem* item = listWidget->item(i);
            if (!item->isHidden()) {
                selectedNodeName = item->data(Qt::UserRole).toString();
                dialog->accept();
                return;
            }
        }
    });
    
    // Position dialog at cursor
    dialog->move(mapToGlobal(pos));
    searchEdit->setFocus();
    
    if (dialog->exec() == QDialog::Accepted && !selectedNodeName.isEmpty()) {
        Node* newNode = NodeRegistry::instance().createNode(selectedNodeName);
        if (newNode) {
            addNode(newNode, scenePos);
            emit parameterChanged();
        }
    }
    
    delete dialog;
}

// Node search menu for connection drop - returns selected node name
QString NodeEditorWidget::showNodeSearchMenuForConnection(const QPoint& pos, NodeSocket* dragSocket) {
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(AppSettings::instance().translate("Connect to Node"));
    dialog->setWindowFlags(Qt::Popup);
    dialog->setStyleSheet("QDialog { background-color: #404040; border: 1px solid #555; }");
    dialog->setMinimumSize(250, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);
    
    // Search input
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(AppSettings::instance().translate("Search..."));
    searchEdit->setStyleSheet("QLineEdit { background: #333; color: white; border: 1px solid #555; padding: 5px; }");
    layout->addWidget(searchEdit);
    
    // Node list
    QListWidget* listWidget = new QListWidget();
    listWidget->setStyleSheet(
        "QListWidget { background: #383838; color: white; border: none; }"
        "QListWidget::item { padding: 5px; }"
        "QListWidget::item:hover { background: #505050; }"
        "QListWidget::item:selected { background: #4a90d9; }"
    );
    layout->addWidget(listWidget);
    
    // Populate list with compatible nodes
    QStringList categories = NodeRegistry::instance().getCategories();
    for (const QString& category : categories) {
        QStringList nodeNames = NodeRegistry::instance().getNodesByCategory(category);
        for (const QString& nodeName : nodeNames) {
            // Skip Material Output
            if (nodeName == "Material Output") continue;
            
            QString translatedName = AppSettings::instance().translate(nodeName);
            QString translatedCategory = AppSettings::instance().translate(category);
            QListWidgetItem* item = new QListWidgetItem(translatedName + " [" + translatedCategory + "]");
            item->setData(Qt::UserRole, nodeName);
            listWidget->addItem(item);
        }
    }
    
    // Filter function
    connect(searchEdit, &QLineEdit::textChanged, [listWidget](const QString& text) {
        QString filter = text.toLower();
        for (int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem* item = listWidget->item(i);
            bool visible = filter.isEmpty() || item->text().toLower().contains(filter);
            item->setHidden(!visible);
        }
    });
    
    QString selectedNodeName;
    
    // Single click to select
    connect(listWidget, &QListWidget::itemClicked, [&selectedNodeName, dialog](QListWidgetItem* item) {
        selectedNodeName = item->data(Qt::UserRole).toString();
        dialog->accept();
    });
    
    connect(searchEdit, &QLineEdit::returnPressed, [listWidget, &selectedNodeName, dialog]() {
        for (int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem* item = listWidget->item(i);
            if (!item->isHidden()) {
                selectedNodeName = item->data(Qt::UserRole).toString();
                dialog->accept();
                return;
            }
        }
    });
    
    dialog->move(mapToGlobal(pos));
    searchEdit->setFocus();
    dialog->exec();
    
    delete dialog;
    return selectedNodeName;
}
