#include "separatexyznode.h"

SeparateXYZNode::SeparateXYZNode() : Node("Separate XYZ") {
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vectorInput);

    m_xOutput = new NodeSocket("X", SocketType::Float, SocketDirection::Output, this);
    m_yOutput = new NodeSocket("Y", SocketType::Float, SocketDirection::Output, this);
    m_zOutput = new NodeSocket("Z", SocketType::Float, SocketDirection::Output, this);

    addOutputSocket(m_xOutput);
    addOutputSocket(m_yOutput);
    addOutputSocket(m_zOutput);
}

SeparateXYZNode::~SeparateXYZNode() {}

QVector<Node::ParameterInfo> SeparateXYZNode::parameters() const {
    return {
        ParameterInfo("Vector", -10000.0, 10000.0, 0.0)
    };
}

void SeparateXYZNode::evaluate() {
    // Stateless
}

QVariant SeparateXYZNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D input = pos;
    if (m_vectorInput->isConnected()) {
        input = m_vectorInput->getValue(pos).value<QVector3D>();
    }

    if (socket == m_xOutput) return input.x();
    if (socket == m_yOutput) return input.y();
    if (socket == m_zOutput) return input.z();

    return 0.0;
}
