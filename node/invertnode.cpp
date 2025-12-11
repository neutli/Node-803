#include "invertnode.h"
#include <QColor>

InvertNode::InvertNode() : Node("Invert") {
    // Input: Color to invert
    m_colorInput = new NodeSocket("Color", SocketType::Color, SocketDirection::Input, this);
    m_colorInput->setDefaultValue(QColor(255, 255, 255)); // White default
    
    // Input: Inversion factor (0 = original, 1 = fully inverted)
    m_facInput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Input, this);
    m_facInput->setDefaultValue(1.0); // Fully inverted by default
    
    addInputSocket(m_colorInput);
    addInputSocket(m_facInput);
    
    // Output: Inverted color
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
}

InvertNode::~InvertNode() {}

QVector<Node::ParameterInfo> InvertNode::parameters() const {
    return {
        ParameterInfo("Fac", 0.0, 1.0, 1.0)
    };
}

void InvertNode::evaluate() {
    // Stateless evaluation
}

QVariant InvertNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // Get input color
    QColor inputColor;
    if (m_colorInput->isConnected()) {
        QVariant colorVar = m_colorInput->getValue(pos);
        if (colorVar.canConvert<QColor>()) {
            inputColor = colorVar.value<QColor>();
        } else if (colorVar.canConvert<double>()) {
            // Grayscale input
            int gray = static_cast<int>(colorVar.toDouble() * 255);
            gray = qBound(0, gray, 255);
            inputColor = QColor(gray, gray, gray);
        } else {
            inputColor = QColor(0, 0, 0);
        }
    } else {
        inputColor = m_colorInput->defaultValue().value<QColor>();
    }
    
    // Get inversion factor
    double fac = m_facInput->isConnected() ? 
        m_facInput->getValue(pos).toDouble() : 
        this->fac();
    fac = qBound(0.0, fac, 1.0); // Clamp to 0-1
    
    // Invert color with factor
    // inverted = original * (1 - fac) + (1 - original) * fac
    QColor invertedColor;
    invertedColor.setRgbF(
        inputColor.redF() * (1.0 - fac) + (1.0 - inputColor.redF()) * fac,
        inputColor.greenF() * (1.0 - fac) + (1.0 - inputColor.greenF()) * fac,
        inputColor.blueF() * (1.0 - fac) + (1.0 - inputColor.blueF()) * fac,
        inputColor.alphaF() // Preserve alpha
    );
    
    return invertedColor;
}

double InvertNode::fac() const {
    return m_facInput->defaultValue().toDouble();
}

void InvertNode::setFac(double v) {
    m_facInput->setDefaultValue(v);
    setDirty(true);
}
