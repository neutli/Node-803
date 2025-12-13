#include "gabortexturenode.h"
#include <QVector3D>
#include <cmath>

GaborTextureNode::GaborTextureNode()
    : Node("Gabor Texture")
    , m_noise(std::make_unique<PerlinNoise>(803))
{
    // Input sockets
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    m_vectorInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_vectorInput);
    
    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(5.0);
    addInputSocket(m_scaleInput);
    
    // Gabor-specific parameters
    m_frequencyInput = new NodeSocket("Frequency", SocketType::Float, SocketDirection::Input, this);
    m_frequencyInput->setDefaultValue(2.0);
    addInputSocket(m_frequencyInput);
    
    m_anisotropyInput = new NodeSocket("Anisotropy", SocketType::Float, SocketDirection::Input, this);
    m_anisotropyInput->setDefaultValue(1.0);
    addInputSocket(m_anisotropyInput);
    
    m_orientationInput = new NodeSocket("Orientation", SocketType::Vector, SocketDirection::Input, this);
    m_orientationInput->setDefaultValue(QVector3D(1, 0, 0));
    addInputSocket(m_orientationInput);
    
    // Distortion (domain warping)
    m_distortionInput = new NodeSocket("Distortion", SocketType::Float, SocketDirection::Input, this);
    m_distortionInput->setDefaultValue(0.0);
    addInputSocket(m_distortionInput);
    
    // Output sockets
    m_valueOutput = new NodeSocket("Value", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_valueOutput);
    
    m_phaseOutput = new NodeSocket("Phase", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_phaseOutput);
    
    m_intensityOutput = new NodeSocket("Intensity", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_intensityOutput);
}

GaborTextureNode::~GaborTextureNode() {}

void GaborTextureNode::evaluate() {
    // Stateless - computation happens in compute()
}

QVariant GaborTextureNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QMutexLocker locker(&m_mutex);
    
    // Get input values
    QVector3D vec;
    if (m_vectorInput->isConnected()) {
        vec = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        // Normalize pixel coordinates to 0-1 range
        vec = QVector3D(pos.x() / 512.0, pos.y() / 512.0, 0.0);
    }
    
    double scaleVal = m_scaleInput->isConnected() ? m_scaleInput->getValue(pos).toDouble() : m_scaleInput->defaultValue().toDouble();
    double frequencyVal = m_frequencyInput->isConnected() ? m_frequencyInput->getValue(pos).toDouble() : m_frequencyInput->defaultValue().toDouble();
    double anisotropyVal = m_anisotropyInput->isConnected() ? m_anisotropyInput->getValue(pos).toDouble() : m_anisotropyInput->defaultValue().toDouble();
    QVector3D orientation = m_orientationInput->isConnected() ? m_orientationInput->getValue(pos).value<QVector3D>() : m_orientationInput->defaultValue().value<QVector3D>();
    double distortionVal = m_distortionInput->isConnected() ? m_distortionInput->getValue(pos).toDouble() : m_distortionInput->defaultValue().toDouble();
    
    // Apply scale
    const double NOISE_OFFSET = 100.0;
    double x = vec.x() * scaleVal + NOISE_OFFSET;
    double y = vec.y() * scaleVal + NOISE_OFFSET;
    double z = vec.z() * scaleVal;
    
    // Apply distortion (domain warping)
    if (distortionVal > 0.0) {
        double dx = m_noise->noise(x + 5.3, y + 2.7, z - 1.4) * distortionVal;
        double dy = m_noise->noise(x - 4.2, y + 8.1, z + 3.3) * distortionVal;
        double dz = m_noise->noise(x + 1.9, y - 6.5, z + 0.2) * distortionVal;
        x += dx;
        y += dy;
        z += dz;
    }
    
    // Clamp anisotropy
    anisotropyVal = qBound(0.0, anisotropyVal, 1.0);
    
    // Compute Gabor noise
    PerlinNoise::GaborResult result = m_noise->gaborNoise(x, y, z, frequencyVal, anisotropyVal, orientation);
    
    if (socket == m_valueOutput) {
        return result.value;
    } else if (socket == m_phaseOutput) {
        return result.phase;
    } else if (socket == m_intensityOutput) {
        return result.intensity;
    }
    
    return result.value;
}

QVector<Node::ParameterInfo> GaborTextureNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // Core parameters
    params.append(ParameterInfo("Scale", 0.01, 100.0, 5.0, 0.1, "Overall scale"));
    params.append(ParameterInfo("Distortion", 0.0, 10.0, 0.0, 0.1, "Domain warping"));
    
    // Gabor-specific parameters
    params.append(ParameterInfo("Frequency", 0.1, 20.0, 2.0, 0.1, "Wave frequency"));
    params.append(ParameterInfo("Anisotropy", 0.0, 1.0, 1.0, 0.01, "0=isotropic, 1=directional"));
    params.append(ParameterInfo("Orientation", -10.0, 10.0, QVector3D(1, 0, 0), 0.1, "Wave direction (3D)"));
    
    return params;
}

QJsonObject GaborTextureNode::save() const {
    QJsonObject json = Node::save();
    json["type"] = "Gabor Texture";
    return json;
}

void GaborTextureNode::restore(const QJsonObject& json) {
    Node::restore(json);
}
