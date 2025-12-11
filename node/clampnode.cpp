#include "clampnode.h"
#include <algorithm>

ClampNode::ClampNode() : Node("Clamp") {
    m_valueInput = new NodeSocket("Value", SocketType::Float, SocketDirection::Input, this);
    m_minInput = new NodeSocket("Min", SocketType::Float, SocketDirection::Input, this);
    m_minInput->setDefaultValue(0.0);
    m_maxInput = new NodeSocket("Max", SocketType::Float, SocketDirection::Input, this);
    m_maxInput->setDefaultValue(1.0);

    addInputSocket(m_valueInput);
    addInputSocket(m_minInput);
    addInputSocket(m_maxInput);

    m_output = new NodeSocket("Result", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_output);
}

ClampNode::~ClampNode() {}

QVector<Node::ParameterInfo> ClampNode::parameters() const {
    return {
        ParameterInfo("Min", -100.0, 100.0, 0.0),
        ParameterInfo("Max", -100.0, 100.0, 1.0)
    };
}

void ClampNode::evaluate() {
    // No internal state
}

QVariant ClampNode::compute(const QVector3D& pos, NodeSocket* socket) {
    double val = m_valueInput->getValue(pos).toDouble();
    double min = m_minInput->getValue(pos).toDouble();
    double max = m_maxInput->getValue(pos).toDouble();

    return std::max(min, std::min(val, max));
}
