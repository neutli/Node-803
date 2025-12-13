#include "scatteronpointsnode.h"
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ScatterOnPointsNode::ScatterOnPointsNode()
    : Node("Scatter on Points")
{
    // Input sockets
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    m_vectorInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_vectorInput);
    
    m_textureInput = new NodeSocket("Texture", SocketType::Color, SocketDirection::Input, this);
    m_textureInput->setDefaultValue(QVector4D(1, 1, 1, 1));
    addInputSocket(m_textureInput);
    
    m_densityInput = new NodeSocket("Density", SocketType::Float, SocketDirection::Input, this);
    m_densityInput->setDefaultValue(1.0);
    addInputSocket(m_densityInput);
    
    // Output sockets
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
    
    m_valueOutput = new NodeSocket("Value", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_valueOutput);
}

ScatterOnPointsNode::~ScatterOnPointsNode() {}

void ScatterOnPointsNode::evaluate() {
    // Stateless
}

QVariant ScatterOnPointsNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QMutexLocker locker(&m_mutex);
    
    // Get input coordinates
    QVector3D vec;
    if (m_vectorInput->isConnected()) {
        vec = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        vec = QVector3D(pos.x() / 512.0, pos.y() / 512.0, 0.0);
    }
    
    double x = vec.x();
    double y = vec.y();
    
    // Generate grid points and check each
    std::mt19937 rng(m_seed);
    std::uniform_real_distribution<double> dist(-0.5, 0.5);
    
    QVector4D resultColor(0, 0, 0, 0);
    double resultValue = 0.0;
    
    double cellWidth = 1.0 / m_pointsX;
    double cellHeight = 1.0 / m_pointsY;
    
    // Check nearby cells
    int cellX = static_cast<int>(x * m_pointsX);
    int cellY = static_cast<int>(y * m_pointsY);
    
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = cellX + dx;
            int cy = cellY + dy;
            
            if (cx < 0 || cx >= m_pointsX || cy < 0 || cy >= m_pointsY) continue;
            
            // Seed RNG for this cell
            rng.seed(m_seed + cx * 1000 + cy);
            
            // Check density mask
            double centerX = (cx + 0.5) * cellWidth;
            double centerY = (cy + 0.5) * cellHeight;
            
            if (m_densityInput->isConnected()) {
                QVector3D densityPos(centerX * 512.0, centerY * 512.0, 0);
                double density = m_densityInput->getValue(densityPos).toDouble();
                if (dist(rng) + 0.5 > density) continue;
            }
            
            // Random variations for this instance
            double instanceScale = m_scale * (1.0 + dist(rng) * m_scaleVariation * 2.0);
            double instanceRotation = m_rotation + dist(rng) * m_rotationVariation * 2.0;
            
            // Transform coordinates relative to cell center
            double localX = (x - centerX) / instanceScale;
            double localY = (y - centerY) / instanceScale;
            
            // Apply rotation
            double rotRad = instanceRotation * M_PI / 180.0;
            double cosR = std::cos(rotRad);
            double sinR = std::sin(rotRad);
            double rotX = localX * cosR - localY * sinR;
            double rotY = localX * sinR + localY * cosR;
            
            // Check if inside instance bounds
            if (std::abs(rotX) > 0.5 || std::abs(rotY) > 0.5) continue;
            
            // Sample texture at transformed coordinates
            if (m_textureInput->isConnected()) {
                QVector3D texPos((rotX + 0.5) * 512.0, (rotY + 0.5) * 512.0, 0);
                QVariant texVal = m_textureInput->getValue(texPos);
                
                if (texVal.canConvert<QVector4D>()) {
                    QVector4D color = texVal.value<QVector4D>();
                    if (color.w() > resultColor.w()) {
                        resultColor = color;
                        resultValue = (color.x() + color.y() + color.z()) / 3.0;
                    }
                } else {
                    double v = texVal.toDouble();
                    if (v > resultValue) {
                        resultValue = v;
                        resultColor = QVector4D(v, v, v, 1.0);
                    }
                }
            } else {
                // No texture connected, just show point
                resultColor = QVector4D(1, 1, 1, 1);
                resultValue = 1.0;
            }
        }
    }
    
    if (socket == m_colorOutput) {
        return QVariant::fromValue(resultColor);
    }
    return resultValue;
}

QVector<Node::ParameterInfo> ScatterOnPointsNode::parameters() const {
    QVector<ParameterInfo> params;
    
    params.append(ParameterInfo("Points X", 1.0, 20.0, (double)m_pointsX, 1.0, "Grid columns"));
    params.append(ParameterInfo("Points Y", 1.0, 20.0, (double)m_pointsY, 1.0, "Grid rows"));
    params.append(ParameterInfo("Scale", 0.01, 1.0, m_scale, 0.01, "Instance scale"));
    params.append(ParameterInfo("Scale Var", 0.0, 1.0, m_scaleVariation, 0.01, "Random scale variation"));
    params.append(ParameterInfo("Rotation", 0.0, 360.0, m_rotation, 1.0, "Base rotation"));
    params.append(ParameterInfo("Rotation Var", 0.0, 180.0, m_rotationVariation, 1.0, "Random rotation variation"));
    
    ParameterInfo seedInfo;
    seedInfo.type = ParameterInfo::Int;
    seedInfo.name = "Seed";
    seedInfo.min = 0;
    seedInfo.max = 9999;
    seedInfo.defaultValue = m_seed;
    seedInfo.step = 1;
    seedInfo.tooltip = "Random seed";
    seedInfo.setter = [this](const QVariant& v) {
        const_cast<ScatterOnPointsNode*>(this)->m_seed = v.toInt();
        const_cast<ScatterOnPointsNode*>(this)->setDirty(true);
    };
    params.append(seedInfo);
    
    return params;
}

QJsonObject ScatterOnPointsNode::save() const {
    QJsonObject json = Node::save();
    json["type"] = "Scatter on Points";
    json["pointsX"] = m_pointsX;
    json["pointsY"] = m_pointsY;
    json["scale"] = m_scale;
    json["scaleVariation"] = m_scaleVariation;
    json["rotation"] = m_rotation;
    json["rotationVariation"] = m_rotationVariation;
    json["seed"] = m_seed;
    return json;
}

void ScatterOnPointsNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("pointsX")) m_pointsX = json["pointsX"].toInt();
    if (json.contains("pointsY")) m_pointsY = json["pointsY"].toInt();
    if (json.contains("scale")) m_scale = json["scale"].toDouble();
    if (json.contains("scaleVariation")) m_scaleVariation = json["scaleVariation"].toDouble();
    if (json.contains("rotation")) m_rotation = json["rotation"].toDouble();
    if (json.contains("rotationVariation")) m_rotationVariation = json["rotationVariation"].toDouble();
    if (json.contains("seed")) m_seed = json["seed"].toInt();
}
