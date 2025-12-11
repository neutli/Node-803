#include "bumpnode.h"
#include <cmath>

BumpNode::BumpNode() : Node("Bump"), m_invert(false) {
    m_strengthInput = new NodeSocket("Strength", SocketType::Float, SocketDirection::Input, this);
    m_strengthInput->setDefaultValue(1.0);
    addInputSocket(m_strengthInput);
    
    m_distanceInput = new NodeSocket("Distance", SocketType::Float, SocketDirection::Input, this);
    m_distanceInput->setDefaultValue(1.0);
    addInputSocket(m_distanceInput);
    
    m_heightInput = new NodeSocket("Height", SocketType::Float, SocketDirection::Input, this);
    m_heightInput->setDefaultValue(0.0);
    addInputSocket(m_heightInput);
    
    m_normalInput = new NodeSocket("Normal", SocketType::Vector, SocketDirection::Input, this);
    m_normalInput->setDefaultValue(QVector3D(0, 0, 1));
    addInputSocket(m_normalInput);
    
    m_normalOutput = new NodeSocket("Normal", SocketType::Vector, SocketDirection::Output, this);
    addOutputSocket(m_normalOutput);
}

BumpNode::~BumpNode() {}

void BumpNode::evaluate() {
    // Nothing to pre-calculate
}

void BumpNode::setInvert(bool inv) {
    if (m_invert == inv) return;
    m_invert = inv;
    setDirty(true);
}

QVector<Node::ParameterInfo> BumpNode::parameters() const {
    return {
        ParameterInfo("Strength", 0.0, 1.0, 1.0, 0.01, "Bump strength"),
        ParameterInfo("Distance", 0.0, 100.0, 1.0, 0.1, "Bump distance")
    };
}

QVariant BumpNode::compute(const QVector3D& pos, NodeSocket* socket) {
    if (socket == m_normalOutput) {
        // Mute support: Pass through Normal input or default
        if (isMuted()) {
            if (m_normalInput->isConnected()) {
                return m_normalInput->getValue(pos);
            }
            return m_normalInput->defaultValue();
        }

        // Simple bump mapping simulation
        // We need gradients. Since we don't have analytical derivatives, we use finite differences.
        
        double strength = m_strengthInput->isConnected() ? m_strengthInput->getValue(pos).toDouble() : m_strengthInput->value().toDouble();
        double distance = m_distanceInput->isConnected() ? m_distanceInput->getValue(pos).toDouble() : m_distanceInput->value().toDouble();
        
        if (m_invert) distance *= -1.0;
        
        QVector3D normal = m_normalInput->isConnected() ? m_normalInput->getValue(pos).value<QVector3D>() : m_normalInput->value().value<QVector3D>();
        normal.normalize();
        
        // Calculate gradient
        double delta = 1.0; // 1 pixel step
        double h_center = 0.0;
        
        if (m_heightInput->isConnected()) {
            h_center = m_heightInput->getValue(pos).toDouble();
            double h_x = m_heightInput->getValue(pos + QVector3D(delta, 0, 0)).toDouble();
            double h_y = m_heightInput->getValue(pos + QVector3D(0, delta, 0)).toDouble();
            
            double dHdX = (h_x - h_center); // / delta (which is 1.0)
            double dHdY = (h_y - h_center);
            
            // Perturb normal
            // Surface gradient in pixel space.
            // Strength scales the height effect.
            // Distance is usually world units, but here arbitrary.
            
            double factor = strength * distance; // Simplified scaling
            
            // Normal = normalize(vec3(-dHdX, -dHdY, 1.0))
            // This assumes the base surface is flat (0,0,1)
            QVector3D perturbed(-dHdX * factor, -dHdY * factor, 1.0);
            return perturbed.normalized();
        }
        
        return normal;
    }
    
    return QVariant();
}

QJsonObject BumpNode::save() const {
    QJsonObject json = Node::save();
    json["invert"] = m_invert;
    return json;
}

void BumpNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("invert")) m_invert = json["invert"].toBool();
}
