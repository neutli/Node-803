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
        // Handle input color (support both QColor and QVector4D from Image Node)
        QVector4D baseVec(0.8f, 0.8f, 0.8f, 1.0f); // Default gray
        
        if (m_baseColorInput->isConnected()) {
            QVariant val = m_baseColorInput->getValue(pos);
            if (val.canConvert<QVector4D>()) {
                baseVec = val.value<QVector4D>();
            } else if (val.canConvert<QColor>()) {
                QColor c = val.value<QColor>();
                baseVec = QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF());
            }
        } else {
             QColor c = m_baseColorInput->value().value<QColor>();
             baseVec = QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        }

        double metallic = m_metallicInput->isConnected() ? m_metallicInput->getValue(pos).toDouble() : m_metallicInput->value().toDouble();
        double roughness = m_roughnessInput->isConnected() ? m_roughnessInput->getValue(pos).toDouble() : m_roughnessInput->value().toDouble();
        
        QVector3D normal(0, 0, 1);
        if (m_normalInput->isConnected()) {
             QVariant nVal = m_normalInput->getValue(pos);
             if (nVal.canConvert<QVector3D>()) {
                 normal = nVal.value<QVector3D>();
             }
        }

        // Simple lighting simulation
        QVector3D lightDir(0.5f, 0.5f, 1.0f);
        lightDir.normalize();
        
        double dot = std::max(0.0f, QVector3D::dotProduct(normal.normalized(), lightDir));
        
        // Simple Diffuse + Fake Specular logic
        // Mix base color with lighting
        float r = baseVec.x() * (dot * (1.0 - metallic) + 0.1); // Ambient 0.1
        float g = baseVec.y() * (dot * (1.0 - metallic) + 0.1);
        float b = baseVec.z() * (dot * (1.0 - metallic) + 0.1);
        
        // Output as QVector4D to prevent channel swapping issues
        return QVector4D(std::min(1.0f, r), std::min(1.0f, g), std::min(1.0f, b), 1.0f);
    }
    
    return QVariant();
}
