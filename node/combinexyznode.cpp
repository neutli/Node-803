#include "combinexyznode.h"

CombineXYZNode::CombineXYZNode() : Node("Combine XYZ") {
    m_xInput = new NodeSocket("X", SocketType::Float, SocketDirection::Input, this);
    m_yInput = new NodeSocket("Y", SocketType::Float, SocketDirection::Input, this);
    m_zInput = new NodeSocket("Z", SocketType::Float, SocketDirection::Input, this);
    
    addInputSocket(m_xInput);
    addInputSocket(m_yInput);
    addInputSocket(m_zInput);

    m_vectorOutput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Output, this);
    addOutputSocket(m_vectorOutput);
}

CombineXYZNode::~CombineXYZNode() {}

QVector<Node::ParameterInfo> CombineXYZNode::parameters() const {
    return {};
}

void CombineXYZNode::evaluate() {
    // Stateless
}

QVariant CombineXYZNode::compute(const QVector3D& pos, NodeSocket* socket) {
    double x = m_xInput->isConnected() ? m_xInput->getValue(pos).toDouble() : m_xInput->defaultValue().toDouble();
    double y = m_yInput->isConnected() ? m_yInput->getValue(pos).toDouble() : m_yInput->defaultValue().toDouble();
    double z = m_zInput->isConnected() ? m_zInput->getValue(pos).toDouble() : m_zInput->defaultValue().toDouble();

    return QVector3D(x, y, z);
}
