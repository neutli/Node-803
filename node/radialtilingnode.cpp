#include "radialtilingnode.h"
#include <cmath>

RadialTilingNode::RadialTilingNode() : Node("Radial Tiling") {
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    
    m_sidesInput = new NodeSocket("Sides", SocketType::Float, SocketDirection::Input, this);
    m_sidesInput->setDefaultValue(5.0);
    
    m_roundnessInput = new NodeSocket("Roundness", SocketType::Float, SocketDirection::Input, this);
    m_roundnessInput->setDefaultValue(1.0);

    addInputSocket(m_vectorInput);
    addInputSocket(m_sidesInput);
    addInputSocket(m_roundnessInput);

    m_output = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Output, this);
    addOutputSocket(m_output);
}

RadialTilingNode::~RadialTilingNode() {}

QVector<Node::ParameterInfo> RadialTilingNode::parameters() const {
    return {
        ParameterInfo("Sides", 1.0, 32.0, 5.0),
        ParameterInfo("Roundness", 0.0, 1.0, 1.0)
    };
}

void RadialTilingNode::evaluate() {
    // No internal state to update
}

QVariant RadialTilingNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D p;
    if (m_vectorInput->isConnected()) {
        p = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        p = pos;
    }

    double sides = m_sidesInput->getValue(pos).toDouble();
    if (sides < 1.0) sides = 1.0;

    // Center UV
    double u = p.x() - 0.5;
    double v = p.y() - 0.5;

    // Polar
    double angle = std::atan2(v, u); // -PI to PI
    double radius = std::sqrt(u*u + v*v);

    // Normalize angle to 0-1
    double angleNorm = (angle / (2.0 * M_PI)) + 0.5; // 0 to 1

    // Tiling
    // Multiply by sides
    double sector = angleNorm * sides;
    double sectorIndex = std::floor(sector);
    double sectorFraction = sector - sectorIndex; // 0-1 within sector

    // Map back to UV?
    // Usually Radial Tiling maps (angle, radius) to (u, v)
    // u = sectorFraction
    // v = radius
    
    return QVector3D(sectorFraction, radius, 0.0);
}
