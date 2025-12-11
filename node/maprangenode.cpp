#include "maprangenode.h"
#include <algorithm>

MapRangeNode::MapRangeNode() : Node("Map Range"), m_clamp(false) {
    m_valueInput = new NodeSocket("Value", SocketType::Float, SocketDirection::Input, this);
    m_valueInput->setDefaultValue(0.5);
    addInputSocket(m_valueInput);
    
    m_fromMinInput = new NodeSocket("From Min", SocketType::Float, SocketDirection::Input, this);
    m_fromMinInput->setDefaultValue(0.0);
    addInputSocket(m_fromMinInput);
    
    m_fromMaxInput = new NodeSocket("From Max", SocketType::Float, SocketDirection::Input, this);
    m_fromMaxInput->setDefaultValue(1.0);
    addInputSocket(m_fromMaxInput);
    
    m_toMinInput = new NodeSocket("To Min", SocketType::Float, SocketDirection::Input, this);
    m_toMinInput->setDefaultValue(0.0);
    addInputSocket(m_toMinInput);
    
    m_toMaxInput = new NodeSocket("To Max", SocketType::Float, SocketDirection::Input, this);
    m_toMaxInput->setDefaultValue(1.0);
    addInputSocket(m_toMaxInput);
    
    m_resultOutput = new NodeSocket("Result", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_resultOutput);
}

MapRangeNode::~MapRangeNode() {}

void MapRangeNode::evaluate() {
    // Nothing to pre-calculate
}

void MapRangeNode::setClamp(bool c) {
    if (m_clamp == c) return;
    m_clamp = c;
    setDirty(true);
}

QVector<Node::ParameterInfo> MapRangeNode::parameters() const {
    return {
        ParameterInfo("Clamp", m_clamp, 
            [this](const QVariant& v) { 
                const_cast<MapRangeNode*>(this)->setClamp(v.toBool()); 
            }),
        ParameterInfo("Value", -10000.0, 10000.0, 0.5, 0.01),
        ParameterInfo("From Min", -1000.0, 1000.0, 0.0, 0.1),
        ParameterInfo("From Max", -1000.0, 1000.0, 1.0, 0.1),
        ParameterInfo("To Min", -1000.0, 1000.0, 0.0, 0.1),
        ParameterInfo("To Max", -1000.0, 1000.0, 1.0, 0.1)
    };
}

QVariant MapRangeNode::compute(const QVector3D& pos, NodeSocket* socket) {
    if (socket == m_resultOutput) {
        double val = m_valueInput->isConnected() ? m_valueInput->getValue(pos).toDouble() : m_valueInput->value().toDouble();
        double fromMin = m_fromMinInput->isConnected() ? m_fromMinInput->getValue(pos).toDouble() : m_fromMinInput->value().toDouble();
        double fromMax = m_fromMaxInput->isConnected() ? m_fromMaxInput->getValue(pos).toDouble() : m_fromMaxInput->value().toDouble();
        double toMin = m_toMinInput->isConnected() ? m_toMinInput->getValue(pos).toDouble() : m_toMinInput->value().toDouble();
        double toMax = m_toMaxInput->isConnected() ? m_toMaxInput->getValue(pos).toDouble() : m_toMaxInput->value().toDouble();
        
        if (std::abs(fromMax - fromMin) < 0.000001) return toMin;
        
        double result = toMin + (val - fromMin) / (fromMax - fromMin) * (toMax - toMin);
        
        if (m_clamp) {
            double rMin = std::min(toMin, toMax);
            double rMax = std::max(toMin, toMax);
            result = std::max(rMin, std::min(rMax, result));
        }
        
        return result;
    }
    
    return QVariant();
}

QJsonObject MapRangeNode::save() const {
    QJsonObject json = Node::save();
    json["clamp"] = m_clamp;
    return json;
}

void MapRangeNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("clamp")) m_clamp = json["clamp"].toBool();
}
