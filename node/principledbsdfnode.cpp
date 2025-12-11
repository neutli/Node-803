#include "principledbsdfnode.h"
#include <QColor>

PrincipledBSDFNode::PrincipledBSDFNode() : Node("Principled BSDF") {
    m_baseColorInput = new NodeSocket("Base Color", SocketType::Color, SocketDirection::Input, this);
    m_baseColorInput->setDefaultValue(QColor(200, 200, 200));
    addInputSocket(m_baseColorInput);
    
    m_metallicInput = new NodeSocket("Metallic", SocketType::Float, SocketDirection::Input, this);
    m_metallicInput->setDefaultValue(0.0);
    addInputSocket(m_metallicInput);
    
    m_roughnessInput = new NodeSocket("Roughness", SocketType::Float, SocketDirection::Input, this);
    m_roughnessInput->setDefaultValue(0.5);
    addInputSocket(m_roughnessInput);
    
    m_iorInput = new NodeSocket("IOR", SocketType::Float, SocketDirection::Input, this);
    m_iorInput->setDefaultValue(1.45);
    addInputSocket(m_iorInput);
    
    m_alphaInput = new NodeSocket("Alpha", SocketType::Float, SocketDirection::Input, this);
    m_alphaInput->setDefaultValue(1.0);
    addInputSocket(m_alphaInput);
    
    m_normalInput = new NodeSocket("Normal", SocketType::Vector, SocketDirection::Input, this);
    m_normalInput->setDefaultValue(QVector3D(0, 0, 1));
    addInputSocket(m_normalInput);
    
    // Output is technically a Shader, but we'll use Color for visualization in this simplified engine
    m_bsdfOutput = new NodeSocket("BSDF", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_bsdfOutput);
}

PrincipledBSDFNode::~PrincipledBSDFNode() {}

void PrincipledBSDFNode::evaluate() {
    // Nothing to pre-calculate
}

QVector<Node::ParameterInfo> PrincipledBSDFNode::parameters() const {
    return {
        ParameterInfo("Metallic", 0.0, 1.0, 0.0, 0.01),
        ParameterInfo("Roughness", 0.0, 1.0, 0.5, 0.01),
        ParameterInfo("IOR", 0.0, 3.0, 1.45, 0.01),
        ParameterInfo("Alpha", 0.0, 1.0, 1.0, 0.01)
    };
}

QVariant PrincipledBSDFNode::compute(const QVector3D& pos, NodeSocket* socket) {
    if (socket == m_bsdfOutput) {
        // Simplified shading for visualization
        QColor baseColor = m_baseColorInput->isConnected() ? m_baseColorInput->getValue(pos).value<QColor>() : m_baseColorInput->value().value<QColor>();
        double metallic = m_metallicInput->isConnected() ? m_metallicInput->getValue(pos).toDouble() : m_metallicInput->value().toDouble();
        double roughness = m_roughnessInput->isConnected() ? m_roughnessInput->getValue(pos).toDouble() : m_roughnessInput->value().toDouble();
        QVector3D normal = m_normalInput->isConnected() ? m_normalInput->getValue(pos).value<QVector3D>() : m_normalInput->value().value<QVector3D>();
        
        // Simple lighting simulation
        QVector3D lightDir(1, 1, 1);
        lightDir.normalize();
        
        double dot = QVector3D::dotProduct(normal.normalized(), lightDir);
        double diff = std::max(0.0, dot);
        
        // Mix with metallic (darkens diffuse)
        double r = baseColor.redF() * (diff * (1.0 - metallic) + metallic * 0.2); // Fake metallic reflection
        double g = baseColor.greenF() * (diff * (1.0 - metallic) + metallic * 0.2);
        double b = baseColor.blueF() * (diff * (1.0 - metallic) + metallic * 0.2);
        
        // Specular highlight (very fake)
        if (dot > 0.9 + roughness * 0.1) {
            double spec = (dot - (0.9 + roughness * 0.1)) * 10.0;
            r += spec; g += spec; b += spec;
        }
        
        return QColor::fromRgbF(std::min(1.0, r), std::min(1.0, g), std::min(1.0, b));
    }
    
    return QVariant();
}
