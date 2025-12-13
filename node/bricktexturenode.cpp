#include "bricktexturenode.h"
#include <cmath>
#include <QRandomGenerator>
#include <QColor>

BrickTextureNode::BrickTextureNode() : Node("Brick Texture") {
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    
    m_color1Input = new NodeSocket("Color1", SocketType::Color, SocketDirection::Input, this);
    m_color1Input->setDefaultValue(QColor(204, 204, 204)); // Grey
    
    m_color2Input = new NodeSocket("Color2", SocketType::Color, SocketDirection::Input, this);
    m_color2Input->setDefaultValue(QColor(51, 51, 51)); // Dark Grey
    
    m_mortarInput = new NodeSocket("Mortar", SocketType::Color, SocketDirection::Input, this);
    m_mortarInput->setDefaultValue(QColor(0, 0, 0)); // Black
    
    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(5.0);
    
    m_mortarSizeInput = new NodeSocket("Mortar Size", SocketType::Float, SocketDirection::Input, this);
    m_mortarSizeInput->setDefaultValue(0.02);
    
    m_mortarSmoothInput = new NodeSocket("Mortar Smooth", SocketType::Float, SocketDirection::Input, this);
    m_mortarSmoothInput->setDefaultValue(0.1);
    
    m_biasInput = new NodeSocket("Bias", SocketType::Float, SocketDirection::Input, this);
    m_biasInput->setDefaultValue(0.0);
    
    m_brickWidthInput = new NodeSocket("Brick Width", SocketType::Float, SocketDirection::Input, this);
    m_brickWidthInput->setDefaultValue(0.5);
    
    m_rowHeightInput = new NodeSocket("Row Height", SocketType::Float, SocketDirection::Input, this);
    m_rowHeightInput->setDefaultValue(0.25);

    addInputSocket(m_vectorInput);
    addInputSocket(m_color1Input);
    addInputSocket(m_color2Input);
    addInputSocket(m_mortarInput);
    addInputSocket(m_scaleInput);
    addInputSocket(m_mortarSizeInput);
    addInputSocket(m_mortarSmoothInput);
    addInputSocket(m_biasInput);
    addInputSocket(m_brickWidthInput);
    addInputSocket(m_rowHeightInput);

    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    m_facOutput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Output, this);

    addOutputSocket(m_colorOutput);
    addOutputSocket(m_facOutput);
}

BrickTextureNode::~BrickTextureNode() {}

void BrickTextureNode::evaluate() {
    // Stateless
}
QVector<Node::ParameterInfo> BrickTextureNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // Socket-matching parameters (enables UI widgets for unconnected sockets)
    params.append(ParameterInfo("Scale", 0.1, 50.0, m_scaleInput->defaultValue().toDouble(), 0.1, "Overall scale"));
    params.append(ParameterInfo("Mortar Size", 0.0, 0.5, m_mortarSizeInput->defaultValue().toDouble(), 0.01, "Mortar width"));
    params.append(ParameterInfo("Mortar Smooth", 0.0, 1.0, m_mortarSmoothInput->defaultValue().toDouble(), 0.01, "Mortar smoothness"));
    params.append(ParameterInfo("Bias", -1.0, 1.0, m_biasInput->defaultValue().toDouble(), 0.01, "Color bias"));
    params.append(ParameterInfo("Brick Width", 0.01, 1.0, m_brickWidthInput->defaultValue().toDouble(), 0.01, "Brick width ratio"));
    params.append(ParameterInfo("Row Height", 0.01, 1.0, m_rowHeightInput->defaultValue().toDouble(), 0.01, "Row height ratio"));
    
    // Custom parameters (not sockets)
    ParameterInfo offsetParam("Offset", 0.0, 1.0, m_offset, 0.01, "Row Offset");
    offsetParam.setter = [this](const QVariant& v) { const_cast<BrickTextureNode*>(this)->setOffset(v.toDouble()); };
    params.append(offsetParam);

    ParameterInfo offsetFreqParam("Offset Frequency", 1.0, 10.0, m_offsetFrequency, 1.0, "Offset Frequency");
    offsetFreqParam.setter = [this](const QVariant& v) { const_cast<BrickTextureNode*>(this)->setOffsetFrequency(v.toInt()); };
    params.append(offsetFreqParam);

    ParameterInfo squashParam("Squash", 0.0, 10.0, m_squash, 0.1, "Squash Amount");
    squashParam.setter = [this](const QVariant& v) { const_cast<BrickTextureNode*>(this)->setSquash(v.toDouble()); };
    params.append(squashParam);

    ParameterInfo squashFreqParam("Squash Frequency", 1.0, 10.0, m_squashFrequency, 1.0, "Squash Frequency");
    squashFreqParam.setter = [this](const QVariant& v) { const_cast<BrickTextureNode*>(this)->setSquashFrequency(v.toInt()); };
    params.append(squashFreqParam);

    return params;
}

void BrickTextureNode::setOffset(double v) { m_offset = v; setDirty(true); }
void BrickTextureNode::setOffsetFrequency(int v) { m_offsetFrequency = v; setDirty(true); }
void BrickTextureNode::setSquash(double v) { m_squash = v; setDirty(true); }
void BrickTextureNode::setSquashFrequency(int v) { m_squashFrequency = v; setDirty(true); }

QVariant BrickTextureNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D p = pos;
    if (m_vectorInput->isConnected()) {
        p = m_vectorInput->getValue(pos).value<QVector3D>();
    }

    double scale = m_scaleInput->getValue(pos).toDouble();
    double mortarSize = m_mortarSizeInput->getValue(pos).toDouble();
    double brickWidth = m_brickWidthInput->getValue(pos).toDouble();
    double rowHeight = m_rowHeightInput->getValue(pos).toDouble();
    double bias = m_biasInput->getValue(pos).toDouble(); // -1 to 1

    QColor c1 = m_color1Input->getValue(pos).value<QColor>();
    QColor c2 = m_color2Input->getValue(pos).value<QColor>();
    QColor mortar = m_mortarInput->getValue(pos).value<QColor>();

    // Apply scale
    double x = p.x() * scale;
    double y = p.y() * scale;

    // Row calculation
    double row = y / rowHeight;
    int rowIndex = static_cast<int>(std::floor(row));
    
    // Offset logic
    double rowOffset = 0.0;
    if (m_offsetFrequency > 0 && (rowIndex % m_offsetFrequency) != 0) { // Simplified logic
         // Blender logic: if (row_index % offset_freq) offset = offset_amount
         // Actually it's usually alternating.
         if (rowIndex % 2 != 0) rowOffset = m_offset * brickWidth; // Simple alternating
    }

    // Brick calculation
    double col = (x + rowOffset) / brickWidth;
    int colIndex = static_cast<int>(std::floor(col));

    // Local coordinates within brick cell (0-1)
    double u = col - colIndex;
    double v = row - rowIndex;

    // Mortar logic
    // Mortar size should be consistent in absolute units (relative to scale?)
    // Blender's Mortar Size is 0-1.
    // If we assume Mortar Size is "fraction of the larger dimension" or similar?
    // Actually, to make it visually consistent, we need to account for the aspect ratio of the brick.
    // Threshold U = MortarSize / BrickWidth
    // Threshold V = MortarSize / RowHeight
    // But we need to handle division by zero.
    
    double thresholdU = (brickWidth > 0.0001) ? (mortarSize / brickWidth) : 0.0;
    double thresholdV = (rowHeight > 0.0001) ? (mortarSize / rowHeight) : 0.0;
    
    double halfU = thresholdU * 0.5;
    double halfV = thresholdV * 0.5;
    
    bool inMortar = (u < halfU || u > 1.0 - halfU || v < halfV || v > 1.0 - halfV);

    if (inMortar) {
        if (socket == m_facOutput) return 0.0; // Mortar is 0? Or 1? Usually 0 in Fac?
        return mortar;
    }

    // Brick Color
    // Randomize based on row/col index
    double seed = rowIndex * 34.0 + colIndex * 12.0;
    double rand = std::abs(std::sin(seed) * 1000.0);
    rand = rand - std::floor(rand); // 0-1

    // Bias shifts the probability
    // Bias -1 -> Color1, 1 -> Color2
    // 0 -> 50/50
    // Threshold = 0.5 - bias * 0.5
    double threshold = 0.5 - bias * 0.5;
    
    QColor brickColor = (rand < threshold) ? c1 : c2;

    if (socket == m_facOutput) return 1.0;
    return brickColor;
}
