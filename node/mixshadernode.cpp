#include "mixshadernode.h"
#include <QColor>

MixShaderNode::MixShaderNode() : Node("Mix Shader") {
    m_facInput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Input, this);
    m_facInput->setDefaultValue(0.5);
    addInputSocket(m_facInput);
    
    m_shader1Input = new NodeSocket("Shader 1", SocketType::Color, SocketDirection::Input, this); // Using Color for Shader
    m_shader1Input->setDefaultValue(QColor(Qt::black));
    addInputSocket(m_shader1Input);
    
    m_shader2Input = new NodeSocket("Shader 2", SocketType::Color, SocketDirection::Input, this);
    m_shader2Input->setDefaultValue(QColor(Qt::white));
    addInputSocket(m_shader2Input);
    
    m_shaderOutput = new NodeSocket("Shader", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_shaderOutput);
}

MixShaderNode::~MixShaderNode() {}

void MixShaderNode::evaluate() {
    // Nothing to pre-calculate
}

QVector<Node::ParameterInfo> MixShaderNode::parameters() const {
    return {
        ParameterInfo("Fac", 0.0, 1.0, 0.5, 0.01, "Mixing factor")
    };
}

QVariant MixShaderNode::compute(const QVector3D& pos, NodeSocket* socket) {
    if (socket == m_shaderOutput) {
        double fac = m_facInput->isConnected() ? m_facInput->getValue(pos).toDouble() : m_facInput->value().toDouble();
        fac = std::max(0.0, std::min(1.0, fac));
        
        QColor c1 = m_shader1Input->isConnected() ? m_shader1Input->getValue(pos).value<QColor>() : m_shader1Input->value().value<QColor>();
        QColor c2 = m_shader2Input->isConnected() ? m_shader2Input->getValue(pos).value<QColor>() : m_shader2Input->value().value<QColor>();
        
        double r = c1.redF() * (1.0 - fac) + c2.redF() * fac;
        double g = c1.greenF() * (1.0 - fac) + c2.greenF() * fac;
        double b = c1.blueF() * (1.0 - fac) + c2.blueF() * fac;
        double a = c1.alphaF() * (1.0 - fac) + c2.alphaF() * fac;
        
        return QColor::fromRgbF(r, g, b, a);
    }
    
    return QVariant();
}
