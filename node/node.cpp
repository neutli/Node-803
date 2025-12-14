#include "node.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QColor>

// NodeSocket implementation
NodeSocket::NodeSocket(const QString& name, SocketType type, SocketDirection direction, Node* parentNode)
    : m_name(name)
    , m_type(type)
    , m_direction(direction)
    , m_parentNode(parentNode)
    , m_labelVisible(true)
    , m_visible(true)
{
}

NodeSocket::~NodeSocket() {
    // 接続されている他のソケットから自分を削除
    // リストが変更されるためコピーを使用
    auto connectionsCopy = m_connections;
    for (NodeSocket* other : connectionsCopy) {
        if (other) {
            other->removeConnection(this);
        }
    }
    m_connections.clear();
}

void NodeSocket::addConnection(NodeSocket* other) {
    if (!m_connections.contains(other)) {
        m_connections.append(other);
        if (m_parentNode) {
            qDebug() << "NodeSocket::addConnection" << m_name << "to" << other->name() << "Parent:" << m_parentNode->name();
            m_parentNode->setDirty(true);
            m_parentNode->notifyStructureChanged();
        }
    }
}

void NodeSocket::removeConnection(NodeSocket* other) {
    m_connections.removeAll(other);
    if (m_parentNode) {
        qDebug() << "NodeSocket::removeConnection" << m_name << "from" << other->name() << "Parent:" << m_parentNode->name();
        m_parentNode->setDirty(true);
        m_parentNode->notifyStructureChanged();
    }
}

QVariant NodeSocket::value() const {
    // 入力ソケットで接続がある場合は、接続先の値を返す
    if (m_direction == SocketDirection::Input && !m_connections.isEmpty()) {
        NodeSocket* source = m_connections.first();
        if (source && source->parentNode()) {
            // 必要なら上流ノードを評価
            if (source->parentNode()->isDirty()) {
                source->parentNode()->evaluate();
            }
            return source->value();
        }
    }
    
    // 値が設定されていればそれを返す
    if (m_value.isValid()) {
        return m_value;
    }
    
    // 入力ソケットで値が未設定ならデフォルト値を返す
    if (m_direction == SocketDirection::Input) {
        return m_defaultValue;
    }
    
    return m_value;
}

QVariant NodeSocket::getValue(const QVector3D& pos) const {
    // Recursion protection
    static thread_local int depth = 0;
    const int MAX_DEPTH = 100;
    
    if (depth > MAX_DEPTH) {
        return m_defaultValue;
    }
    
    struct DepthGuard {
        DepthGuard() { depth++; }
        ~DepthGuard() { depth--; }
    } guard;

    if (m_direction == SocketDirection::Input && !m_connections.isEmpty()) {
        // Get value from connected output socket
        NodeSocket* sourceSocket = m_connections[0];
        if (sourceSocket && sourceSocket->parentNode()) {
            Node* sourceNode = sourceSocket->parentNode();
            
            // Bypass: if source node is muted, pass through compatible input
            if (sourceNode->isMuted()) {
                // Find an input socket with the same type as the output we're asking for
                SocketType targetType = sourceSocket->type();
                NodeSocket* bypassInput = nullptr;
                
                // First pass: exact type match
                for (NodeSocket* input : sourceNode->inputSockets()) {
                    if (input->type() == targetType && input->isConnected()) {
                        bypassInput = input;
                        break;
                    }
                }
                
                // Second pass: any connected input
                if (!bypassInput) {
                    for (NodeSocket* input : sourceNode->inputSockets()) {
                        if (input->isConnected()) {
                            bypassInput = input;
                            break;
                        }
                    }
                }
                
                if (bypassInput) {
                    return bypassInput->getValue(pos);
                }
                
                // No connected input, return default for target type
                return sourceSocket->defaultValue();
            }
            
            QVariant val = sourceNode->compute(pos, sourceSocket);
            
            // Type conversion
            if (sourceSocket->type() == m_type) {
                return val;
            }
            
            // Float -> Vector
            if (sourceSocket->type() == SocketType::Float && m_type == SocketType::Vector) {
                double v = val.toDouble();
                return QVector3D(v, v, v);
            }
            
            // Float -> Color
            if (sourceSocket->type() == SocketType::Float && m_type == SocketType::Color) {
                double v = val.toDouble();
                int gray = static_cast<int>(v * 255);
                return QColor(gray, gray, gray);
            }
            
            // Vector -> Color
            if (sourceSocket->type() == SocketType::Vector && m_type == SocketType::Color) {
                QVector3D v = val.value<QVector3D>();
                return QColor::fromRgbF(qBound(0.0f, v.x(), 1.0f), 
                                      qBound(0.0f, v.y(), 1.0f), 
                                      qBound(0.0f, v.z(), 1.0f));
            }
            
            // Color -> Vector
            if (sourceSocket->type() == SocketType::Color && m_type == SocketType::Vector) {
                QColor c = val.value<QColor>();
                return QVector3D(c.redF(), c.greenF(), c.blueF());
            }

            // Color -> Float (Luminance)
            if (sourceSocket->type() == SocketType::Color && m_type == SocketType::Float) {
                QColor c = val.value<QColor>();
                // Rec. 709 luminance
                return 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF();
            }

            // Vector -> Float (Average)
            if (sourceSocket->type() == SocketType::Vector && m_type == SocketType::Float) {
                QVector3D v = val.value<QVector3D>();
                return (v.x() + v.y() + v.z()) / 3.0;
            }
            
            return val;
        }
    }
    
    // 接続がない場合は自身の値（またはデフォルト値）を返す
    if (m_direction == SocketDirection::Input) {
        if (m_value.isValid()) {
            return m_value;
        }
        return m_defaultValue;
    }
    return m_value;
}

// NodeConnection implementation
NodeConnection::NodeConnection(NodeSocket* from, NodeSocket* to)
    : m_from(from)
    , m_to(to)
{
    if (from && to) {
        from->addConnection(to);
        to->addConnection(from);
    }
}

bool NodeConnection::isValid() const {
    return isValid(m_from, m_to);
}

bool NodeConnection::isValid(NodeSocket* from, NodeSocket* to) {
    if (!from || !to) return false;
    if (from->direction() != SocketDirection::Output) return false;
    if (to->direction() != SocketDirection::Input) return false;
    
    // Check for cycles
    // We are connecting Output (from) -> Input (to)
    // Check if to's node is an ancestor of from's node
    Node* sourceNode = from->parentNode();
    Node* targetNode = to->parentNode();
    
    if (sourceNode == targetNode) return false; // Self connection
    
    // BFS to check if targetNode is reachable from sourceNode's inputs (upstream)
    // Wait, connection is Source -> Target.
    // Cycle exists if there is a path Target -> ... -> Source.
    // So we need to check if Source is reachable from Target's outputs.
    
    QList<Node*> queue;
    QSet<Node*> visited;
    queue.append(targetNode);
    visited.insert(targetNode);
    
    while (!queue.isEmpty()) {
        Node* current = queue.takeFirst();
        if (current == sourceNode) return false; // Cycle detected
        
        for (NodeSocket* output : current->outputSockets()) {
            for (NodeSocket* input : output->connections()) {
                Node* nextNode = input->parentNode();
                if (nextNode && !visited.contains(nextNode)) {
                    visited.insert(nextNode);
                    queue.append(nextNode);
                }
            }
        }
    }

    // Allow implicit conversions
    // Float -> Vector/Color
    if (from->type() == SocketType::Float && 
        (to->type() == SocketType::Vector || to->type() == SocketType::Color)) return true;
        
    // Vector <-> Color
    if ((from->type() == SocketType::Vector && to->type() == SocketType::Color) ||
        (from->type() == SocketType::Color && to->type() == SocketType::Vector)) return true;

    // Color/Vector -> Float
    if ((from->type() == SocketType::Color || from->type() == SocketType::Vector) && 
        to->type() == SocketType::Float) return true;

    if (from->type() != to->type()) return false;
    return true;
}

// Node implementation
Node::Node(const QString& name)
    : m_name(name)
    , m_position(0, 0)
    , m_dirty(true)
{
}

Node::~Node() {
    // ソケットのクリーンアップ
    qDeleteAll(m_inputSockets);
    qDeleteAll(m_outputSockets);
}

void Node::addInputSocket(NodeSocket* socket) {
    m_inputSockets.append(socket);
}

void Node::addOutputSocket(NodeSocket* socket) {
    m_outputSockets.append(socket);
}

NodeSocket* Node::findInputSocket(const QString& name) const {
    qDebug() << "Node::findInputSocket" << m_name << name << "this:" << this << "sockets:" << m_inputSockets.size();
    for (NodeSocket* socket : m_inputSockets) {
        if (socket->name() == name) {
            return socket;
        }
    }
    return nullptr;
}

NodeSocket* Node::findOutputSocket(const QString& name) const {
    for (NodeSocket* socket : m_outputSockets) {
        if (socket->name() == name) {
            return socket;
        }
    }
    return nullptr;
}

void Node::setDirty(bool dirty) {
    bool stateChanged = (m_dirty != dirty);
    m_dirty = dirty;
    
    if (dirty) {
        // Debug: trace excessive setDirty calls
        static int callCount = 0;
        static QString lastName;
        callCount++;
        if (callCount > 10 && m_name == lastName) {
            qDebug() << "WARNING: Excessive setDirty calls for:" << m_name << "Count:" << callCount;
        }
        if (m_name != lastName) {
            callCount = 1;
            lastName = m_name;
        }
        
        // Always notify UI when marking dirty (even if already dirty)
        // This ensures repeated changes trigger UI updates
        notifyDirty();
        
        // Always propagate to downstream nodes, even if state didn't change.
        // Downstream nodes might have been cleaned (rendered) while this node remained dirty.
        for (NodeSocket* output : m_outputSockets) {
            for (NodeSocket* input : output->connections()) {
                if (input && input->parentNode()) {
                    input->parentNode()->setDirty(true);
                }
            }
        }
    }
}

QVariant Node::compute(const QVector3D& pos, NodeSocket* socket) {
    // デフォルト実装：現在の値を返す（定数ノードなど）
    // 多くのノードはこれをオーバーライドして座標に応じた値を返す
    if (socket) {
        return socket->value();
    }
    return QVariant();
}

QJsonObject Node::save() const {
    QJsonObject json;
    json["name"] = m_name;
    json["x"] = m_position.x();
    json["y"] = m_position.y();
    
    QJsonArray inputsArray;
    for (NodeSocket* socket : m_inputSockets) {
        inputsArray.append(socket->save());
    }
    json["inputs"] = inputsArray;
    
    // Save Parameters (Settings)
    QJsonObject paramsJson;
    for (const auto& param : parameters()) {
        // Save using the parameter name as key
        // Note: This assumes parameters have unique names
        // defaultValue in ParameterInfo MUST contain the CURRENT value for this to work
        paramsJson[param.name] = QJsonValue::fromVariant(param.defaultValue);
    }
    json["parameters"] = paramsJson;
    
    return json;
}

void Node::restore(const QJsonObject& json) {
    m_position.setX(json["x"].toDouble());
    m_position.setY(json["y"].toDouble());
    
    QJsonArray inputsArray = json["inputs"].toArray();
    for (const QJsonValue& val : inputsArray) {
        QJsonObject socketJson = val.toObject();
        QString socketName = socketJson["name"].toString();
        
        NodeSocket* socket = findInputSocket(socketName);
        if (socket) {
            socket->restore(socketJson);
        }
    }
    
    // Restore Parameters
    if (json.contains("parameters")) {
        QJsonObject paramsJson = json["parameters"].toObject();
        QVector<ParameterInfo> params = parameters();
        
        for (const auto& param : params) {
            if (paramsJson.contains(param.name)) {
                QJsonValue val = paramsJson[param.name];
                if (param.setter) {
                    param.setter(val.toVariant());
                }
            }
        }
    }
}

// NodeSocket Serialization
QJsonObject NodeSocket::save() const {
    QJsonObject json;
    json["name"] = m_name;
    
    // Save default value based on type
    if (m_type == SocketType::Float) {
        json["value"] = m_defaultValue.toDouble();
    } else if (m_type == SocketType::Integer) {
        json["value"] = m_defaultValue.toInt();
    } else if (m_type == SocketType::Vector) {
        QVector3D vec = m_defaultValue.value<QVector3D>();
        QJsonObject vecJson;
        vecJson["x"] = vec.x();
        vecJson["y"] = vec.y();
        vecJson["z"] = vec.z();
        json["value"] = vecJson;
    } else if (m_type == SocketType::Color) {
        QColor col = m_defaultValue.value<QColor>();
        QJsonObject colJson;
        colJson["r"] = col.red();
        colJson["g"] = col.green();
        colJson["b"] = col.blue();
        colJson["a"] = col.alpha();
        json["value"] = colJson;
    }
    
    return json;
}

void NodeSocket::restore(const QJsonObject& json) {
    if (!json.contains("value")) return;
    
    QJsonValue val = json["value"];
    
    if (m_type == SocketType::Float) {
        m_defaultValue = val.toDouble();
    } else if (m_type == SocketType::Integer) {
        m_defaultValue = val.toInt();
    } else if (m_type == SocketType::Vector) {
        QJsonObject vecJson = val.toObject();
        m_defaultValue = QVector3D(
            vecJson["x"].toDouble(),
            vecJson["y"].toDouble(),
            vecJson["z"].toDouble()
        );
    } else if (m_type == SocketType::Color) {
        QJsonObject colJson = val.toObject();
        m_defaultValue = QColor(
            colJson["r"].toInt(),
            colJson["g"].toInt(),
            colJson["b"].toInt(),
            colJson["a"].toInt()
        );
    }
}
